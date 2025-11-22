#ifndef PHP_COMPILER_ELEMENTTYPE_H
#define PHP_COMPILER_ELEMENTTYPE_H

#include <string>

using std::string;

enum ElementType {
    ELEMENT_UNKNOWN,
    ELEMENT_EMPTY,
    ELEMENT_PROGRAM_LIST,
    ELEMENT_STATEMENT,
    ELEMENT_CLASS_DECL,
    ELEMENT_FUNC_DECL,
};

inline string toString(ElementType type) {
    switch (type) {
        case ElementType::ELEMENT_UNKNOWN: return "ELEMENT_UNKNOWN";
        case ElementType::ELEMENT_EMPTY: return "ELEMENT_EMPTY";
        case ElementType::ELEMENT_PROGRAM_LIST: return "ELEMENT_PROGRAM_LIST";
        case ElementType::ELEMENT_STATEMENT: return "ELEMENT_STATEMENT";
        case ElementType::ELEMENT_CLASS_DECL: return "ELEMENT_CLASS";
        case ElementType::ELEMENT_FUNC_DECL: return "ELEMENT_FUNCTION";
        default: return "ERROR";
    }
}

inline string toSymbol(ElementType type) {
    switch (type) {
        case ElementType::ELEMENT_UNKNOWN: return "unknown";
        case ElementType::ELEMENT_EMPTY: return "empty";
        case ElementType::ELEMENT_PROGRAM_LIST: return "program list";
        case ElementType::ELEMENT_STATEMENT: return "statement";
        case ElementType::ELEMENT_CLASS_DECL: return "class";
        case ElementType::ELEMENT_FUNC_DECL: return "function";
        default: return "ERROR";
    }
}

#endif //PHP_COMPILER_ELEMENTTYPE_H
