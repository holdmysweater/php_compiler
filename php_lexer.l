%option noyywrap
%option never-interactive
%option yylineno

ID				[a-zA-Z_\x7f-\xff][a-zA-Z0-9_\x7f-\xff]*
DIGIT           [0-9]
NUMBER_INT      {DIGIT}+
NUMBER_FLOAT    ({DIGIT}*\.{NUMBER_INT})|({NUMBER_INT}\.{DIGIT}*)
NUMBER_EXPONENT	(({NUMBER_INT}|{NUMBER_FLOAT})[eE][+-]?{NUMBER_INT})

NUMBER_DECIMAL 			[1-9]{DIGIT}*|0
NUMBER_HEXADECIMAL 		0[xX][0-9a-fA-F]+
NUMBER_OCTAL 			0[0-7]*
NUMBER_BINARY 			0[bB][01]+

TRUE 		[Tt][Rr][Uu][Ee]
FALSE 		[Ff][Aa][Ll][Ss][Ee]
NULL 		[Nn][Uu][Ll]{2}

INT         [Ii][Nn][Tt]
FLOAT       [Ff][Ll][Oo][Aa][Tt]
STRING      [Ss][Tt][Rr][Ii][Nn][Gg]
BOOL        [Bb][Oo][Oo][Ll]
ARRAY       [Aa][Rr][Rr][Aa][Yy]
OBJECT      [Oo][Bb][Jj][Ee][Cc][Tt]
MIXED       [Mm][Ii][Xx][Ee][Dd]
CALLABLE    [Cc][Aa][Ll]{2}[Aa][Bb][Ll][Ee]
VOID        [Vv][Oo][Ii][Dd]
NEVER       [Nn][Ee][Vv][Ee][Rr]
ITERABLE    [Ii][Tt][Ee][Rr][Aa][Bb][Ll][Ee]
RESOURCE    [Rr][Ee][Ss][Oo][Uu][Rr][Cc][Ee]

CONST       [Cc][Oo][Nn][Ss][Tt]
CLASS       [Cc][Ll][Aa][Ss][Ss]
EXTENDS     [Ee][Xx][Tt][Ee][Nn][Dd][Ss]
PUBLIC      [Pp][Uu][Bb][Ll][Ii][Cc]
PROTECTED   [Pp][Rr][Oo][Tt][Ee][Cc][Tt][Ee][Dd]
PRIVATE     [Pp][Rr][Ii][Vv][Aa][Tt][Ee]
NEW         [Nn][Ee][Ww]
AS          [Aa][Ss]
AND         [Aa][Nn][Dd]
OR          [Oo][Rr]
XOR         [Xx][Oo][Rr]
BREAK       [Bb][Rr][Ee][Aa][Kk]
CONTINUE    [Cc][Oo][Nn][Tt][Ii][Nn][Uu][Ee]
DO          [Dd][Oo]
WHILE       [Ww][Hh][Ii][Ll][Ee]
ENDWHILE    [Ee][Nn][Dd][Ww][Hh][Ii][Ll][Ee]
FOR         [Ff][Oo][Rr]
ENDFOR      [Ee][Nn][Dd][Ff][Oo][Rr]
FOREACH     [Ff][Oo][Rr][Ee][Aa][Cc][Hh]
ENDFOREACH  [Ee][Nn][Dd][Ff][Oo][Rr][Ee][Aa][Cc][Hh]
ECHO_KW     [Ee][Cc][Hh][Oo]
IF          [Ii][Ff]
ELSEIF      [Ee][Ll][Ss][Ee][Ii][Ff]
ELSE        [Ee][Ll][Ss][Ee]
ENDIF       [Ee][Nn][Dd][Ii][Ff]
SWITCH      [Ss][Ww][Ii][Tt][Cc][Hh]
CASE        [Cc][Aa][Ss][Ee]
DEFAULT     [Dd][Ee][Ff][Aa][Uu][Ll][Tt]
ENDSWITCH   [Ee][Nn][Dd][Ss][Ww][Ii][Tt][Cc][Hh]
FUNCTION    [Ff][Uu][Nn][Cc][Tt][Ii][Oo][Nn]
INSTANCEOF  [Ii][Nn][Ss][Tt][Aa][Nn][Cc][Ee][Oo][Ff]
RETURN      [Rr][Ee][Tt][Uu][Rr][Nn]
STATIC      [Ss][Tt][Aa][Tt][Ii][Cc]
VAR         [Vv][Aa][Rr]
TRY         [Tt][Rr][Yy]
CATCH       [Cc][Aa][Tt][Cc][Hh]
THROW       [Tt][Hh][Rr][Oo][Ww]
FINALLY     [Ff][Ii][Nn][Aa][Ll][Ll][Yy]
INTERFACE   [Ii][Nn][Tt][Ee][Rr][Ff][Aa][Cc][Ee]

%x PURE_PHP
%x INTERPOLATED_PHP
%x COMMENT_HTML
%x COMMENT_MULTILINE
%x STRING_SINGLE_QUOTES
%x STRING_DOUBLE_QUOTES
%x STRING_HEREDOC_OR_NOWDOC
%x STRING_HEREDOC
%x STRING_NOWDOC
%x ARRAY_CONTENT
%x INDENT_HEREDOC_OR_NOWDOC

%{
	#include "cmake-build-debug/generated/php_parser.hpp"
	#include "core/helpers/Console.h"
	#include "core/helpers/StatesTree.h"
	#include "core/helpers/Buffer.h"
	#include "core/helpers/Token.h"
	#include "core/helpers/TokenQueue.h"

    static Buffer* buffer = nullptr;
    static Buffer* html = nullptr;
    static StatesTree* states = nullptr;
    static TokenQueue* tokens = nullptr;

    static bool initialized = false;

    void logSigilAndPushId(char *id, string name);
%}

%%

%{
    if (!initialized) {
        Console::SystemTitle("Lexer initiation...");

        buffer = new Buffer();
        buffer->reset();

        html = new Buffer();
        html->reset();

        states = new StatesTree();
        tokens = new TokenQueue();

        BEGIN(states->pushAndPeek(INITIAL));

        initialized = true;
    }

    if (!tokens->isEmpty()) {
        Token token = tokens->pop();

        switch (token.type) {
            case Token::Type::TOKEN_INT:
                yylval.int_const = token.value_int;
                return INT;

            case Token::Type::TOKEN_DOUBLE:
                yylval.float_const = token.value_double;
                return FLOAT;

            case Token::Type::TOKEN_STRING:
                yylval.string_const = new std::string(token.value_string);
                return STRING;

            case Token::Type::TOKEN_IDENTIFIER:
                yylval.string_const = new std::string(token.value_string);
                return ID;

            case Token::Type::TOKEN_TYPE:
                yylval.string_const = new std::string(token.value_string);
                return TYPE;

            case Token::Type::TOKEN:
                return token.value_int;
        }
    }
%}

\<\?=		            {
                            if (!html->current().empty()) {
                                Console::LexLog(html->getStartLine() > 0 ? html->getStartLine() : 1, yylineno, html->current(), "html content");
                            }

                            Console::LexLog(yylineno, yytext, "opening echo php tag");
                            BEGIN(states->pushAndPeek(PURE_PHP));

                            if (!html->current().empty()) {
                                tokens->push(Token(Token::Type::TOKEN_STRING, html->current()));
                                tokens->push(Token(';', Token::Type::TOKEN));
                                tokens->push(Token(ECHO_KW, Token::Type::TOKEN));
                                html->reset();
                                html->setStartLine(yylineno);
                                return ECHO_KW;
                            }

                            return ECHO_KW;
                        }

\<\?([pP][hH][pP])?		{
                            if (!html->current().empty()) {
                                Console::LexLog(html->getStartLine() > 0 ? html->getStartLine() : 1, yylineno, html->current(), "html content");
                            }

                            Console::LexLog(yylineno, yytext, "opening php tag");
                            BEGIN(states->pushAndPeek(PURE_PHP));

                            if (!html->current().empty()) {
                                tokens->push(Token(Token::Type::TOKEN_STRING, html->current()));
                                tokens->push(Token(';', Token::Type::TOKEN));
                                html->reset();
                                html->setStartLine(yylineno);
                                return ECHO_KW;
                            }
                        }

<PURE_PHP>\?\>			{
                                    Console::LexLog(yylineno, yytext, "closing php tag");
                                    BEGIN(states->popAndPeek());
                                    html->reset();
                                    html->setStartLine(yylineno);

                                    return ';';
                                }
<PURE_PHP><<EOF>>		{
                                    Console::LexLog(yylineno, yytext, "end of file - closing php tag");
                                    BEGIN(states->popAndPeek());
                                    html->reset();
                                    html->setStartLine(yylineno);

                                    return 0;
                                }

"<!--"					{ BEGIN(states->pushAndPeek(COMMENT_HTML)); html->append(yytext); }
<COMMENT_HTML>[^\-]+	{ html->append(yytext); }
<COMMENT_HTML>"-"		{ html->append(yytext); }
<COMMENT_HTML>\-{2}\>	{ html->append(yytext); BEGIN(states->popAndPeek()); }

<PURE_PHP>("//"|"#").*  { Console::LexLog(yylineno, yytext, "comment", true); }

<PURE_PHP>"/*"    		 {
                                    BEGIN(states->pushAndPeek(COMMENT_MULTILINE));
                                    buffer->reset();
                                    buffer->setStartLine(yylineno);
                                    buffer->append(yytext);
                                }
<COMMENT_MULTILINE>[^*]+        { buffer->append(yytext); }
<COMMENT_MULTILINE>"*"+[^*/]    { buffer->append(yytext); }
<COMMENT_MULTILINE>"*"+"/"      {
									buffer->append(yytext);
									Console::LexLog(buffer->getStartLine(), yylineno, buffer->current(), "multiline comment");
									buffer->reset();
									buffer->setStartLine(yylineno);
									BEGIN(states->popAndPeek());
								}

<PURE_PHP,ARRAY_CONTENT>\'	{ BEGIN(states->pushAndPeek(STRING_SINGLE_QUOTES)); buffer->reset(); buffer->setStartLine(yylineno); }
<STRING_SINGLE_QUOTES>\\\n			{ buffer->append(yytext); }
<STRING_SINGLE_QUOTES>\\\'			{ buffer->append("\'"); }
<STRING_SINGLE_QUOTES>\\\\     		{ buffer->append("\\"); }
<STRING_SINGLE_QUOTES>\\[^\\\']     { buffer->append(yytext); }
<STRING_SINGLE_QUOTES>[^\\\']+	{ buffer->append(yytext); }
<STRING_SINGLE_QUOTES>\'			{
										Console::LexLog(buffer->getStartLine(), yylineno, buffer->current(), "string in STRING_SINGLE_QUOTES");
                                        yylval.string_const = new std::string(buffer->current());

										buffer->reset();
										buffer->setStartLine(yylineno);
										BEGIN(states->popAndPeek());

                                        return STRING;
									}

<PURE_PHP>"<<<"              				{ BEGIN(states->pushAndPeek(STRING_HEREDOC_OR_NOWDOC)); }
<STRING_HEREDOC_OR_NOWDOC>[ \t]*            		;
<STRING_HEREDOC_OR_NOWDOC>(\'{ID}\'|{ID})[^\n]      {
                                                        Console::LexError("Invalid '<<<' identifier in STRING_HEREDOC_OR_NOWDOC", yylineno);
                                                        BEGIN(states->popAndPeek());
                                                        return ERROR;
                                                    }

<STRING_HEREDOC_OR_NOWDOC>\'{ID}\'\n    {
											states->popAndPeek();
											buffer->reset();
											buffer->setDocId(yytext+1, 2);
											buffer->setStartLine(yylineno);
											Console::LexLog(yylineno-1, "<<<'" + buffer->getDocId() + "'", "start of STRING_NOWDOC");
											BEGIN(states->pushAndPeek(STRING_NOWDOC));
											BEGIN(states->pushAndPeek(INDENT_HEREDOC_OR_NOWDOC));
										}
<STRING_HEREDOC_OR_NOWDOC>{ID}\n    	{
											states->popAndPeek();
											buffer->reset();
											buffer->setDocId(yytext, 1);
											buffer->setStartLine(yylineno);
											Console::LexLog(yylineno-1, "<<<" + buffer->getDocId(), "start of STRING_HEREDOC");
											BEGIN(states->pushAndPeek(STRING_HEREDOC));
                                            BEGIN(states->pushAndPeek(INDENT_HEREDOC_OR_NOWDOC));

                                            return INTERPOLATABLE_START;
										}

<INDENT_HEREDOC_OR_NOWDOC>[\t ]+  { buffer->append(yytext); BEGIN(states->popAndPeek()); }
<INDENT_HEREDOC_OR_NOWDOC>\n+     { buffer->appendNewLines(yytext); BEGIN(states->popAndPeek()); }
<INDENT_HEREDOC_OR_NOWDOC>.?      { buffer->append(yytext); BEGIN(states->popAndPeek()); }

<INDENT_HEREDOC_OR_NOWDOC>^{ID};	{
                                        if (string(yytext, strcspn(yytext, ";")) != buffer->getDocId()) {
                                            buffer->append(yytext);
                                        }
                                        else {
                                            int errorLine = buffer->validateByLastLineAsIndent();
                                            if (errorLine != -1) {
                                                Console::LexError("Invalid indent in STRING_NOWDOC/STRING_HEREDOC", buffer->getStartLine() + errorLine);
                                                return ERROR;
                                            }

                                            Console::LexLog(buffer->getStartLine(), yylineno-1, buffer->current(), "text in STRING_NOWDOC/STRING_HEREDOC");
                                            yylval.string_const = new std::string(buffer->current());

                                            Console::LexLog(yylineno, buffer->getDocId() + ";", "end of STRING_NOWDOC/STRING_HEREDOC");

                                            buffer->reset();
                                            buffer->setStartLine(yylineno);
                                            BEGIN(states->popAndPeek());

                                            if (states->current() == STRING_HEREDOC) {
                                                tokens->push(Token(INTERPOLATABLE_END, Token::Type::TOKEN));
                                            }

                                            tokens->push(Token(';'));

                                            BEGIN(states->popAndPeek());

                                            return STRING;
                                        }
                                    }

<STRING_NOWDOC,STRING_HEREDOC>[\t ]+    { buffer->append(yytext); }
<STRING_NOWDOC,STRING_HEREDOC>{ID};	    {
                                            if (string(yytext, strcspn(yytext, ";")) != buffer->getDocId()) {
                                                buffer->append(yytext);
                                            }
                                            else {
                                                int errorLine = buffer->validateByLastLineAsIndent();
                                                if (errorLine != -1) {
                                                    Console::LexError("Invalid indent in STRING_NOWDOC/STRING_HEREDOC", buffer->getStartLine() + errorLine);
                                                    return ERROR;
                                                }

                                                Console::LexLog(buffer->getStartLine(), yylineno-1, buffer->current(), "text in STRING_NOWDOC/STRING_HEREDOC");
                                                yylval.string_const = new std::string(buffer->current());

                                                Console::LexLog(yylineno, buffer->getDocId() + ";", "end of STRING_NOWDOC/STRING_HEREDOC");

                                                buffer->reset();
                                                buffer->setStartLine(yylineno);

                                                if (states->current() == STRING_HEREDOC) {
                                                    tokens->push(Token(INTERPOLATABLE_END, Token::Type::TOKEN));
                                                }

                                                tokens->push(Token(';'));

                                                BEGIN(states->popAndPeek());

                                                return STRING;
                                            }
                                        }

<STRING_NOWDOC>[^\n]+                   { buffer->append(yytext); }

<STRING_NOWDOC,STRING_HEREDOC>\n+		{ buffer->appendNewLines(yytext); BEGIN(states->pushAndPeek(INDENT_HEREDOC_OR_NOWDOC)); }

<PURE_PHP,ARRAY_CONTENT>"\""	{
                                            BEGIN(states->pushAndPeek(STRING_DOUBLE_QUOTES));
                                            buffer->reset();
                                            buffer->setStartLine(yylineno);

                                            return INTERPOLATABLE_START;
                                        }

<STRING_DOUBLE_QUOTES>[^\\\"\$\{]+	{ buffer->append(yytext); }

<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\\$	    { buffer->append("$"); }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\\{	    { buffer->append("{"); }

<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\${ID}		{
                                                    Console::LexLog(buffer->getStartLine(), yylineno, buffer->current(), "string inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
                                                    yylval.string_const = new std::string(buffer->current());

                                                    tokens->push(Token(SIMPLE_INTERPOLATION_START, Token::Type::TOKEN));

                                                    Console::LexLog(yylineno, "$", "operator '$' inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
                                                    tokens->push(Token('$', Token::Type::TOKEN));

                                                    Console::LexLog(yylineno, yytext+1, "identifier inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
                                                    tokens->push(Token(Token::Type::TOKEN_IDENTIFIER, yytext+1));

                                                    tokens->push(Token(INTERPOLATION_END, Token::Type::TOKEN));

                                                    buffer->reset();
                                                    buffer->setStartLine(yylineno);

                                                    return STRING;
                                                }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\${ID}\[	{
                                                    Console::LexLog(buffer->getStartLine(), yylineno, buffer->current(), "string inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
                                                    yylval.string_const = new std::string(buffer->current());

                                                    tokens->push(Token(SIMPLE_INTERPOLATION_START, Token::Type::TOKEN));

                                                    Console::LexLog(yylineno, "$", "operator '$' inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
                                                    tokens->push(Token('$', Token::Type::TOKEN));

                                                    Console::LexLog(yylineno, Buffer::CutStringEnd(yytext+1, 1), "identifier inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
                                                    tokens->push(Token(Token::Type::TOKEN_IDENTIFIER, Buffer::CutStringEnd(yytext+1, 1)));

                                                    buffer->reset();
                                                    buffer->setStartLine(yylineno);

                                                    Console::LexLog(yylineno, "[", "operator [");
                                                    tokens->push(Token('[', Token::Type::TOKEN));

                                                    BEGIN(states->pushAndPeek(ARRAY_CONTENT));

                                                    return STRING;
                                                }

<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\${ID}->{ID}	{
                                                        Console::LexLog(buffer->getStartLine(), yylineno, buffer->current(), "string inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
                                                        yylval.string_const = new std::string(buffer->current());

                                                        tokens->push(Token(SIMPLE_INTERPOLATION_START, Token::Type::TOKEN));

                                                        Console::LexLog(yylineno, "$", "operator '$' inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
                                                        tokens->push(Token('$', Token::Type::TOKEN));

                                                        Console::LexLog(yylineno, Buffer::getLeftPropertyAccess(yytext), "identifier inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
                                                        tokens->push(Token(Token::Type::TOKEN_IDENTIFIER, Buffer::getLeftPropertyAccess(yytext)));

                                                        Console::LexLog(yylineno, "->", "operator '->' inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
                                                        tokens->push(Token(PROPERTY_ACCESS, Token::Type::TOKEN));

                                                        Console::LexLog(yylineno, Buffer::getRightPropertyAccess(yytext), "identifier inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
                                                        tokens->push(Token(Token::Type::TOKEN_IDENTIFIER, Buffer::getRightPropertyAccess(yytext)));

                                                        tokens->push(Token(INTERPOLATION_END, Token::Type::TOKEN));

                                                        buffer->reset();
                                                        buffer->setStartLine(yylineno);

                                                        return STRING;
                                                    }

<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\$			{ buffer->append("$"); }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\{\$    	{
                                                    Console::LexError("Expected identifier after '$' at the start of INTERPOLATED_PHP", yylineno);
                                                    return ERROR;
                                                }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\{\${ID} 	{
                                                    Console::LexLog(buffer->getStartLine(), yylineno, buffer->current(), "string inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
                                                    yylval.string_const = new std::string(buffer->current());

                                                    Console::LexLog(yylineno, "{", "operator { inside STRING_DOUBLE_QUOTES/STRING_HEREDOC before INTERPOLATED_PHP");
                                                    tokens->push(Token(COMPLEX_INTERPOLATION_START, Token::Type::TOKEN));

                                                    Console::LexLog(yylineno, "$", "operator $ inside INTERPOLATED_PHP");
                                                    tokens->push(Token('$', Token::Type::TOKEN));

                                                    buffer->reset();
                                                    buffer->setStartLine(yylineno);
                                                    BEGIN(states->pushAndPeek(INTERPOLATED_PHP));

                                                    Console::LexLog(yylineno, yytext+2, "identifier inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
                                                    tokens->push(Token(Token::Type::TOKEN_IDENTIFIER, yytext+2));

                                                    return STRING;
                                                }

<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\{						{ buffer->append(yytext); }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\\\					{ buffer->append("\\"); }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\n					{ buffer->append("\n"); }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\r					{ buffer->append("\r"); }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\t					{ buffer->append("\t"); }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\v			 		{ buffer->append("\v"); }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\e			 		{ buffer->append("\e"); }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\f			 		{ buffer->append("\f"); }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\[0-7]{1,3}  			{ buffer->append(string(1, (char)strtol(yytext+1,NULL,8))); }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\x[0-9A-Fa-f]{1,2}   	{ buffer->append(string(1, (char)strtol(yytext+2,NULL,16))); }
<STRING_DOUBLE_QUOTES>\\\"			 						{ buffer->append("\""); }
<STRING_DOUBLE_QUOTES>\"			 						{
                                                                Console::LexLog(yylineno, buffer->current(), "string inside STRING_DOUBLE_QUOTES");
                                                                yylval.string_const = new std::string(buffer->current());

                                                                tokens->push(Token(INTERPOLATABLE_END, Token::Type::TOKEN));

                                                                buffer->reset();
                                                                buffer->setStartLine(yylineno);
                                                                BEGIN(states->popAndPeek());

                                                                return STRING;
                                                            }

<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\	    { buffer->append("\\"); }
<STRING_HEREDOC>[^\\\$\{\n\t ]+ 		    { buffer->append(yytext); }


<COMMENT_HTML,COMMENT_MULTILINE,STRING_HEREDOC_OR_NOWDOC,STRING_NOWDOC,STRING_HEREDOC,STRING_SINGLE_QUOTES,STRING_DOUBLE_QUOTES,ARRAY_CONTENT><<EOF>> {
    Console::LexError("End of file", yylineno);
    BEGIN(INITIAL);
    return ERROR;
}

<INTERPOLATED_PHP>\}    {
                            Console::LexLog(yylineno, yytext, "operator } in INTERPOLATED_PHP");
                            BEGIN(states->popAndPeek());
                            return INTERPOLATION_END;
                        }

<ARRAY_CONTENT>"]"	    {
                            Console::LexLog(yylineno, yytext, "operator ']' in ARRAY_CONTENT");
                            BEGIN(states->popAndPeek());

                            if (states->current() == STRING_HEREDOC || states->current() == STRING_DOUBLE_QUOTES) {
                                tokens->push(Token(INTERPOLATION_END, Token::Type::TOKEN));
                            }

                            return ']';
                        }

<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\$PHP_EOL   { logSigilAndPushId(yytext+1, "PHP_EOL"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${ARRAY}   { logSigilAndPushId(yytext+1, "ARRAY"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${ENDFOREACH}   { logSigilAndPushId(yytext+1, "ENDFOREACH"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${ENDWHILE}   { logSigilAndPushId(yytext+1, "ENDWHILE"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${ENDSWITCH}   { logSigilAndPushId(yytext+1, "ENDSWITCH"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${ENDFOR}   { logSigilAndPushId(yytext+1, "ENDFOR"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${ENDIF}   { logSigilAndPushId(yytext+1, "ENDIF"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${FOREACH}   { logSigilAndPushId(yytext+1, "FOREACH"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${PROTECTED}   { logSigilAndPushId(yytext+1, "PROTECTED"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${CONTINUE}   { logSigilAndPushId(yytext+1, "CONTINUE"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${FUNCTION}   { logSigilAndPushId(yytext+1, "FUNCTION"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${EXTENDS}   { logSigilAndPushId(yytext+1, "EXTENDS"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${STATIC}   { logSigilAndPushId(yytext+1, "STATIC"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${PUBLIC}   { logSigilAndPushId(yytext+1, "PUBLIC"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${PRIVATE}   { logSigilAndPushId(yytext+1, "PRIVATE"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${DEFAULT}   { logSigilAndPushId(yytext+1, "DEFAULT"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${ELSEIF}   { logSigilAndPushId(yytext+1, "ELSEIF"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${SWITCH}   { logSigilAndPushId(yytext+1, "SWITCH"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${RETURN}   { logSigilAndPushId(yytext+1, "RETURN"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${BREAK}   { logSigilAndPushId(yytext+1, "BREAK"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${CONST}   { logSigilAndPushId(yytext+1, "CONST"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${CLASS}   { logSigilAndPushId(yytext+1, "CLASS"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${INTERFACE}   { logSigilAndPushId(yytext+1, "INTERFACE"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${WHILE}   { logSigilAndPushId(yytext+1, "WHILE"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${ELSE}   { logSigilAndPushId(yytext+1, "ELSE"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${CASE}   { logSigilAndPushId(yytext+1, "CASE"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${ECHO_KW}   { logSigilAndPushId(yytext+1, "ECHO_KW"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${VAR}   { logSigilAndPushId(yytext+1, "VAR"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${TRY}   { logSigilAndPushId(yytext+1, "TRY"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${CATCH}   { logSigilAndPushId(yytext+1, "CATCH"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${THROW}   { logSigilAndPushId(yytext+1, "THROW"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${FINALLY}   { logSigilAndPushId(yytext+1, "FINALLY"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${FOR}   { logSigilAndPushId(yytext+1, "FOR"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${AS}   { logSigilAndPushId(yytext+1, "AS"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${IF}   { logSigilAndPushId(yytext+1, "IF"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${DO}   { logSigilAndPushId(yytext+1, "DO"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${AND}   { logSigilAndPushId(yytext+1, "AND"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${XOR}   { logSigilAndPushId(yytext+1, "XOR"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${NEW}   { logSigilAndPushId(yytext+1, "NEW"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${OR}   { logSigilAndPushId(yytext+1, "OR"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${INT}   { logSigilAndPushId(yytext+1, "INT"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${FLOAT}   { logSigilAndPushId(yytext+1, "FLOAT"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${STRING}   { logSigilAndPushId(yytext+1, "STRING"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${BOOL}   { logSigilAndPushId(yytext+1, "BOOL"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${OBJECT}   { logSigilAndPushId(yytext+1, "OBJECT"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${MIXED}   { logSigilAndPushId(yytext+1, "MIXED"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${CALLABLE}   { logSigilAndPushId(yytext+1, "CALLABLE"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${VOID}   { logSigilAndPushId(yytext+1, "VOID"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${NEVER}   { logSigilAndPushId(yytext+1, "NEVER"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${ITERABLE}   { logSigilAndPushId(yytext+1, "ITERABLE"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${RESOURCE}   { logSigilAndPushId(yytext+1, "RESOURCE"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${INSTANCEOF}   { logSigilAndPushId(yytext+1, "INSTANCEOF"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${TRUE}   { logSigilAndPushId(yytext+1, "TRUE"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${FALSE}   { logSigilAndPushId(yytext+1, "FALSE"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${NULL}   { logSigilAndPushId(yytext+1, "NULL"); return '$'; }

<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>PHP_EOL     { Console::LexLog(yylineno, yytext, "keyword PHP_EOL"); yylval.string_const = new std::string("\n"); return STRING; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ARRAY}     { Console::LexLog(yylineno, yytext, "keyword ARRAY"); return ARRAY; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ENDFOREACH}   { Console::LexLog(yylineno, yytext, "keyword ENDFOREACH"); return ENDFOREACH; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ENDWHILE}     { Console::LexLog(yylineno, yytext, "keyword ENDWHILE"); return ENDWHILE; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ENDSWITCH}    { Console::LexLog(yylineno, yytext, "keyword ENDSWITCH"); return ENDSWITCH; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ENDFOR}       { Console::LexLog(yylineno, yytext, "keyword ENDFOR"); return ENDFOR; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ENDIF}        { Console::LexLog(yylineno, yytext, "keyword ENDIF"); return ENDIF; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{FOREACH}      { Console::LexLog(yylineno, yytext, "keyword FOREACH"); return FOREACH; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{PROTECTED}    { Console::LexLog(yylineno, yytext, "keyword PROTECTED"); return PROTECTED; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{CONTINUE}     { Console::LexLog(yylineno, yytext, "keyword CONTINUE"); return CONTINUE; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{FUNCTION}     { Console::LexLog(yylineno, yytext, "keyword FUNCTION"); return FUNCTION; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{EXTENDS}      { Console::LexLog(yylineno, yytext, "keyword EXTENDS"); return EXTENDS; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{STATIC}       { Console::LexLog(yylineno, yytext, "keyword STATIC"); return STATIC; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{PUBLIC}       { Console::LexLog(yylineno, yytext, "keyword PUBLIC"); return PUBLIC; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{PRIVATE}      { Console::LexLog(yylineno, yytext, "keyword PRIVATE"); return PRIVATE; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{DEFAULT}      { Console::LexLog(yylineno, yytext, "keyword DEFAULT"); return DEFAULT; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ELSEIF}       { Console::LexLog(yylineno, yytext, "keyword ELSEIF"); return ELSEIF; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{SWITCH}       { Console::LexLog(yylineno, yytext, "keyword SWITCH"); return SWITCH; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{RETURN}       { Console::LexLog(yylineno, yytext, "keyword RETURN"); return RETURN; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{BREAK}        { Console::LexLog(yylineno, yytext, "keyword BREAK"); return BREAK; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{CONST}        { Console::LexLog(yylineno, yytext, "keyword CONST"); return CONST; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{CLASS}        { Console::LexLog(yylineno, yytext, "keyword CLASS"); return CLASS; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{INTERFACE}    { Console::LexLog(yylineno, yytext, "keyword INTERFACE"); return INTERFACE; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{WHILE}        { Console::LexLog(yylineno, yytext, "keyword WHILE"); return WHILE; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ELSE}         { Console::LexLog(yylineno, yytext, "keyword ELSE"); return ELSE; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{CASE}         { Console::LexLog(yylineno, yytext, "keyword CASE"); return CASE; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ECHO_KW}      { Console::LexLog(yylineno, yytext, "keyword ECHO_KW"); return ECHO_KW; }

<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{VAR}          { Console::LexLog(yylineno, yytext, "keyword VAR"); return VAR; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{TRY}          { Console::LexLog(yylineno, yytext, "keyword TRY"); return TRY; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{CATCH}        { Console::LexLog(yylineno, yytext, "keyword CATCH"); return CATCH; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{THROW}        { Console::LexLog(yylineno, yytext, "keyword THROW"); return THROW; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{FINALLY}      { Console::LexLog(yylineno, yytext, "keyword FINALLY"); return FINALLY; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{FOR}          { Console::LexLog(yylineno, yytext, "keyword FOR"); return FOR; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{AS}           { Console::LexLog(yylineno, yytext, "keyword AS"); return AS; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{IF}           { Console::LexLog(yylineno, yytext, "keyword IF"); return IF; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{DO}           { Console::LexLog(yylineno, yytext, "keyword DO"); return DO; }

<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{AND}          { Console::LexLog(yylineno, yytext, "keyword AND"); return AND; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{XOR}          { Console::LexLog(yylineno, yytext, "keyword XOR"); return XOR; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{NEW}          { Console::LexLog(yylineno, yytext, "keyword NEW"); return NEW; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{OR}           { Console::LexLog(yylineno, yytext, "keyword OR"); return OR; }

<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{INT}          { Console::LexLog(yylineno, yytext, "keyword INT"); yylval.string_const = new std::string("int"); return TYPE; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{FLOAT}        { Console::LexLog(yylineno, yytext, "keyword FLOAT"); yylval.string_const = new std::string("float"); return TYPE; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{STRING}       { Console::LexLog(yylineno, yytext, "keyword STRING"); yylval.string_const = new std::string("string"); return TYPE; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{BOOL}         { Console::LexLog(yylineno, yytext, "keyword BOOL"); yylval.string_const = new std::string("bool"); return TYPE; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{OBJECT}       { Console::LexLog(yylineno, yytext, "keyword OBJECT"); yylval.string_const = new std::string("object"); return TYPE; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{MIXED}        { Console::LexLog(yylineno, yytext, "keyword MIXED"); yylval.string_const = new std::string("mixed"); return TYPE; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{CALLABLE}     { Console::LexLog(yylineno, yytext, "keyword CALLABLE"); yylval.string_const = new std::string("callable"); return TYPE; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{VOID}         { Console::LexLog(yylineno, yytext, "keyword VOID"); yylval.string_const = new std::string("void"); return TYPE; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{NEVER}        { Console::LexLog(yylineno, yytext, "keyword NEVER"); yylval.string_const = new std::string("never"); return TYPE; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ITERABLE}     { Console::LexLog(yylineno, yytext, "keyword ITERABLE"); yylval.string_const = new std::string("iterable"); return TYPE; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{RESOURCE}     { Console::LexLog(yylineno, yytext, "keyword RESOURCE"); yylval.string_const = new std::string("resource"); return TYPE; }

<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{INSTANCEOF}   { Console::LexLog(yylineno, yytext, "keyword INSTANCEOF"); return INSTANCEOF; }

<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"="   	{ Console::LexLog(yylineno, yytext, "operator '='"); return *yytext; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"/="  	{ Console::LexLog(yylineno, yytext, "operator '/='"); return DIV_ASSIGN; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"%="  	{ Console::LexLog(yylineno, yytext, "operator '%='"); return MOD_ASSIGN; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"*="  	{ Console::LexLog(yylineno, yytext, "operator '*='"); return MULT_ASSIGN; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"**="  	{ Console::LexLog(yylineno, yytext, "operator '**='"); return POW_ASSIGN; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"+="  	{ Console::LexLog(yylineno, yytext, "operator '+='"); return PLUS_ASSIGN; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"-="  	{ Console::LexLog(yylineno, yytext, "operator '-='"); return MINUS_ASSIGN; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>".="  	{ Console::LexLog(yylineno, yytext, "operator '.='"); return CONCAT_ASSIGN; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"<<="  	{ Console::LexLog(yylineno, yytext, "operator '<<='"); return LEFT_SHIFT_ASSIGN; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>">>="  	{ Console::LexLog(yylineno, yytext, "operator '>>='"); return RIGHT_SHIFT_ASSIGN; }

<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"..." 	{ Console::LexLog(yylineno, yytext, "operator '...'"); return SPREAD_OPERATOR; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"===" 	{ Console::LexLog(yylineno, yytext, "operator '==='"); return IDENTICALLY_EQUAL; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"!==" 	{ Console::LexLog(yylineno, yytext, "operator '!=='"); return IDENTICALLY_NOT_EQUAL; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"::"  	{ Console::LexLog(yylineno, yytext, "operator '::'"); return STATIC_PROPERTY_ACCESS; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"**"  	{ Console::LexLog(yylineno, yytext, "operator '**'"); return POW; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"<="  	{ Console::LexLog(yylineno, yytext, "operator '<='"); return LESS_OR_EQUAL; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>">="  	{ Console::LexLog(yylineno, yytext, "operator '>='"); return GREAT_OR_EQUAL; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"<<"  	{ Console::LexLog(yylineno, yytext, "operator '<<'"); return LEFT_SHIFT; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>">>"  	{ Console::LexLog(yylineno, yytext, "operator '>>'"); return RIGHT_SHIFT; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"=="  	{ Console::LexLog(yylineno, yytext, "operator '=='"); return EQUAL; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"!="  	{ Console::LexLog(yylineno, yytext, "operator '!='"); return NOT_EQUAL; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"<>"  	{ Console::LexLog(yylineno, yytext, "operator '<>'"); return NOT_EQUAL_BITWISE; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"<=>"  	{ Console::LexLog(yylineno, yytext, "operator '<=>'"); return SPACESHIP; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"&&"  	{ Console::LexLog(yylineno, yytext, "operator '&&'"); return LOGIC_AND; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"||"  	{ Console::LexLog(yylineno, yytext, "operator '||'"); return LOGIC_OR; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"??"  	{ Console::LexLog(yylineno, yytext, "operator '??'"); return NULL_COALESCING; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"++"      { Console::LexLog(yylineno, yytext, "operator '++'"); return INCREMENT; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"--"      { Console::LexLog(yylineno, yytext, "operator '--'"); return DECREMENT; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"&"   	{ Console::LexLog(yylineno, yytext, "operator '&'"); return *yytext; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"|"   	{ Console::LexLog(yylineno, yytext, "operator '|'"); return *yytext; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"^"   	{ Console::LexLog(yylineno, yytext, "operator '^'"); return *yytext; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"!"   	{ Console::LexLog(yylineno, yytext, "operator '!'"); return *yytext; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"~"   	{ Console::LexLog(yylineno, yytext, "operator '~'"); return *yytext; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"*"   	{ Console::LexLog(yylineno, yytext, "operator '*'"); return *yytext; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"/"   	{ Console::LexLog(yylineno, yytext, "operator '/'"); return *yytext; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"%"   	{ Console::LexLog(yylineno, yytext, "operator '%'"); return *yytext; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"+"   	{ Console::LexLog(yylineno, yytext, "operator '+'"); return *yytext; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"."   	{ Console::LexLog(yylineno, yytext, "operator '.'"); return *yytext; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"-"   	{ Console::LexLog(yylineno, yytext, "operator '-'"); return *yytext; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"<"   	{ Console::LexLog(yylineno, yytext, "operator '<'"); return *yytext; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>">"   	{ Console::LexLog(yylineno, yytext, "operator '>'"); return *yytext; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>";"	    { Console::LexLog(yylineno, yytext, "operator ';'"); return *yytext; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>":"	    { Console::LexLog(yylineno, yytext, "operator ':'"); return *yytext; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"?"	    { Console::LexLog(yylineno, yytext, "operator '?'"); return *yytext; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"{"	    { Console::LexLog(yylineno, yytext, "operator '{'"); return *yytext; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"}"	    { Console::LexLog(yylineno, yytext, "operator '}'"); return *yytext; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"]"	    { Console::LexLog(yylineno, yytext, "operator ']'"); return *yytext; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>","	    { Console::LexLog(yylineno, yytext, "operator ','"); return *yytext; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"$"       { Console::LexLog(yylineno, yytext, "operator '$'"); return *yytext; }

<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{TRUE}	{ Console::LexLog(yylineno, yytext, "value true"); yylval.int_const = 1; return BOOL; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{FALSE}	{ Console::LexLog(yylineno, yytext, "value false"); yylval.int_const = 0; return BOOL; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{NULL}	{ Console::LexLog(yylineno, yytext, "value null"); return NIL; }

<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"(" 	{ Console::LexLog(yylineno, yytext, "operator '('"); return *yytext; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>")" 	{ Console::LexLog(yylineno, yytext, "operator ')'"); return *yytext; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"->"  { Console::LexLog(yylineno, yytext, "operator '->'"); return PROPERTY_ACCESS; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"=>"  { Console::LexLog(yylineno, yytext, "operator '=>'"); return KEY_ACCESS; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"["	{ Console::LexLog(yylineno, yytext, "operator '['"); BEGIN(states->pushAndPeek(ARRAY_CONTENT)); return *yytext; }

<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{NUMBER_DECIMAL}		{
                                                                            Console::LexLog(yylineno, (int)strtol(yytext,NULL,10), "value NUMBER_DECIMAL");
                                                                            yylval.int_const = (int)strtol(yytext,NULL,10);
                                                                            return INT;
                                                                        }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{NUMBER_HEXADECIMAL}	{
                                                                            Console::LexLog(yylineno, (int)strtol(yytext,NULL,16), "value NUMBER_HEXADECIMAL");
                                                                            yylval.int_const = (int)strtol(yytext,NULL,16);
                                                                            return INT;
                                                                        }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{NUMBER_OCTAL}		{
                                                                            Console::LexLog(yylineno, (int)strtol(yytext,NULL,8), "value NUMBER_OCTAL");
                                                                            yylval.int_const = (int)strtol(yytext,NULL,8);
                                                                            return INT;
                                                                        }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{NUMBER_BINARY}		{
                                                                            Console::LexLog(yylineno, (int)strtol(yytext+2,NULL,2), "value NUMBER_BINARY");
                                                                            yylval.int_const = (int)strtol(yytext+2,NULL,2);
                                                                            return INT;
                                                                        }

<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{NUMBER_FLOAT}	{
                                                                        Console::LexLog(yylineno, atof(yytext), "value NUMBER_FLOAT");
                                                                        yylval.float_const = atof(yytext);
                                                                        return FLOAT;
                                                                    }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{NUMBER_EXPONENT}	{
                                                                        Console::LexLog(yylineno, atof(yytext), "value NUMBER_EXPONENT");
                                                                        yylval.float_const = atof(yytext);
                                                                        return FLOAT;
                                                                    }

<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>\${ID}	{ logSigilAndPushId(yytext+1, "ID"); return '$'; }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ID}	{
                                                    Console::LexLog(yylineno, yytext, "value ID");
                                                    yylval.string_const = new std::string(yytext);
                                                    return ID;
                                                }

<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>[ \t\n]+	;

<INTERPOLATED_PHP>.     {
                            Console::LexError("Expected closing '}' in INTERPOLATED_PHP, but found: '" + Buffer::CutStringEnd(yytext, 0) + "'", yylineno);
                            return ERROR;
                        }

<INITIAL>[^<]*	{ html->append(yytext); }
<INITIAL>\<	    { html->append(yytext); }


<INITIAL><<EOF>> {
    if (!html->current().empty()) {
        Console::LexLog(html->getStartLine() > 0 ? html->getStartLine() : 1, yylineno, html->current(), "html content");
        tokens->push(Token(Token::Type::TOKEN_STRING, html->current()));
        tokens->push(Token(';', Token::Type::TOKEN));

        html->reset();
        html->setStartLine(yylineno);

        return ECHO_KW;
    }

    return 0;
}

<*>.			{
                    Console::LexError("Unrecognized symbol '" + Buffer::CutStringEnd(yytext, 0) + "'", yylineno);
                    return ERROR;
                }

%%

void logSigilAndPushId(char *id, string name) {
    Console::LexLog(yylineno, "$", "operator '$' before keyword");
    Console::LexLog(yylineno, id, "identifier " + name);
    tokens->push(Token(Token::Type::TOKEN_IDENTIFIER, id));
}