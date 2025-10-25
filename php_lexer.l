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

TRUE 			[Tt][Rr][Uu][Ee]
FALSE 			[Ff][Aa][Ll][Ss][Ee]
NULL 			[Nn][Uu][Ll]{2}

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


%{
	#define CONSOLE_LOG_ENABLED
	#define CONSOLE_STATE_LOG_ENABLED

	#include <iostream>
	#include <vector>
	#include <cstring>
	#include <cstdlib>
	#include <sstream>

	using std::ostringstream;
	using std::string;
	using std::vector;
	using std::cout;

	class Console {
	public:
		static void LogHeader();
		static void Log(const string message);
		static void Log(int line, const string content, const string info);
		static void Log(int line, int content, const string info);
		static void Log(int line, double content, const string info);
		static void Log(int line, const string content, const string info, bool isSecondaryColor);
		static void Log(int lineStart, int lineEnd, const string content, const string info);
		static void Error(const string message, int line);
	};

	class State {
	public:
		static vector<int> _states;
		
		static int PushAndPeek(int state);
		static int PopAndPeek();
		static int Current();
		static int Previous();

		static string GetStateTree();
		static string StateCodeToString(int state);
	};

	class Buffer {
	public:
		static string _buffer;
		static string _docId;
		static int _startLine;

		static string Current();
		static void Reset();
		static void Append(const string text);
		static void SetStartLine(int line);
		static int GetStartLine();
		static void SetDocId(const string id, int erase = 0);
		static string GetDocId();
		static string CutStringEnd(const string str, int count);
	};
%}


%%


%{
	Console::LogHeader();
	BEGIN(State::PushAndPeek(INITIAL));
%}

\<\?([pP][hH][pP])?				{ Console::Log(yylineno, yytext, "opening php tag"); BEGIN(State::PushAndPeek(PURE_PHP)); }
<PURE_PHP>\?\>					{ Console::Log(yylineno, yytext, "closing php tag"); BEGIN(State::PushAndPeek(INITIAL)); }
<PURE_PHP><<EOF>>				{ Console::Log(yylineno, yytext, "end of file - closing php tag"); BEGIN(State::PushAndPeek(INITIAL)); }

"<!--"							{ BEGIN(State::PushAndPeek(COMMENT_HTML)); }
<COMMENT_HTML>[^\-]+			;
<COMMENT_HTML>"-"				;
<COMMENT_HTML>\-{2}\>			{ BEGIN(State::PopAndPeek()); }

<PURE_PHP>("//"|"#").*       	{ Console::Log(yylineno, yytext, "comment", true); }

<PURE_PHP>"/*"    				{ 
									BEGIN(State::PushAndPeek(COMMENT_MULTILINE)); 
									Buffer::Reset(); 
									Buffer::SetStartLine(yylineno);
									Buffer::Append(yytext);
								}
<COMMENT_MULTILINE>[^*]+        { Buffer::Append(yytext); }
<COMMENT_MULTILINE>"*"+[^*/]    { Buffer::Append(yytext); }
<COMMENT_MULTILINE>"*"+"/"      { 
									Buffer::Append(yytext);
									Console::Log(Buffer::GetStartLine(), yylineno, Buffer::Current(), "multiline comment");
									Buffer::Reset();
									Buffer::SetStartLine(yylineno);
									BEGIN(State::PopAndPeek()); 
								}

<PURE_PHP,ARRAY_CONTENT>\'			{ BEGIN(State::PushAndPeek(STRING_SINGLE_QUOTES)); Buffer::Reset(); Buffer::SetStartLine(yylineno); }
<STRING_SINGLE_QUOTES>\n			{ Console::Error("new line in STRING_SINGLE_QUOTES", yylineno); }
<STRING_SINGLE_QUOTES>\\\n			{ Buffer::Append(yytext); }
<STRING_SINGLE_QUOTES>\\\'			{ Buffer::Append("\'"); }
<STRING_SINGLE_QUOTES>\\\\     		{ Buffer::Append("\\"); }
<STRING_SINGLE_QUOTES>\\[^\\\']     { Buffer::Append(yytext); }
<STRING_SINGLE_QUOTES>[^\\\'\n]+	{ Buffer::Append(yytext); }
<STRING_SINGLE_QUOTES>\'			{
										Console::Log(Buffer::GetStartLine(), yylineno, Buffer::Current(), "string in STRING_SINGLE_QUOTES");
										Buffer::Reset();
										Buffer::SetStartLine(yylineno);
										BEGIN(State::PopAndPeek());
									}

<PURE_PHP>"<<<"              						{  BEGIN(State::PushAndPeek(STRING_HEREDOC_OR_NOWDOC)); }
<STRING_HEREDOC_OR_NOWDOC>[ \t]*            		;
<STRING_HEREDOC_OR_NOWDOC>(\'{ID}\'|{ID})[^\n]      { Console::Error("Invalid \"<<<\" identifier in STRING_HEREDOC_OR_NOWDOC", yylineno); BEGIN(State::PopAndPeek()); }

<STRING_HEREDOC_OR_NOWDOC>\'{ID}\'\n    { 
											State::PopAndPeek();  
											Buffer::Reset();
											Buffer::SetDocId(yytext+1, 2); 
											Buffer::SetStartLine(yylineno); 
											Console::Log(yylineno, Buffer::GetDocId(), "start of STRING_NOWDOC");
											BEGIN(State::PushAndPeek(STRING_NOWDOC)); 
										}
<STRING_HEREDOC_OR_NOWDOC>{ID}\n    	{ 
											State::PopAndPeek();  
											Buffer::Reset(); 
											Buffer::SetDocId(yytext, 1); 
											Buffer::SetStartLine(yylineno); 
											Console::Log(yylineno, Buffer::GetDocId(), "start of STRING_HEREDOC");
											BEGIN(State::PushAndPeek(STRING_HEREDOC)); 
										}

<STRING_NOWDOC,STRING_HEREDOC>^{ID};\n			{
													if (string(yytext, strcspn(yytext, ";")) != Buffer::GetDocId()) {
														Buffer::Append(yytext);
													}
													else {
														Console::Log(Buffer::GetStartLine(), yylineno-2, Buffer::Current(), "text in STRING_NOWDOC/STRING_HEREDOC");
														Console::Log(yylineno-1, Buffer::GetDocId(), "end of STRING_NOWDOC/STRING_HEREDOC");
														Buffer::Reset();
														Buffer::SetStartLine(yylineno);
														BEGIN(State::PopAndPeek());
													}
												}

<STRING_NOWDOC>\n+					{ Buffer::Append(yytext); }
<STRING_NOWDOC>[^\s\n]+				{ Buffer::Append(yytext); }

<PURE_PHP,ARRAY_CONTENT>"\""						{  BEGIN(State::PushAndPeek(STRING_DOUBLE_QUOTES)); Buffer::Reset(); Buffer::SetStartLine(yylineno); }

<STRING_DOUBLE_QUOTES>\n			 				{ Console::Error("new line in STRING_DOUBLE_QUOTES", yylineno); }

<STRING_DOUBLE_QUOTES>[^\\\"\$\{\n]+					{ Buffer::Append(yytext); }

<STRING_HEREDOC>[^\\\$\{]							{ Buffer::Append(yytext); }

<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\\$			{ Buffer::Append("$"); }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\\{			{ Buffer::Append("{"); }

<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\${ID}			{
														Console::Log(Buffer::GetStartLine(), yylineno, Buffer::Current(), "string inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
														Console::Log(yylineno, "$", "operator \"$\" inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
														Console::Log(yylineno, yytext+1, "identifier inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
														Buffer::Reset();
														Buffer::SetStartLine(yylineno);
													}
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\${ID}\[		{
														Console::Log(Buffer::GetStartLine(), yylineno, Buffer::Current(), "string inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
														Console::Log(yylineno, "$", "operator \"$\" inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
														Console::Log(yylineno, Buffer::CutStringEnd(yytext+1, 1), "identifier inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
														Buffer::Reset();
														Buffer::SetStartLine(yylineno);
														Console::Log(yylineno, "[", "operator [");
														BEGIN(State::PushAndPeek(ARRAY_CONTENT));
														
													}
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\$				{ Buffer::Append("$"); }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\{\$ 			{
														Console::Log(Buffer::GetStartLine(), yylineno, Buffer::Current(), "string inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
														Console::Log(yylineno, "{", "operator { inside STRING_DOUBLE_QUOTES/STRING_HEREDOC before INTERPOLATED_PHP");
														Console::Log(yylineno, "$", "operator $ inside INTERPOLATED_PHP");
														Buffer::Reset();
														Buffer::SetStartLine(yylineno);
														BEGIN(State::PushAndPeek(INTERPOLATED_PHP));
													}
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\$\{\$ 		{
														Console::Log(Buffer::GetStartLine(), yylineno, Buffer::Current(), "string inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
														Console::Log(yylineno, "$", "operator \"$\" inside STRING_DOUBLE_QUOTES/STRING_HEREDOC before INTERPOLATED_PHP");
														Console::Log(yylineno, "{", "operator { inside STRING_DOUBLE_QUOTES/STRING_HEREDOC before INTERPOLATED_PHP");
														Console::Log(yylineno, "$", "operator \"$\" inside INTERPOLATED_PHP");
														Buffer::Reset();
														Buffer::SetStartLine(yylineno);
														BEGIN(State::PushAndPeek(INTERPOLATED_PHP));
													}

<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\{						{ Buffer::Append(yytext); }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\\\					{ Buffer::Append("\\"); }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\n					{ Buffer::Append("\n"); }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\r					{ Buffer::Append("\r"); }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\t					{ Buffer::Append("\t"); }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\v			 		{ Buffer::Append("\v"); }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\e			 		{ Buffer::Append("\e"); }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\f			 		{ Buffer::Append("\f"); }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\[0-7]{1,3}  			{ Buffer::Append(string(1, (char)strtol(yytext+1,NULL,8))); }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\x[0-9A-Fa-f]{1,2}   	{ Buffer::Append(string(1, (char)strtol(yytext+2,NULL,16))); }
<STRING_DOUBLE_QUOTES>\\\"			 						{ Buffer::Append("\""); }
<STRING_DOUBLE_QUOTES>\"			 						{ Console::Log(yylineno, Buffer::Current(), "string inside STRING_DOUBLE_QUOTES"); Buffer::Reset(); BEGIN(State::PopAndPeek()); }

<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\		{ Buffer::Append("\\"); }
<STRING_HEREDOC>[^\\\"\$\{\n]+ 				{ Buffer::Append(yytext); }


<COMMENT_HTML,COMMENT_MULTILINE,STRING_HEREDOC_OR_NOWDOC,STRING_NOWDOC,STRING_HEREDOC,STRING_SINGLE_QUOTES,STRING_DOUBLE_QUOTES,ARRAY_CONTENT><<EOF>> { Console::Error("end of file", yylineno); BEGIN(INITIAL); }

<INTERPOLATED_PHP>\}	{ Console::Log(yylineno, yytext, "operator } in INTERPOLATED_PHP"); BEGIN(State::PopAndPeek()); }

<ARRAY_CONTENT>"]"		{ Console::Log(yylineno, yytext, "operator ] in ARRAY_CONTENT"); BEGIN(State::PopAndPeek()); }

<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ENDFOREACH}   { Console::Log(yylineno, yytext, "keyword ENDFOREACH"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ENDWHILE}     { Console::Log(yylineno, yytext, "keyword ENDWHILE"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ENDSWITCH}    { Console::Log(yylineno, yytext, "keyword ENDSWITCH"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ENDFOR}       { Console::Log(yylineno, yytext, "keyword ENDFOR"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ENDIF}        { Console::Log(yylineno, yytext, "keyword ENDIF"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{FOREACH}      { Console::Log(yylineno, yytext, "keyword FOREACH"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{INSTANCEOF}   { Console::Log(yylineno, yytext, "keyword INSTANCEOF"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{PROTECTED}    { Console::Log(yylineno, yytext, "keyword PROTECTED"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{CONTINUE}     { Console::Log(yylineno, yytext, "keyword CONTINUE"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{FUNCTION}     { Console::Log(yylineno, yytext, "keyword FUNCTION"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{EXTENDS}      { Console::Log(yylineno, yytext, "keyword EXTENDS"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{STATIC}       { Console::Log(yylineno, yytext, "keyword STATIC"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{PUBLIC}       { Console::Log(yylineno, yytext, "keyword PUBLIC"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{PRIVATE}      { Console::Log(yylineno, yytext, "keyword PRIVATE"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{DEFAULT}      { Console::Log(yylineno, yytext, "keyword DEFAULT"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ELSEIF}       { Console::Log(yylineno, yytext, "keyword ELSEIF"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{SWITCH}       { Console::Log(yylineno, yytext, "keyword SWITCH"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{RETURN}       { Console::Log(yylineno, yytext, "keyword RETURN"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{BREAK}        { Console::Log(yylineno, yytext, "keyword BREAK"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{CONST}        { Console::Log(yylineno, yytext, "keyword CONST"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{CLASS}        { Console::Log(yylineno, yytext, "keyword CLASS"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{WHILE}        { Console::Log(yylineno, yytext, "keyword WHILE"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ELSE}         { Console::Log(yylineno, yytext, "keyword ELSE"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{CASE}         { Console::Log(yylineno, yytext, "keyword CASE"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ECHO}         { Console::Log(yylineno, yytext, "keyword ECHO"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{AND}          { Console::Log(yylineno, yytext, "keyword AND"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{XOR}          { Console::Log(yylineno, yytext, "keyword XOR"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{NEW}          { Console::Log(yylineno, yytext, "keyword NEW"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{VAR}          { Console::Log(yylineno, yytext, "keyword VAR"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{FOR}          { Console::Log(yylineno, yytext, "keyword FOR"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{OR}           { Console::Log(yylineno, yytext, "keyword OR"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{AS}           { Console::Log(yylineno, yytext, "keyword AS"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{IF}           { Console::Log(yylineno, yytext, "keyword IF"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{DO}           { Console::Log(yylineno, yytext, "keyword DO"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>array			 { Console::Log(yylineno, yytext, "type array"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>callable		 { Console::Log(yylineno, yytext, "type callable"); }

<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"===" 	{ Console::Log(yylineno, yytext, "operator ==="); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"!==" 	{ Console::Log(yylineno, yytext, "operator !=="); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"::"  	{ Console::Log(yylineno, yytext, "operator ::"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"**"  	{ Console::Log(yylineno, yytext, "operator **"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"<="  	{ Console::Log(yylineno, yytext, "operator <="); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>">="  	{ Console::Log(yylineno, yytext, "operator >="); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"=="  	{ Console::Log(yylineno, yytext, "operator =="); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"!="  	{ Console::Log(yylineno, yytext, "operator !="); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"<>"  	{ Console::Log(yylineno, yytext, "operator <>"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"&&"  	{ Console::Log(yylineno, yytext, "operator &&"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"||"  	{ Console::Log(yylineno, yytext, "operator ||"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"!"   	{ Console::Log(yylineno, yytext, "operator !"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"*"   	{ Console::Log(yylineno, yytext, "operator *"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"/"   	{ Console::Log(yylineno, yytext, "operator /"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"%"   	{ Console::Log(yylineno, yytext, "operator %"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"++"   { Console::Log(yylineno, yytext, "operator ++"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"+"   	{ Console::Log(yylineno, yytext, "operator +"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"."   	{ Console::Log(yylineno, yytext, "operator ."); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"--"   { Console::Log(yylineno, yytext, "operator --"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"-"   	{ Console::Log(yylineno, yytext, "operator -"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"<"   	{ Console::Log(yylineno, yytext, "operator <"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>">"   	{ Console::Log(yylineno, yytext, "operator >"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"="   	{ Console::Log(yylineno, yytext, "operator ="); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>";"	{ Console::Log(yylineno, yytext, "operator ;"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>":"	{ Console::Log(yylineno, yytext, "operator :"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"?"	{ Console::Log(yylineno, yytext, "operator ?"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"{"	{ Console::Log(yylineno, yytext, "operator {"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"}"	{ Console::Log(yylineno, yytext, "operator }"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"]"	{ Console::Log(yylineno, yytext, "operator ]"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>","	{ Console::Log(yylineno, yytext, "operator ,"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"$"   	{ Console::Log(yylineno, yytext, "operator $"); }

<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{TRUE}		{ Console::Log(yylineno, yytext, "value true"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{FALSE}		{ Console::Log(yylineno, yytext, "value false"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{NULL}		{ Console::Log(yylineno, yytext, "value null"); }

<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"(" 	{ Console::Log(yylineno, yytext, "operator (");}
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>")" 	{ Console::Log(yylineno, yytext, "operator )"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"->"   { Console::Log(yylineno, yytext, "operator ->"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"=>"   { Console::Log(yylineno, yytext, "operator =>"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>"["	{ Console::Log(yylineno, yytext, "operator ["); BEGIN(State::PushAndPeek(ARRAY_CONTENT)); }

<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{ID}	{ Console::Log(yylineno, yytext, "value ID"); }

<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{NUMBER_DECIMAL}		{ Console::Log(yylineno, (int)strtol(yytext,NULL,10), "value NUMBER_DECIMAL"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{NUMBER_HEXADECIMAL}	{ Console::Log(yylineno, (int)strtol(yytext,NULL,16), "value NUMBER_HEXADECIMAL"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{NUMBER_OCTAL}			{ Console::Log(yylineno, (int)strtol(yytext,NULL,8), "value NUMBER_OCTAL"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{NUMBER_BINARY}			{ Console::Log(yylineno, (int)strtol(yytext+2,NULL,2), "value NUMBER_BINARY"); }

<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{NUMBER_FLOAT}		{ Console::Log(yylineno, atof(yytext), "value NUMBER_FLOAT"); }
<PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>{NUMBER_EXPONENT}	{ Console::Log(yylineno, atof(yytext), "value NUMBER_EXPONENT"); }

<INITIAL,PURE_PHP,INTERPOLATED_PHP,ARRAY_CONTENT>[ \t\n]+		;

<INITIAL>.*		;

<*>.			{ Console::Error("unrecognized symbol '" + Buffer::CutStringEnd(yytext, 0) + "'", yylineno); }

%%


vector<int> State::_states;

string Buffer::_buffer;
string Buffer::_docId;
int Buffer::_startLine = -1;

class Color {
public:
    static string Reset()     		{ return "\033[0m"; }
    static string Red()       		{ return "\033[31m"; }
    static string Green()     		{ return "\033[32m"; }
    static string Yellow()    		{ return "\033[33m"; }
    static string Blue()      		{ return "\033[94m"; }
    static string Magenta()   		{ return "\033[35m"; }
    static string Cyan()      		{ return "\033[36m"; }
	static string Grey() 	  		{ return "\033[90m"; }
    static string White()     		{ return "\033[37m"; }
    static string Bold()      		{ return "\033[1m"; }
    static string Underline()		{ return "\033[4m"; }
	static string Strikethrough()	{ return "\033[9m"; }
};

void Console::LogHeader() {
	#ifdef CONSOLE_LOG_ENABLED
		Console::Log(Color::Bold() + Color::Blue() + "=============================");
		Console::Log(Color::Bold() + Color::Blue() + "        LEXER RUNNING        ");
		Console::Log(Color::Bold() + Color::Blue() + "=============================");
		Console::Log(Color::Bold() + Color::Blue() + "% CONSOLE_LOG_ENABLED");
		
		#ifdef CONSOLE_STATE_LOG_ENABLED
		Console::Log(Color::Bold() + Color::Blue() + "% CONSOLE_STATE_LOG_ENABLED");
		#endif
		Console::Log(Color::Bold() + Color::Blue() + "=============================");
	#endif
}

void Console::Log(const string message) {
	#ifdef CONSOLE_LOG_ENABLED
    	cout << message << "\n" << Color::Reset();
	#endif
}

void Console::Log(int line, const string content, const string info) {
	Console::Log(line, content, info, false);
}

void Console::Log(int line, const string content, const string info, bool isSecondaryColor) {
	#ifdef CONSOLE_LOG_ENABLED
		cout << Color::Yellow() << "L = " << line << Color::Grey() << " \t'" << (isSecondaryColor ? Color::Cyan() : Color::Green()) << content << Color::Grey() << "' (" << info << ")" << "\n" << Color::Reset();
	#endif
}

void Console::Log(int line, int content, const string info) {
	#ifdef CONSOLE_LOG_ENABLED
    	cout << Color::Yellow() << "L = " << line << Color::Grey() << " \t'" << Color::Green() << content << Color::Grey() << "' (" << info << ")" << "\n" << Color::Reset();
	#endif
}

void Console::Log(int line, double content, const string info) {
	#ifdef CONSOLE_LOG_ENABLED
		cout << Color::Yellow() << "L = " << line << Color::Grey() << " \t'" << Color::Green() << content << Color::Grey() << "' (" << info << ")" << "\n" << Color::Reset();
	#endif
}

void Console::Log(int lineStart, int lineEnd, const string content, const string info) {
	#ifdef CONSOLE_LOG_ENABLED
		cout << Color::Yellow() << "L = " << lineStart << "-" << lineEnd << Color::Grey() << "\t'" << Color::Cyan() << content << Color::Grey() << "' (" << info << ")" << "\n" << Color::Reset();
	#endif
}

void Console::Error(const string message, int line) {
	#ifdef CONSOLE_LOG_ENABLED
		cout << Color::Bold() << Color::Red() << "ERROR in line " << line << ": " << message << "\n" << Color::Reset();
	#endif
}

int State::PushAndPeek(int state) {
	_states.push_back(state);

	#ifdef CONSOLE_STATE_LOG_ENABLED
		Console::Log(Color::Magenta() + "S = " + State::GetStateTree());
	#endif

    return Current();
}

int State::PopAndPeek() {
	int prev = _states.back();
    _states.pop_back();

	#ifdef CONSOLE_STATE_LOG_ENABLED
    	Console::Log(Color::Magenta() + "S = " + State::GetStateTree() + " (was " + State::StateCodeToString(prev) + ")");
	#endif

    return _states.back();
}

int State::Current() {
    return _states.back();
}

int State::Previous() {
    return _states[_states.size() - 2];
}

string State::GetStateTree() {
	if (_states.empty()) {
		return "";
	}

	std::ostringstream oss;
	oss << Color::Magenta();
	for (auto rit = _states.rbegin(); rit != _states.rend(); ++rit) {
		oss << State::StateCodeToString(*rit) << Color::Grey();
		if (rit + 1 != _states.rend()) {
			oss << " <- ";
		}
	}
	return oss.str();
}

string State::StateCodeToString(int state) {
	switch (state) {
		case 0:  return "INITIAL";
		case 1:  return "PURE_PHP";
		case 2:  return "INTERPOLATED_PHP";
		case 3:  return "COMMENT_HTML";
		case 4:  return "COMMENT_MULTILINE";
		case 5:  return "STRING_SINGLE_QUOTES";
		case 6:  return "STRING_DOUBLE_QUOTES";
		case 7:  return "STRING_HEREDOC_OR_NOWDOC";
		case 8:  return "STRING_HEREDOC";
		case 9:  return "STRING_NOWDOC";
		case 10: return "ARRAY_CONTENT";
		default: return "UNKNOWN";
	}
}

string Buffer::Current() {
    return _buffer;
}

void Buffer::Reset() {
    _buffer.clear();
}

void Buffer::Append(const string text) {
    _buffer += text;
}

void Buffer::SetStartLine(int line) {
    _startLine = line;
}

int Buffer::GetStartLine() {
    return _startLine;
}

void Buffer::SetDocId(const string id, int erase) {
    _docId = id;
    _docId.erase(_docId.size() - erase);
}

string Buffer::GetDocId() {
    return _docId;
}

string Buffer::CutStringEnd(const string str, int count) {
    if (count <= 0) return str;
    if ((size_t)count >= str.size()) return "";
    return str.substr(0, str.size() - count);
}
