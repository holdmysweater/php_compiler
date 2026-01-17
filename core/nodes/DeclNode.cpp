#include "DeclNode.h"
#include "core/helpers/Console.h"
#include "json.hpp"
#include <string>
#include <cctype>

using json = nlohmann::json;

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

            //!!! TODO maybe check if the function names / var / const names are the same (separately)
            Warn("DT_CLASS implementation is unfinished");
            break;

        case DT_PROPERTY:
            // TODO property logic
            Warn("DT_PROPERTY not implemented");
            break;

        case DT_PARAMETER:
            // TODO parameter logic
            Warn("DT_PARAMETER not implemented");
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

            //!!! TODO check if all paths return something and type check for the returns
            Warn("DT_FUNCTION/DT_METHOD implementation is unfinished");
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

DeclNode *DeclNode::ParamDeclType(string *name, ValueNode *type) {
    auto node = new DeclNode();
    node->type = DT_PARAMETER;
    node->name = *name;
    node->valueType = type;
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

DeclNode *DeclNode::ParamDeclExprType(string *name, ExprNode *expr, ValueNode *type) {
    auto node = new DeclNode();
    node->type = DT_PARAMETER;
    node->name = *name;
    node->expr = expr;
    node->valueType = type;
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
