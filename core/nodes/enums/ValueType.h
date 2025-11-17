#ifndef PHP_COMPILER_VALUETYPE_H
#define PHP_COMPILER_VALUETYPE_H

#include <string>

using std::string;

enum class ValueType {
    TYPE_UNKNOWN,
    TYPE_VALUE_LIST,
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_BOOL,
    TYPE_STRING,
    TYPE_ARRAY,
    TYPE_CLASS,
    TYPE_IDENTIFIER,
    TYPE_FUNCTION,
    TYPE_TYPE,
};

inline string toString(ValueType type) {
    switch (type) {
        case ValueType::TYPE_UNKNOWN: return "TYPE_UNKNOWN";
        case ValueType::TYPE_VALUE_LIST: return "TYPE_VALUE_LIST";
        case ValueType::TYPE_INT: return "TYPE_INT";
        case ValueType::TYPE_FLOAT: return "TYPE_FLOAT";
        case ValueType::TYPE_BOOL: return "TYPE_BOOL";
        case ValueType::TYPE_STRING: return "TYPE_STRING";
        case ValueType::TYPE_ARRAY: return "TYPE_ARRAY";
        case ValueType::TYPE_CLASS: return "TYPE_CLASS";
        case ValueType::TYPE_IDENTIFIER: return "TYPE_IDENTIFIER";
        case ValueType::TYPE_FUNCTION: return "TYPE_FUNCTION";
        case ValueType::TYPE_TYPE: return "TYPE_TYPE";
        default: return "ERROR";
    }
}

inline string toSymbol(ValueType type) {
    switch (type) {
        case ValueType::TYPE_UNKNOWN: return "unknown";
        case ValueType::TYPE_VALUE_LIST: return "value list";
        case ValueType::TYPE_INT: return "int";
        case ValueType::TYPE_FLOAT: return "float";
        case ValueType::TYPE_BOOL: return "bool";
        case ValueType::TYPE_STRING: return "string";
        case ValueType::TYPE_ARRAY: return "array";
        case ValueType::TYPE_CLASS: return "class";
        case ValueType::TYPE_IDENTIFIER: return "id";
        case ValueType::TYPE_FUNCTION: return "fn";
        case ValueType::TYPE_TYPE: return "type";
        default: return "ERROR";
    }
}

#endif //PHP_COMPILER_VALUETYPE_H
