#ifndef PHP_COMPILER_ELEMENTTYPE_H
#define PHP_COMPILER_ELEMENTTYPE_H

#include <string>

using std::string;

enum ElementType {
    ELEMENT_UNKNOWN,
    ELEMENT_PROGRAM_LIST,
    ELEMENT_HTML,
    ELEMENT_PHP,
    ELEMENT_ECHO_PHP,
    ELEMENT_PHP_LIST,
    ELEMENT_STATEMENT,
    ELEMENT_CLASS,
    ELEMENT_FUNCTION,
};

inline string toString(ElementType type) {
    switch (type) {
        case ElementType::ELEMENT_UNKNOWN: return "ELEMENT_UNKNOWN";
        case ElementType::ELEMENT_PROGRAM_LIST: return "ELEMENT_PROGRAM_LIST";
        case ElementType::ELEMENT_HTML: return "ELEMENT_HTML";
        case ElementType::ELEMENT_PHP: return "ELEMENT_PHP";
        case ElementType::ELEMENT_ECHO_PHP: return "ELEMENT_ECHO_PHP";
        case ElementType::ELEMENT_PHP_LIST: return "ELEMENT_PHP_LIST";
        case ElementType::ELEMENT_STATEMENT: return "ELEMENT_STATEMENT";
        case ElementType::ELEMENT_CLASS: return "ELEMENT_CLASS";
        case ElementType::ELEMENT_FUNCTION: return "ELEMENT_FUNCTION";
        default: return "ERROR";
    }
}

inline string toSymbol(ElementType type) {
    switch (type) {
        case ElementType::ELEMENT_UNKNOWN: return "unknown";
        case ElementType::ELEMENT_PROGRAM_LIST: return "program list";
        case ElementType::ELEMENT_HTML: return "html";
        case ElementType::ELEMENT_PHP: return "php";
        case ElementType::ELEMENT_ECHO_PHP: return "php echo";
        case ElementType::ELEMENT_PHP_LIST: return "php list";
        case ElementType::ELEMENT_STATEMENT: return "statement";
        case ElementType::ELEMENT_CLASS: return "class";
        case ElementType::ELEMENT_FUNCTION: return "function";
        default: return "ERROR";
    }
}

#endif //PHP_COMPILER_ELEMENTTYPE_H
