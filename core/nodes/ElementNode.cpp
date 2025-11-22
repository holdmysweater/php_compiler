#include "ElementNode.h"
#include "json.hpp"

using json = nlohmann::json;

string ElementNode::toJson() const {
    json j;
    j["id"] = GetId();
    j["type"] = toString(type);

    if (!content.empty()) {
        j["content"] = content;
    }

    if (!children.empty()) {
        json childrenArray = json::array();
        for (const auto &child: children) {
            childrenArray.push_back(json::parse(child->toJson()));
        }
        j["children"] = childrenArray;
    }

    if (decl != nullptr) {
        j["decl"] = json::parse(decl->toJson());
    }

    if (stmt != nullptr) {
        j["stmt"] = json::parse(stmt->toJson());
    }

    if (expr != nullptr) {
        j["expr"] = json::parse(expr->toJson());
    }

    return j.dump(2);
}

string ElementNode::toDot() const {
    string result;
    string label;

#ifdef DOT_DEBUG
    label += "(P) ";
#endif

    label += toString(type);

#ifdef DOT_DEBUG
    label += "\\n" + toString(type);
    label += "\\nID: " + std::to_string(GetId());
#endif

    if (!content.empty()) {
        label += "\\nContent: " + content;
    }

    std::string::size_type pos = 0;
    while ((pos = label.find('"', pos)) != std::string::npos) {
        label.replace(pos, 1, "\\\"");
        pos += 2;
    }

    pos = 0;
    while ((pos = label.find('\n', pos)) != std::string::npos) {
        label.replace(pos, 1, "\\n");
        pos += 2;
    }

    pos = 0;
    while ((pos = label.find('\r', pos)) != std::string::npos) {
        label.replace(pos, 1, "\\r");
        pos += 2;
    }

    result += "  node" + std::to_string(GetId()) + " [label=\"" + label +
            "\", fillcolor=\"lightgrey\", style=filled];\n";

    for (const auto &child: children) {
        result += "  node" + std::to_string(GetId()) + " -> node" + std::to_string(child->GetId()) +
                " [label=children];\n";
        result += child->toDot();
    }

    if (decl != nullptr) {
        result += "  node" + std::to_string(GetId()) + " -> node" + std::to_string(decl->GetId()) + " [label=decl];\n";
        result += decl->toDot();
    }

    if (stmt != nullptr) {
        result += "  node" + std::to_string(GetId()) + " -> node" + std::to_string(stmt->GetId()) + " [label=stmt];\n";
        result += stmt->toDot();
    }

    if (expr != nullptr) {
        result += "  node" + std::to_string(GetId()) + " -> node" + std::to_string(expr->GetId()) + " [label=expr];\n";
        result += expr->toDot();
    }

    return result;
}

ElementNode *ElementNode::EmptyElement() {
    auto node = new ElementNode();
    node->type = ElementType::ELEMENT_PROGRAM_LIST;
    node->WriteToJsonFile();
    return node;
}

ElementNode *ElementNode::ElementList(ElementNode *element, ElementType type) {
    auto node = new ElementNode();
    node->type = type;
    if (element != nullptr) {
        node->children.push_back(element);
    }
    node->WriteToJsonFile();
    return node;
}

ElementNode *ElementNode::AppendToElementList(ElementNode *elementList, ElementNode *newElement) {
    if (newElement != nullptr) {
        elementList->children.push_back(newElement);
    }
    elementList->WriteToJsonFile();
    return elementList;
}

ElementNode *ElementNode::PhpDecl(DeclNode *declList) {
    auto node = new ElementNode();
    node->type = ElementType::ELEMENT_STATEMENT;
    node->decl = declList;
    node->WriteToJsonFile();
    return node;
}

ElementNode *ElementNode::PhpStmt(StmtNode *stmt) {
    if (stmt == nullptr) {
        return nullptr;
    }
    auto node = new ElementNode();
    node->type = ElementType::ELEMENT_STATEMENT;
    node->stmt = stmt;
    node->WriteToJsonFile();
    return node;
}

ElementNode *ElementNode::PhpEchoContent(ExprNode *expr) {
    auto node = new ElementNode();
    node->type = ElementType::ELEMENT_ECHO_PHP;
    node->expr = expr;
    node->WriteToJsonFile();
    return node;
}

ElementNode *ElementNode::HtmlContent(string *content) {
    auto node = new ElementNode();
    node->type = ElementType::ELEMENT_HTML;
    node->content = *content;
    node->WriteToJsonFile();
    return node;
}
