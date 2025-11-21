#ifndef PHP_COMPILER_EXPRTYPE_H
#define PHP_COMPILER_EXPRTYPE_H

#include <string>

using std::string;

enum class ExprType {
    ET_UNKNOWN,
    ET_EXPR_LIST,
    ET_OR,
    ET_OR_LOWER,
    ET_XOR,
    ET_XOR_BITWISE,
    ET_AND,
    ET_AND_LOWER,
    ET_OR_BITWISE,
    ET_AND_BITWISE,
    ET_ASSIGN,
    ET_TERNARY,
    ET_MULT_ASSIGN,
    ET_POW_ASSIGN,
    ET_DIV_ASSIGN,
    ET_MOD_ASSIGN,
    ET_PLUS_ASSIGN,
    ET_MINUS_ASSIGN,
    ET_CONCAT_ASSIGN,
    ET_LEFT_SHIFT_ASSIGN,
    ET_RIGHT_SHIFT_ASSIGN,
    ET_EQUAL,
    ET_NOT_EQUAL,
    ET_NOT_EQUAL_BITWISE,
    ET_IDENTICALLY_EQUAL,
    ET_IDENTICALLY_NOT_EQUAL,
    ET_SPACESHIP,
    ET_NULL_COALESCING,
    ET_LESS_THAN,
    ET_GREATER_THAN,
    ET_LESS_OR_EQUAL,
    ET_GREAT_OR_EQUAL,
    ET_LEFT_SHIFT,
    ET_RIGHT_SHIFT,
    ET_ADD,
    ET_SUBTRACT,
    ET_CONCAT,
    ET_MULT,
    ET_MOD,
    ET_DIV,
    ET_INSTANCEOF,
    ET_POW,
    ET_PROPERTY_ACCESS,
    ET_METHOD_ACCESS,
    ET_STATIC_PROPERTY_ACCESS,
    ET_INCREMENT_POST,
    ET_DECREMENT_POST,
    ET_INCREMENT_PRE,
    ET_DECREMENT_PRE,
    ET_NOT,
    ET_NOT_BITWISE,
    ET_UPLUS,
    ET_UMINUS,
    ET_SIGIL,
    ET_ARRAY_ELEMENT_LIST,
    ET_ARRAY_INDEX,
    ET_ARRAY_APPEND,
    ET_ARRAY_EMPTY,
    ET_ARRAY_KEY_ACCESS,
    ET_PARENTHESIZED,
    ET_FUNCTION_CALL,
    ET_NEW,
    ET_COMPLEX_STRING,
    ET_STRING,
    ET_ID,
    ET_INT,
    ET_FLOAT,
    ET_BOOL,
    ET_NIL,
};

inline string toString(ExprType type) {
    switch (type) {
        case ExprType::ET_UNKNOWN: return "ET_UNKNOWN";
        case ExprType::ET_EXPR_LIST: return "ET_EXPR_LIST";
        case ExprType::ET_OR: return "ET_OR";
        case ExprType::ET_OR_LOWER: return "ET_OR_LOWER";
        case ExprType::ET_XOR: return "ET_XOR";
        case ExprType::ET_XOR_BITWISE: return "ET_XOR_BITWISE";
        case ExprType::ET_AND: return "ET_AND";
        case ExprType::ET_AND_LOWER: return "ET_AND_LOWER";
        case ExprType::ET_OR_BITWISE: return "ET_OR_BITWISE";
        case ExprType::ET_AND_BITWISE: return "ET_AND_BITWISE";
        case ExprType::ET_ASSIGN: return "ET_ASSIGN";
        case ExprType::ET_TERNARY: return "ET_TERNARY";
        case ExprType::ET_MULT_ASSIGN: return "ET_MULT_ASSIGN";
        case ExprType::ET_POW_ASSIGN: return "ET_POW_ASSIGN";
        case ExprType::ET_DIV_ASSIGN: return "ET_DIV_ASSIGN";
        case ExprType::ET_MOD_ASSIGN: return "ET_MOD_ASSIGN";
        case ExprType::ET_PLUS_ASSIGN: return "ET_PLUS_ASSIGN";
        case ExprType::ET_MINUS_ASSIGN: return "ET_MINUS_ASSIGN";
        case ExprType::ET_CONCAT_ASSIGN: return "ET_CONCAT_ASSIGN";
        case ExprType::ET_LEFT_SHIFT_ASSIGN: return "ET_LEFT_SHIFT_ASSIGN";
        case ExprType::ET_RIGHT_SHIFT_ASSIGN: return "ET_RIGHT_SHIFT_ASSIGN";
        case ExprType::ET_EQUAL: return "ET_EQUAL";
        case ExprType::ET_NOT_EQUAL: return "ET_NOT_EQUAL";
        case ExprType::ET_NOT_EQUAL_BITWISE: return "ET_NOT_EQUAL_BITWISE";
        case ExprType::ET_IDENTICALLY_EQUAL: return "ET_IDENTICALLY_EQUAL";
        case ExprType::ET_IDENTICALLY_NOT_EQUAL: return "ET_IDENTICALLY_NOT_EQUAL";
        case ExprType::ET_SPACESHIP: return "ET_SPACESHIP";
        case ExprType::ET_NULL_COALESCING: return "ET_NULL_COALESCING";
        case ExprType::ET_LESS_THAN: return "ET_LESS_THAN";
        case ExprType::ET_GREATER_THAN: return "ET_GREATER_THAN";
        case ExprType::ET_LESS_OR_EQUAL: return "ET_LESS_OR_EQUAL";
        case ExprType::ET_GREAT_OR_EQUAL: return "ET_GREAT_OR_EQUAL";
        case ExprType::ET_LEFT_SHIFT: return "ET_LEFT_SHIFT";
        case ExprType::ET_RIGHT_SHIFT: return "ET_RIGHT_SHIFT";
        case ExprType::ET_ADD: return "ET_ADD";
        case ExprType::ET_SUBTRACT: return "ET_SUBTRACT";
        case ExprType::ET_CONCAT: return "ET_CONCAT";
        case ExprType::ET_MULT: return "ET_MULT";
        case ExprType::ET_MOD: return "ET_MOD";
        case ExprType::ET_DIV: return "ET_DIV";
        case ExprType::ET_INSTANCEOF: return "ET_INSTANCEOF";
        case ExprType::ET_POW: return "ET_POW";
        case ExprType::ET_PROPERTY_ACCESS: return "ET_PROPERTY_ACCESS";
        case ExprType::ET_METHOD_ACCESS: return "ET_METHOD_ACCESS";
        case ExprType::ET_STATIC_PROPERTY_ACCESS: return "ET_STATIC_PROPERTY_ACCESS";
        case ExprType::ET_INCREMENT_POST: return "ET_INCREMENT_POST";
        case ExprType::ET_DECREMENT_POST: return "ET_DECREMENT_POST";
        case ExprType::ET_INCREMENT_PRE: return "ET_INCREMENT_PRE";
        case ExprType::ET_DECREMENT_PRE: return "ET_DECREMENT_PRE";
        case ExprType::ET_NOT: return "ET_NOT";
        case ExprType::ET_NOT_BITWISE: return "ET_NOT_BITWISE";
        case ExprType::ET_UPLUS: return "ET_UPLUS";
        case ExprType::ET_UMINUS: return "ET_UMINUS";
        case ExprType::ET_SIGIL: return "ET_SIGIL";
        case ExprType::ET_ARRAY_ELEMENT_LIST: return "ET_ARRAY_ELEMENT_LIST";
        case ExprType::ET_ARRAY_INDEX: return "ET_ARRAY_INDEX";
        case ExprType::ET_ARRAY_APPEND: return "ET_ARRAY_APPEND";
        case ExprType::ET_ARRAY_EMPTY: return "ET_ARRAY_EMPTY";
        case ExprType::ET_ARRAY_KEY_ACCESS: return "ET_ARRAY_KEY_ACCESS";
        case ExprType::ET_PARENTHESIZED: return "ET_PARENTHESIZED";
        case ExprType::ET_FUNCTION_CALL: return "ET_FUNCTION_CALL";
        case ExprType::ET_NEW: return "ET_NEW";
        case ExprType::ET_COMPLEX_STRING: return "ET_COMPLEX_STRING";
        case ExprType::ET_STRING: return "ET_STRING";
        case ExprType::ET_ID: return "ET_ID";
        case ExprType::ET_INT: return "ET_INT";
        case ExprType::ET_FLOAT: return "ET_FLOAT";
        case ExprType::ET_BOOL: return "ET_BOOL";
        case ExprType::ET_NIL: return "ET_NIL";
        default: return "ERROR";
    }
}

inline string toSymbol(ExprType type) {
    switch (type) {
        case ExprType::ET_UNKNOWN: return "unknown";
        case ExprType::ET_EXPR_LIST: return "expr list";
        case ExprType::ET_OR: return "||";
        case ExprType::ET_OR_LOWER: return "or";
        case ExprType::ET_XOR: return "xor";
        case ExprType::ET_XOR_BITWISE: return "^";
        case ExprType::ET_AND: return "&&";
        case ExprType::ET_AND_LOWER: return "and";
        case ExprType::ET_OR_BITWISE: return "|";
        case ExprType::ET_AND_BITWISE: return "&";
        case ExprType::ET_ASSIGN: return "=";
        case ExprType::ET_TERNARY: return "?:";
        case ExprType::ET_MULT_ASSIGN: return "*=";
        case ExprType::ET_POW_ASSIGN: return "**=";
        case ExprType::ET_DIV_ASSIGN: return "/=";
        case ExprType::ET_MOD_ASSIGN: return "%=";
        case ExprType::ET_PLUS_ASSIGN: return "+=";
        case ExprType::ET_MINUS_ASSIGN: return "-=";
        case ExprType::ET_CONCAT_ASSIGN: return ".=";
        case ExprType::ET_LEFT_SHIFT_ASSIGN: return "<<=";
        case ExprType::ET_RIGHT_SHIFT_ASSIGN: return ">>=";
        case ExprType::ET_EQUAL: return "==";
        case ExprType::ET_NOT_EQUAL: return "!=";
        case ExprType::ET_NOT_EQUAL_BITWISE: return "<>";
        case ExprType::ET_IDENTICALLY_EQUAL: return "===";
        case ExprType::ET_IDENTICALLY_NOT_EQUAL: return "!==";
        case ExprType::ET_SPACESHIP: return "<=>";
        case ExprType::ET_NULL_COALESCING: return "??";
        case ExprType::ET_LESS_THAN: return "<";
        case ExprType::ET_GREATER_THAN: return ">";
        case ExprType::ET_LESS_OR_EQUAL: return "<=";
        case ExprType::ET_GREAT_OR_EQUAL: return ">=";
        case ExprType::ET_LEFT_SHIFT: return "<<";
        case ExprType::ET_RIGHT_SHIFT: return ">>";
        case ExprType::ET_ADD: return "+";
        case ExprType::ET_SUBTRACT: return "-";
        case ExprType::ET_CONCAT: return ".";
        case ExprType::ET_MULT: return "*";
        case ExprType::ET_MOD: return "%";
        case ExprType::ET_DIV: return "/";
        case ExprType::ET_INSTANCEOF: return "instanceof";
        case ExprType::ET_POW: return "**";
        case ExprType::ET_PROPERTY_ACCESS: return "->";
        case ExprType::ET_METHOD_ACCESS: return "->()";
        case ExprType::ET_STATIC_PROPERTY_ACCESS: return "::";
        case ExprType::ET_INCREMENT_POST: return "++";
        case ExprType::ET_DECREMENT_POST: return "--";
        case ExprType::ET_INCREMENT_PRE: return "++";
        case ExprType::ET_DECREMENT_PRE: return "--";
        case ExprType::ET_NOT: return "!";
        case ExprType::ET_NOT_BITWISE: return "~";
        case ExprType::ET_UPLUS: return "+";
        case ExprType::ET_UMINUS: return "-";
        case ExprType::ET_SIGIL: return "$";
        case ExprType::ET_ARRAY_ELEMENT_LIST: return "[list]";
        case ExprType::ET_ARRAY_INDEX: return "index[]";
        case ExprType::ET_ARRAY_APPEND: return "[]=";
        case ExprType::ET_ARRAY_EMPTY: return "empty []";
        case ExprType::ET_ARRAY_KEY_ACCESS: return "[ => ]";
        case ExprType::ET_PARENTHESIZED: return "()";
        case ExprType::ET_FUNCTION_CALL: return "function call";
        case ExprType::ET_NEW: return "new";
        case ExprType::ET_COMPLEX_STRING: return "complex string";
        case ExprType::ET_STRING: return "string";
        case ExprType::ET_ID: return "identifier";
        case ExprType::ET_INT: return "int";
        case ExprType::ET_FLOAT: return "float";
        case ExprType::ET_BOOL: return "bool";
        case ExprType::ET_NIL: return "null";
        default: return "ERROR";
    }
}

#endif //PHP_COMPILER_EXPRTYPE_H
