#ifndef PHP_COMPILER_TOKEN_H
#define PHP_COMPILER_TOKEN_H

#include <string>

using std::string;

class Token {
public:
    enum Type {
        TOKEN_INT,
        TOKEN_DOUBLE,
        TOKEN_STRING,
        TOKEN_IDENTIFIER,
        TOKEN_TYPE
    };

    Type type;
    int value_int = 0;
    double value_double = 0;
    string value_string;

    explicit Token(int value);

    explicit Token(double value);

    explicit Token(string value);

    explicit Token(string value, Type type);

    string toString() const;
};

#endif //PHP_COMPILER_TOKEN_H
