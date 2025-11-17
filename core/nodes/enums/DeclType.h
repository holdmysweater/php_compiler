#ifndef PHP_COMPILER_DECLTYPE_H
#define PHP_COMPILER_DECLTYPE_H

#include <string>

using std::string;

enum DeclType {
    DT_UNKNOWN,
    DT_LIST,
    DT_CLASS,
    DT_VARIABLE,
    DT_PROPERTY,
    DT_PARAMETER,
    DT_CONSTANT,
    DT_FUNCTION,
    DT_METHOD,
};

inline string toString(DeclType type) {
    switch (type) {
        case DeclType::DT_UNKNOWN: return "DT_UNKNOWN";
        case DeclType::DT_LIST: return "DT_LIST";
        case DeclType::DT_CLASS: return "DT_CLASS";
        case DeclType::DT_VARIABLE: return "DT_VARIABLE";
        case DeclType::DT_PROPERTY: return "DT_PROPERTY";
        case DeclType::DT_PARAMETER: return "DT_PARAMETER";
        case DeclType::DT_CONSTANT: return "DT_CONSTANT";
        case DeclType::DT_FUNCTION: return "DT_FUNCTION";
        case DeclType::DT_METHOD: return "DT_METHOD";
        default: return "ERROR";
    }
}

inline string toSymbol(DeclType type) {
    switch (type) {
        case DeclType::DT_UNKNOWN: return "unknown";
        case DeclType::DT_LIST: return "decl list";
        case DeclType::DT_CLASS: return "class";
        case DeclType::DT_VARIABLE: return "variable";
        case DeclType::DT_PROPERTY: return "property";
        case DeclType::DT_PARAMETER: return "parameter";
        case DeclType::DT_CONSTANT: return "constant";
        case DeclType::DT_FUNCTION: return "function";
        case DeclType::DT_METHOD: return "method";
        default: return "ERROR";
    }
}

#endif //PHP_COMPILER_DECLTYPE_H
