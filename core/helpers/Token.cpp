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
        {264, "ERROR"},
        {265, "NIL"},
        {266, "CONST"},
        {267, "CLASS"},
        {268, "EXTENDS"},
        {269, "PUBLIC"},
        {270, "PROTECTED"},
        {271, "PRIVATE"},
        {272, "AS"},
        {273, "BREAK"},
        {274, "CONTINUE"},
        {275, "DO"},
        {276, "WHILE"},
        {277, "ENDWHILE"},
        {278, "FOR"},
        {279, "ENDFOR"},
        {280, "FOREACH"},
        {281, "ENDFOREACH"},
        {282, "ECHO_KW"},
        {283, "IF"},
        {284, "ELSEIF"},
        {285, "ELSE"},
        {286, "ENDIF"},
        {287, "SWITCH"},
        {288, "CASE"},
        {289, "DEFAULT"},
        {290, "ENDSWITCH"},
        {291, "FUNCTION"},
        {292, "RETURN"},
        {293, "STATIC"},
        {294, "VAR"},
        {295, "INTERPOLATABLE_START"},
        {296, "INTERPOLATABLE_END"},
        {297, "SIMPLE_INTERPOLATION_START"},
        {298, "COMPLEX_INTERPOLATION_START"},
        {299, "INTERPOLATION_END"},
        {300, "KEY_ACCESS"},
        {301, "ARRAY"},
        {302, "SPREAD_OPERATOR"},
        {303, "TRY"},
        {304, "CATCH"},
        {305, "THROW"},
        {306, "FINALLY"},
        {307, "OR"},
        {308, "AND"},
        {309, "XOR"},
        {310, "MULT_ASSIGN"},
        {311, "POW_ASSIGN"},
        {312, "DIV_ASSIGN"},
        {313, "PLUS_ASSIGN"},
        {314, "MINUS_ASSIGN"},
        {315, "CONCAT_ASSIGN"},
        {316, "LEFT_SHIFT_ASSIGN"},
        {317, "RIGHT_SHIFT_ASSIGN"},
        {318, "MOD_ASSIGN"},
        {319, "NULL_COALESCING"},
        {320, "LOGIC_OR"},
        {321, "LOGIC_AND"},
        {322, "EQUAL"},
        {323, "NOT_EQUAL"},
        {324, "IDENTICALLY_EQUAL"},
        {325, "IDENTICALLY_NOT_EQUAL"},
        {326, "NOT_EQUAL_BITWISE"},
        {327, "LESS_OR_EQUAL"},
        {328, "GREAT_OR_EQUAL"},
        {329, "SPACESHIP"},
        {330, "LEFT_SHIFT"},
        {331, "RIGHT_SHIFT"},
        {332, "POW"},
        {333, "UMINUS"},
        {334, "UPLUS"},
        {335, "INSTANCEOF"},
        {336, "INCREMENT"},
        {337, "DECREMENT"},
        {338, "PROPERTY_ACCESS"},
        {339, "STATIC_PROPERTY_ACCESS"},
        {340, "NEW"},

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
        case Type::TOKEN:
            return "{ TOKEN = '" + TokenIdToString(this->value_int) + "' }";
        default:
            return "{}";
    }
}
