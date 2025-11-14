#include "Token.h"

Token::Token(int value) {
    this->type = TokenType::TOKEN_INT;
    this->value_int = value;
}

Token::Token(float value) {
    this->type = TokenType::TOKEN_FLOAT;
    this->value_float = value;
}

Token::Token(string value) {
    this->type = TokenType::TOKEN_STRING;
    this->value_string = value;
}

Token::Token(string value, TokenType type) {
    this->type = type;
    this->value_string = value;
}

string Token::toString() const {
    switch (this->type) {
        case TokenType::TOKEN_INT:
            return "{ type: INT; value: " + std::to_string(this->value_int) + "; }";
        case TokenType::TOKEN_FLOAT:
            return "{ type: FLOAT; value: " + std::to_string(this->value_float) + "; }";
        case TokenType::TOKEN_STRING:
            return "{ type: TOKEN_STRING; value: " + this->value_string + "; }";
        case TokenType::TOKEN_IDENTIFIER:
            return "{ type: TOKEN_IDENTIFIER; value: " + this->value_string + "; }";
        case TokenType::TOKEN_TYPE:
            return "{ type: TOKEN_TYPE; value: " + this->value_string + "; }";
        default:
            return "{}";
    }
}
