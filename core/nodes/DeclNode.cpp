#include "DeclNode.h"
#include "core/helpers/Console.h"
#include "json.hpp"
#include <string>
#include <cctype>
#include <vector>

#include "jvm/class.h"
#include "jvm/method.h"
#include "jvm/attribute-code.h"
#include "jvm/descriptor-field.h"
#include "jvm/descriptor-method.h"
#include "jvm/field.h"

#include "core/bytecode/ExprBuilder.h"

using json = nlohmann::json;
using namespace jvm;

static std::string toLowerAscii(std::string s) {
    for (char &c: s) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    return s;
}

// Add near the top of DeclNode.cpp (helpers)
static std::string sanitizeJavaMemberIdent(const std::string &raw) {
    std::string s = raw;
    if (!s.empty() && s[0] == '$') s.erase(s.begin());

    std::string out;
    out.reserve(s.size() + 2);

    for (char c: s) {
        if ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            c == '_') {
            out.push_back(c);
        } else {
            out.push_back('_');
        }
    }

    if (out.empty()) out = "member";
    if (out[0] >= '0' && out[0] <= '9') out.insert(out.begin(), '_');
    return out;
}

static Method *getOrCreateClinit(Class *cls) {
    Method *m = cls->getOrCreateMethod("<clinit>", DescriptorMethod(std::nullopt, {}));
    m->addFlag(Method::ACC_STATIC);
    return m;
}

// PHP namespaces '\' -> JVM '/' and make it case-insensitive like PHP classes
static std::string toJvmInternalName(std::string s) {
    s = toLowerAscii(s);
    for (char &c: s) {
        if (c == '\\') c = '/';
    }
    while (!s.empty() && s.front() == '/') s.erase(s.begin());
    return s;
}

static DescriptorMethod descPhpFunction() {
    // BasePhpValue f(BasePhpValue[] args)
    return DescriptorMethod(
        DescriptorField("com/phpjvm/BasePhpValue"),
        {DescriptorField("com/phpjvm/BasePhpValue", 1)}
    );
}

static DescriptorMethod descPhpInstanceMethod() {
    // BasePhpValue m(PhpObject self, BasePhpValue[] args)
    return DescriptorMethod(
        DescriptorField("com/phpjvm/BasePhpValue"),
        {DescriptorField("com/phpjvm/PhpObject"), DescriptorField("com/phpjvm/BasePhpValue", 1)}
    );
}

static DescriptorMethod descPhpStaticMethod() {
    // BasePhpValue m(PhpClass calledClass, BasePhpValue[] args)
    return DescriptorMethod(
        DescriptorField("com/phpjvm/BasePhpValue"),
        {DescriptorField("com/phpjvm/PhpClass"), DescriptorField("com/phpjvm/BasePhpValue", 1)}
    );
}

static void emitDefaultCtor(Class *cls, const std::string &superInternalName) {
    Method *init = cls->getOrCreateMethod("<init>", DescriptorMethod(std::nullopt, {}));
    init->addFlag(Method::ACC_PUBLIC);

    AttributeCode *code = init->getCodeAttribute();

    const std::string superName = superInternalName.empty()
                                      ? "java/lang/Object"
                                      : superInternalName;

    auto *superInit = cls->getOrCreateMethodrefConstant(
        superName,
        "<init>",
        DescriptorMethod(std::nullopt, {})
    );

    *code << code->LoadReference(0); // aload_0
    *code << code->InvokeSpecial(superInit); // invokespecial super.<init>
    *code << code->ReturnVoid();
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

// Emit: String[] {"int","string",...} for union types.
// NOTE: This assumes ValueNode has: type == TYPE_ARRAY and `elements` list where each element has `.name`.
// If your ValueNode differs, adapt this helper.
static std::vector<std::string> collectTypeNames(ValueNode *vt) {
    std::vector<std::string> out;
    if (!vt) return out;

    // Common layout in your JSON: TYPE_ARRAY with elements [{name:"int"}, {name:"string"}]
    // Try to read vt->elements
    try {
        // If your ValueNode has a public `elements` member:
        // out.push_back(toLowerAscii(el->name));
        for (auto *el: vt->valueList) {
            if (!el) continue;
            if (!el->name.empty()) out.push_back(toLowerAscii(el->name));
        }
    } catch (...) {
        // If your ValueNode is different, you will see this at compile time.
    }
    return out;
}

static void emitStringArrayConst(Class *root, AttributeCode *code, const std::vector<std::string> &items) {
    auto *stringClass = root->getOrCreateClassConstant("java/lang/String");

    *code << code->PushInt((int32_t) items.size());
    *code << code->NewArray(stringClass); // String[]

    for (size_t i = 0; i < items.size(); i++) {
        *code << code->Duplicate(); // arr arr
        *code << code->PushInt((int32_t) i); // arr arr i
        *code << code->PushString(items[i]); // arr arr i str
        *code << code->StoreReferenceToArray(); // arr
    }
}

// Binds parameters into locals. For by-ref params, we support globals via marker strings:
// PhpRuntime.REF_PREFIX + "<hostClassDot>#<fieldName>"
//
// Layout we use:
// - For each parameter:
///   value slot: BasePhpValue
//   if by-ref: refDesc slot: java/lang/String (descriptor without prefix)
static void emitBindParamsFromArgs(
    Class *root,
    Method *m,
    AttributeCode *code,
    DeclNode *params,
    const std::string &fnName,
    uint16_t argsSlot,
    uint16_t baseLocalSlot
) {
    auto *nullValueField = root->getOrCreateFieldrefConstant(
        "com/phpjvm/BasePhpValue",
        "NULL_VALUE",
        DescriptorField("com/phpjvm/BasePhpValue")
    );

    auto *requireGlobalRef = root->getOrCreateMethodrefConstant(
        "com/phpjvm/PhpRuntime",
        "requireGlobalRef",
        DescriptorMethod(
            DescriptorField("java/lang/String"),
            {
                DescriptorField("com/phpjvm/BasePhpValue"),
                DescriptorField("java/lang/String"),
                DescriptorField("java/lang/String")
            }
        )
    );

    auto *getGlobalRefValue = root->getOrCreateMethodrefConstant(
        "com/phpjvm/PhpRuntime",
        "getGlobalRefValue",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("java/lang/String")}
        )
    );

    std::vector<DeclNode *> ps = flattenParamList(params);

    // ✅ Create scope immediately so DefineLocal works
    ExprBuilder::BeginLocalScope(m, baseLocalSlot);

    uint16_t nextSlot = baseLocalSlot;

    std::vector<ExprBuilder::ByRefPair> byRefPairs;
    byRefPairs.reserve(ps.size());

    for (size_t i = 0; i < ps.size(); i++) {
        DeclNode *p = ps[i];
        const std::string pname = p ? p->name : "";
        const bool byRef = (p && p->hasAddressOperator);

        const uint16_t valueSlot = nextSlot++;
        uint16_t refDescSlot = 0;
        if (byRef) {
            refDescSlot = nextSlot++; // String
            byRefPairs.push_back({valueSlot, refDescSlot});
        }

        // ✅ Now this actually registers
        ExprBuilder::DefineLocal(m, pname, valueSlot);

        auto *L_missing = code->CodeLabel();
        auto *L_end = code->CodeLabel();

        *code << code->LoadReference(argsSlot);
        *code << code->ArrayLength();
        *code << code->PushInt(static_cast<int32_t>(i + 1));
        *code << code->IfWithCompare(Instruction::Compare::LessThan, L_missing);

        if (!byRef) {
            *code << code->LoadReference(argsSlot);
            *code << code->PushInt(static_cast<int32_t>(i));
            *code << code->LoadReferenceFromArray();
            *code << code->StoreReference(valueSlot);
        } else {
            *code << code->LoadReference(argsSlot);
            *code << code->PushInt(static_cast<int32_t>(i));
            *code << code->LoadReferenceFromArray();

            *code << code->PushString(fnName);
            *code << code->PushString(pname);
            *code << code->InvokeStatic(requireGlobalRef);
            *code << code->StoreReference(refDescSlot);

            *code << code->LoadReference(refDescSlot);
            *code << code->InvokeStatic(getGlobalRefValue);
            *code << code->StoreReference(valueSlot);
        }

        *code << code->GoTo(L_end);

        *code << L_missing;
        if (byRef) {
            *code << code->GetStatic(nullValueField);
            *code << code->StoreReference(valueSlot);
            *code << code->PushNull();
            *code << code->StoreReference(refDescSlot);
        } else {
            if (p && p->expr) ExprBuilder::EmitValue(root, m, code, p->expr);
            else *code << code->GetStatic(nullValueField);
            *code << code->StoreReference(valueSlot);
        }

        *code << L_end;
    }

    ExprBuilder::ReserveNextLocal(m, nextSlot);
    ExprBuilder::SetByRefLayout(m, byRefPairs);
}

// Flush by-ref params back to globals (copy-out) at function end.
static void emitFlushByRefParams(
    Class *root,
    Method *m,
    AttributeCode *code,
    DeclNode *params,
    uint16_t baseLocalSlot
) {
    auto *setGlobalRefValue = root->getOrCreateMethodrefConstant(
        "com/phpjvm/PhpRuntime",
        "setGlobalRefValue",
        DescriptorMethod(
            std::nullopt,
            {DescriptorField("java/lang/String"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );

    std::vector<DeclNode *> ps = flattenParamList(params);

    uint16_t nextSlot = baseLocalSlot;
    for (size_t i = 0; i < ps.size(); i++) {
        DeclNode *p = ps[i];
        const bool byRef = (p && p->hasAddressOperator);

        const uint16_t valueSlot = nextSlot++;
        uint16_t refDescSlot = 0;
        if (byRef) refDescSlot = nextSlot++;

        if (!byRef) continue;

        auto *L_skip = code->CodeLabel();
        auto *L_done = code->CodeLabel();

        // if (refDescSlot == null) skip
        *code << code->LoadReference(refDescSlot);
        *code << code->Duplicate();
        *code << code->IfNull(L_skip);
        *code << code->PopOne();

        // PhpRuntime.setGlobalRefValue(refDesc, value)
        *code << code->LoadReference(refDescSlot);
        *code << code->LoadReference(valueSlot);
        *code << code->InvokeStatic(setGlobalRefValue);
        *code << code->GoTo(L_done);

        *code << L_skip;
        *code << code->PopOne();
        *code << L_done;
    }
}

static bool hasAnyByRefParam(DeclNode *params) {
    std::vector<DeclNode *> ps = flattenParamList(params);
    for (auto *p: ps) {
        if (p && p->hasAddressOperator) return true;
    }
    return false;
}

static uint16_t firstByRefValueSlot(DeclNode *params, uint16_t baseLocalSlot) {
    std::vector<DeclNode *> ps = flattenParamList(params);
    uint16_t nextSlot = baseLocalSlot;
    for (size_t i = 0; i < ps.size(); i++) {
        DeclNode *p = ps[i];
        const bool byRef = (p && p->hasAddressOperator);
        uint16_t valueSlot = nextSlot++;
        if (byRef) {
            // skip refdesc slot
            nextSlot++;
            return valueSlot;
        }
        // if not byRef, continue (no extra slot)
    }
    return baseLocalSlot;
}

static void emitReturnNull(Class *root, AttributeCode *code) {
    auto *nullValueField = root->getOrCreateFieldrefConstant(
        "com/phpjvm/BasePhpValue",
        "NULL_VALUE",
        DescriptorField("com/phpjvm/BasePhpValue")
    );

    *code << code->GetStatic(nullValueField);
    *code << code->ReturnReference();
}

static void applyVisibility(Method *m, VisibilityType vis) {
    switch (vis) {
        case VISIBILITY_PUBLIC: m->addFlag(Method::ACC_PUBLIC);
            break;
        case VISIBILITY_PRIVATE: m->addFlag(Method::ACC_PRIVATE);
            break;
        case VISIBILITY_PROTECTED: m->addFlag(Method::ACC_PROTECTED);
            break;
        default: m->addFlag(Method::ACC_PUBLIC);
            break;
    }
}

static void applyVisibility(Field *f, VisibilityType vis) {
    switch (vis) {
        case VISIBILITY_PUBLIC: f->addFlag(Field::ACC_PUBLIC);
            break;
        case VISIBILITY_PRIVATE: f->addFlag(Field::ACC_PRIVATE);
            break;
        case VISIBILITY_PROTECTED: f->addFlag(Field::ACC_PROTECTED);
            break;
        default: f->addFlag(Field::ACC_PUBLIC);
            break;
    }
}

string DeclNode::_getClassName() const {
    return "DeclNode";
}

string DeclNode::toJson() const {
    json j;
    j["id"] = GetId();
    j["type"] = toString(type);

    if (!name.empty()) j["name"] = name;
    if (!className.empty()) j["className"] = className;
    if (!classNameExtended.empty()) j["classNameExtended"] = classNameExtended;

    if (isStatic != -1) j["isStatic"] = isStatic;
    if (hasAddressOperator) j["hasAddressOperator"] = hasAddressOperator;

    if (visibilityType != VisibilityType::VISIBILITY_UNKNOWN) {
        j["visibilityType"] = toString(visibilityType);
    }

    if (!children.empty()) {
        json childrenArray = json::array();
        for (const auto &child: children) {
            childrenArray.push_back(json::parse(child->toJson()));
        }
        j["children"] = childrenArray;
    }

    if (declList != nullptr) j["declList"] = json::parse(declList->toJson());
    if (params != nullptr) j["params"] = json::parse(params->toJson());
    if (expr != nullptr) j["expr"] = json::parse(expr->toJson());
    if (stmt != nullptr) j["stmt"] = json::parse(stmt->toJson());
    if (valueType != nullptr) j["valueType"] = json::parse(valueType->toJson());

    return j.dump(2);
}

string DeclNode::toDot() const {
    string result;
    string label;

#ifdef NODE_DOT_LABEL_DEBUG
    label += "(D) ";
#endif

    label += toSymbol(type);

#ifdef NODE_DOT_LABEL_DEBUG
    label += "\\n" + toString(type);
    label += "\\nID: " + std::to_string(GetId());
#endif

    if (!name.empty()) label += "\\nName: " + name;
    if (!className.empty()) label += "\\nClass: " + className;
    if (!classNameExtended.empty()) label += "\\nExtends: " + classNameExtended;
    if (isStatic != -1) label += "\\nStatic: " + std::to_string(isStatic);
    if (hasAddressOperator) label += "\\nReference (&)";
    if (visibilityType != VisibilityType::VISIBILITY_UNKNOWN) label += "\\nVisibility: " + toSymbol(visibilityType);

    std::string::size_type pos = 0;
    while ((pos = label.find('"', pos)) != std::string::npos) {
        label.replace(pos, 1, "\\\"");
        pos += 2;
    }

    result += " node" + std::to_string(GetId()) + " [label=\"" + label + "\", fillcolor=\"#FFD580\", style=filled];\n";

    int i = 0;
    for (const auto &child: children) {
        result += " node" + std::to_string(GetId()) + " -> node" + std::to_string(child->GetId()) +
                " [label=child" + std::to_string(i++) + "];\n";
        result += child->toDot();
    }

    if (declList != nullptr) {
        result += " node" + std::to_string(GetId()) + " -> node" + std::to_string(declList->GetId()) +
                " [label=declList];\n";
        result += declList->toDot();
    }

    if (params != nullptr) {
        result += " node" + std::to_string(GetId()) + " -> node" + std::to_string(params->GetId()) +
                " [label=params];\n";
        result += params->toDot();
    }

    if (expr != nullptr) {
        result += " node" + std::to_string(GetId()) + " -> node" + std::to_string(expr->GetId()) + " [label=expr];\n";
        result += expr->toDot();
    }

    if (stmt != nullptr) {
        result += " node" + std::to_string(GetId()) + " -> node" + std::to_string(stmt->GetId()) + " [label=stmt];\n";
        result += stmt->toDot();
    }

    if (valueType != nullptr) {
        result += " node" + std::to_string(GetId()) + " -> node" + std::to_string(valueType->GetId()) +
                " [label=valueType];\n";
        result += valueType->toDot();
    }

    return result;
}

bool DeclNode::doSemantics() {
    Log("starting semantics for " + toString(type) + "...");

    bool isOk = true;
    switch (type) {
        case DT_UNKNOWN:
            Warn("unknown type");
            return true;

        case DT_LIST:
            for (const auto &child: children) {
                isOk = isOk && child->doSemantics();
            }
            break;

        case DT_INTERFACE:
        case DT_CLASS:
            for (char &c: name) c = tolower(static_cast<unsigned char>(c));
            if (declList != nullptr) isOk = isOk && declList->doSemantics();
            break;

        case DT_PROPERTY:
            Warn("DT_PROPERTY not implemented");
            break;

        case DT_PARAMETER:
            if (expr != nullptr) expr->doSemantics();
            break;

        case DT_CONSTANT:
            Warn("DT_CONSTANT not implemented");
            break;

        case DT_FUNCTION:
        case DT_METHOD:
            for (char &c: name) c = tolower(static_cast<unsigned char>(c));
            if (params != nullptr) isOk = isOk && params->doSemantics();
            if (stmt != nullptr) isOk = isOk && stmt->doSemantics();
            break;

        default:
            Error("unknown enum type");
            return false;
    }

    if (isOk) Log("finished semantics for " + toString(type) + "");
    else Error("semantics for " + toString(type) + " failed");

    return isOk;
}

Class *DeclNode::processClass(Class *root, std::vector<Class *> &list) {
    Log("starting bytecode generation for " + toString(type) + "...");

    bool isOk = true;

    auto visToInt = [&](VisibilityType v) -> int {
        switch (v) {
            case VISIBILITY_PROTECTED: return 2;
            case VISIBILITY_PRIVATE: return 3;
            case VISIBILITY_PUBLIC:
            default: return 1;
        }
    };

    switch (type) {
        case DT_LIST: {
            for (auto *child: children) {
                if (child) root = child->processClass(root, list);
            }
            break;
        }

        case DT_CLASS: {
            std::string clsName = toJvmInternalName(name);

            // JVM-level superclass for the generated .class file
            std::string superInternal = classNameExtended.empty()
                                            ? "java/lang/Object"
                                            : toJvmInternalName(classNameExtended);

            // PHP-runtime parent (empty = "no parent class", i.e. only Object)
            std::string runtimeParent = classNameExtended.empty()
                                            ? ""
                                            : toJvmInternalName(classNameExtended);

            auto *cls = new Class(clsName, superInternal);
            cls->addFlag(Class::ACC_PUBLIC);
            cls->addFlag(Class::ACC_SUPER);

            // must call direct super <init>
            emitDefaultCtor(cls, superInternal);

            // Create clinit early so declList can append to it if needed.
            Method *clinit = getOrCreateClinit(cls);
            AttributeCode *cc = clinit->getCodeAttribute();

            // PhpRuntime.defineClass(String name, String parentName) : PhpClass
            auto *defineClass = cls->getOrCreateMethodrefConstant(
                "com/phpjvm/PhpRuntime",
                "defineClass",
                DescriptorMethod(
                    DescriptorField("com/phpjvm/PhpClass"),
                    {DescriptorField("java/lang/String"), DescriptorField("java/lang/String")}
                )
            );

            // Call defineClass(clsName, runtimeParent) and drop returned PhpClass
            *cc << cc->PushString(clsName);
            *cc << cc->PushString(runtimeParent);
            *cc << cc->InvokeStatic(defineClass);
            *cc << cc->PopOne();

            // Generate methods/fields/etc (may also append static init logic into <clinit>)
            if (declList) declList->processClass(cls, list);

            *cc << cc->ReturnVoid();

            list.push_back(cls);
            break;
        }

        case DT_FUNCTION: {
            std::string fn = toLowerAscii(name);

            ExprBuilder::RegisterFunctionSignature(fn, params, valueType);

            Method *m = root->getOrCreateMethod(fn, descPhpFunction());
            ExprBuilder::SetPhpMethodHasThis(m, false);
            ExprBuilder::SetPhpCallerClass(m, "");
            m->addFlag(Method::ACC_PUBLIC);
            m->addFlag(Method::ACC_STATIC);

            AttributeCode *code = m->getCodeAttribute();

            emitBindParamsFromArgs(root, m, code, params, fn, /*argsSlot*/0, /*baseLocalSlot*/1);

            uint16_t retSlot = ExprBuilder::AllocTempLocal(m);
            auto *L_epilogue = code->CodeLabel();

            auto *nullValueField = root->getOrCreateFieldrefConstant(
                "com/phpjvm/BasePhpValue",
                "NULL_VALUE",
                DescriptorField("com/phpjvm/BasePhpValue")
            );

            *code << code->GetStatic(nullValueField);
            *code << code->StoreReference(retSlot);

            StmtNode::BeginReturnCtx(m, L_epilogue, retSlot);

            if (stmt) {
                stmt->addStmt(root, m, code, /*isMain*/false);
            }

            StmtNode::EndReturnCtx(m);

            *code << code->GoTo(L_epilogue);

            *code << L_epilogue;
            ExprBuilder::EmitFlushByRefIfNeeded(root, m, code);

            *code << code->LoadReference(retSlot);
            *code << code->ReturnReference();

            ExprBuilder::EndLocalScope(m);
            break;
        }

        case DT_METHOD: {
            std::string mn = toLowerAscii(name);

            std::string owner = toLowerAscii(root->getClassName());
            ExprBuilder::RegisterMethodSignature(owner, mn, params, valueType);

            const bool phpStatic = (isStatic == 1);
            DescriptorMethod sig = phpStatic ? descPhpStaticMethod() : descPhpInstanceMethod();

            Method *m = root->getOrCreateMethod(mn, sig);
            ExprBuilder::SetPhpMethodHasThis(m, !phpStatic);
            ExprBuilder::SetPhpCallerClass(m, owner);
            applyVisibility(m, visibilityType);
            m->addFlag(Method::ACC_STATIC);

            AttributeCode *code = m->getCodeAttribute();

            emitBindParamsFromArgs(root, m, code, params, owner + "::" + mn, /*argsSlot*/1, /*baseLocalSlot*/2);

            uint16_t retSlot = ExprBuilder::AllocTempLocal(m);
            auto *L_epilogue = code->CodeLabel();

            auto *nullValueField = root->getOrCreateFieldrefConstant(
                "com/phpjvm/BasePhpValue",
                "NULL_VALUE",
                DescriptorField("com/phpjvm/BasePhpValue")
            );

            *code << code->GetStatic(nullValueField);
            *code << code->StoreReference(retSlot);

            StmtNode::BeginReturnCtx(m, L_epilogue, retSlot);

            if (stmt) {
                stmt->addStmt(root, m, code, /*isMain*/false);
            }

            StmtNode::EndReturnCtx(m);

            *code << code->GoTo(L_epilogue);

            *code << L_epilogue;
            ExprBuilder::EmitFlushByRefIfNeeded(root, m, code);

            *code << code->LoadReference(retSlot);
            *code << code->ReturnReference();

            ExprBuilder::EndLocalScope(m);
            break;
        }

        case DT_CONSTANT: {
            std::string fieldName = sanitizeJavaMemberIdent(name);

            Field *f = root->getOrCreateField(fieldName, DescriptorField("com/phpjvm/BasePhpValue"));
            f->addFlag(Field::ACC_STATIC);
            // Keep JVM visibility roughly aligned (runtime enforces real PHP visibility)
            switch (visibilityType) {
                case VISIBILITY_PRIVATE: f->addFlag(Field::ACC_PRIVATE);
                    break;
                case VISIBILITY_PROTECTED: f->addFlag(Field::ACC_PROTECTED);
                    break;
                case VISIBILITY_PUBLIC:
                default: f->addFlag(Field::ACC_PUBLIC);
                    break;
            }

            auto *fieldRef = root->getOrCreateFieldrefConstant(
                root->getClassName(),
                fieldName,
                DescriptorField("com/phpjvm/BasePhpValue")
            );

            Method *clinit = getOrCreateClinit(root);
            AttributeCode *cc = clinit->getCodeAttribute();

            // PhpRuntime.defineConst(String className, String constName, int visibility, BasePhpValue value) : void
            auto *defineConst = root->getOrCreateMethodrefConstant(
                "com/phpjvm/PhpRuntime",
                "defineConst",
                DescriptorMethod(
                    std::nullopt,
                    {
                        DescriptorField("java/lang/String"),
                        DescriptorField("java/lang/String"),
                        DescriptorField("I"),
                        DescriptorField("com/phpjvm/BasePhpValue")
                    }
                )
            );

            int visInt = visToInt(visibilityType);

            *cc << cc->PushString(root->getClassName());
            *cc << cc->PushString(name);
            *cc << cc->PushInt(visInt);

            // value
            ExprBuilder::EmitValue(root, clinit, cc, expr);

            // duplicate so we can both set the JVM field and register in runtime
            *cc << cc->Duplicate();
            *cc << cc->PutStatic(fieldRef);

            // register into PhpRuntime (same value still on stack)
            *cc << cc->InvokeStatic(defineConst);
            break;
        }

        case DT_PROPERTY: {
            Method *clinit = getOrCreateClinit(root);
            AttributeCode *cc = clinit->getCodeAttribute();

            auto *nullValueField = root->getOrCreateFieldrefConstant(
                "com/phpjvm/BasePhpValue",
                "NULL_VALUE",
                DescriptorField("com/phpjvm/BasePhpValue")
            );

            // PhpRuntime.defineProperty(String className, String propName, boolean isStatic, int visibility, BasePhpValue defaultValue) : void
            auto *defineProperty = root->getOrCreateMethodrefConstant(
                "com/phpjvm/PhpRuntime",
                "defineProperty",
                DescriptorMethod(
                    std::nullopt,
                    {
                        DescriptorField("java/lang/String"),
                        DescriptorField("java/lang/String"),
                        DescriptorField("Z"),
                        DescriptorField("I"),
                        DescriptorField("com/phpjvm/BasePhpValue")
                    }
                )
            );

            int visInt = visToInt(visibilityType);
            bool st = (isStatic == 1);

            *cc << cc->PushString(root->getClassName());
            *cc << cc->PushString(name);
            *cc << cc->PushInt(st ? 1 : 0);
            *cc << cc->PushInt(visInt);

            if (expr) {
                ExprBuilder::EmitValue(root, clinit, cc, expr);
            } else {
                *cc << cc->GetStatic(nullValueField);
            }

            *cc << cc->InvokeStatic(defineProperty);
            break;
        }

        case DT_PARAMETER:
            Warn("DeclNode::processClass: " + toString(type) + " not implemented yet");
            break;

        default:
            Warn("no processing implementation for " + toString(type));
            break;
    }

    if (isOk) Log("finished semantics for " + toString(type) + "");
    else Error("semantics for " + toString(type) + " failed");

    return root;
}

DeclNode *DeclNode::DeclList(DeclNode *decl) {
    auto node = new DeclNode();
    node->type = DT_LIST;
    node->children.push_back(decl);
    node->WriteToFiles();
    return node;
}

DeclNode *DeclNode::AppendToDeclList(DeclNode *declList, DeclNode *newDecl) {
    declList->children.push_back(newDecl);
    declList->WriteToFiles();
    return declList;
}

DeclNode *DeclNode::SetModsToDecl(DeclNode *decl, RawDeclModifier *modifier) {
    if (decl->type == DeclType::DT_LIST) {
        for (DeclNode *child: decl->children) {
            child->visibilityType = modifier->visibility;
            child->isStatic = modifier->isStatic;
        }
    } else {
        decl->visibilityType = modifier->visibility;
        decl->isStatic = modifier->isStatic;
    }
    decl->WriteToFiles();
    return decl;
}

DeclNode *DeclNode::SetModsToDecl(DeclNode *decl, RawDeclModifier *modifier, ValueNode *type) {
    if (decl->type == DeclType::DT_LIST) {
        for (DeclNode *child: decl->children) {
            child->visibilityType = modifier->visibility;
            child->isStatic = modifier->isStatic;
            child->valueType = type;
        }
    } else {
        decl->visibilityType = modifier->visibility;
        decl->isStatic = modifier->isStatic;
        decl->valueType = type;
    }
    decl->WriteToFiles();
    return decl;
}

DeclNode *DeclNode::SetTypeToDecl(DeclNode *decl, DeclType type) {
    decl->type = type;
    return decl;
}

DeclNode *DeclNode::SetValueTypeToDecl(DeclNode *decl, ValueNode *type) {
    if (decl->type == DeclType::DT_LIST) {
        for (DeclNode *child: decl->children) {
            child->valueType = type;
        }
    } else {
        decl->valueType = type;
    }
    decl->WriteToFiles();
    return decl;
}

DeclNode *DeclNode::ClassDecl(string *className, DeclNode *declList) {
    auto node = new DeclNode();
    node->type = DT_CLASS;
    node->name = *className;
    node->declList = declList;
    node->WriteToFiles();
    return node;
}

DeclNode *DeclNode::InterfaceDecl(string *className, DeclNode *declList) {
    auto node = new DeclNode();
    node->type = DT_INTERFACE;
    node->name = *className;
    node->declList = declList;
    node->WriteToFiles();
    return node;
}

DeclNode *DeclNode::ClassDecl(string *className, string *extendedClassName, DeclNode *declList) {
    auto node = new DeclNode();
    node->type = DT_CLASS;
    node->name = *className;
    node->classNameExtended = *extendedClassName;
    node->declList = declList;
    node->WriteToFiles();
    return node;
}

DeclNode *DeclNode::PropertyDecl(string *name) {
    auto node = new DeclNode();
    node->visibilityType = VISIBILITY_PUBLIC;
    node->type = DT_PROPERTY;
    node->name = *name;
    node->WriteToFiles();
    return node;
}

DeclNode *DeclNode::PropertyDecl(string *name, ExprNode *expr) {
    auto node = new DeclNode();
    node->visibilityType = VISIBILITY_PUBLIC;
    node->type = DT_PROPERTY;
    node->name = *name;
    node->expr = expr;
    node->WriteToFiles();
    return node;
}

DeclNode *DeclNode::ConstDecl(string *name, ExprNode *expr) {
    auto node = new DeclNode();
    node->visibilityType = VISIBILITY_PUBLIC;
    node->type = DT_CONSTANT;
    node->name = *name;
    node->expr = expr;
    node->WriteToFiles();
    return node;
}

DeclNode *DeclNode::ParamDecl(string *name) {
    auto node = new DeclNode();
    node->type = DT_PARAMETER;
    node->name = *name;
    node->WriteToFiles();
    return node;
}

DeclNode *DeclNode::ParamDeclWithAddressOp(string *name) {
    auto node = new DeclNode();
    node->type = DT_PARAMETER;
    node->name = *name;
    node->hasAddressOperator = true;
    node->WriteToFiles();
    return node;
}

DeclNode *DeclNode::ParamDeclType(string *name, ValueNode *type) {
    auto node = new DeclNode();
    node->type = DT_PARAMETER;
    node->name = *name;
    node->valueType = type;
    node->WriteToFiles();
    return node;
}

DeclNode *DeclNode::ParamDeclTypeWithAddressOp(string *name, ValueNode *type) {
    auto node = new DeclNode();
    node->type = DT_PARAMETER;
    node->name = *name;
    node->valueType = type;
    node->hasAddressOperator = true;
    node->WriteToFiles();
    return node;
}

DeclNode *DeclNode::ParamDeclExpr(string *name, ExprNode *expr) {
    auto node = new DeclNode();
    node->type = DT_PARAMETER;
    node->name = *name;
    node->expr = expr;
    node->WriteToFiles();
    return node;
}

DeclNode *DeclNode::ParamDeclExprWithAddressOp(string *name, ExprNode *expr) {
    auto node = new DeclNode();
    node->type = DT_PARAMETER;
    node->name = *name;
    node->expr = expr;
    node->hasAddressOperator = true;
    node->WriteToFiles();
    return node;
}

DeclNode *DeclNode::ParamDeclExprType(string *name, ExprNode *expr, ValueNode *type) {
    auto node = new DeclNode();
    node->type = DT_PARAMETER;
    node->name = *name;
    node->expr = expr;
    node->valueType = type;
    node->WriteToFiles();
    return node;
}

DeclNode *DeclNode::ParamDeclExprTypeWithAddressOp(string *name, ExprNode *expr, ValueNode *type) {
    auto node = new DeclNode();
    node->type = DT_PARAMETER;
    node->name = *name;
    node->expr = expr;
    node->valueType = type;
    node->hasAddressOperator = true;
    node->WriteToFiles();
    return node;
}

DeclNode *DeclNode::FunctionDecl(string *name, DeclNode *params) {
    auto node = new DeclNode();
    node->type = DT_FUNCTION;
    node->name = *name;
    node->params = params;
    node->WriteToFiles();
    return node;
}

DeclNode *DeclNode::FunctionDecl(string *name, DeclNode *params, ValueNode *type) {
    auto node = new DeclNode();
    node->type = DT_FUNCTION;
    node->name = *name;
    node->params = params;
    node->valueType = type;
    node->WriteToFiles();
    return node;
}

DeclNode *DeclNode::FunctionAddBody(DeclNode *func, StmtNode *body) {
    func->stmt = body;
    func->WriteToFiles();
    return func;
}
