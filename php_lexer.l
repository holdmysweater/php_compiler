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
FOREACH     [Ff][Oo][Rr][Ee][Aa][Cc][Cc][Hh]
ENDFOREACH  [Ee][Nn][Dd][Ff][Oo][Rr][Ee][Aa][Cc][Cc][Hh]
ECHO        [Ee][Cc][Hh][Oo]
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
%x ECHO_PHP
%x INDENT_HEREDOC_OR_NOWDOC

%{
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
%}

%%

%{
    if (!initialized) {
        Console::Header("LEXER");

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
            case Token::TokenType::TOKEN_INT:
                yylval.int_const = token.value_int;
                return INT;

            case Token::TokenType::TOKEN_FLOAT:
                yylval.float_const = token.value_float;
                return FLOAT;

            case Token::TokenType::TOKEN_STRING:
                yylval.str_const = token.value_string;
                return STRING;

            case Token::TokenType::TOKEN_IDENTIFIER:
                yylval.id_const = token.value_string;
                return ID;

            case Token::TokenType::TOKEN_TYPE:
                yylval.type_const = token.value_string;
                return TYPE;
        }
    }
%}

\<\?=		            { 
                            if (!html->current().empty()) {
                                Console::LexLog(html->getStartLine() > 0 ? html->getStartLine() : 1, yylineno, html->current(), "html content"); 
                            }

                            Console::LexLog(yylineno, yytext, "opening echo php tag"); 
                            BEGIN(states->pushAndPeek(ECHO_PHP));
                        }

\<\?([pP][hH][pP])?		{ 
                            if (!html->current().empty()) {
                                Console::LexLog(html->getStartLine() > 0 ? html->getStartLine() : 1, yylineno, html->current(), "html content"); 
                            }

                            Console::LexLog(yylineno, yytext, "opening php tag"); 
                            BEGIN(states->pushAndPeek(PURE_PHP)); 
                        }

<PURE_PHP,ECHO_PHP>\?\>			{ 
                                    Console::LexLog(yylineno, yytext, "closing php tag"); 
                                    BEGIN(states->popAndPeek()); 
                                    html->reset(); 
                                    html->setStartLine(yylineno); 
                                }
<PURE_PHP,ECHO_PHP><<EOF>>		{ 
                                    Console::LexLog(yylineno, yytext, "end of file - closing php tag"); 
                                    BEGIN(states->popAndPeek()); 
                                    html->reset(); 
                                    html->setStartLine(yylineno); 
                                }

"<!--"					{ BEGIN(states->pushAndPeek(COMMENT_HTML)); }
<COMMENT_HTML>[^\-]+	;
<COMMENT_HTML>"-"		;
<COMMENT_HTML>\-{2}\>	{ BEGIN(states->popAndPeek()); }

<PURE_PHP,ECHO_PHP>("//"|"#").*  { Console::LexLog(yylineno, yytext, "comment", true); }

<PURE_PHP,ECHO_PHP>"/*"    		 {
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

<PURE_PHP,ECHO_PHP,ARRAY_CONTENT>\'	{ BEGIN(states->pushAndPeek(STRING_SINGLE_QUOTES)); buffer->reset(); buffer->setStartLine(yylineno); }
<STRING_SINGLE_QUOTES>\\\n			{ buffer->append(yytext); }
<STRING_SINGLE_QUOTES>\\\'			{ buffer->append("\'"); }
<STRING_SINGLE_QUOTES>\\\\     		{ buffer->append("\\"); }
<STRING_SINGLE_QUOTES>\\[^\\\']     { buffer->append(yytext); }
<STRING_SINGLE_QUOTES>[^\\\']+	{ buffer->append(yytext); }
<STRING_SINGLE_QUOTES>\'			{
										Console::LexLog(buffer->getStartLine(), yylineno, buffer->current(), "string in STRING_SINGLE_QUOTES");
										buffer->reset();
										buffer->setStartLine(yylineno);
										BEGIN(states->popAndPeek());
									}

<PURE_PHP,ECHO_PHP>"<<<"              				{ BEGIN(states->pushAndPeek(STRING_HEREDOC_OR_NOWDOC)); }
<STRING_HEREDOC_OR_NOWDOC>[ \t]*            		;
<STRING_HEREDOC_OR_NOWDOC>(\'{ID}\'|{ID})[^\n]      { Console::LexError("Invalid \"<<<\" identifier in STRING_HEREDOC_OR_NOWDOC", yylineno); BEGIN(states->popAndPeek()); }

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
                                            }
                                            Console::LexLog(buffer->getStartLine(), yylineno-1, buffer->current(), "text in STRING_NOWDOC/STRING_HEREDOC");
                                            Console::LexLog(yylineno, buffer->getDocId() + ";", "end of STRING_NOWDOC/STRING_HEREDOC");
                                            buffer->reset();
                                            buffer->setStartLine(yylineno);
                                            BEGIN(states->popAndPeek());
                                            BEGIN(states->popAndPeek());
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
                                                }
                                                Console::LexLog(buffer->getStartLine(), yylineno-1, buffer->current(), "text in STRING_NOWDOC/STRING_HEREDOC");
                                                Console::LexLog(yylineno, buffer->getDocId() + ";", "end of STRING_NOWDOC/STRING_HEREDOC");
                                                buffer->reset();
                                                buffer->setStartLine(yylineno);
                                                BEGIN(states->popAndPeek());
                                            }
                                        }

<STRING_NOWDOC>[^\n]+                   { buffer->append(yytext); }

<STRING_NOWDOC,STRING_HEREDOC>\n+		{ buffer->appendNewLines(yytext); BEGIN(states->pushAndPeek(INDENT_HEREDOC_OR_NOWDOC)); }

<PURE_PHP,ECHO_PHP,ARRAY_CONTENT>"\""	{ BEGIN(states->pushAndPeek(STRING_DOUBLE_QUOTES)); buffer->reset(); buffer->setStartLine(yylineno); }

<STRING_DOUBLE_QUOTES>[^\\\"\$\{]+	{ buffer->append(yytext); }

<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\\$	    { buffer->append("$"); }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\\{	    { buffer->append("{"); }

<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\${ID}		{
                                                    Console::LexLog(buffer->getStartLine(), yylineno, buffer->current(), "string inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
                                                    Console::LexLog(yylineno, "$", "operator \"$\" inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
                                                    Console::LexLog(yylineno, yytext+1, "identifier inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
                                                    buffer->reset();
                                                    buffer->setStartLine(yylineno);
                                                }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\${ID}\[	{
                                                    Console::LexLog(buffer->getStartLine(), yylineno, buffer->current(), "string inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
                                                    Console::LexLog(yylineno, "$", "operator \"$\" inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
                                                    Console::LexLog(yylineno, Buffer::CutStringEnd(yytext+1, 1), "identifier inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
                                                    buffer->reset();
                                                    buffer->setStartLine(yylineno);
                                                    Console::LexLog(yylineno, "[", "operator [");
                                                    BEGIN(states->pushAndPeek(ARRAY_CONTENT));
                                                }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\$			{ buffer->append("$"); }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\{\$ 		{
                                                    Console::LexLog(buffer->getStartLine(), yylineno, buffer->current(), "string inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
                                                    Console::LexLog(yylineno, "{", "operator { inside STRING_DOUBLE_QUOTES/STRING_HEREDOC before INTERPOLATED_PHP");
                                                    Console::LexLog(yylineno, "$", "operator $ inside INTERPOLATED_PHP");
                                                    buffer->reset();
                                                    buffer->setStartLine(yylineno);
                                                    BEGIN(states->pushAndPeek(INTERPOLATED_PHP));
                                                }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\$\{\$ 	{
                                                    Console::LexLog(buffer->getStartLine(), yylineno, buffer->current(), "string inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
                                                    Console::LexLog(yylineno, "$", "operator \"$\" inside STRING_DOUBLE_QUOTES/STRING_HEREDOC before INTERPOLATED_PHP");
                                                    Console::LexLog(yylineno, "{", "operator { inside STRING_DOUBLE_QUOTES/STRING_HEREDOC before INTERPOLATED_PHP");
                                                    Console::LexLog(yylineno, "$", "operator \"$\" inside INTERPOLATED_PHP");
                                                    buffer->reset();
                                                    buffer->setStartLine(yylineno);
                                                    BEGIN(states->pushAndPeek(INTERPOLATED_PHP));
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
<STRING_DOUBLE_QUOTES>\"			 						{ Console::LexLog(yylineno, buffer->current(), "string inside STRING_DOUBLE_QUOTES"); buffer->reset(); BEGIN(states->popAndPeek()); }

<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\	    { buffer->append("\\"); }
<STRING_HEREDOC>[^\\\$\{\n\t ]+ 		    { buffer->append(yytext); }


<COMMENT_HTML,COMMENT_MULTILINE,STRING_HEREDOC_OR_NOWDOC,STRING_NOWDOC,STRING_HEREDOC,STRING_SINGLE_QUOTES,STRING_DOUBLE_QUOTES,ARRAY_CONTENT><<EOF>> { Console::LexError("end of file", yylineno); BEGIN(INITIAL); }

<INTERPOLATED_PHP>\}    { Console::LexLog(yylineno, yytext, "operator } in INTERPOLATED_PHP"); BEGIN(states->popAndPeek()); }

<ARRAY_CONTENT>"]"	    { Console::LexLog(yylineno, yytext, "operator ] in ARRAY_CONTENT"); BEGIN(states->popAndPeek()); }

<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ENDFOREACH}   { Console::LexLog(yylineno, yytext, "keyword ENDFOREACH"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ENDWHILE}     { Console::LexLog(yylineno, yytext, "keyword ENDWHILE"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ENDSWITCH}    { Console::LexLog(yylineno, yytext, "keyword ENDSWITCH"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ENDFOR}       { Console::LexLog(yylineno, yytext, "keyword ENDFOR"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ENDIF}        { Console::LexLog(yylineno, yytext, "keyword ENDIF"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{FOREACH}      { Console::LexLog(yylineno, yytext, "keyword FOREACH"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{INSTANCEOF}   { Console::LexLog(yylineno, yytext, "keyword INSTANCEOF"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{PROTECTED}    { Console::LexLog(yylineno, yytext, "keyword PROTECTED"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{CONTINUE}     { Console::LexLog(yylineno, yytext, "keyword CONTINUE"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{FUNCTION}     { Console::LexLog(yylineno, yytext, "keyword FUNCTION"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{EXTENDS}      { Console::LexLog(yylineno, yytext, "keyword EXTENDS"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{STATIC}       { Console::LexLog(yylineno, yytext, "keyword STATIC"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{PUBLIC}       { Console::LexLog(yylineno, yytext, "keyword PUBLIC"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{PRIVATE}      { Console::LexLog(yylineno, yytext, "keyword PRIVATE"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{DEFAULT}      { Console::LexLog(yylineno, yytext, "keyword DEFAULT"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ELSEIF}       { Console::LexLog(yylineno, yytext, "keyword ELSEIF"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{SWITCH}       { Console::LexLog(yylineno, yytext, "keyword SWITCH"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{RETURN}       { Console::LexLog(yylineno, yytext, "keyword RETURN"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{BREAK}        { Console::LexLog(yylineno, yytext, "keyword BREAK"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{CONST}        { Console::LexLog(yylineno, yytext, "keyword CONST"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{CLASS}        { Console::LexLog(yylineno, yytext, "keyword CLASS"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{WHILE}        { Console::LexLog(yylineno, yytext, "keyword WHILE"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ELSE}         { Console::LexLog(yylineno, yytext, "keyword ELSE"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{CASE}         { Console::LexLog(yylineno, yytext, "keyword CASE"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ECHO}         { Console::LexLog(yylineno, yytext, "keyword ECHO"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{AND}          { Console::LexLog(yylineno, yytext, "keyword AND"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{XOR}          { Console::LexLog(yylineno, yytext, "keyword XOR"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{NEW}          { Console::LexLog(yylineno, yytext, "keyword NEW"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{VAR}          { Console::LexLog(yylineno, yytext, "keyword VAR"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{FOR}          { Console::LexLog(yylineno, yytext, "keyword FOR"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{OR}           { Console::LexLog(yylineno, yytext, "keyword OR"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{AS}           { Console::LexLog(yylineno, yytext, "keyword AS"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{IF}           { Console::LexLog(yylineno, yytext, "keyword IF"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{DO}           { Console::LexLog(yylineno, yytext, "keyword DO"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>array			 { Console::LexLog(yylineno, yytext, "type array"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>callable	     { Console::LexLog(yylineno, yytext, "type callable"); }

<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"===" 	{ Console::LexLog(yylineno, yytext, "operator ==="); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"!==" 	{ Console::LexLog(yylineno, yytext, "operator !=="); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"::"  	{ Console::LexLog(yylineno, yytext, "operator ::"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"**"  	{ Console::LexLog(yylineno, yytext, "operator **"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"<="  	{ Console::LexLog(yylineno, yytext, "operator <="); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>">="  	{ Console::LexLog(yylineno, yytext, "operator >="); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"=="  	{ Console::LexLog(yylineno, yytext, "operator =="); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"!="  	{ Console::LexLog(yylineno, yytext, "operator !="); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"<>"  	{ Console::LexLog(yylineno, yytext, "operator <>"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"&&"  	{ Console::LexLog(yylineno, yytext, "operator &&"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"||"  	{ Console::LexLog(yylineno, yytext, "operator ||"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"!"   	{ Console::LexLog(yylineno, yytext, "operator !"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"*"   	{ Console::LexLog(yylineno, yytext, "operator *"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"/"   	{ Console::LexLog(yylineno, yytext, "operator /"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"%"   	{ Console::LexLog(yylineno, yytext, "operator %"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"++"      { Console::LexLog(yylineno, yytext, "operator ++"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"+"   	{ Console::LexLog(yylineno, yytext, "operator +"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"."   	{ Console::LexLog(yylineno, yytext, "operator ."); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"--"      { Console::LexLog(yylineno, yytext, "operator --"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"-"   	{ Console::LexLog(yylineno, yytext, "operator -"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"<"   	{ Console::LexLog(yylineno, yytext, "operator <"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>">"   	{ Console::LexLog(yylineno, yytext, "operator >"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"="   	{ Console::LexLog(yylineno, yytext, "operator ="); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>";"	    { Console::LexLog(yylineno, yytext, "operator ;"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>":"	    { Console::LexLog(yylineno, yytext, "operator :"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"?"	    { Console::LexLog(yylineno, yytext, "operator ?"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"{"	    { Console::LexLog(yylineno, yytext, "operator {"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"}"	    { Console::LexLog(yylineno, yytext, "operator }"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"]"	    { Console::LexLog(yylineno, yytext, "operator ]"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>","	    { Console::LexLog(yylineno, yytext, "operator ,"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"$"       { Console::LexLog(yylineno, yytext, "operator $"); }

<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{TRUE}	{ Console::LexLog(yylineno, yytext, "value true"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{FALSE}	{ Console::LexLog(yylineno, yytext, "value false"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{NULL}	{ Console::LexLog(yylineno, yytext, "value null"); }

<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"(" 	{ Console::LexLog(yylineno, yytext, "operator (");}
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>")" 	{ Console::LexLog(yylineno, yytext, "operator )"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"->"  { Console::LexLog(yylineno, yytext, "operator ->"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"=>"  { Console::LexLog(yylineno, yytext, "operator =>"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"["	{ Console::LexLog(yylineno, yytext, "operator ["); BEGIN(states->pushAndPeek(ARRAY_CONTENT)); }

<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ID}	{ Console::LexLog(yylineno, yytext, "value ID"); }

<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{NUMBER_DECIMAL}		{ Console::LexLog(yylineno, (int)strtol(yytext,NULL,10), "value NUMBER_DECIMAL"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{NUMBER_HEXADECIMAL}	{ Console::LexLog(yylineno, (int)strtol(yytext,NULL,16), "value NUMBER_HEXADECIMAL"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{NUMBER_OCTAL}		{ Console::LexLog(yylineno, (int)strtol(yytext,NULL,8), "value NUMBER_OCTAL"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{NUMBER_BINARY}		{ Console::LexLog(yylineno, (int)strtol(yytext+2,NULL,2), "value NUMBER_BINARY"); }

<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{NUMBER_FLOAT}	{ Console::LexLog(yylineno, atof(yytext), "value NUMBER_FLOAT"); }
<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{NUMBER_EXPONENT}	{ Console::LexLog(yylineno, atof(yytext), "value NUMBER_EXPONENT"); }

<PURE_PHP,ECHO_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>[ \t\n]+	;

<INTERPOLATED_PHP>.  { Console::LexError("expected closing '}' in INTERPOLATED_PHP, but found: '" + Buffer::CutStringEnd(yytext, 0) + "'", yylineno); }

<INITIAL>[^<]*	{ html->append(yytext); }
<INITIAL>\<	    { html->append(yytext); }

<*>.			{ Console::LexError("unrecognized symbol '" + Buffer::CutStringEnd(yytext, 0) + "'", yylineno); }

%%
