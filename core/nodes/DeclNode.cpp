#include "DeclNode.h"
#include "core/helpers/Console.h"
#include "json.hpp"
#include <string>
#include <cctype>
#include "jvm/class.h"
#include "jvm/method.h"
#include "jvm/attribute-code.h"
#include "jvm/descriptor-field.h"
#include "jvm/descriptor-method.h"
#include "core/bytecode/ExprBuilder.h"
#include "jvm/field.h"

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
        {DescriptorField("com/phpjvm/BasePhpValue", 1)} // BasePhpValue[]
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

static void emitDefaultCtor(Class *cls) {
    Method *init = cls->getOrCreateMethod("<init>", DescriptorMethod(std::nullopt, {}));
    init->addFlag(Method::ACC_PUBLIC);

    AttributeCode *code = init->getCodeAttribute();

    auto *superInit = cls->getOrCreateMethodrefConstant(
        "java/lang/Object",
        "<init>",
        DescriptorMethod(std::nullopt, {})
    );

    *code << code->LoadReference(0);
    *code << code->InvokeSpecial(superInit);
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

static void emitBindParamsFromArgs(
    Class *root,
    Method *m,
    AttributeCode *code,
    DeclNode *params,
    uint16_t argsSlot,
    uint16_t baseLocalSlot
) {
    auto *nullValueField = root->getOrCreateFieldrefConstant(
        "com/phpjvm/BasePhpValue",
        "NULL_VALUE",
        DescriptorField("com/phpjvm/BasePhpValue")
    );

    std::vector<DeclNode *> ps = flattenParamList(params);

    // Let ExprBuilder know we are in a local scope; locals after params can be allocated starting here.
    ExprBuilder::BeginLocalScope(m, static_cast<uint16_t>(baseLocalSlot + ps.size()));

    for (size_t i = 0; i < ps.size(); i++) {
        DeclNode *p = ps[i];
        const std::string pname = p ? p->name : "";
        const uint16_t slot = static_cast<uint16_t>(baseLocalSlot + i);

        // Register local name -> slot (so $a loads local slot)
        ExprBuilder::DefineLocal(m, pname, slot);

        auto *L_missing = code->CodeLabel();
        auto *L_end = code->CodeLabel();

        // if (args.length < i+1) goto missing
        *code << code->LoadReference(argsSlot);
        *code << code->ArrayLength();
        *code << code->PushInt(static_cast<int32_t>(i + 1));
        *code << code->IfWithCompare(Instruction::Compare::LessThan, L_missing);

        // present: slot = args[i]
        *code << code->LoadReference(argsSlot);
        *code << code->PushInt(static_cast<int32_t>(i));
        *code << code->LoadReferenceFromArray();
        *code << code->StoreReference(slot);
        *code << code->GoTo(L_end);

        // missing: slot = default or NULL
        *code << L_missing;
        if (p && p->expr) {
            ExprBuilder::EmitValue(root, m, code, p->expr);
        } else {
            *code << code->GetStatic(nullValueField);
        }
        *code << code->StoreReference(slot);

        *code << L_end;
    }
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
    // remove nothing; just add what you want (your builder likely enforces "only one")
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

    if (!name.empty()) {
        j["name"] = name;
    }

    if (!className.empty()) {
        j["className"] = className;
    }

    if (!classNameExtended.empty()) {
        j["classNameExtended"] = classNameExtended;
    }

    if (isStatic != -1) {
        j["isStatic"] = isStatic;
    }

    if (hasAddressOperator) {
        j["hasAddressOperator"] = hasAddressOperator;
    }

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

    if (declList != nullptr) {
        j["declList"] = json::parse(declList->toJson());
    }

    if (params != nullptr) {
        j["params"] = json::parse(params->toJson());
    }

    if (expr != nullptr) {
        j["expr"] = json::parse(expr->toJson());
    }

    if (stmt != nullptr) {
        j["stmt"] = json::parse(stmt->toJson());
    }

    if (valueType != nullptr) {
        j["valueType"] = json::parse(valueType->toJson());
    }

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

    if (!name.empty()) {
        label += "\\nName: " + name;
    }

    if (!className.empty()) {
        label += "\\nClass: " + className;
    }

    if (!classNameExtended.empty()) {
        label += "\\nExtends: " + classNameExtended;
    }

    if (isStatic != -1) {
        label += "\\nStatic: " + std::to_string(isStatic);
    }

    if (hasAddressOperator) {
        label += "\\nReference (&)";
    }

    if (visibilityType != VisibilityType::VISIBILITY_UNKNOWN) {
        label += "\\nVisibility: " + toSymbol(visibilityType);
    }

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

        case DT_CLASS:
            // Class names are case-insensitive
            for (char &c: name) {
                c = tolower(static_cast<unsigned char>(c));
            }

            // Semantics for declarations of the class
            if (this->declList != nullptr) {
                isOk = isOk && this->declList->doSemantics();
            } else {
                Log("skipped decl list");
            }

            break;

        case DT_PROPERTY:
            // TODO property logic
            Warn("DT_PROPERTY not implemented");
            break;

        case DT_PARAMETER:
            if (expr != nullptr) {
                expr->doSemantics();
            }
            break;

        case DT_CONSTANT:
            // TODO const logic
            Warn("DT_CONSTANT not implemented");
            break;

        case DT_FUNCTION:
        case DT_METHOD:
            // Function names are case-insensitive
            for (char &c: name) {
                c = tolower(static_cast<unsigned char>(c));
            }

            // Semantics for parameters
            if (this->params != nullptr) {
                isOk = isOk && this->params->doSemantics();
            } else {
                Log("skipped params");
            }

            // Semantics for body
            if (this->stmt != nullptr) {
                isOk = isOk && this->stmt->doSemantics();
            } else {
                Log("skipped body (no stmt)");
            }

            break;

        default:
            Error("unknown enum type");
            return false;
    }

    if (isOk) {
        Log("finished semantics for " + toString(type) + "");
    } else {
        Error("semantics for " + toString(type) + " failed");
    }

    return isOk;
}

Class *DeclNode::processClass(Class *root, std::vector<Class *> &list) {
    Log("starting bytecode generation for " + toString(type) + "...");

    bool isOk = true;

    switch (type) {
        case DT_LIST: {
            for (auto *child: children) {
                if (child) root = child->processClass(root, list);
            }
            break;
        }

        case DT_CLASS: {
            std::string clsName = toJvmInternalName(name);
            std::string parentName = classNameExtended.empty()
                                         ? "java/lang/Object"
                                         : toJvmInternalName(classNameExtended);

            auto *cls = new Class(clsName, parentName);
            cls->addFlag(Class::ACC_PUBLIC);
            cls->addFlag(Class::ACC_SUPER);

            emitDefaultCtor(cls);

            // Generate class members into cls
            if (declList) {
                declList->processClass(cls, list);
            }

            // IMPORTANT: ensure <clinit> ends with RETURN if it exists / was used.
            // Safest approach: always create it and just return if it's empty.
            {
                Method *clinit = getOrCreateClinit(cls);
                AttributeCode *cc = clinit->getCodeAttribute();
                *cc << cc->ReturnVoid();
            }

            list.push_back(cls);
            break;
        }

        case DT_FUNCTION: {
            std::string fn = toLowerAscii(name);

            Method *m = root->getOrCreateMethod(fn, descPhpFunction());
            m->addFlag(Method::ACC_PUBLIC);
            m->addFlag(Method::ACC_STATIC);

            AttributeCode *code = m->getCodeAttribute();

            // args is local slot 0 in static function signature: (BasePhpValue[] args)
            // params will be stored starting at local slot 1
            emitBindParamsFromArgs(root, m, code, params, /*argsSlot*/0, /*baseLocalSlot*/1);

            if (stmt) {
                stmt->addStmt(root, m, code);
            }

            emitReturnNull(root, code);

            // Important: end scope so future methods don't see these locals
            ExprBuilder::EndLocalScope(m);
            break;
        }

        case DT_METHOD: {
            std::string mn = toLowerAscii(name);

            const bool phpStatic = (isStatic == 1);
            DescriptorMethod sig = phpStatic ? descPhpStaticMethod() : descPhpInstanceMethod();

            Method *m = root->getOrCreateMethod(mn, sig);

            applyVisibility(m, visibilityType);
            m->addFlag(Method::ACC_STATIC);

            AttributeCode *code = m->getCodeAttribute();

            // For both class-static and instance methods, args is in local slot 1:
            //   instance: (PhpObject self, BasePhpValue[] args)
            //   static:   (PhpClass calledClass, BasePhpValue[] args)
            // baseLocalSlot = 2 because slot0 + slot1 are occupied.
            emitBindParamsFromArgs(root, m, code, params, /*argsSlot*/1, /*baseLocalSlot*/2);

            if (stmt) {
                stmt->addStmt(root, m, code);
            }

            emitReturnNull(root, code);

            ExprBuilder::EndLocalScope(m);
            break;
        }

        case DT_CONSTANT: {
            // We are inside a *class* here: `root` is the class being built.
            // Create: public static BasePhpValue X;
            std::string fieldName = sanitizeJavaMemberIdent(name);

            Field *f = root->getOrCreateField(fieldName, DescriptorField("com/phpjvm/BasePhpValue"));
            f->addFlag(Field::ACC_PUBLIC);
            f->addFlag(Field::ACC_STATIC);
            // optional: f->addFlag(Field::ACC_FINAL);

            auto *fieldRef = root->getOrCreateFieldrefConstant(
                root->getClassName(),
                fieldName,
                DescriptorField("com/phpjvm/BasePhpValue")
            );

            // Emit initialization into <clinit>:
            // <clinit>:
            //   <push value>  (BasePhpValue)
            //   putstatic X
            Method *clinit = getOrCreateClinit(root);
            AttributeCode *cc = clinit->getCodeAttribute();

            // IMPORTANT: DON'T use stmt->addStmt here; just emit the expression value.
            // ExprBuilder already returns BasePhpValue on stack.
            ExprBuilder::EmitValue(root, clinit, cc, expr);
            *cc << cc->PutStatic(fieldRef);

            break;
        }

        case DT_PROPERTY:
        case DT_PARAMETER:
            Warn("DeclNode::processClass: " + toString(type) + " not implemented yet");
            break;

        default:
            Warn("no processing implementation for " + toString(type));
            break;
    }

    if (isOk) {
        Log("finished semantics for " + toString(type) + "");
    } else {
        Error("semantics for " + toString(type) + " failed");
    }

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
