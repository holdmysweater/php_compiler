#include "ValueNode.h"
#include "core/helpers/Console.h"
#include "json.hpp"

using json = nlohmann::json;

string ValueNode::_getClassName() const {
    return "ValueNode";
}

string ValueNode::toJson() const {
    json j;
    j["id"] = GetId();
    j["type"] = toString(type);

    if (!name.empty()) {
        j["name"] = name;
    }

    switch (type) {
        case ValueType::TYPE_INT:
            j["value"] = intValue;
            break;
        case ValueType::TYPE_FLOAT:
            j["value"] = floatValue;
            break;
        case ValueType::TYPE_BOOL:
            j["value"] = boolValue;
            break;
        case ValueType::TYPE_STRING:
            j["value"] = stringValue;
            break;
        case ValueType::TYPE_IDENTIFIER:
        case ValueType::TYPE_TYPE:
        case ValueType::TYPE_CLASS:
        case ValueType::TYPE_FUNCTION:
            break;
        case ValueType::TYPE_ARRAY:
            j["dimensions"] = dimensions;
            if (!valueList.empty()) {
                json arrElements = json::array();
                for (const auto &elem: valueList) {
                    arrElements.push_back(json::parse(elem->toJson()));
                }
                j["elements"] = arrElements;
            }
            break;
        default:
            break;
    }

    return j.dump(2);
}

string ValueNode::toDot() const {
    string result;
    string label;

#ifdef DOT_DEBUG
    label += "(V) ";
#endif

    label += toSymbol(type);

#ifdef DOT_DEBUG
    label += "\\nID: " + std::to_string(GetId());
#endif

    switch (type) {
        case ValueType::TYPE_INT:
            label += "\\nval: " + std::to_string(intValue);
            break;
        case ValueType::TYPE_FLOAT:
            label += "\\nval: " + std::to_string(floatValue);
            break;
        case ValueType::TYPE_BOOL:
            label += "\\nval: " + string(boolValue ? "true" : "false");
            break;
        case ValueType::TYPE_STRING:
            label += "\\nval: '" + stringValue + "'";
            break;
        case ValueType::TYPE_IDENTIFIER:
        case ValueType::TYPE_CLASS:
        case ValueType::TYPE_FUNCTION:
        case ValueType::TYPE_TYPE:
            label += "\\nname: '" + name + "'";
            break;
        case ValueType::TYPE_ARRAY:
            label += "\\ndims: " + std::to_string(dimensions);
            break;
        default:
            break;
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

    result += "  node" + std::to_string(GetId()) + " [label=\"" + label + "\", fillcolor=\"#FFFFE0\", style=filled];\n";

    if (value != nullptr) {
        result += "  node" + std::to_string(GetId()) + " -> node" + std::to_string(value->GetId()) +
                " [label=value];\n";
        result += value->toDot();
    }

    for (const auto &elem: valueList) {
        result += "  node" + std::to_string(GetId()) + " -> node" + std::to_string(elem->GetId()) +
                " [label=valueList];\n";
        result += elem->toDot();
    }

    return result;
}

bool ValueNode::doSemantics() const {
    Console::Warning("ValueNode::doSemantics is empty");
    return true;
}

ValueNode *ValueNode::ValueList(ValueNode *value) {
    auto list = new ValueNode();
    list->type = ValueType::TYPE_ARRAY;
    list->valueList.push_back(value);
    list->WriteToFiles();
    return list;
}

ValueNode *ValueNode::AppendToValueList(ValueNode *valueList, ValueNode *newValue) {
    valueList->valueList.push_back(newValue);
    valueList->WriteToFiles();
    return valueList;
}

ValueNode *ValueNode::CreateInt(int val) {
    auto v = new ValueNode();
    v->type = ValueType::TYPE_INT;
    v->intValue = val;
    v->WriteToFiles();
    return v;
}

ValueNode *ValueNode::CreateFloat(float val) {
    auto v = new ValueNode();
    v->type = ValueType::TYPE_FLOAT;
    v->floatValue = val;
    v->WriteToFiles();
    return v;
}

ValueNode *ValueNode::CreateBool(bool val) {
    auto v = new ValueNode();
    v->type = ValueType::TYPE_BOOL;
    v->boolValue = val;
    v->WriteToFiles();
    return v;
}

ValueNode *ValueNode::CreateString(string *val) {
    auto v = new ValueNode();
    v->type = ValueType::TYPE_STRING;
    v->stringValue = *val;
    v->WriteToFiles();
    return v;
}

ValueNode *ValueNode::CreateIdentifier(string *name) {
    auto v = new ValueNode();
    v->type = ValueType::TYPE_IDENTIFIER;
    v->name = *name;
    v->WriteToFiles();
    return v;
}

ValueNode *ValueNode::CreateClass(string *name) {
    auto v = new ValueNode();
    v->type = ValueType::TYPE_CLASS;
    v->name = *name;
    v->WriteToFiles();
    return v;
}

ValueNode *ValueNode::CreateFunction(string *name) {
    auto v = new ValueNode();
    v->type = ValueType::TYPE_FUNCTION;
    v->name = *name;
    v->WriteToFiles();
    return v;
}

ValueNode *ValueNode::CreateType(string *name) {
    auto v = new ValueNode();
    v->type = ValueType::TYPE_TYPE;
    v->name = *name;
    v->WriteToFiles();
    return v;
}

ValueNode *ValueNode::CreateTypeNull() {
    auto v = new ValueNode();
    v->type = ValueType::TYPE_TYPE;
    v->name = "null";
    v->WriteToFiles();
    return v;
}
