#include "Token.h"

#include <unordered_map>

Token::Token(int value) {
    this->type = Type::TOKEN;
    this->value_int = value;
}

Token::Token(int value, Type type) {
    this->type = type;
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

Token::Token(Type type, string value) {
    this->type = type;
    this->value_string = value;
}

inline std::string TokenIdToString(int token_id) {
    static const std::unordered_map<int, std::string> token_names = {
        {258, "INT"},
        {259, "BOOL"},
        {260, "FLOAT"},
        {261, "STRING"},
        {262, "ID"},
        {263, "TYPE"},
        {264, "HTML_CONTENT"},
        {265, "ERROR"},
        {266, "NIL"},
        {267, "CONST"},
        {268, "CLASS"},
        {269, "EXTENDS"},
        {270, "PUBLIC"},
        {271, "PROTECTED"},
        {272, "PRIVATE"},
        {273, "AS"},
        {274, "BREAK"},
        {275, "CONTINUE"},
        {276, "DO"},
        {277, "WHILE"},
        {278, "ENDWHILE"},
        {279, "FOR"},
        {280, "ENDFOR"},
        {281, "FOREACH"},
        {282, "ENDFOREACH"},
        {283, "ECHO_KW"},
        {284, "IF"},
        {285, "ELSEIF"},
        {286, "ELSE"},
        {287, "ENDIF"},
        {288, "SWITCH"},
        {289, "CASE"},
        {290, "DEFAULT"},
        {291, "ENDSWITCH"},
        {292, "FUNCTION"},
        {293, "RETURN"},
        {294, "STATIC"},
        {295, "VAR"},
        {296, "START_TAG"},
        {297, "START_ECHO_TAG"},
        {298, "END_TAG"},
        {299, "INTERPOLATABLE_START"},
        {300, "INTERPOLATABLE_END"},
        {301, "SIMPLE_INTERPOLATION_START"},
        {302, "COMPLEX_INTERPOLATION_START"},
        {303, "INTERPOLATION_END"},
        {304, "KEY_ACCESS"},
        {305, "OR"},
        {306, "AND"},
        {307, "XOR"},
        {308, "MULT_ASSIGN"},
        {309, "POW_ASSIGN"},
        {310, "DIV_ASSIGN"},
        {311, "PLUS_ASSIGN"},
        {312, "MINUS_ASSIGN"},
        {313, "CONCAT_ASSIGN"},
        {314, "LEFT_SHIFT_ASSIGN"},
        {315, "RIGHT_SHIFT_ASSIGN"},
        {316, "LOGIC_OR"},
        {317, "LOGIC_AND"},
        {318, "EQUAL"},
        {319, "NOT_EQUAL"},
        {320, "IDENTICALLY_EQUAL"},
        {321, "IDENTICALLY_NOT_EQUAL"},
        {322, "NOT_EQUAL_BITWISE"},
        {323, "LESS_OR_EQUAL"},
        {324, "GREAT_OR_EQUAL"},
        {325, "LEFT_SHIFT"},
        {326, "RIGHT_SHIFT"},
        {327, "POW"},
        {328, "UMINUS"},
        {329, "UPLUS"},
        {330, "INSTANCEOF"},
        {331, "INCREMENT"},
        {332, "DECREMENT"},
        {333, "PROPERTY_ACCESS"},
        {334, "STATIC_PROPERTY_ACCESS"},
        {335, "NEW"},

        // Single character operators
        {'+', "+"},
        {'-', "-"},
        {'*', "*"},
        {'/', "/"},
        {'%', "%"},
        {'.', "."},
        {'!', "!"},
        {'=', "="},
        {'<', "<"},
        {'>', ">"},
        {';', ";"},
        {':', ":"},
        {'{', "{"},
        {'}', "}"},
        {'[', "["},
        {']', "]"},
        {',', ","},
        {'$', "$"},
        {'(', "("},
        {')', ")"},
        {'?', "?"},
        {'|', "|"},
        {'^', "^"},
        {'&', "&"},
        {'~', "~"},
    };

    auto it = token_names.find(token_id);
    if (it != token_names.end()) {
        return it->second;
    }

    return "UNKNOWN_TOKEN_" + std::to_string(token_id);
}

string Token::toString() const {
    switch (this->type) {
        case Type::TOKEN_INT:
            return "{ INT = " + std::to_string(this->value_int) + " }";
        case Type::TOKEN_DOUBLE:
            return "{ DOUBLE = " + std::to_string(this->value_double) + " }";
        case Type::TOKEN_STRING:
            return "{ STRING = '" + this->value_string + "' }";
        case Type::TOKEN_IDENTIFIER:
            return "{ ID = '" + this->value_string + "' }";
        case Type::TOKEN_TYPE:
            return "{ TYPE = '" + this->value_string + "' }";
        case Type::TOKEN_HTML:
            return "{ HTML = '" + this->value_string + "' }";
        case Type::TOKEN:
            return "{ TOKEN = '" + TokenIdToString(this->value_int) + "' }";
        default:
            return "{}";
    }
}
