#include "ExprBuilder.h"

#include <sstream>
#include <iomanip>
#include <vector>

#include "core/helpers/Console.h"
#include "jvm/descriptor-field.h"
#include "jvm/descriptor-method.h"
#include "jvm/field.h"
#include <unordered_map>
#include <utility>

using namespace jvm;

struct LocalScopeInfo {
    std::unordered_map<std::string, uint16_t> slots;
    uint16_t nextFree = 0;
};

static std::unordered_map<const jvm::Method *, LocalScopeInfo> g_localScopes;

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

void ExprBuilder::EndLocalScope(jvm::Method *method) {
    if (!method) return;
    g_localScopes.erase(method);
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

static std::string toLowerAscii(std::string s) {
    for (char &c: s) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    return s;
}

// PHP namespaces '\' -> JVM '/' and make it case-insensitive like PHP classes
static std::string toJvmInternalNameExpr(std::string s) {
    s = toLowerAscii(s);
    for (char &c: s) {
        if (c == '\\') c = '/';
    }
    while (!s.empty() && s.front() == '/') s.erase(s.begin());
    return s;
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
    emitValue(root, method, code, left); // push BasePhpValue
    emitValue(root, method, code, right); // push BasePhpValue
    *code << code->InvokeStatic(op); // pop2 push1
}

static void emitUnary(
    Class *root,
    Method *method,
    AttributeCode *code,
    const ExprNode *operand,
    ConstantMethodref *op,
    void (*emitValue)(Class *, Method *, AttributeCode *, const ExprNode *)
) {
    emitValue(root, method, code, operand); // push BasePhpValue
    *code << code->InvokeStatic(op); // pop1 push1
}

static std::string sanitizeJavaIdentLocal(const std::string &raw) {
    std::string s = raw;
    if (!s.empty() && s[0] == '$') s.erase(0, 1);

    std::string out;
    out.reserve(s.size() + 4);
    out += "g_"; // global var prefix

    for (char c: s) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') out.push_back(c);
        else out.push_back('_');
    }

    if (out.size() == 2) out += "var"; // "g_" only
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

// pushes current value of $var (NULL_VALUE if field is null)
static void emitLoadGlobalVar(
    Class *root,
    AttributeCode *code,
    ConstantFieldref *fieldRef,
    ConstantFieldref *nullValueField
) {
    auto *L_nonNull = code->CodeLabel();
    auto *L_end = code->CodeLabel();

    *code << code->GetStatic(fieldRef); // v
    *code << code->Duplicate(); // v v
    *code << code->IfNotNull(L_nonNull); // pops one v; stack: v
    *code << code->PopOne(); // pop null
    *code << code->GetStatic(nullValueField);
    *code << code->GoTo(L_end);

    *code << L_nonNull;
    *code << L_end;
}

static std::string idText(const ExprNode *e) {
    if (!e) return "";
    if (!e->name.empty()) return e->name;
    if (e->value) {
        // in your semantics you sometimes use value->name for ET_ID
        if (!e->value->name.empty()) return e->value->name;
        // fallback if you ever store it differently
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
    out += "g_"; // global var prefix

    for (char c: s) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_') out.push_back(c);
        else out.push_back('_');
    }

    if (out.size() == 2) out += "var"; // "g_" only
    if (out.size() >= 3 && (out[2] >= '0' && out[2] <= '9')) out.insert(out.begin() + 2, '_');
    return out;
}

// Emits BasePhpValue[] on stack
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
    *code << code->NewArray(basePhpValueClass); // BasePhpValue[]

    for (size_t i = 0; i < args.size(); i++) {
        *code << code->Duplicate(); // arr, arr
        *code << code->PushInt((int32_t) i); // arr, arr, i
        ExprBuilder::EmitValue(root, method, code, args[i]); // arr, arr, i, val
        *code << code->StoreReferenceToArray(); // arr
    }
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
            {DescriptorField("Z")} // boolean
        )
    );

    auto *ofLong = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "of",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("J")} // long
        )
    );

    auto *ofDouble = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "of",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("D")} // double
        )
    );

    // arithmetic / concat already exist in your RTL
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

    // comparisons you already have:
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

    // NEW helpers you add to RTL in step (1):
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

    // BasePhpValue instance helpers
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

    // PhpRuntime calls
    auto *rtCallMethod = root->getOrCreateMethodrefConstant(
        "com/phpjvm/PhpRuntime",
        "callMethod",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {
                DescriptorField("com/phpjvm/PhpObject"),
                DescriptorField("java/lang/String"),
                DescriptorField("com/phpjvm/BasePhpValue", 1) // BasePhpValue[]
            }
        )
    );

    auto *rtCallStatic = root->getOrCreateMethodrefConstant(
        "com/phpjvm/PhpRuntime",
        "callStatic",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {
                DescriptorField("com/phpjvm/PhpClass"),
                DescriptorField("java/lang/String"),
                DescriptorField("com/phpjvm/BasePhpValue", 1)
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

    // PhpObject property access
    auto *objGetProp = root->getOrCreateMethodrefConstant(
        "com/phpjvm/PhpObject",
        "getProperty",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("java/lang/String")}
        )
    );

    // Arrays
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
            std::nullopt, // void
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
            std::nullopt, // void
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
                DescriptorField("com/phpjvm/BasePhpValue", 1) // BasePhpValue[]
            }
        )
    );


    if (expr == nullptr) {
        *code << code->GetStatic(nullValueField);
        return;
    }

    // ---------- Dispatch ----------
    switch (expr->type) {
        case ExprType::ET_EXPR_LIST: {
            if (expr->children.empty()) {
                *code << code->GetStatic(nullValueField);
                return;
            }

            for (size_t i = 0; i < expr->children.size(); i++) {
                EmitValue(root, method, code, expr->children[i]);
                if (i + 1 < expr->children.size()) {
                    *code << code->PopOne(); // discard intermediate results
                }
            }
            return; // last value remains
        }

        // ===== literals (already working) =====
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

        // ===== parentheses =====
        case ExprType::ET_PARENTHESIZED: {
            const ExprNode *inner = childOrNull(expr, 0);
            EmitValue(root, method, code, inner);
            return;
        }

        // ===== concat list (already in your code) =====
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

        // ===== arithmetic =====
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

        // ===== string concat "." =====
        case ExprType::ET_CONCAT: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), concat, &ExprBuilder::EmitValue);
            return;
        }

        // ===== comparisons =====
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

        // ===== boolean ops (truthiness) =====
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

        // ===== variables: $x =====
        case ExprType::ET_SIGIL: {
            const ExprNode *id = childOrNull(expr, 0);
            std::string var = idText(id);

            // If we are inside a function/method local scope and this var is known -> load local
            uint16_t localIndex = 0;
            if (ExprBuilder::TryGetLocal(method, var, localIndex)) {
                auto *L_nonNull = code->CodeLabel();
                auto *L_end = code->CodeLabel();

                *code << code->LoadReference(localIndex); // v
                *code << code->Duplicate(); // v v
                *code << code->IfNotNull(L_nonNull); // pops one v; stack: v
                *code << code->PopOne(); // pop null
                *code << code->GetStatic(nullValueField); // NULL_VALUE
                *code << code->GoTo(L_end);

                *code << L_nonNull;
                *code << L_end;
                return;
            }

            // Otherwise fallback to global static field behavior
            std::string fieldName = sanitizeJavaIdent(var);

            Field *f = root->getOrCreateField(fieldName, DescriptorField("com/phpjvm/BasePhpValue"));
            f->addFlag(Field::ACC_PUBLIC);
            f->addFlag(Field::ACC_STATIC);

            auto *fieldRef = root->getOrCreateFieldrefConstant(
                root->getClassName(),
                fieldName,
                DescriptorField("com/phpjvm/BasePhpValue")
            );

            auto *L_nonNull = code->CodeLabel();
            auto *L_end = code->CodeLabel();

            *code << code->GetStatic(fieldRef); // v
            *code << code->Duplicate(); // v v
            *code << code->IfNotNull(L_nonNull);
            *code << code->PopOne(); // pop null
            *code << code->GetStatic(nullValueField);
            *code << code->GoTo(L_end);

            *code << L_nonNull;
            *code << L_end;
            return;
        }

        // ===== assignment: $x = rhs =====
        case ExprType::ET_ASSIGN: {
            const ExprNode *lhs = childOrNull(expr, 0);
            const ExprNode *rhs = childOrNull(expr, 1);

            if (lhs && lhs->type == ExprType::ET_SIGIL) {
                const ExprNode *id = childOrNull(lhs, 0);
                std::string var = idText(id);

                // Prefer local if in scope (or allocate local if in scope)
                uint16_t localIndex = 0;
                if (ExprBuilder::TryGetLocal(method, var, localIndex) || allocLocalIfInScope(method, var, localIndex)) {
                    EmitValue(root, method, code, rhs); // value
                    *code << code->Duplicate(); // value value
                    *code << code->StoreReference(localIndex); // value
                    return;
                }

                // Otherwise global
                std::string fieldName = sanitizeJavaIdent(var);

                Field *f = root->getOrCreateField(fieldName, DescriptorField("com/phpjvm/BasePhpValue"));
                f->addFlag(Field::ACC_PUBLIC);
                f->addFlag(Field::ACC_STATIC);

                auto *fieldRef = root->getOrCreateFieldrefConstant(
                    root->getClassName(),
                    fieldName,
                    DescriptorField("com/phpjvm/BasePhpValue")
                );

                EmitValue(root, method, code, rhs);
                *code << code->Duplicate();
                *code << code->PutStatic(fieldRef);
                return;
            }

            Console::Warning("ET_ASSIGN bytecode: only $var = expr implemented (pushing NULL)");
            *code << code->GetStatic(nullValueField);
            return;
        }

        // ===== method call: $obj->m(a,b) =====
        case ExprType::ET_METHOD_ACCESS: {
            const ExprNode *objExpr = childOrNull(expr, 0);
            const ExprNode *nameExpr = childOrNull(expr, 1);
            const ExprNode *argsNode = childOrNull(expr, 2);

            std::string methodName = idText(nameExpr);
            if (methodName.empty()) {
                Console::Warning("ET_METHOD_ACCESS: missing method name (pushing NULL)");
                *code << code->GetStatic(nullValueField);
                return;
            }

            EmitValue(root, method, code, objExpr); // BasePhpValue (object)
            *code << code->InvokeVirtual(asObject); // PhpObject

            *code << code->PushString(methodName); // String
            emitArgsArray(root, method, code, argsNode); // BasePhpValue[]

            *code << code->InvokeStatic(rtCallMethod); // BasePhpValue
            return;
        }

        // ===== property read: $obj->prop =====
        case ExprType::ET_PROPERTY_ACCESS: {
            const ExprNode *objExpr = childOrNull(expr, 0);
            const ExprNode *nameExpr = childOrNull(expr, 1);

            std::string propName = idText(nameExpr);
            if (propName.empty()) {
                Console::Warning("ET_PROPERTY_ACCESS: missing property name (pushing NULL)");
                *code << code->GetStatic(nullValueField);
                return;
            }

            EmitValue(root, method, code, objExpr); // BasePhpValue
            *code << code->InvokeVirtual(asObject); // PhpObject
            *code << code->PushString(propName); // String
            *code << code->InvokeVirtual(objGetProp); // BasePhpValue
            return;
        }

        // ===== new Foo(a,b) =====
        case ExprType::ET_NEW: {
            const ExprNode *classNameExpr = nullptr;
            const ExprNode *argsNode = nullptr;

            if (expr->children.size() == 2) {
                classNameExpr = childOrNull(expr, 0);
                argsNode = childOrNull(expr, 1);
            } else if (expr->children.size() == 1) {
                // you have New(args) overload; without class name we can't resolve it safely yet
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
            *code << code->InvokeStatic(rtNewObject); // BasePhpValue (OBJECT)
            return;
        }

        // ===== arrays: [] =====
        case ExprType::ET_ARRAY_EMPTY: {
            *code << code->InvokeStatic(arrFactory); // BasePhpValue array
            return;
        }

        // ===== arrays: $arr[$k] =====
        case ExprType::ET_ARRAY_INDEX: {
            EmitValue(root, method, code, childOrNull(expr, 0)); // arr
            EmitValue(root, method, code, childOrNull(expr, 1)); // key
            *code << code->InvokeStatic(arrGet); // value
            return;
        }

        // ===== arrays: [$k => $v] pair node (used inside list) =====
        case ExprType::ET_ARRAY_KEY_ACCESS: {
            // This node is usually handled by ET_ARRAY_ELEMENT_LIST,
            // but if it appears alone, make a single-element array.
            *code << code->InvokeStatic(arrFactory); // arr
            *code << code->Duplicate(); // arr arr
            EmitValue(root, method, code, childOrNull(expr, 0)); // arr arr key
            EmitValue(root, method, code, childOrNull(expr, 1)); // arr arr key val
            *code << code->InvokeStatic(arrSet); // arr
            return;
        }

        // ===== arrays: element list =====
        case ExprType::ET_ARRAY_ELEMENT_LIST: {
            // children[0] is "elements" (often ET_EXPR_LIST)
            const ExprNode *elements = childOrNull(expr, 0);

            *code << code->InvokeStatic(arrFactory); // arr

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
                    *code << code->Duplicate(); // arr arr
                    EmitValue(root, method, code, childOrNull(p, 0)); // key
                    EmitValue(root, method, code, childOrNull(p, 1)); // val
                    *code << code->InvokeStatic(arrSet); // arr
                } else {
                    *code << code->Duplicate(); // arr arr
                    EmitValue(root, method, code, p); // arr arr val
                    *code << code->InvokeStatic(arrAppend); // arr
                }
            }
            return;
        }

        // ===== arrays: $arr[] = $v OR array_append expr node =====
        case ExprType::ET_ARRAY_APPEND: {
            EmitValue(root, method, code, childOrNull(expr, 0)); // arr
            *code << code->Duplicate(); // arr arr
            EmitValue(root, method, code, childOrNull(expr, 1)); // arr arr val
            *code << code->InvokeStatic(arrAppend); // arr
            return;
        }

        // ===== arrays: $arr[$k] = $v (your semantics rewrites ET_ASSIGN into this) =====
        case ExprType::ET_ARRAY_ASSIGNMENT: {
            EmitValue(root, method, code, childOrNull(expr, 0)); // arr
            *code << code->Duplicate(); // arr arr
            EmitValue(root, method, code, childOrNull(expr, 1)); // key
            EmitValue(root, method, code, childOrNull(expr, 2)); // val
            *code << code->InvokeStatic(arrSet); // arr
            return;
        }

        // ===== ternary: cond ? a : b =====
        case ExprType::ET_TERNARY: {
            const ExprNode *cond = childOrNull(expr, 0);
            const ExprNode *a = childOrNull(expr, 1);
            const ExprNode *b = childOrNull(expr, 2);

            auto *L_false = code->CodeLabel();
            auto *L_end = code->CodeLabel();

            EmitValue(root, method, code, cond); // BasePhpValue
            *code << code->InvokeVirtual(toBool); // int(0/1)
            *code << code->If(Instruction::Compare::Equal, L_false); // if 0 -> false

            EmitValue(root, method, code, a);
            *code << code->GoTo(L_end);

            *code << L_false;
            EmitValue(root, method, code, b);

            *code << L_end;
            return;
        }

        // ===== null coalescing: a ?? b =====
        case ExprType::ET_NULL_COALESCING: {
            const ExprNode *a = childOrNull(expr, 0);
            const ExprNode *b = childOrNull(expr, 1);

            auto *L_isNull = code->CodeLabel();
            auto *L_end = code->CodeLabel();

            EmitValue(root, method, code, a); // v
            *code << code->Duplicate(); // v v
            *code << code->InvokeVirtual(isNull); // v bool
            *code << code->If(Instruction::Compare::NotEqual, L_isNull); // if true -> jump; pops bool; leaves v

            // not null: keep v
            *code << code->GoTo(L_end);

            *code << L_isNull;
            *code << code->PopOne(); // drop v (it is NULL)
            EmitValue(root, method, code, b); // push b

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

            // ET_FUNCTION_CALL(ET_NEW(...), args) -> newObject
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
                emitArgsArray(root, method, code, argsNode); // args ignored by runtime for now
                *code << code->InvokeStatic(rtCallFunction);
                return;
            }

            // ---- user-defined global functions: static on program class ----
            auto *callFn = root->getOrCreateMethodrefConstant(
                root->getClassName(),
                fnName,
                DescriptorMethod(
                    DescriptorField("com/phpjvm/BasePhpValue"),
                    {DescriptorField("com/phpjvm/BasePhpValue", 1)}
                )
            );

            emitArgsArray(root, method, code, argsNode);
            *code << code->InvokeStatic(callFn);
            return;
        }

        // ===== identifier constant (e.g. STDIN) =====
        case ExprType::ET_ID: {
            std::string name = idText(expr);

            // Special stream constants (you can extend this later)
            if (name == "STDIN" || name == "STDOUT" || name == "STDERR") {
                *code << code->PushString(name);
                *code << code->InvokeStatic(ofString); // BasePhpValue.of(String)
                return;
            }

            // If a bare identifier ever appears as an expression, treat it as a string constant for now.
            // (Better: add a proper "constant table" / define() support later.)
            *code << code->PushString(name);
            *code << code->InvokeStatic(ofString);
            return;
        }

        // ===== "and/or" lower-precedence tokens: map to same runtime helpers for now =====
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
            *code << code->InvokeStatic(sub); // 0 - x
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
            // PHP "<>" is the same as "!="
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

            // Local?
            uint16_t localIndex = 0;
            if (ExprBuilder::TryGetLocal(method, var, localIndex)) {
                // load current
                *code << code->LoadReference(localIndex);
                // rhs
                EmitValue(root, method, code, rhs);
                // apply op
                *code << code->InvokeStatic(op);
                // store + return value
                *code << code->Duplicate();
                *code << code->StoreReference(localIndex);
                return;
            }

            // Global fallback
            auto *fieldRef = ensureGlobalVarField(root, var);
            emitLoadGlobalVar(root, code, fieldRef, nullValueField);

            EmitValue(root, method, code, rhs);

            *code << code->InvokeStatic(op);
            *code << code->Duplicate();
            *code << code->PutStatic(fieldRef);
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

            bool isInc =
                    (expr->type == ExprType::ET_INCREMENT_PRE) ||
                    (expr->type == ExprType::ET_INCREMENT_POST);

            bool isPost =
                    (expr->type == ExprType::ET_INCREMENT_POST) ||
                    (expr->type == ExprType::ET_DECREMENT_POST);

            // Local?
            uint16_t localIndex = 0;
            if (ExprBuilder::TryGetLocal(method, var, localIndex)) {
                if (isPost) {
                    *code << code->LoadReference(localIndex); // old
                    *code << code->Duplicate(); // old old

                    *code << code->PushLong(1);
                    *code << code->InvokeStatic(ofLong); // old old one

                    *code << code->InvokeStatic(isInc ? add : sub); // old new

                    *code << code->Duplicate(); // old new new
                    *code << code->StoreReference(localIndex); // old new
                    *code << code->PopOne(); // old
                    return;
                } else {
                    *code << code->LoadReference(localIndex); // old

                    *code << code->PushLong(1);
                    *code << code->InvokeStatic(ofLong); // old one

                    *code << code->InvokeStatic(isInc ? add : sub); // new
                    *code << code->Duplicate(); // new new
                    *code << code->StoreReference(localIndex); // new
                    return;
                }
            }

            // Global fallback
            auto *fieldRef = ensureGlobalVarField(root, var);

            if (isPost) {
                emitLoadGlobalVar(root, code, fieldRef, nullValueField); // old
                *code << code->Duplicate(); // old old

                *code << code->PushLong(1);
                *code << code->InvokeStatic(ofLong); // old old one

                *code << code->InvokeStatic(isInc ? add : sub); // old new

                *code << code->Duplicate(); // old new new
                *code << code->PutStatic(fieldRef); // old new
                *code << code->PopOne(); // old
                return;
            } else {
                emitLoadGlobalVar(root, code, fieldRef, nullValueField); // old

                *code << code->PushLong(1);
                *code << code->InvokeStatic(ofLong); // old one

                *code << code->InvokeStatic(isInc ? add : sub); // new
                *code << code->Duplicate();
                *code << code->PutStatic(fieldRef);
                return;
            }
        }

        case ExprType::ET_STATIC_PROPERTY_ACCESS: {
            const ExprNode *classExpr = childOrNull(expr, 0);
            const ExprNode *nameExpr = childOrNull(expr, 1);

            std::string clsName = idText(classExpr);
            std::string member = idText(nameExpr);

            if (clsName.empty() || member.empty()) {
                Console::Warning("ET_STATIC_PROPERTY_ACCESS: missing class or member (pushing NULL)");
                *code << code->GetStatic(nullValueField);
                return;
            }

            // Your DeclNode lowercases JVM class names (PHP class names are case-insensitive)
            std::string owner = toJvmInternalNameExpr(clsName); // <- use the helper above
            // NOTE: constants/fields ARE case-sensitive, so keep member as-is ("X")

            auto *fieldRef = root->getOrCreateFieldrefConstant(
                owner,
                member,
                DescriptorField("com/phpjvm/BasePhpValue")
            );

            // Load static; if null -> BasePhpValue.NULL_VALUE (safety)
            auto *L_nonNull = code->CodeLabel();
            auto *L_end = code->CodeLabel();

            *code << code->GetStatic(fieldRef); // v
            *code << code->Duplicate(); // v v
            *code << code->IfNotNull(L_nonNull); // pops one v; stack: v
            *code << code->PopOne(); // pop null
            *code << code->GetStatic(nullValueField);
            *code << code->GoTo(L_end);

            *code << L_nonNull;
            *code << L_end;
            return;
        }

        default:
            Console::Warning("ExprBuilder::EmitValue not implemented for " + toString(expr->type) + " (pushing NULL)");
            *code << code->GetStatic(nullValueField);
            return;
    }
}
