%option noyywrap
%option never-interactive
%option yylineno


%{
	#include <iostream>
	#include <math.h>
	#include <stdlib.h>
	#include <string.h>
	#include <conio.h>
	#include <locale.h>
%}


ID				[a-zA-Z_\x7f-\xff][a-zA-Z0-9_\x7f-\xff]*

DIGIT           [0-9]
NUMBER_INT      {DIGIT}+
NUMBER_FLOAT    ({DIGIT}*[\.]{NUMBER_INT})|({NUMBER_INT}[\.]{DIGIT}*)
NUMBER_EXPONENT	(({NUMBER_INT}|{NUMBER_FLOAT})[eE][+-]?{NUMBER_INT})

NUMBER_DECIMAL 			[1-9][0-9]*|0
NUMBER_HEXADECIMAL 		0[xX][0-9a-fA-F]+
NUMBER_OCTAL 			0[0-7]*
NUMBER_BINARY 			0[bB][01]+

TRUE 			[Tt][Rr][Uu][Ee]
FALSE 			[Ff][Aa][Ll][Ss][Ee]
NULL 			[Nn][Uu][Ll]{2}
