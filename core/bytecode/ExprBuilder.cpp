#include "ExprBuilder.h"

#include <sstream>
#include <iomanip>
#include <vector>
#include <unordered_map>

#include "core/helpers/Console.h"
#include "core/nodes/DeclNode.h"
#include "core/nodes/enums/DeclType.h"
#include "jvm/descriptor-field.h"
#include "jvm/descriptor-method.h"
#include "jvm/field.h"

class DeclNode;
using namespace jvm;

struct LocalScopeInfo {
    std::unordered_map<std::string, uint16_t> slots;
    uint16_t nextFree = 0;
};

static std::unordered_map<const jvm::Method *, LocalScopeInfo> g_localScopes;

static std::unordered_map<const jvm::Method *, bool> g_phpHasThis;

void ExprBuilder::SetPhpMethodHasThis(jvm::Method *method, bool hasThis) {
    if (!method) return;
    g_phpHasThis[method] = hasThis;
}

bool ExprBuilder::PhpMethodHasThis(jvm::Method *method) {
    if (!method) return false;
    auto it = g_phpHasThis.find(method);
    if (it == g_phpHasThis.end()) return false;
    return it->second;
}

uint16_t ExprBuilder::PhpThisLocalSlot() {
    return 0;
}

static std::string normalizeVarName(std::string s) {
    if (!s.empty() && s[0] == '$') s.erase(s.begin());
    return s;
}

void ExprBuilder::BeginLocalScope(jvm::Method *method, uint16_t nextFreeLocal) {
    if (!method) return;
    LocalScopeInfo info;
    info.nextFree = nextFreeLocal;
    g_localScopes[method] = std::move(info);
}

void ExprBuilder::DefineLocal(jvm::Method *method, const std::string &varName, uint16_t localIndex) {
    if (!method) return;
    auto it = g_localScopes.find(method);
    if (it == g_localScopes.end()) return;
    it->second.slots[normalizeVarName(varName)] = localIndex;
    if (localIndex >= it->second.nextFree) it->second.nextFree = static_cast<uint16_t>(localIndex + 1);
}

bool ExprBuilder::TryGetLocal(jvm::Method *method, const std::string &varName, uint16_t &outIndex) {
    if (!method) return false;
    auto it = g_localScopes.find(method);
    if (it == g_localScopes.end()) return false;

    auto key = normalizeVarName(varName);
    auto jt = it->second.slots.find(key);
    if (jt == it->second.slots.end()) return false;

    outIndex = jt->second;
    return true;
}

static std::unordered_map<const jvm::Method *, std::vector<ExprBuilder::ByRefPair> > g_byRefLayouts;
static std::unordered_map<const jvm::Method *, std::string> g_phpCallerClass;

void ExprBuilder::EndLocalScope(jvm::Method *method) {
    if (!method) return;
    g_localScopes.erase(method);
    g_byRefLayouts.erase(method);
    g_phpHasThis.erase(method);
    g_phpCallerClass.erase(method);
}

void ExprBuilder::ReserveNextLocal(jvm::Method *method, uint16_t nextFreeLocal) {
    if (!method) return;
    auto it = g_localScopes.find(method);
    if (it == g_localScopes.end()) return;
    if (nextFreeLocal > it->second.nextFree) it->second.nextFree = nextFreeLocal;
}

uint16_t ExprBuilder::AllocTempLocal(jvm::Method *method) {
    if (!method) return 0;
    auto it = g_localScopes.find(method);
    if (it == g_localScopes.end()) return 0;
    return it->second.nextFree++;
}

void ExprBuilder::SetByRefLayout(jvm::Method *method, const std::vector<ByRefPair> &pairs) {
    if (!method) return;
    g_byRefLayouts[method] = pairs;
}

const std::vector<ExprBuilder::ByRefPair> *ExprBuilder::GetByRefLayout(jvm::Method *method) {
    if (!method) return nullptr;
    auto it = g_byRefLayouts.find(method);
    if (it == g_byRefLayouts.end()) return nullptr;
    return &it->second;
}

void ExprBuilder::EmitFlushByRefIfNeeded(jvm::Class *root, jvm::Method *method, jvm::AttributeCode *code) {
    if (!root || !method || !code) return;

    const auto *pairs = GetByRefLayout(method);
    if (!pairs || pairs->empty()) return;

    auto *setGlobalRefValue = root->getOrCreateMethodrefConstant(
        "com/phpjvm/PhpRuntime",
        "setGlobalRefValue",
        DescriptorMethod(
            std::nullopt,
            {DescriptorField("java/lang/String"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );

    for (const auto &p: *pairs) {
        auto *L_skip = code->CodeLabel();

        // if (refKey == null) skip
        *code << code->LoadReference(p.refKeySlot);
        *code << code->IfNull(L_skip);

        // PhpRuntime.setGlobalRefValue(refKey, value)
        *code << code->LoadReference(p.refKeySlot);
        *code << code->LoadReference(p.valueSlot);
        *code << code->InvokeStatic(setGlobalRefValue);

        *code << L_skip;
    }
}

// Allocate a new local slot for a variable name if we are in a local scope.
// Returns true if allocated, false if not in local scope.
static bool allocLocalIfInScope(jvm::Method *method, const std::string &varName, uint16_t &outIndex) {
    auto it = g_localScopes.find(method);
    if (it == g_localScopes.end()) return false;

    const std::string key = normalizeVarName(varName);

    // already exists?
    auto jt = it->second.slots.find(key);
    if (jt != it->second.slots.end()) {
        outIndex = jt->second;
        return true;
    }

    // allocate
    outIndex = it->second.nextFree;
    it->second.nextFree = static_cast<uint16_t>(it->second.nextFree + 1);
    it->second.slots[key] = outIndex;
    return true;
}

static const ExprNode *childOrNull(const ExprNode *e, size_t i) {
    if (!e) return nullptr;
    if (i >= e->children.size()) return nullptr;
    return e->children[i];
}

static void emitBinary(
    Class *root,
    Method *method,
    AttributeCode *code,
    const ExprNode *left,
    const ExprNode *right,
    ConstantMethodref *op,
    void (*emitValue)(Class *, Method *, AttributeCode *, const ExprNode *)
) {
    emitValue(root, method, code, left);
    emitValue(root, method, code, right);
    *code << code->InvokeStatic(op);
}

static void emitUnary(
    Class *root,
    Method *method,
    AttributeCode *code,
    const ExprNode *operand,
    ConstantMethodref *op,
    void (*emitValue)(Class *, Method *, AttributeCode *, const ExprNode *)
) {
    emitValue(root, method, code, operand);
    *code << code->InvokeStatic(op);
}

static std::string sanitizeJavaIdentLocal(const std::string &raw) {
    std::string s = raw;
    if (!s.empty() && s[0] == '$') s.erase(0, 1);

    std::string out;
    out.reserve(s.size() + 4);
    out += "g_";

    for (char c: s) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') out.push_back(c);
        else out.push_back('_');
    }

    if (out.size() == 2) out += "var";
    if (out.size() >= 3 && (out[2] >= '0' && out[2] <= '9')) out.insert(out.begin() + 2, '_');
    return out;
}

static ConstantFieldref *ensureGlobalVarField(Class *root, const std::string &varName) {
    std::string fieldName = sanitizeJavaIdentLocal(varName);

    Field *f = root->getOrCreateField(fieldName, DescriptorField("com/phpjvm/BasePhpValue"));
    f->addFlag(Field::ACC_PUBLIC);
    f->addFlag(Field::ACC_STATIC);

    return root->getOrCreateFieldrefConstant(
        root->getClassName(),
        fieldName,
        DescriptorField("com/phpjvm/BasePhpValue")
    );
}

static void emitLoadLocalVar(
    AttributeCode *code,
    uint16_t slot,
    ConstantFieldref *nullValueField
) {
    auto *L_nonNull = code->CodeLabel();
    auto *L_end = code->CodeLabel();

    *code << code->LoadReference(slot);
    *code << code->Duplicate();
    *code << code->IfNotNull(L_nonNull);
    *code << code->PopOne();
    *code << code->GetStatic(nullValueField);
    *code << code->GoTo(L_end);

    *code << L_nonNull;
    *code << L_end;
}

// pushes current value of $var (NULL_VALUE if field is null)
static void emitLoadGlobalVar(
    Class *root,
    AttributeCode *code,
    ConstantFieldref *fieldRef,
    ConstantFieldref *nullValueField
) {
    auto *L_nonNull = code->CodeLabel();
    auto *L_end = code->CodeLabel();

    *code << code->GetStatic(fieldRef);
    *code << code->Duplicate();
    *code << code->IfNotNull(L_nonNull);
    *code << code->PopOne();
    *code << code->GetStatic(nullValueField);
    *code << code->GoTo(L_end);

    *code << L_nonNull;
    *code << L_end;
}

static std::string idText(const ExprNode *e) {
    if (!e) return "";
    if (!e->name.empty()) return e->name;
    if (e->value) {
        if (!e->value->name.empty()) return e->value->name;
        if (e->value->stringValue.data()) return std::string(e->value->stringValue);
    }
    return "";
}

static std::string lowerCopy(std::string s) {
    for (char &c: s) c = (char) tolower((unsigned char) c);
    return s;
}

static std::string sanitizeJavaIdent(const std::string &raw) {
    std::string s = raw;
    if (!s.empty() && s[0] == '$') s.erase(0, 1);

    std::string out;
    out.reserve(s.size() + 4);
    out += "g_";

    for (char c: s) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') out.push_back(c);
        else out.push_back('_');
    }

    if (out.size() == 2) out += "var";
    if (out.size() >= 3 && (out[2] >= '0' && out[2] <= '9')) out.insert(out.begin() + 2, '_');
    return out;
}

// ----------------------------------------------------------------------------
// Signature registry (for call-site checks + by-ref marker emission)
// ----------------------------------------------------------------------------
struct PhpParamSig {
    bool byRef = false;
    std::vector<std::string> types; // lowercase, union expanded
};

struct PhpFuncSig {
    std::vector<PhpParamSig> params;
    std::vector<std::string> returnTypes; // lowercase
};

static std::unordered_map<std::string, PhpFuncSig> g_functionSigs;
static std::unordered_map<std::string, PhpFuncSig> g_methodSigs; // key: "class::method" lower

// NOTE: This assumes ValueNode has .elements with .name like your JSON.
// If your ValueNode differs, adapt these helpers.
static std::vector<std::string> collectTypeNames(ValueNode *vt) {
    std::vector<std::string> out;
    if (!vt) return out;
    try {
        for (auto *el: vt->valueList) {
            if (!el) continue;
            if (!el->name.empty()) out.push_back(lowerCopy(el->name));
        }
    } catch (...) {
    }
    return out;
}

static std::vector<DeclNode *> flattenParamList(DeclNode *params) {
    std::vector<DeclNode *> out;
    if (!params) return out;

    if (params->type == DT_LIST) {
        for (auto *ch: params->children) {
            if (ch && ch->type == DT_PARAMETER) out.push_back(ch);
        }
    } else if (params->type == DT_PARAMETER) {
        out.push_back(params);
    }
    return out;
}

void ExprBuilder::RegisterFunctionSignature(const std::string &fnLower, DeclNode *params, ValueNode *retType) {
    PhpFuncSig sig;
    auto ps = flattenParamList(params);
    sig.params.reserve(ps.size());

    for (auto *p: ps) {
        PhpParamSig psig;
        psig.byRef = (p && p->hasAddressOperator);
        psig.types = collectTypeNames(p ? p->valueType : nullptr);
        sig.params.push_back(psig);
    }
    sig.returnTypes = collectTypeNames(retType);
    g_functionSigs[lowerCopy(fnLower)] = sig;
}

void ExprBuilder::RegisterMethodSignature(const std::string &classLower, const std::string &methodLower,
                                          DeclNode *params, ValueNode *retType) {
    PhpFuncSig sig;
    auto ps = flattenParamList(params);
    sig.params.reserve(ps.size());

    for (auto *p: ps) {
        PhpParamSig psig;
        psig.byRef = (p && p->hasAddressOperator);
        psig.types = collectTypeNames(p ? p->valueType : nullptr);
        sig.params.push_back(psig);
    }
    sig.returnTypes = collectTypeNames(retType);

    std::string key = lowerCopy(classLower) + "::" + lowerCopy(methodLower);
    g_methodSigs[key] = sig;
}

static const PhpFuncSig *findFunctionSig(const std::string &fnLower) {
    auto it = g_functionSigs.find(lowerCopy(fnLower));
    if (it == g_functionSigs.end()) return nullptr;
    return &it->second;
}

// Emits BasePhpValue[] on stack (old helper)
static void emitArgsArray(Class *root, Method *method, AttributeCode *code, const ExprNode *argsNode) {
    auto *basePhpValueClass = root->getOrCreateClassConstant("com/phpjvm/BasePhpValue");

    std::vector<const ExprNode *> args;
    if (argsNode) {
        if (argsNode->type == ExprType::ET_EXPR_LIST) {
            for (auto *ch: argsNode->children) if (ch) args.push_back(ch);
        } else {
            args.push_back(argsNode);
        }
    }

    *code << code->PushInt((int32_t) args.size());
    *code << code->NewArray(basePhpValueClass);

    for (size_t i = 0; i < args.size(); i++) {
        *code << code->Duplicate();
        *code << code->PushInt((int32_t) i);
        ExprBuilder::EmitValue(root, method, code, args[i]);
        *code << code->StoreReferenceToArray();
    }
}

static void emitStringArrayConst(Class *root, AttributeCode *code, const std::vector<std::string> &items) {
    auto *stringClass = root->getOrCreateClassConstant("java/lang/String");

    *code << code->PushInt((int32_t) items.size());
    *code << code->NewArray(stringClass);

    for (size_t i = 0; i < items.size(); i++) {
        *code << code->Duplicate();
        *code << code->PushInt((int32_t) i);
        *code << code->PushString(items[i]);
        *code << code->StoreReferenceToArray();
    }
}

void ExprBuilder::SetPhpCallerClass(jvm::Method *method, const std::string &callerClassLower) {
    if (!method) return;
    g_phpCallerClass[method] = callerClassLower;
}

std::string ExprBuilder::PhpCallerClass(jvm::Method *method) {
    if (!method) return "";
    auto it = g_phpCallerClass.find(method);
    if (it == g_phpCallerClass.end()) return "";
    return it->second;
}

void ExprBuilder::EmitValue(Class *root, Method *method, AttributeCode *code, const ExprNode *expr) {
    if (!root) throw std::logic_error("ExprBuilder::EmitValue: root is null");
    if (!method) throw std::logic_error("ExprBuilder::EmitValue: method is null");
    if (!code) throw std::logic_error("ExprBuilder::EmitValue: code is null");

    // ---------- Runtime symbols ----------
    auto *nullValueField = root->getOrCreateFieldrefConstant(
        "com/phpjvm/BasePhpValue",
        "NULL_VALUE",
        DescriptorField("com/phpjvm/BasePhpValue")
    );

    auto *ofString = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "of",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("java/lang/String")}
        )
    );

    auto *ofBool = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "of",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("Z")}
        )
    );

    auto *ofLong = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "of",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("J")}
        )
    );

    auto *ofDouble = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "of",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("D")}
        )
    );

    auto *add = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "add",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *sub = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "sub",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *mul = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "mul",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *div = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "div",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *mod = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "mod",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *concat = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "concat",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );

    auto *xorOp = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "xor",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );

    auto *bitAnd = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "bitAnd",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );

    auto *bitOr = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "bitOr",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );

    auto *bitXor = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "bitXor",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );

    auto *bitNot = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "bitNot",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );

    auto *shl = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "shl",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );

    auto *shr = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "shr",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );

    auto *powOp = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "pow",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );

    auto *instanceOfOp = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "instanceOf",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );

    auto *eq = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "eq",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *identical = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "identical",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *lt = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "lt",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *le = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "le",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );

    auto *ne = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "ne",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *notIdentical = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "notIdentical",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *gt = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "gt",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *ge = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "ge",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *boolNot = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "not",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *boolAnd = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "and",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *boolOr = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "or",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *spaceship = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "spaceship",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );

    auto *toBool = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "toBool",
        DescriptorMethod(DescriptorField("Z"), {})
    );

    auto *isNull = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "isNull",
        DescriptorMethod(DescriptorField("Z"), {})
    );

    auto *asObject = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "asObject",
        DescriptorMethod(DescriptorField("com/phpjvm/PhpObject"), {})
    );

    auto *rtCallMethodCtx = root->getOrCreateMethodrefConstant(
        "com/phpjvm/PhpRuntime",
        "callMethodCtx",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {
                DescriptorField("com/phpjvm/PhpObject"),
                DescriptorField("java/lang/String"),
                DescriptorField("com/phpjvm/BasePhpValue", 1),
                DescriptorField("java/lang/String")
            }
        )
    );

    auto *rtCallStaticCtx = root->getOrCreateMethodrefConstant(
        "com/phpjvm/PhpRuntime",
        "callStaticCtx",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {
                DescriptorField("com/phpjvm/PhpClass"),
                DescriptorField("java/lang/String"),
                DescriptorField("com/phpjvm/BasePhpValue", 1),
                DescriptorField("java/lang/String")
            }
        )
    );

    auto *rtNewObject = root->getOrCreateMethodrefConstant(
        "com/phpjvm/PhpRuntime",
        "newObject",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {
                DescriptorField("java/lang/String"),
                DescriptorField("com/phpjvm/BasePhpValue", 1)
            }
        )
    );

    auto *objGetProp = root->getOrCreateMethodrefConstant(
        "com/phpjvm/PhpObject",
        "getProperty",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("java/lang/String")}
        )
    );

    auto *arrFactory = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "array",
        DescriptorMethod(DescriptorField("com/phpjvm/BasePhpValue"), {})
    );

    auto *arrGet = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "arrayGet",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );

    auto *arrSet = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "arraySet",
        DescriptorMethod(
            std::nullopt,
            {
                DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue"),
                DescriptorField("com/phpjvm/BasePhpValue")
            }
        )
    );

    auto *arrAppend = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "arrayAppend",
        DescriptorMethod(
            std::nullopt,
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );

    auto *rtCallFunction = root->getOrCreateMethodrefConstant(
        "com/phpjvm/PhpRuntime",
        "callFunction",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {
                DescriptorField("java/lang/String"),
                DescriptorField("com/phpjvm/BasePhpValue", 1)
            }
        )
    );

    // NEW: typecheck + ref marker maker
    auto *rtAssertParamType = root->getOrCreateMethodrefConstant(
        "com/phpjvm/PhpRuntime",
        "assertParamType",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {
                DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("java/lang/String", 1),
                DescriptorField("java/lang/String"), DescriptorField("java/lang/String")
            }
        )
    );

    auto *rtAssertReturnType = root->getOrCreateMethodrefConstant(
        "com/phpjvm/PhpRuntime",
        "assertReturnType",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {
                DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("java/lang/String", 1),
                DescriptorField("java/lang/String")
            }
        )
    );

    auto *rtMakeGlobalRef = root->getOrCreateMethodrefConstant(
        "com/phpjvm/PhpRuntime",
        "makeGlobalRef",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("java/lang/String"), DescriptorField("java/lang/String")}
        )
    );

    auto *rtRequireClass = root->getOrCreateMethodrefConstant(
        "com/phpjvm/PhpRuntime",
        "requireClass",
        DescriptorMethod(
            DescriptorField("com/phpjvm/PhpClass"),
            {DescriptorField("java/lang/String")}
        )
    );

    auto *objGetPhpClass = root->getOrCreateMethodrefConstant(
        "com/phpjvm/PhpObject",
        "getPhpClass",
        DescriptorMethod(DescriptorField("com/phpjvm/PhpClass"), {})
    );

    auto *rtCallParent = root->getOrCreateMethodrefConstant(
        "com/phpjvm/PhpRuntime",
        "callParent",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {
                DescriptorField("com/phpjvm/PhpObject"),
                DescriptorField("java/lang/String"),
                DescriptorField("java/lang/String"),
                DescriptorField("com/phpjvm/BasePhpValue", 1)
            }
        )
    );

    auto *rtCallParentStatic = root->getOrCreateMethodrefConstant(
        "com/phpjvm/PhpRuntime",
        "callParentStatic",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {
                DescriptorField("java/lang/String"),
                DescriptorField("java/lang/String"),
                DescriptorField("com/phpjvm/BasePhpValue", 1)
            }
        )
    );

    auto *rtGetStaticPropCtx = root->getOrCreateMethodrefConstant(
        "com/phpjvm/PhpRuntime",
        "getStaticPropCtx",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {
                DescriptorField("com/phpjvm/PhpClass"),
                DescriptorField("java/lang/String"),
                DescriptorField("java/lang/String")
            }
        )
    );

    auto *rtSetStaticPropCtx = root->getOrCreateMethodrefConstant(
        "com/phpjvm/PhpRuntime",
        "setStaticPropCtx",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {
                DescriptorField("com/phpjvm/PhpClass"),
                DescriptorField("java/lang/String"),
                DescriptorField("com/phpjvm/BasePhpValue"),
                DescriptorField("java/lang/String")
            }
        )
    );

    auto *rtGetConstCtx = root->getOrCreateMethodrefConstant(
        "com/phpjvm/PhpRuntime",
        "getConstCtx",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {
                DescriptorField("com/phpjvm/PhpClass"),
                DescriptorField("java/lang/String"),
                DescriptorField("java/lang/String")
            }
        )
    );

    auto *rtGetParentStaticPropCtx = root->getOrCreateMethodrefConstant(
        "com/phpjvm/PhpRuntime",
        "getParentStaticPropCtx",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {
                DescriptorField("java/lang/String"),
                DescriptorField("java/lang/String"),
                DescriptorField("java/lang/String")
            }
        )
    );

    auto *rtSetParentStaticPropCtx = root->getOrCreateMethodrefConstant(
        "com/phpjvm/PhpRuntime",
        "setParentStaticPropCtx",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {
                DescriptorField("java/lang/String"),
                DescriptorField("java/lang/String"),
                DescriptorField("com/phpjvm/BasePhpValue"),
                DescriptorField("java/lang/String")
            }
        )
    );

    auto *rtGetParentConstCtx = root->getOrCreateMethodrefConstant(
        "com/phpjvm/PhpRuntime",
        "getParentConstCtx",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {
                DescriptorField("java/lang/String"),
                DescriptorField("java/lang/String"),
                DescriptorField("java/lang/String")
            }
        )
    );

    auto *rtGetPropCtx = root->getOrCreateMethodrefConstant(
        "com/phpjvm/PhpRuntime",
        "getPropCtx",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {
                DescriptorField("com/phpjvm/PhpObject"),
                DescriptorField("java/lang/String"),
                DescriptorField("java/lang/String")
            }
        )
    );

    auto *rtSetPropCtx = root->getOrCreateMethodrefConstant(
        "com/phpjvm/PhpRuntime",
        "setPropCtx",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {
                DescriptorField("com/phpjvm/PhpObject"),
                DescriptorField("java/lang/String"),
                DescriptorField("com/phpjvm/BasePhpValue"),
                DescriptorField("java/lang/String")
            }
        )
    );

    // Add MethodRef constants for the native PHP functions
    auto *rtCount = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "count",
        DescriptorMethod(DescriptorField("com/phpjvm/BasePhpValue"), {DescriptorField("com/phpjvm/BasePhpValue")})
    );

    auto *rtImplode = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "implode", DescriptorMethod(DescriptorField("com/phpjvm/BasePhpValue"), {
                                        DescriptorField("com/phpjvm/BasePhpValue"),
                                        DescriptorField("com/phpjvm/BasePhpValue")
                                    })
    );

    auto *rtTrim = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "trim",
        DescriptorMethod(DescriptorField("com/phpjvm/BasePhpValue"), {DescriptorField("com/phpjvm/BasePhpValue")})
    );

    if (expr == nullptr) {
        *code << code->GetStatic(nullValueField);
        return;
    }

    switch (expr->type) {
        case ExprType::ET_EXPR_LIST: {
            if (expr->children.empty()) {
                *code << code->GetStatic(nullValueField);
                return;
            }

            for (size_t i = 0; i < expr->children.size(); i++) {
                EmitValue(root, method, code, expr->children[i]);
                if (i + 1 < expr->children.size()) {
                    *code << code->PopOne();
                }
            }
            return;
        }

        case ExprType::ET_STRING: {
            std::string s = (expr->value ? expr->value->stringValue : "");
            *code << code->PushString(s);
            *code << code->InvokeStatic(ofString);
            return;
        }

        case ExprType::ET_INT: {
            int64_t v = (expr->value ? (int64_t) expr->value->intValue : 0);
            *code << code->PushLong(v);
            *code << code->InvokeStatic(ofLong);
            return;
        }

        case ExprType::ET_FLOAT: {
            double v = (expr->value ? (double) expr->value->floatValue : 0.0);
            *code << code->PushDouble(v);
            *code << code->InvokeStatic(ofDouble);
            return;
        }

        case ExprType::ET_BOOL: {
            bool v = (expr->value ? expr->value->boolValue : false);
            *code << code->PushInt(v ? 1 : 0);
            *code << code->InvokeStatic(ofBool);
            return;
        }

        case ExprType::ET_NIL: {
            *code << code->GetStatic(nullValueField);
            return;
        }

        case ExprType::ET_PARENTHESIZED: {
            const ExprNode *inner = childOrNull(expr, 0);
            EmitValue(root, method, code, inner);
            return;
        }

        case ExprType::ET_COMPLEX_STRING: {
            if (expr->children.empty() || expr->children[0] == nullptr) {
                *code << code->PushString("");
                *code << code->InvokeStatic(ofString);
                return;
            }

            const ExprNode *args = expr->children[0];

            std::vector<const ExprNode *> parts;
            if (args->type == ExprType::ET_EXPR_LIST) {
                for (auto *p: args->children) {
                    if (p) parts.push_back(p);
                }
            } else {
                parts.push_back(args);
            }

            if (parts.empty()) {
                *code << code->PushString("");
                *code << code->InvokeStatic(ofString);
                return;
            }

            EmitValue(root, method, code, parts[0]);
            for (size_t i = 1; i < parts.size(); i++) {
                EmitValue(root, method, code, parts[i]);
                *code << code->InvokeStatic(concat);
            }
            return;
        }

        case ExprType::ET_ADD: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), add, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_SUBTRACT: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), sub, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_MULT: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), mul, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_DIV: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), div, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_MOD: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), mod, &ExprBuilder::EmitValue);
            return;
        }

        case ExprType::ET_CONCAT: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), concat, &ExprBuilder::EmitValue);
            return;
        }

        case ExprType::ET_EQUAL: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), eq, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_NOT_EQUAL: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), ne, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_IDENTICALLY_EQUAL: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), identical,
                       &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_IDENTICALLY_NOT_EQUAL: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), notIdentical,
                       &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_LESS_THAN: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), lt, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_LESS_OR_EQUAL: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), le, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_GREATER_THAN: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), gt, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_GREAT_OR_EQUAL: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), ge, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_SPACESHIP: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), spaceship,
                       &ExprBuilder::EmitValue);
            return;
        }

        case ExprType::ET_NOT: {
            emitUnary(root, method, code, childOrNull(expr, 0), boolNot, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_AND: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), boolAnd,
                       &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_OR: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), boolOr, &ExprBuilder::EmitValue);
            return;
        }

        case ExprType::ET_SIGIL: {
            const ExprNode *id = childOrNull(expr, 0);
            std::string var = idText(id);

            // Special: $this inside instance methods
            if (lowerCopy(var) == "this") {
                if (ExprBuilder::PhpMethodHasThis(method)) {
                    auto *ofObject = root->getOrCreateMethodrefConstant(
                        "com/phpjvm/BasePhpValue",
                        "object",
                        DescriptorMethod(
                            DescriptorField("com/phpjvm/BasePhpValue"),
                            {DescriptorField("com/phpjvm/PhpObject")}
                        )
                    );

                    *code << code->LoadReference(ExprBuilder::PhpThisLocalSlot()); // PhpObject
                    *code << code->InvokeStatic(ofObject); // BasePhpValue
                    return;
                }

                Console::Warning("$this used in static context (pushing NULL)");
                *code << code->GetStatic(nullValueField);
                return;
            }

            uint16_t localIndex = 0;

            if (allocLocalIfInScope(method, var, localIndex)) {
                emitLoadLocalVar(code, localIndex, nullValueField);
                return;
            }

            // Not in a local scope => global
            auto *fieldRef = ensureGlobalVarField(root, var);
            emitLoadGlobalVar(root, code, fieldRef, nullValueField);
            return;
        }

        case ExprType::ET_ASSIGN: {
            const ExprNode *lhs = childOrNull(expr, 0);
            const ExprNode *rhs = childOrNull(expr, 1);

            // ---- $var = expr ----
            if (lhs && lhs->type == ExprType::ET_SIGIL) {
                const ExprNode *id = childOrNull(lhs, 0);
                std::string var = idText(id);

                uint16_t localIndex = 0;

                if (allocLocalIfInScope(method, var, localIndex)) {
                    EmitValue(root, method, code, rhs);
                    *code << code->Duplicate();
                    *code << code->StoreReference(localIndex);
                    return;
                }

                auto *fieldRef = ensureGlobalVarField(root, var);
                EmitValue(root, method, code, rhs);
                *code << code->Duplicate();
                *code << code->PutStatic(fieldRef);
                return;
            }

            // ---- A::$prop = expr  (only if member is ET_SIGIL) ----
            if (lhs && lhs->type == ExprType::ET_STATIC_PROPERTY_ACCESS) {
                const ExprNode *classExpr = childOrNull(lhs, 0);
                const ExprNode *memberExpr = childOrNull(lhs, 1);

                std::string classToken = lowerCopy(idText(classExpr));
                bool isStaticProp = (memberExpr && memberExpr->type == ExprType::ET_SIGIL);
                std::string memberName = isStaticProp ? idText(childOrNull(memberExpr, 0)) : idText(memberExpr);

                if (!isStaticProp) {
                    Console::Warning("ET_ASSIGN: cannot assign to class constant (pushing NULL)");
                    *code << code->GetStatic(nullValueField);
                    return;
                }

                std::string callerCtx = ExprBuilder::PhpCallerClass(method);

                if (classToken == "parent") {
                    *code << code->PushString(root->getClassName());
                    *code << code->PushString(memberName);
                    EmitValue(root, method, code, rhs);
                    *code << code->PushString(callerCtx);
                    *code << code->InvokeStatic(rtSetParentStaticPropCtx);
                    return;
                }

                if (classToken == "self") {
                    *code << code->PushString(root->getClassName());
                    *code << code->InvokeStatic(rtRequireClass);
                } else if (classToken == "static") {
                    if (ExprBuilder::PhpMethodHasThis(method)) {
                        *code << code->LoadReference(ExprBuilder::PhpThisLocalSlot());
                        *code << code->InvokeVirtual(objGetPhpClass);
                    } else {
                        *code << code->LoadReference(0);
                    }
                } else {
                    *code << code->PushString(classToken);
                    *code << code->InvokeStatic(rtRequireClass);
                }

                *code << code->PushString(memberName);
                EmitValue(root, method, code, rhs);
                *code << code->PushString(callerCtx);
                *code << code->InvokeStatic(rtSetStaticPropCtx);
                return;
            }

            // ---- $obj->prop = expr ----
            if (lhs && lhs->type == ExprType::ET_PROPERTY_ACCESS) {
                const ExprNode *objExpr = childOrNull(lhs, 0);
                const ExprNode *nameExpr = childOrNull(lhs, 1);

                std::string propName = idText(nameExpr);
                if (propName.empty()) {
                    Console::Warning("ET_ASSIGN(ET_PROPERTY_ACCESS): missing property name (pushing NULL)");
                    *code << code->GetStatic(nullValueField);
                    return;
                }

                EmitValue(root, method, code, objExpr);
                *code << code->InvokeVirtual(asObject); // PhpObject
                *code << code->PushString(propName);
                EmitValue(root, method, code, rhs);
                *code << code->PushString(ExprBuilder::PhpCallerClass(method));
                *code << code->InvokeStatic(rtSetPropCtx);
                return;
            }

            Console::Warning("ET_ASSIGN bytecode: unsupported lhs (pushing NULL)");
            *code << code->GetStatic(nullValueField);
            return;
        }

        case ExprType::ET_PROPERTY_ACCESS: {
            const ExprNode *objExpr = childOrNull(expr, 0);
            const ExprNode *nameExpr = childOrNull(expr, 1);

            std::string propName = idText(nameExpr);
            if (propName.empty()) {
                Console::Warning("ET_PROPERTY_ACCESS: missing property name (pushing NULL)");
                *code << code->GetStatic(nullValueField);
                return;
            }

            EmitValue(root, method, code, objExpr);
            *code << code->InvokeVirtual(asObject); // PhpObject
            *code << code->PushString(propName); // name
            *code << code->PushString(ExprBuilder::PhpCallerClass(method)); // caller ctx
            *code << code->InvokeStatic(rtGetPropCtx); // BasePhpValue
            return;
        }

        case ExprType::ET_NEW: {
            const ExprNode *classNameExpr = nullptr;
            const ExprNode *argsNode = nullptr;

            if (expr->children.size() == 2) {
                classNameExpr = childOrNull(expr, 0);
                argsNode = childOrNull(expr, 1);
            } else if (expr->children.size() == 1) {
                Console::Warning("ET_NEW: missing class name in AST (pushing NULL)");
                *code << code->GetStatic(nullValueField);
                return;
            }

            std::string className = idText(classNameExpr);
            if (className.empty()) {
                Console::Warning("ET_NEW: empty class name (pushing NULL)");
                *code << code->GetStatic(nullValueField);
                return;
            }

            *code << code->PushString(className);
            emitArgsArray(root, method, code, argsNode);
            *code << code->InvokeStatic(rtNewObject);
            return;
        }

        case ExprType::ET_ARRAY_EMPTY: {
            *code << code->InvokeStatic(arrFactory);
            return;
        }

        case ExprType::ET_ARRAY_INDEX: {
            EmitValue(root, method, code, childOrNull(expr, 0));
            EmitValue(root, method, code, childOrNull(expr, 1));
            *code << code->InvokeStatic(arrGet);
            return;
        }

        case ExprType::ET_ARRAY_KEY_ACCESS: {
            *code << code->InvokeStatic(arrFactory);
            *code << code->Duplicate();
            EmitValue(root, method, code, childOrNull(expr, 0));
            EmitValue(root, method, code, childOrNull(expr, 1));
            *code << code->InvokeStatic(arrSet);
            return;
        }

        case ExprType::ET_ARRAY_ELEMENT_LIST: {
            const ExprNode *elements = childOrNull(expr, 0);

            *code << code->InvokeStatic(arrFactory);

            std::vector<const ExprNode *> parts;
            if (elements) {
                if (elements->type == ExprType::ET_EXPR_LIST) {
                    for (auto *ch: elements->children) if (ch) parts.push_back(ch);
                } else {
                    parts.push_back(elements);
                }
            }

            for (auto *p: parts) {
                if (!p) continue;

                if (p->type == ExprType::ET_ARRAY_KEY_ACCESS) {
                    *code << code->Duplicate();
                    EmitValue(root, method, code, childOrNull(p, 0));
                    EmitValue(root, method, code, childOrNull(p, 1));
                    *code << code->InvokeStatic(arrSet);
                } else {
                    *code << code->Duplicate();
                    EmitValue(root, method, code, p);
                    *code << code->InvokeStatic(arrAppend);
                }
            }
            return;
        }

        case ExprType::ET_ARRAY_APPEND: {
            EmitValue(root, method, code, childOrNull(expr, 0));
            *code << code->Duplicate();
            EmitValue(root, method, code, childOrNull(expr, 1));
            *code << code->InvokeStatic(arrAppend);
            return;
        }

        case ExprType::ET_ARRAY_ASSIGNMENT: {
            EmitValue(root, method, code, childOrNull(expr, 0));
            *code << code->Duplicate();
            EmitValue(root, method, code, childOrNull(expr, 1));
            EmitValue(root, method, code, childOrNull(expr, 2));
            *code << code->InvokeStatic(arrSet);
            return;
        }

        case ExprType::ET_TERNARY: {
            const ExprNode *cond = childOrNull(expr, 0);
            const ExprNode *a = childOrNull(expr, 1);
            const ExprNode *b = childOrNull(expr, 2);

            auto *L_false = code->CodeLabel();
            auto *L_end = code->CodeLabel();

            EmitValue(root, method, code, cond);
            *code << code->InvokeVirtual(toBool);
            *code << code->If(Instruction::Compare::Equal, L_false);

            EmitValue(root, method, code, a);
            *code << code->GoTo(L_end);

            *code << L_false;
            EmitValue(root, method, code, b);

            *code << L_end;
            return;
        }

        case ExprType::ET_NULL_COALESCING: {
            const ExprNode *a = childOrNull(expr, 0);
            const ExprNode *b = childOrNull(expr, 1);

            auto *L_isNull = code->CodeLabel();
            auto *L_end = code->CodeLabel();

            EmitValue(root, method, code, a);
            *code << code->Duplicate();
            *code << code->InvokeVirtual(isNull);
            *code << code->If(Instruction::Compare::NotEqual, L_isNull);

            *code << code->GoTo(L_end);

            *code << L_isNull;
            *code << code->PopOne();
            EmitValue(root, method, code, b);

            *code << L_end;
            return;
        }

        // ===== function call: foo(a,b) OR (new A)(args) =====
        case ExprType::ET_FUNCTION_CALL: {
            const ExprNode *fn = childOrNull(expr, 0);
            const ExprNode *argsNode = childOrNull(expr, 1);

            if (!fn) {
                Console::Warning("ET_FUNCTION_CALL: missing callee (pushing NULL)");
                *code << code->GetStatic(nullValueField);
                return;
            }

            // ---------------------------------------------------------------------
            // NEW: static access used as a call: X::m(...)
            // AST shape: ET_FUNCTION_CALL( ET_STATIC_PROPERTY_ACCESS(X, m), args )
            // where X can be: parent/self/static/ClassName
            // ---------------------------------------------------------------------
            if (fn->type == ExprType::ET_STATIC_PROPERTY_ACCESS) {
                const ExprNode *classExpr = childOrNull(fn, 0);
                const ExprNode *memberExpr = childOrNull(fn, 1);

                std::string classToken = lowerCopy(idText(classExpr));
                std::string memberName = idText(memberExpr);
                std::string memberLower = lowerCopy(memberName);

                if (memberLower.empty()) {
                    Console::Warning("ET_FUNCTION_CALL(ET_STATIC_PROPERTY_ACCESS): missing member name (pushing NULL)");
                    *code << code->GetStatic(nullValueField);
                    return;
                }

                // parent::m(...)
                if (classToken == "parent") {
                    if (ExprBuilder::PhpMethodHasThis(method)) {
                        // instance context => implicit $this (PhpObject in local 0)
                        *code << code->LoadReference(ExprBuilder::PhpThisLocalSlot()); // PhpObject
                        *code << code->PushString(root->getClassName()); // lexical class
                        *code << code->PushString(memberLower);
                        emitArgsArray(root, method, code, argsNode);
                        *code << code->InvokeStatic(rtCallParent);
                        return;
                    }

                    // static context => parent static
                    *code << code->PushString(root->getClassName()); // lexical class
                    *code << code->PushString(memberLower);
                    emitArgsArray(root, method, code, argsNode);
                    *code << code->InvokeStatic(rtCallParentStatic);
                    return;
                }

                // self::m(...) => lexical class (the class where the method is defined)
                if (classToken == "self") {
                    *code << code->PushString(root->getClassName());
                    *code << code->InvokeStatic(rtRequireClass); // PhpClass
                    *code << code->PushString(memberLower);
                    emitArgsArray(root, method, code, argsNode);
                    *code << code->PushString(ExprBuilder::PhpCallerClass(method));
                    *code << code->InvokeStatic(rtCallStaticCtx);
                    return;
                }

                // static::m(...) => late static binding (called class)
                if (classToken == "static") {
                    if (ExprBuilder::PhpMethodHasThis(method)) {
                        *code << code->LoadReference(ExprBuilder::PhpThisLocalSlot()); // PhpObject
                        *code << code->InvokeVirtual(objGetPhpClass); // PhpClass
                    } else {
                        *code << code->LoadReference(0); // PhpClass
                    }

                    *code << code->PushString(memberLower);
                    emitArgsArray(root, method, code, argsNode);
                    *code << code->PushString(ExprBuilder::PhpCallerClass(method));
                    *code << code->InvokeStatic(rtCallStaticCtx);
                    return;
                }

                // Explicit class name: Foo::m(...)
                *code << code->PushString(classToken);
                *code << code->InvokeStatic(rtRequireClass); // PhpClass
                *code << code->PushString(memberLower);
                emitArgsArray(root, method, code, argsNode);
                *code << code->PushString(ExprBuilder::PhpCallerClass(method));
                *code << code->InvokeStatic(rtCallStaticCtx);
                return;
            }

            // ---------------------------------------------------------------------
            // NEW: instance method access used as a call: $obj->m(...)
            // AST shape: ET_FUNCTION_CALL( ET_METHOD_ACCESS(obj, m), args )
            // ---------------------------------------------------------------------
            if (fn->type == ExprType::ET_METHOD_ACCESS) {
                const ExprNode *objExpr = childOrNull(fn, 0);
                const ExprNode *nameExpr = childOrNull(fn, 1);

                std::string methodName = lowerCopy(idText(nameExpr));
                if (methodName.empty()) {
                    Console::Warning("ET_FUNCTION_CALL(ET_METHOD_ACCESS): missing method name (pushing NULL)");
                    *code << code->GetStatic(nullValueField);
                    return;
                }

                // Evaluate object -> PhpObject
                EmitValue(root, method, code, objExpr); // BasePhpValue
                *code << code->InvokeVirtual(asObject); // PhpObject

                // callMethodCtx(obj, "method", args, callerCtx)
                *code << code->PushString(methodName);
                emitArgsArray(root, method, code, argsNode);
                *code << code->PushString(ExprBuilder::PhpCallerClass(method));
                *code << code->InvokeStatic(rtCallMethodCtx);
                return;
            }

            // Existing behavior: ET_FUNCTION_CALL(ET_NEW(...), args) -> newObject
            if (fn->type == ExprType::ET_NEW) {
                const ExprNode *classNameExpr = childOrNull(fn, 0);
                std::string className = idText(classNameExpr);

                if (className.empty()) {
                    Console::Warning("ET_FUNCTION_CALL(ET_NEW ...): empty class name (pushing NULL)");
                    *code << code->GetStatic(nullValueField);
                    return;
                }

                *code << code->PushString(className);
                emitArgsArray(root, method, code, argsNode);
                *code << code->InvokeStatic(rtNewObject);
                return;
            }

            std::string fnName = lowerCopy(idText(fn));
            if (fnName.empty()) {
                Console::Warning("ET_FUNCTION_CALL: missing function name (pushing NULL)");
                *code << code->GetStatic(nullValueField);
                return;
            }

            // ---- builtins (stdin) ----
            if (fnName == "fgets" || fnName == "fgetc") {
                *code << code->PushString(fnName);
                emitArgsArray(root, method, code, argsNode);
                *code << code->InvokeStatic(rtCallFunction);
                return;
            }

            std::vector<const ExprNode *> args;
            if (argsNode) {
                if (argsNode->type == ExprType::ET_EXPR_LIST) {
                    for (auto *ch: argsNode->children) if (ch) args.push_back(ch);
                } else {
                    args.push_back(argsNode);
                }
            }

            // ---- BasePhpValue-native builtins ----
            if (fnName == "count") {
                // evaluate arg0 (or NULL)
                if (!args.empty()) EmitValue(root, method, code, args[0]);
                else *code << code->GetStatic(nullValueField);

                // evaluate extra args for side-effects, then drop them
                for (size_t i = 1; i < args.size(); i++) {
                    EmitValue(root, method, code, args[i]);
                    *code << code->PopOne();
                }

                *code << code->InvokeStatic(rtCount);
                return;
            }

            if (fnName == "trim") {
                if (!args.empty()) EmitValue(root, method, code, args[0]);
                else *code << code->GetStatic(nullValueField);

                for (size_t i = 1; i < args.size(); i++) {
                    EmitValue(root, method, code, args[i]);
                    *code << code->PopOne();
                }

                *code << code->InvokeStatic(rtTrim);
                return;
            }

            // PHP: implode($glue, $pieces) OR implode($pieces) OR join(...)
            if (fnName == "implode" || fnName == "join") {
                if (args.size() == 0) {
                    // glue="" , pieces=NULL
                    *code << code->PushString("");
                    *code << code->InvokeStatic(ofString);
                    *code << code->GetStatic(nullValueField);
                    *code << code->InvokeStatic(rtImplode);
                    return;
                }

                if (args.size() == 1) {
                    // glue="" , pieces=args[0]
                    *code << code->PushString("");
                    *code << code->InvokeStatic(ofString);
                    EmitValue(root, method, code, args[0]);
                    *code << code->InvokeStatic(rtImplode);
                    return;
                }

                // glue=args[0], pieces=args[1]
                EmitValue(root, method, code, args[0]);
                EmitValue(root, method, code, args[1]);

                // evaluate extras for side effects, then drop
                for (size_t i = 2; i < args.size(); i++) {
                    EmitValue(root, method, code, args[i]);
                    *code << code->PopOne();
                }

                *code << code->InvokeStatic(rtImplode);
                return;
            }

            // ---- user-defined global functions: static on program class ----
            auto *callFn = root->getOrCreateMethodrefConstant(
                root->getClassName(),
                fnName,
                DescriptorMethod(
                    DescriptorField("com/phpjvm/BasePhpValue"),
                    {
                        DescriptorField("com/phpjvm/PhpClass"),
                        DescriptorField("com/phpjvm/BasePhpValue", 1)
                    }
                )
            );

            const PhpFuncSig *sig = findFunctionSig(fnName);

            auto *basePhpValueClass = root->getOrCreateClassConstant("com/phpjvm/BasePhpValue");

            // push PhpClass (first argument of every generated PHP function)
            *code << code->PushString(root->getClassName());
            *code << code->InvokeStatic(rtRequireClass); // -> PhpClass

            *code << code->PushInt((int32_t) args.size());
            *code << code->NewArray(basePhpValueClass);

            for (size_t i = 0; i < args.size(); i++) {
                *code << code->Duplicate();
                *code << code->PushInt((int32_t) i);

                bool wantByRef = false;
                std::vector<std::string> wantTypes;
                if (sig && i < sig->params.size()) {
                    wantByRef = sig->params[i].byRef;
                    wantTypes = sig->params[i].types;
                }

                const ExprNode *ai = args[i];

                if (wantByRef) {
                    if (ai && ai->type == ExprType::ET_SIGIL) {
                        const ExprNode *id = childOrNull(ai, 0);
                        std::string var = idText(id);
                        std::string fieldName = sanitizeJavaIdent(var);

                        std::string hostDot = root->getClassName();
                        for (char &c: hostDot) if (c == '/') c = '.';

                        *code << code->PushString(hostDot);
                        *code << code->PushString(fieldName);
                        *code << code->InvokeStatic(rtMakeGlobalRef);
                    } else {
                        EmitValue(root, method, code, ai);
                    }
                } else {
                    EmitValue(root, method, code, ai);

                    if (sig && i < sig->params.size() && !wantTypes.empty()) {
                        emitStringArrayConst(root, code, wantTypes);
                        *code << code->PushString(fnName);
                        *code << code->PushString(std::string("arg") + std::to_string(i + 1));
                        *code << code->InvokeStatic(rtAssertParamType);
                    }
                }

                *code << code->StoreReferenceToArray();
            }

            *code << code->InvokeStatic(callFn);

            if (sig && !sig->returnTypes.empty()) {
                emitStringArrayConst(root, code, sig->returnTypes);
                *code << code->PushString(fnName);
                *code << code->InvokeStatic(rtAssertReturnType);
            }

            return;
        }

        case ExprType::ET_METHOD_ACCESS: {
            const ExprNode *objExpr = childOrNull(expr, 0);
            const ExprNode *nameExpr = childOrNull(expr, 1);

            std::string methodName = lowerCopy(idText(nameExpr));
            if (methodName.empty()) {
                Console::Warning("ET_METHOD_ACCESS: missing method name (pushing NULL)");
                *code << code->GetStatic(nullValueField);
                return;
            }

            auto *basePhpValueClass = root->getOrCreateClassConstant("com/phpjvm/BasePhpValue");

            // object -> PhpObject
            EmitValue(root, method, code, objExpr); // BasePhpValue
            *code << code->InvokeVirtual(asObject); // PhpObject

            // callMethodCtx(obj, "method", [], callerCtx)
            *code << code->PushString(methodName);
            *code << code->PushInt(0);
            *code << code->NewArray(basePhpValueClass); // BasePhpValue[0]
            *code << code->PushString(ExprBuilder::PhpCallerClass(method));
            *code << code->InvokeStatic(rtCallMethodCtx);
            return;
        }

        case ExprType::ET_ID: {
            std::string name = idText(expr);

            if (name == "STDIN" || name == "STDOUT" || name == "STDERR") {
                *code << code->PushString(name);
                *code << code->InvokeStatic(ofString);
                return;
            }

            *code << code->PushString(name);
            *code << code->InvokeStatic(ofString);
            return;
        }

        case ExprType::ET_AND_LOWER: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), boolAnd,
                       &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_OR_LOWER: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), boolOr, &ExprBuilder::EmitValue);
            return;
        }

        case ExprType::ET_UPLUS: {
            EmitValue(root, method, code, childOrNull(expr, 0));
            return;
        }

        case ExprType::ET_UMINUS: {
            *code << code->PushLong(0);
            *code << code->InvokeStatic(ofLong);
            EmitValue(root, method, code, childOrNull(expr, 0));
            *code << code->InvokeStatic(sub);
            return;
        }

        case ExprType::ET_NOT_BITWISE: {
            emitUnary(root, method, code, childOrNull(expr, 0), bitNot, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_AND_BITWISE: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), bitAnd, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_OR_BITWISE: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), bitOr, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_XOR_BITWISE: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), bitXor, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_LEFT_SHIFT: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), shl, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_RIGHT_SHIFT: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), shr, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_POW: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), powOp, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_XOR: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), xorOp, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_INSTANCEOF: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), instanceOfOp,
                       &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_NOT_EQUAL_BITWISE: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), ne, &ExprBuilder::EmitValue);
            return;
        }

        case ExprType::ET_PLUS_ASSIGN:
        case ExprType::ET_MINUS_ASSIGN:
        case ExprType::ET_MULT_ASSIGN:
        case ExprType::ET_DIV_ASSIGN:
        case ExprType::ET_MOD_ASSIGN:
        case ExprType::ET_CONCAT_ASSIGN:
        case ExprType::ET_LEFT_SHIFT_ASSIGN:
        case ExprType::ET_RIGHT_SHIFT_ASSIGN:
        case ExprType::ET_POW_ASSIGN: {
            const ExprNode *lhs = childOrNull(expr, 0);
            const ExprNode *rhs = childOrNull(expr, 1);

            if (!lhs || lhs->type != ExprType::ET_SIGIL) {
                Console::Warning(std::string("Compound assignment only implemented for $var: ") + toString(expr->type));
                *code << code->GetStatic(nullValueField);
                return;
            }

            const ExprNode *id = childOrNull(lhs, 0);
            std::string var = idText(id);

            uint16_t localIndex = 0;
            bool isLocal = allocLocalIfInScope(method, var, localIndex);

            if (isLocal) {
                emitLoadLocalVar(code, localIndex, nullValueField);
            } else {
                auto *fieldRef = ensureGlobalVarField(root, var);
                emitLoadGlobalVar(root, code, fieldRef, nullValueField);
            }

            EmitValue(root, method, code, rhs);

            ConstantMethodref *op = nullptr;
            switch (expr->type) {
                case ExprType::ET_PLUS_ASSIGN: op = add;
                    break;
                case ExprType::ET_MINUS_ASSIGN: op = sub;
                    break;
                case ExprType::ET_MULT_ASSIGN: op = mul;
                    break;
                case ExprType::ET_DIV_ASSIGN: op = div;
                    break;
                case ExprType::ET_MOD_ASSIGN: op = mod;
                    break;
                case ExprType::ET_CONCAT_ASSIGN: op = concat;
                    break;
                case ExprType::ET_LEFT_SHIFT_ASSIGN: op = shl;
                    break;
                case ExprType::ET_RIGHT_SHIFT_ASSIGN: op = shr;
                    break;
                case ExprType::ET_POW_ASSIGN: op = powOp;
                    break;
                default: break;
            }

            if (!op) {
                Console::Warning("Compound assignment op missing (pushing NULL)");
                *code << code->GetStatic(nullValueField);
                return;
            }

            *code << code->InvokeStatic(op);
            *code << code->Duplicate();

            if (isLocal) {
                *code << code->StoreReference(localIndex);
            } else {
                auto *fieldRef = ensureGlobalVarField(root, var);
                *code << code->PutStatic(fieldRef);
            }
            return;
        }

        case ExprType::ET_INCREMENT_PRE:
        case ExprType::ET_DECREMENT_PRE:
        case ExprType::ET_INCREMENT_POST:
        case ExprType::ET_DECREMENT_POST: {
            const ExprNode *lhs = childOrNull(expr, 0);

            if (!lhs || lhs->type != ExprType::ET_SIGIL) {
                Console::Warning(std::string("Inc/dec only implemented for $var: ") + toString(expr->type));
                *code << code->GetStatic(nullValueField);
                return;
            }

            const ExprNode *id = childOrNull(lhs, 0);
            std::string var = idText(id);

            uint16_t localIndex = 0;
            bool isLocal = allocLocalIfInScope(method, var, localIndex);
            auto *fieldRef = isLocal ? nullptr : ensureGlobalVarField(root, var);

            bool isInc =
                    (expr->type == ExprType::ET_INCREMENT_PRE) ||
                    (expr->type == ExprType::ET_INCREMENT_POST);

            bool isPost =
                    (expr->type == ExprType::ET_INCREMENT_POST) ||
                    (expr->type == ExprType::ET_DECREMENT_POST);

            auto loadVar = [&]() {
                if (isLocal) emitLoadLocalVar(code, localIndex, nullValueField);
                else emitLoadGlobalVar(root, code, fieldRef, nullValueField);
            };

            auto storeVar = [&]() {
                if (isLocal) {
                    *code << code->StoreReference(localIndex);
                } else {
                    *code << code->PutStatic(fieldRef);
                }
            };

            if (isPost) {
                loadVar(); // old
                *code << code->Duplicate(); // old old

                *code << code->PushLong(1);
                *code << code->InvokeStatic(ofLong);

                *code << code->InvokeStatic(isInc ? add : sub); // old new

                *code << code->Duplicate(); // old new new
                storeVar(); // store new
                *code << code->PopOne(); // drop new, leave old
                return;
            } else {
                loadVar();

                *code << code->PushLong(1);
                *code << code->InvokeStatic(ofLong);

                *code << code->InvokeStatic(isInc ? add : sub); // new
                *code << code->Duplicate();
                storeVar();
                return;
            }
        }

        case ExprType::ET_STATIC_PROPERTY_ACCESS: {
            const ExprNode *classExpr = childOrNull(expr, 0);
            const ExprNode *memberExpr = childOrNull(expr, 1);

            std::string classToken = lowerCopy(idText(classExpr));
            if (classToken.empty()) {
                Console::Warning("ET_STATIC_PROPERTY_ACCESS: missing class token (pushing NULL)");
                *code << code->GetStatic(nullValueField);
                return;
            }

            bool isStaticProp = (memberExpr && memberExpr->type == ExprType::ET_SIGIL);
            std::string memberName = isStaticProp ? idText(childOrNull(memberExpr, 0)) : idText(memberExpr);

            if (memberName.empty()) {
                Console::Warning("ET_STATIC_PROPERTY_ACCESS: missing member name (pushing NULL)");
                *code << code->GetStatic(nullValueField);
                return;
            }

            std::string callerCtx = ExprBuilder::PhpCallerClass(method);

            if (isStaticProp) {
                // ---- static property read ----
                if (classToken == "parent") {
                    *code << code->PushString(root->getClassName()); // current class (lexical)
                    *code << code->PushString(memberName); // prop name (no $)
                    *code << code->PushString(callerCtx);
                    *code << code->InvokeStatic(rtGetParentStaticPropCtx);
                    return;
                }

                // resolve called class to PhpClass on stack
                if (classToken == "self") {
                    *code << code->PushString(root->getClassName());
                    *code << code->InvokeStatic(rtRequireClass); // PhpClass
                } else if (classToken == "static") {
                    if (ExprBuilder::PhpMethodHasThis(method)) {
                        *code << code->LoadReference(ExprBuilder::PhpThisLocalSlot()); // PhpObject
                        *code << code->InvokeVirtual(objGetPhpClass); // PhpClass
                    } else {
                        *code << code->LoadReference(0); // PhpClass param in static methods
                    }
                } else {
                    *code << code->PushString(classToken);
                    *code << code->InvokeStatic(rtRequireClass); // PhpClass
                }

                *code << code->PushString(memberName);
                *code << code->PushString(callerCtx);
                *code << code->InvokeStatic(rtGetStaticPropCtx);
                return;
            } else {
                // ---- class constant read ----
                if (classToken == "parent") {
                    *code << code->PushString(root->getClassName()); // current class (lexical)
                    *code << code->PushString(memberName); // const name
                    *code << code->PushString(callerCtx);
                    *code << code->InvokeStatic(rtGetParentConstCtx);
                    return;
                }

                if (classToken == "self") {
                    *code << code->PushString(root->getClassName());
                    *code << code->InvokeStatic(rtRequireClass);
                } else if (classToken == "static") {
                    if (ExprBuilder::PhpMethodHasThis(method)) {
                        *code << code->LoadReference(ExprBuilder::PhpThisLocalSlot());
                        *code << code->InvokeVirtual(objGetPhpClass);
                    } else {
                        *code << code->LoadReference(0);
                    }
                } else {
                    *code << code->PushString(classToken);
                    *code << code->InvokeStatic(rtRequireClass);
                }

                *code << code->PushString(memberName);
                *code << code->PushString(callerCtx);
                *code << code->InvokeStatic(rtGetConstCtx);
                return;
            }
        }

        default:
            Console::Warning("ExprBuilder::EmitValue not implemented for " + toString(expr->type) + " (pushing NULL)");
            *code << code->GetStatic(nullValueField);
            return;
    }
}
