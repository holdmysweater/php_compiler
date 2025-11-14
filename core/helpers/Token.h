#ifndef PHP_COMPILER_TOKEN_H
#define PHP_COMPILER_TOKEN_H

#include <string>

using std::string;

class Token {
public:
    enum TokenType {
        TOKEN_INT,
        TOKEN_FLOAT,
        TOKEN_STRING,
        TOKEN_IDENTIFIER,
        TOKEN_TYPE
    };

    TokenType type;
    int value_int = 0;
    float value_float = 0;
    string value_string;

    explicit Token(int value);

    explicit Token(float value);

    explicit Token(string value);

    explicit Token(string value, TokenType type);

    string toString() const;
};

#endif //PHP_COMPILER_TOKEN_H
