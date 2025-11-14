#include "Token.h"

Token::Token(int value) {
    this->type = Type::TOKEN_INT;
    this->value_int = value;
}

Token::Token(double value) {
    this->type = Type::TOKEN_DOUBLE;
    this->value_double = value;
}

Token::Token(string value) {
    this->type = Type::TOKEN_STRING;
    this->value_string = value;
}

Token::Token(string value, Type type) {
    this->type = type;
    this->value_string = value;
}

string Token::toString() const {
    switch (this->type) {
        case Type::TOKEN_INT:
            return "{ type: INT; value: " + std::to_string(this->value_int) + "; }";
        case Type::TOKEN_DOUBLE:
            return "{ type: DOUBLE; value: " + std::to_string(this->value_double) + "; }";
        case Type::TOKEN_STRING:
            return "{ type: STRING; value: '" + this->value_string + "'; }";
        case Type::TOKEN_IDENTIFIER:
            return "{ type: IDENTIFIER; value: '" + this->value_string + "'; }";
        case Type::TOKEN_TYPE:
            return "{ type: TYPE; value: '" + this->value_string + "'; }";
        default:
            return "{}";
    }
}
