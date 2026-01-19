#include "ElementNode.h"
#include "json.hpp"
#include "jvm/descriptor-method.h"
#include "jvm/method.h"

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

#ifdef NODE_DOT_LABEL_DEBUG
    label += "(P) ";
#endif

    label += toSymbol(type);

#ifdef NODE_DOT_LABEL_DEBUG
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

    int i = 0;
    for (const auto &child: children) {
        result += "  node" + std::to_string(GetId()) + " -> node" + std::to_string(child->GetId()) +
                " [label=child" + std::to_string(i++) + "];\n";
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

bool ElementNode::doSemantics() {
    Log("starting semantics for " + toString(type) + "...");

    bool isOk = true;
    switch (type) {
        case ELEMENT_UNKNOWN:
            Warn("unknown type");
            return true;
        case ELEMENT_EMPTY:
            Warn("empty");
            return true;
        case ELEMENT_PROGRAM_LIST:
            for (const auto &child: children) {
                isOk = isOk && child->doSemantics();
            }
            break;
        case ELEMENT_STATEMENT:
        case ELEMENT_CLASS_DECL:
        case ELEMENT_INTERFACE_DECL:
        case ELEMENT_FUNC_DECL:
            if (type == ELEMENT_STATEMENT) {
                isOk = this->stmt->doSemantics();
            } else {
                isOk = this->decl->doSemantics();
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

Class *ElementNode::processClass(Class *root, std::vector<Class *> &list) {
    Log("starting bytecode generation for " + toString(type) + "...");

    bool isOk = true;

    switch (type) {
        case ElementType::ELEMENT_EMPTY:
        case ElementType::ELEMENT_PROGRAM_LIST: {
            // 1) create main ONCE
            Method *mainMethod = root->getOrCreateMethod(
                "main",
                DescriptorMethod(
                    std::nullopt, // void
                    {DescriptorField("java/lang/String", 1)} // String[] args
                )
            );
            mainMethod->addFlag(Method::ACC_PUBLIC);
            mainMethod->addFlag(Method::ACC_STATIC);

            AttributeCode *code = mainMethod->getCodeAttribute();

            // 2) emit everything into the same main
            for (auto child: children) {
                if (!child) continue;

                switch (child->type) {
                    case ElementType::ELEMENT_STATEMENT:
                        if (child->stmt) child->stmt->addStmt(root, mainMethod, code, true);
                        break;

                    case ElementType::ELEMENT_CLASS_DECL:
                    case ElementType::ELEMENT_FUNC_DECL:
                        if (child->decl) child->decl->processClass(root, list);
                        break;

                    default:
                        root = child->processClass(root, list);
                        break;
                }
            }

            // 3) return once at the end
            *code << code->ReturnVoid();
            break;
        }

        case ElementType::ELEMENT_STATEMENT:
            // Top-level statements are handled by ELEMENT_PROGRAM_LIST now
            break;

        case ElementType::ELEMENT_CLASS_DECL:
        case ElementType::ELEMENT_FUNC_DECL:
            this->decl->processClass(root, list);
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

ElementNode *ElementNode::PhpInterfaceDecl(DeclNode *declList) {
    auto node = new ElementNode();
    node->type = ElementType::ELEMENT_INTERFACE_DECL;
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
