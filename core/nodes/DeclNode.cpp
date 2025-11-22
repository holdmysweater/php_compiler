#include "DeclNode.h"
#include "core/helpers/Console.h"
#include "json.hpp"

using json = nlohmann::json;

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

#ifdef DOT_DEBUG
    label += "(D) ";
#endif

    label += toString(type);

#ifdef DOT_DEBUG
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
        label += "\\nVisibility: " + toString(visibilityType);
    }

    std::string::size_type pos = 0;
    while ((pos = label.find('"', pos)) != std::string::npos) {
        label.replace(pos, 1, "\\\"");
        pos += 2;
    }

    result += " node" + std::to_string(GetId()) + " [label=\"" + label + "\", fillcolor=\"#FFD580\", style=filled];\n";

    for (const auto &child: children) {
        result += " node" + std::to_string(GetId()) + " -> node" + std::to_string(child->GetId()) +
                " [label=children];\n";
        result += child->toDot();
    }

    if (declList != nullptr) {
        result += " node" + std::to_string(GetId()) + " -> node" + std::to_string(declList->GetId()) +
                " [label=declList];\n";
        result += declList->toDot();
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

bool DeclNode::doSemantics() const {
    Console::Warning("DeclNode::doSemantics is empty");
    return true;
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
    node->type = DT_PROPERTY;
    node->name = *name;
    node->WriteToFiles();
    return node;
}

DeclNode *DeclNode::PropertyDecl(string *name, ExprNode *expr) {
    auto node = new DeclNode();
    node->type = DT_PROPERTY;
    node->name = *name;
    node->expr = expr;
    node->WriteToFiles();
    return node;
}

DeclNode *DeclNode::ConstDecl(string *name, ExprNode *expr) {
    auto node = new DeclNode();
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
    node->declList = params;
    node->WriteToFiles();
    return node;
}

DeclNode *DeclNode::FunctionDecl(string *name, DeclNode *params, ValueNode *type) {
    auto node = new DeclNode();
    node->type = DT_FUNCTION;
    node->name = *name;
    node->declList = params;
    node->valueType = type;
    node->WriteToFiles();
    return node;
}

DeclNode *DeclNode::FunctionAddBody(DeclNode *func, StmtNode *body) {
    func->stmt = body;
    func->WriteToFiles();
    return func;
}
