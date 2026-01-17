#include "ExprNode.h"
#include "json.hpp"
#include "core/helpers/Console.h"

using json = nlohmann::json;

string ExprNode::_getClassName() const {
    return "ExprNode";
}

string ExprNode::toJson() const {
    json j;
    j["id"] = GetId();
    j["type"] = toString(type);

    if (value != nullptr) {
        j["value"] = json::parse(value->toJson());
    }

    if (!name.empty()) {
        j["name"] = name;
    }

    if (!children.empty()) {
        json childrenArray = json::array();
        for (const auto &child: children) {
            childrenArray.push_back(json::parse(child->toJson()));
        }
        j["children"] = childrenArray;
    }

    return j.dump(2);
}

string ExprNode::toDot() const {
    string result;
    string label;

#ifdef NODE_DOT_LABEL_DEBUG
    label += "(E) ";
#endif

    label += toSymbol(type);
    if (!name.empty()) {
        label += "\\n" + name;
    }

#ifdef NODE_DOT_LABEL_DEBUG
    label += "\\n" + toString(type);
    label += "\\nID: " + std::to_string(GetId());
#endif

    std::string::size_type pos = 0;
    while ((pos = label.find('"', pos)) != std::string::npos) {
        label.replace(pos, 1, "\\\"");
        pos += 2;
    }

    result += "  node" + std::to_string(GetId()) + " [label=\"" + label + "\", fillcolor=\"";
    result += type == ExprType::ET_ID
                  ? "#DBABFF"
                  : type == ExprType::ET_EXPR_LIST
                        ? "#81C781"
                        : "#90EE90";
    result += "\", style=filled];\n";

    if (value != nullptr) {
        result += "  node" + std::to_string(GetId()) + " -> node" + std::to_string(value->GetId()) +
                " [label=value];\n";
        result += value->toDot();
    }

    int i = 0;
    for (const auto &child: children) {
        result += "  node" + std::to_string(GetId()) + " -> node" + std::to_string(child->GetId()) +
                " [label=child" + std::to_string(i++) + "];\n";
        result += child->toDot();
    }

    return result;
}

bool ExprNode::doSemantics() {
    Log("starting semantics for " + toString(type) + "...");

    bool isOk = true;

    for (const auto &child: children) {
        isOk = isOk && child->doSemantics();
    }

    switch (type) {
        case ExprType::ET_UNKNOWN:
            Warn("unknown type");
            return true;

        case ExprType::ET_FUNCTION_CALL:
            if (this->children[0]->isSimple()) {
                isOk = false;
                Warn("(ET_FUNCTION_CALL) unexpected simple first child");
                break;
            }
            if (this->children[0]->type == ExprType::ET_ID) {
                // Function name to lower case
                for (char &c: this->children[0]->value->name) {
                    c = tolower(static_cast<unsigned char>(c));
                }

                // fgets and fgetc for stdin only
                if ((this->children[0]->value->name == "fgets" || this->children[0]->value->name == "fgetc")
                    && !((this->children[1]->type == ExprType::ET_ID && this->children[1]->value->name == "STDIN")
                         || (this->children[1]->type == ExprType::ET_EXPR_LIST && this->children[1]->children.size() ==
                             1 && this->children[1]->children[0]->type == ExprType::ET_ID && this->children[1]->children
                             [0]->value->name == "STDIN"))) {
                    Error("(ET_FUNCTION_CALL) only STDIN stream allowed");
                    break;
                }
            }
            break;

        case ExprType::ET_ASSIGN:
            if (children[0]->type != ExprType::ET_SIGIL && children[0]->type != ExprType::ET_ARRAY_INDEX) {
                Error("(ET_ASSIGN) left child isn't a SIGIL or ARRAY_INDEX (" + children[0]->toJson() + ")");
                isOk = false;
                break;
            }

            if (children[0]->type == ExprType::ET_ARRAY_INDEX) {
                this->type = ExprType::ET_ARRAY_ASSIGNMENT;
                const ExprNode *left = children[0];
                ExprNode *right = children[1];
                children.clear();
                children.push_back(left->children[0]);
                children.push_back(left->children[1]);
                children.push_back(right);
                delete left;
            }
            break;

        case ExprType::ET_PROPERTY_ACCESS:
        case ExprType::ET_STATIC_PROPERTY_ACCESS:
            if (this->children[0]->isSimple()) {
                isOk = false;
                Warn("(ET_PROPERTY_ACCESS/ET_STATIC_PROPERTY_ACCESS) unexpected simple first child");
                break;
            }
            break;

        case ExprType::ET_EXPR_LIST:
            break;

        default:
            if (this->children.size() == 1 && this->children[0]->type == ExprType::ET_EXPR_LIST) {
                auto list = this->children[0];
                this->children = list->children;
                delete list;
            }
            break;
    }

    if (isOk) {
        Log("finished semantics for " + toString(type) + "");
    } else {
        Error("semantics for " + toString(type) + " failed");
    }

    return isOk;
}

Class *ExprNode::processClass(Class *root, std::vector<Class *> &list) {
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

bool ExprNode::isSimple() const {
    return this->type == ExprType::ET_INT ||
           this->type == ExprType::ET_FLOAT ||
           this->type == ExprType::ET_STRING ||
           this->type == ExprType::ET_BOOL ||
           this->type == ExprType::ET_NIL;
}

// List
ExprNode *ExprNode::ExprList(ExprNode *expr) {
    auto node = new ExprNode();
    node->type = ExprType::ET_EXPR_LIST;
    node->children.push_back(expr);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::AppendToExprList(ExprNode *exprList, ExprNode *newExpr) {
    exprList->children.push_back(newExpr);
    exprList->WriteToFiles();
    return exprList;
}

// Binary operators
ExprNode *ExprNode::Or(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_OR;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::OrLower(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_OR_LOWER;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::Xor(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_XOR;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::XorBitwise(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_XOR_BITWISE;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::And(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_AND;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::AndLower(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_AND_LOWER;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::OrBitwise(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_OR_BITWISE;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::AndBitwise(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_AND_BITWISE;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

// Assignment operators
ExprNode *ExprNode::Assign(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_ASSIGN;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::MultAssign(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_MULT_ASSIGN;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::PowAssign(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_POW_ASSIGN;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::DivAssign(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_DIV_ASSIGN;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::ModAssign(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_MOD_ASSIGN;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::PlusAssign(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_PLUS_ASSIGN;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::MinusAssign(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_MINUS_ASSIGN;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::ConcatAssign(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_CONCAT_ASSIGN;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::LeftShiftAssign(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_LEFT_SHIFT_ASSIGN;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::RightShiftAssign(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_RIGHT_SHIFT_ASSIGN;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

// Comparison operators
ExprNode *ExprNode::Equal(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_EQUAL;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::NotEqual(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_NOT_EQUAL;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::NotEqualBitwise(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_NOT_EQUAL_BITWISE;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::IdenticallyEqual(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_IDENTICALLY_EQUAL;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::IdenticallyNotEqual(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_IDENTICALLY_NOT_EQUAL;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::Spaceship(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_SPACESHIP;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::NullCoaslescing(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_NULL_COALESCING;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::LessThan(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_LESS_THAN;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::GreaterThan(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_GREATER_THAN;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::LessOrEqual(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_LESS_OR_EQUAL;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::GreatOrEqual(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_GREAT_OR_EQUAL;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

// Bitwise shift operators
ExprNode *ExprNode::LeftShift(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_LEFT_SHIFT;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::RightShift(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_RIGHT_SHIFT;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

// Arithmetic operators
ExprNode *ExprNode::Add(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_ADD;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::Subtract(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_SUBTRACT;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::Concat(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_CONCAT;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::Mult(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_MULT;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::Mod(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_MOD;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::Div(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_DIV;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::Pow(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_POW;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

// Other binary operators
ExprNode *ExprNode::Instanceof(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_INSTANCEOF;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::PropertyAccess(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_PROPERTY_ACCESS;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::MethodAccess(ExprNode *left, ExprNode *right, ExprNode *params) {
    auto node = new ExprNode();
    node->type = ExprType::ET_METHOD_ACCESS;
    node->children.push_back(left);
    node->children.push_back(right);
    if (params != nullptr) {
        node->children.push_back(params);
    }
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::StaticPropertyAccess(ExprNode *left, ExprNode *right) {
    auto node = new ExprNode();
    node->type = ExprType::ET_STATIC_PROPERTY_ACCESS;
    node->children.push_back(left);
    node->children.push_back(right);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::ArrayIndex(ExprNode *array, ExprNode *index) {
    auto node = new ExprNode();
    node->type = ExprType::ET_ARRAY_INDEX;
    node->children.push_back(array);
    node->children.push_back(index);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::ArrayKeyAccess(ExprNode *key, ExprNode *value) {
    auto node = new ExprNode();
    node->type = ExprType::ET_ARRAY_KEY_ACCESS;
    node->children.push_back(key);
    node->children.push_back(value);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::ArrayAppend(ExprNode *array, ExprNode *element) {
    auto node = new ExprNode();
    node->type = ExprType::ET_ARRAY_APPEND;
    node->children.push_back(array);
    node->children.push_back(element);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::Array() {
    auto node = new ExprNode();
    node->type = ExprType::ET_ARRAY_EMPTY;
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::SpreadArray(ExprNode *array) {
    auto node = new ExprNode();
    node->type = ExprType::ET_SPREAD_OPERATOR;
    node->children.push_back(array);
    node->WriteToFiles();
    return node;
}

// Ternary operator
ExprNode *ExprNode::Ternary(ExprNode *condition, ExprNode *trueExpr, ExprNode *falseExpr) {
    auto node = new ExprNode();
    node->type = ExprType::ET_TERNARY;
    node->children.push_back(condition);
    node->children.push_back(trueExpr);
    node->children.push_back(falseExpr);
    node->WriteToFiles();
    return node;
}

// Unary operators
ExprNode *ExprNode::IncrementPost(ExprNode *operand) {
    auto node = new ExprNode();
    node->type = ExprType::ET_INCREMENT_POST;
    node->children.push_back(operand);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::DecrementPost(ExprNode *operand) {
    auto node = new ExprNode();
    node->type = ExprType::ET_DECREMENT_POST;
    node->children.push_back(operand);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::IncrementPre(ExprNode *operand) {
    auto node = new ExprNode();
    node->type = ExprType::ET_INCREMENT_PRE;
    node->children.push_back(operand);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::DecrementPre(ExprNode *operand) {
    auto node = new ExprNode();
    node->type = ExprType::ET_DECREMENT_PRE;
    node->children.push_back(operand);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::Not(ExprNode *operand) {
    auto node = new ExprNode();
    node->type = ExprType::ET_NOT;
    node->children.push_back(operand);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::NotBitwise(ExprNode *operand) {
    auto node = new ExprNode();
    node->type = ExprType::ET_NOT_BITWISE;
    node->children.push_back(operand);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::Uplus(ExprNode *operand) {
    auto node = new ExprNode();
    node->type = ExprType::ET_UPLUS;
    node->children.push_back(operand);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::Uminus(ExprNode *operand) {
    auto node = new ExprNode();
    node->type = ExprType::ET_UMINUS;
    node->children.push_back(operand);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::Sigil(ExprNode *operand) {
    auto node = new ExprNode();
    node->type = ExprType::ET_SIGIL;
    node->children.push_back(operand);
    node->WriteToFiles();
    return node;
}

// Array operations
ExprNode *ExprNode::ArrayElementList(ExprNode *elements) {
    auto node = new ExprNode();
    node->type = ExprType::ET_ARRAY_ELEMENT_LIST;
    node->children.push_back(elements);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::Parenthesized(ExprNode *expr) {
    auto node = new ExprNode();
    node->type = ExprType::ET_PARENTHESIZED;
    node->children.push_back(expr);
    node->WriteToFiles();
    return node;
}

// Function/method calls
ExprNode *ExprNode::FunctionCall(ExprNode *function, ExprNode *arguments) {
    auto node = new ExprNode();
    node->type = ExprType::ET_FUNCTION_CALL;
    node->children.push_back(function);
    if (arguments != nullptr) {
        node->children.push_back(arguments);
    }
    node->WriteToFiles();
    return node;
}

// Object instantiation
ExprNode *ExprNode::New(ExprNode *className, ExprNode *arguments) {
    auto node = new ExprNode();
    node->type = ExprType::ET_NEW;
    node->children.push_back(className);
    node->children.push_back(arguments);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::New(ExprNode *arguments) {
    auto node = new ExprNode();
    node->type = ExprType::ET_NEW;
    node->children.push_back(arguments);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::ComplexString(ExprNode *arguments) {
    auto node = new ExprNode();
    node->type = ExprType::ET_COMPLEX_STRING;
    node->children.push_back(arguments);
    node->WriteToFiles();
    return node;
}

// Literals
ExprNode *ExprNode::String(string *value) {
    auto node = new ExprNode();
    node->type = ExprType::ET_STRING;
    node->value = ValueNode::CreateString(value);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::Id(string *name) {
    auto node = new ExprNode();
    node->type = ExprType::ET_ID;
    node->name = *name;
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::Int(int value) {
    auto node = new ExprNode();
    node->type = ExprType::ET_INT;
    node->value = ValueNode::CreateInt(value);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::Float(float value) {
    auto node = new ExprNode();
    node->type = ExprType::ET_FLOAT;
    node->value = ValueNode::CreateFloat(value);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::Bool(bool value) {
    auto node = new ExprNode();
    node->type = ExprType::ET_BOOL;
    node->value = ValueNode::CreateBool(value);
    node->WriteToFiles();
    return node;
}

ExprNode *ExprNode::Nil() {
    auto node = new ExprNode();
    node->type = ExprType::ET_NIL;
    node->WriteToFiles();
    return node;
}
