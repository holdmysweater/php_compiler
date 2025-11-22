#include "ElementNode.h"
#include "core/helpers/Console.h"
#include "json.hpp"

using json = nlohmann::json;

string ElementNode::_getClassName() const {
    return "ElementNode";
}

string ElementNode::toJson() const {
    json j;
    j["id"] = GetId();
    j["type"] = toString(type);

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

    return j.dump(2);
}

string ElementNode::toDot() const {
    string result;
    string label;

#ifdef BASENODE_DOT_DEBUG
    label += "(P) ";
#endif

    label += toSymbol(type);

#ifdef BASENODE_DOT_DEBUG
    label += "\\n" + toString(type);
    label += "\\nID: " + std::to_string(GetId());
#endif

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

    return result;
}

bool ElementNode::doSemantics() const {
    bool isOk = true;
    switch (type) {
        case ELEMENT_UNKNOWN:
            Warn("unknown type");
            return true;
        case ELEMENT_EMPTY:
            Warn("empty");
            return true;
        case ELEMENT_PROGRAM_LIST:
            Log("starting semantics for children of ELEMENT_PROGRAM_LIST...");
             for (const auto &child: children) {
                isOk &= child->doSemantics();
            }

            if (isOk) {
                Log("finished semantics for children of ELEMENT_PROGRAM_LIST!");
            } else {
                Error("semantics for children of ELEMENT_PROGRAM_LIST failed");
            }

            return isOk;
        case ELEMENT_STATEMENT:
            Warn("statement");
            return true;
        case ELEMENT_CLASS_DECL:
            Warn("class decl");
            return true;
        case ELEMENT_FUNC_DECL:
            Warn("function decl");
            return false;
        default:
            Error("unknown enum type");
            return false;
    }

    Console::Warning("ElementNode::doSemantics is empty");
    return true;
}

ElementNode *ElementNode::EmptyElement() {
    auto node = new ElementNode();
    node->type = ElementType::ELEMENT_EMPTY;
    node->WriteToFiles();
    return node;
}

ElementNode *ElementNode::ElementList(ElementNode *element) {
    auto node = new ElementNode();
    node->type = ELEMENT_PROGRAM_LIST;
    if (element != nullptr) {
        node->children.push_back(element);
    }
    node->WriteToFiles();
    return node;
}

ElementNode *ElementNode::AppendToElementList(ElementNode *elementList, ElementNode *newElement) {
    if (newElement != nullptr) {
        elementList->children.push_back(newElement);
    }
    elementList->WriteToFiles();
    return elementList;
}

ElementNode *ElementNode::PhpClassDecl(DeclNode *declList) {
    auto node = new ElementNode();
    node->type = ElementType::ELEMENT_CLASS_DECL;
    node->decl = declList;
    node->WriteToFiles();
    return node;
}

ElementNode *ElementNode::PhpFuncDecl(DeclNode *declList) {
    auto node = new ElementNode();
    node->type = ElementType::ELEMENT_FUNC_DECL;
    node->decl = declList;
    node->WriteToFiles();
    return node;
}

ElementNode *ElementNode::PhpStmt(StmtNode *stmt) {
    if (stmt == nullptr) {
        return nullptr;
    }
    auto node = new ElementNode();
    node->type = ElementType::ELEMENT_STATEMENT;
    node->stmt = stmt;
    node->WriteToFiles();
    return node;
}
