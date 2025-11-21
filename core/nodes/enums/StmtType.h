#ifndef PHP_COMPILER_STMTTYPE_H
#define PHP_COMPILER_STMTTYPE_H

#include <string>

using std::string;

enum StmtType {
    ST_STMT_LIST,
    ST_EXPRESSION,
    ST_WHILE,
    ST_DO_WHILE,
    ST_FOR,
    ST_FOREACH,
    ST_IF,
    ST_ELSE_IF,
    ST_ELSE,
    ST_SWITCH,
    ST_CASE,
    ST_CASE_DEFAULT,
    ST_ECHO,
    ST_RETURN,
    ST_BREAK,
    ST_CONTINUE,
    ST_UNKNOWN
};

inline string toString(StmtType type) {
    switch (type) {
        case StmtType::ST_STMT_LIST: return "ST_STMT_LIST";
        case StmtType::ST_EXPRESSION: return "ST_EXPRESSION";
        case StmtType::ST_WHILE: return "ST_WHILE";
        case StmtType::ST_DO_WHILE: return "ST_DO_WHILE";
        case StmtType::ST_FOR: return "ST_FOR";
        case StmtType::ST_FOREACH: return "ST_FOREACH";
        case StmtType::ST_IF: return "ST_IF";
        case StmtType::ST_ELSE_IF: return "ST_ELSE_IF";
        case StmtType::ST_ELSE: return "ST_ELSE";
        case StmtType::ST_SWITCH: return "ST_SWITCH";
        case StmtType::ST_CASE: return "ST_CASE";
        case StmtType::ST_CASE_DEFAULT: return "ST_CASE_DEFAULT";
        case StmtType::ST_ECHO: return "ST_ECHO";
        case StmtType::ST_RETURN: return "ST_RETURN";
        case StmtType::ST_BREAK: return "ST_BREAK";
        case StmtType::ST_CONTINUE: return "ST_CONTINUE";
        case StmtType::ST_UNKNOWN: return "ST_UNKNOWN";
        default: return "ERROR";
    }
}

inline string toSymbol(StmtType type) {
    switch (type) {
        case StmtType::ST_STMT_LIST: return "stmt list";
        case StmtType::ST_EXPRESSION: return "stmt";
        case StmtType::ST_DO_WHILE: return "do while";
        case StmtType::ST_WHILE: return "while";
        case StmtType::ST_FOR: return "for";
        case StmtType::ST_FOREACH: return "foreach";
        case StmtType::ST_IF: return "if";
        case StmtType::ST_ELSE_IF: return "else if";
        case StmtType::ST_ELSE: return "else";
        case StmtType::ST_SWITCH: return "switch";
        case StmtType::ST_CASE: return "case";
        case StmtType::ST_CASE_DEFAULT: return "default case";
        case StmtType::ST_ECHO: return "echo";
        case StmtType::ST_RETURN: return "return";
        case StmtType::ST_BREAK: return "break";
        case StmtType::ST_CONTINUE: return "continue";
        case StmtType::ST_UNKNOWN: return "unknown";
        default: return "ERROR";
    }
}

#endif //PHP_COMPILER_STMTTYPE_H
