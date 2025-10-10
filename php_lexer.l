%option noyywrap
%option never-interactive
%option yylineno


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
%x INSERTED_PHP
%x COMMENT_HTML
%x COMMENT_MULTILINE
%x STRING_SINGLE_QUOTES
%x STRING_DOUBLE_QUOTES
%x STRING_HEREDOC_OR_NOWDOC
%x STRING_HEREDOC
%x STRING_NOWDOC


%{
	#include <iostream>
	#include <cstring>
	#include <cstdlib>

	using std::string;
	using std::cout;

	class Console {
	public:
		static void Log(const string message) {
			cout << message << "\n";
		}

		static void Log(int line, const string content, const string info) {
			cout << "Line = " << line << ", content = \"" << content << "\" (" << info << ")" << "\n";
		}

		static void Log(int lineStart, int lineEnd, const string content, const string info) {
			cout << "Lines = " << lineStart << " - " << lineEnd << ", content = \"" << content << "\" (" << info << ")" << "\n";
		}

		static void Error(const string message, int line) {
			cout << "ERROR in line " << line << ": " << message << "\n";
		}
	};

	class State {
	public:
		static int _states[100];
		static int _index;

		static int PushAndPeek(int state) {
			_states[++_index] = state;
			return Current();
		}

		static int PopAndPeek() {
			return _states[--_index];
		}

		static int Current() {
			return _states[_index];
		}

		static int PreviousState() {
			return _states[_index - 1];
		}
	};

	class Buffer {
	public:
		static string _buffer;
		static string _docId;
		static int _startLine;

		static string Current() {
			return _buffer;
		}

		static void Reset() {
			_buffer.clear();
		}

		static void Append(string text) {
			_buffer += text;
		}

		static void Cut(int count) {
			_buffer.erase(_buffer.size() - count);
		}

		static void SetStartLine(int line) {
			_startLine = line;
		}

		static int GetStartLine() {
			return _startLine;
		}

		static void SetDocId(string id, int erase = 0) {
			_docId = id;
			_docId.erase(_docId.size() - erase);
		}

		static string GetDocId() {
			return _docId;
		}
	};

	int State::_states[100];
	int State::_index = 0;

	string Buffer::_buffer;
	string Buffer::_docId;
	int Buffer::_startLine = -1;
%}


%%


%{
	State::PushAndPeek(INITIAL);
	BEGIN(State::Current());
%}


"<!--"							{  BEGIN(State::PushAndPeek(COMMENT_HTML)); }
<COMMENT_HTML>[^\-]+			;
<COMMENT_HTML>"-"				;
<COMMENT_HTML>\-{2}\>			{ BEGIN(State::PopAndPeek()); }

<PURE_PHP>("//"|"#").*       	{ Console::Log(yylineno, yytext, "comment"); }

<PURE_PHP>"/*"					{  BEGIN(State::PushAndPeek(COMMENT_MULTILINE)); }
<COMMENT_MULTILINE>[^*]+		;
<COMMENT_MULTILINE>"*"+[^*/] 	;
<COMMENT_MULTILINE>"*"+"/" 		{ BEGIN(State::PopAndPeek()); }

<PURE_PHP>\'						{ BEGIN(STRING_SINGLE_QUOTES); Buffer::Reset(); Buffer::SetStartLine(yylineno); }
<STRING_SINGLE_QUOTES>\\\'			{ Buffer::Append("\'"); }
<STRING_SINGLE_QUOTES>\\\\     		{ Buffer::Append("\\"); }
<STRING_SINGLE_QUOTES>\\[^\\\']     { Buffer::Append(yytext); }
<STRING_SINGLE_QUOTES>[^\\\']+		{ Buffer::Append(yytext); }
<STRING_SINGLE_QUOTES>\'			{
										Console::Log(Buffer::GetStartLine(), yylineno, Buffer::Current(), "string in STRING_SINGLE_QUOTES");
										Buffer::Reset();
										BEGIN(PURE_PHP);
									}

<PURE_PHP>"<<<"              						{  BEGIN(State::PushAndPeek(STRING_HEREDOC_OR_NOWDOC)); }
<STRING_HEREDOC_OR_NOWDOC>[ \t]*            		;
<STRING_HEREDOC_OR_NOWDOC>(\'{ID}\'|{ID})[^\n]      { BEGIN(State::PopAndPeek()); Console::Error("Invalid \"<<<\" identifier in STRING_HEREDOC_OR_NOWDOC", yylineno); }
<STRING_HEREDOC_OR_NOWDOC>\'{ID}\'\n          		{ State::PopAndPeek();  BEGIN(State::PushAndPeek(STRING_NOWDOC)); Buffer::Reset(); Buffer::SetDocId(yytext, 2); Buffer::SetStartLine(yylineno); }
<STRING_HEREDOC_OR_NOWDOC>{ID}\n            		{ State::PopAndPeek();  BEGIN(State::PushAndPeek(STRING_HEREDOC)); Buffer::Reset(); Buffer::SetDocId(yytext, 1); Buffer::SetStartLine(yylineno); }

<STRING_NOWDOC,STRING_HEREDOC>^{ID};\n			{
													if (string(yytext, strcspn(yytext, ";")) != Buffer::GetDocId()) {
														Buffer::Append(yytext);
													}
													else {
														Buffer::Cut(1);
														Console::Log(Buffer::GetStartLine(), yylineno, Buffer::Current(), "text in STRING_NOWDOC/STRING_HEREDOC");
														Buffer::Reset();
														BEGIN(State::PopAndPeek());
													}
												}

<STRING_NOWDOC>\n+					{ Buffer::Append(yytext); }
<STRING_NOWDOC>[^\s\n]+				{ Buffer::Append(yytext); }

<PURE_PHP>"\""						{  BEGIN(State::PushAndPeek(STRING_DOUBLE_QUOTES)); Buffer::Reset(); Buffer::SetStartLine(yylineno); }

<STRING_DOUBLE_QUOTES>[^\\\"\$\{]+					{ Buffer::Append(yytext); }

<STRING_HEREDOC>[^\\\$\{]							{ Buffer::Append(yytext); }

<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\\$			{ Buffer::Append("$"); }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\\{			{ Buffer::Append("{"); }

<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\${ID}			{
														Console::Log(Buffer::GetStartLine(), yylineno, Buffer::Current(), "string inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
														Console::Log(yylineno, "$", "operator \"$\" inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
														Console::Log(yylineno, yytext+1, "identifier inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
														Buffer::Reset();
													}
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\$				{ Buffer::Append("$"); }
<STRING_DOUBLE_QUOTES,STRING_HEREDOC>(\{\$)|(\$\{) 	{
														Console::Log(Buffer::GetStartLine(), yylineno, Buffer::Current(), "string inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
														Console::Log(yylineno, "$", "operator \"$\" inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
														Console::Log(yylineno, yytext+1, "identifier inside STRING_DOUBLE_QUOTES/STRING_HEREDOC");
														Buffer::Reset();
														BEGIN(State::PushAndPeek(INSERTED_PHP));
													}
<INSERTED_PHP>\}									{ BEGIN(State::PopAndPeek()); Console::Log(yylineno, yytext, "operator } inside INSERTED_PHP"); }

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
<STRING_DOUBLE_QUOTES>\"			 						{ BEGIN(PURE_PHP); Console::Log(yylineno, Buffer::Current(), "string inside STRING_DOUBLE_QUOTES"); Buffer::Reset(); }

<STRING_DOUBLE_QUOTES,STRING_HEREDOC>\\		{ Buffer::Append("\\"); }
<STRING_HEREDOC>\s.*						{ Buffer::Append(yytext); }


\<\?([pP][hH][pP])?							{ BEGIN(PURE_PHP); Console::Log(yylineno, yytext, "opening php tag"); }
<PURE_PHP>\?\>								{ BEGIN(INITIAL); Console::Log(yylineno, yytext, "closing php tag"); }
<PURE_PHP>\<\<EOF>>							{ BEGIN(INITIAL); Console::Log(yylineno, yytext, "end of file - closing php tag"); }

<COMMENT_HTML,COMMENT_MULTILINE,STRING_HEREDOC_OR_NOWDOC,STRING_NOWDOC,STRING_HEREDOC,STRING_SINGLE_QUOTES,STRING_DOUBLE_QUOTES>\<\<EOF>> { BEGIN(INITIAL); Console::Error("end of file", yylineno); }

<INSERTED_PHP,PURE_PHP>{ENDFOREACH}   { Console::Log(yylineno, yytext, "keyword ENDFOREACH"); }
<INSERTED_PHP,PURE_PHP>{ENDWHILE}     { Console::Log(yylineno, yytext, "keyword ENDWHILE"); }
<INSERTED_PHP,PURE_PHP>{ENDSWITCH}    { Console::Log(yylineno, yytext, "keyword ENDSWITCH"); }
<INSERTED_PHP,PURE_PHP>{ENDFOR}       { Console::Log(yylineno, yytext, "keyword ENDFOR"); }
<INSERTED_PHP,PURE_PHP>{FOREACH}      { Console::Log(yylineno, yytext, "keyword FOREACH"); }
<INSERTED_PHP,PURE_PHP>{INSTANCEOF}   { Console::Log(yylineno, yytext, "keyword INSTANCEOF"); }
<INSERTED_PHP,PURE_PHP>{PROTECTED}    { Console::Log(yylineno, yytext, "keyword PROTECTED"); }
<INSERTED_PHP,PURE_PHP>{CONTINUE}     { Console::Log(yylineno, yytext, "keyword CONTINUE"); }
<INSERTED_PHP,PURE_PHP>{FUNCTION}     { Console::Log(yylineno, yytext, "keyword FUNCTION"); }
<INSERTED_PHP,PURE_PHP>{EXTENDS}      { Console::Log(yylineno, yytext, "keyword EXTENDS"); }
<INSERTED_PHP,PURE_PHP>{STATIC}       { Console::Log(yylineno, yytext, "keyword STATIC"); }
<INSERTED_PHP,PURE_PHP>{PUBLIC}       { Console::Log(yylineno, yytext, "keyword PUBLIC"); }
<INSERTED_PHP,PURE_PHP>{PRIVATE}      { Console::Log(yylineno, yytext, "keyword PRIVATE"); }
<INSERTED_PHP,PURE_PHP>{DEFAULT}      { Console::Log(yylineno, yytext, "keyword DEFAULT"); }
<INSERTED_PHP,PURE_PHP>{ELSEIF}       { Console::Log(yylineno, yytext, "keyword ELSEIF"); }
<INSERTED_PHP,PURE_PHP>{SWITCH}       { Console::Log(yylineno, yytext, "keyword SWITCH"); }
<INSERTED_PHP,PURE_PHP>{RETURN}       { Console::Log(yylineno, yytext, "keyword RETURN"); }
<INSERTED_PHP,PURE_PHP>{BREAK}        { Console::Log(yylineno, yytext, "keyword BREAK"); }
<INSERTED_PHP,PURE_PHP>{CONST}        { Console::Log(yylineno, yytext, "keyword CONST"); }
<INSERTED_PHP,PURE_PHP>{CLASS}        { Console::Log(yylineno, yytext, "keyword CLASS"); }
<INSERTED_PHP,PURE_PHP>{WHILE}        { Console::Log(yylineno, yytext, "keyword WHILE"); }
<INSERTED_PHP,PURE_PHP>{ELSE}         { Console::Log(yylineno, yytext, "keyword ELSE"); }
<INSERTED_PHP,PURE_PHP>{AND}          { Console::Log(yylineno, yytext, "keyword AND"); }
<INSERTED_PHP,PURE_PHP>{XOR}          { Console::Log(yylineno, yytext, "keyword XOR"); }
<INSERTED_PHP,PURE_PHP>{NEW}          { Console::Log(yylineno, yytext, "keyword NEW"); }
<INSERTED_PHP,PURE_PHP>{VAR}          { Console::Log(yylineno, yytext, "keyword VAR"); }
<INSERTED_PHP,PURE_PHP>{OR}           { Console::Log(yylineno, yytext, "keyword OR"); }
<INSERTED_PHP,PURE_PHP>{AS}           { Console::Log(yylineno, yytext, "keyword AS"); }
<INSERTED_PHP,PURE_PHP>{IF}           { Console::Log(yylineno, yytext, "keyword IF"); }

<PURE_PHP>array			{ Console::Log(yylineno, yytext, "type array"); }
<PURE_PHP>callable		{ Console::Log(yylineno, yytext, "type callable"); }


<INSERTED_PHP,PURE_PHP>"==="       { Console::Log(yylineno, yytext, "operator ==="); }
<INSERTED_PHP,PURE_PHP>"!=="       { Console::Log(yylineno, yytext, "operator !=="); }
<INSERTED_PHP,PURE_PHP>"::"        { Console::Log(yylineno, yytext, "operator ::"); }
<INSERTED_PHP,PURE_PHP>"->"        { Console::Log(yylineno, yytext, "operator ->"); }
<INSERTED_PHP,PURE_PHP>"=>"        { Console::Log(yylineno, yytext, "operator =>"); }
<INSERTED_PHP,PURE_PHP>"**"        { Console::Log(yylineno, yytext, "operator **"); }
<INSERTED_PHP,PURE_PHP>"<="        { Console::Log(yylineno, yytext, "operator <="); }
<INSERTED_PHP,PURE_PHP>">="        { Console::Log(yylineno, yytext, "operator >="); }
<INSERTED_PHP,PURE_PHP>"=="        { Console::Log(yylineno, yytext, "operator =="); }
<INSERTED_PHP,PURE_PHP>"!="        { Console::Log(yylineno, yytext, "operator !="); }
<INSERTED_PHP,PURE_PHP>"<>"        { Console::Log(yylineno, yytext, "operator <>"); }
<INSERTED_PHP,PURE_PHP>"&&"        { Console::Log(yylineno, yytext, "operator &&"); }
<INSERTED_PHP,PURE_PHP>"||"        { Console::Log(yylineno, yytext, "operator ||"); }
<INSERTED_PHP,PURE_PHP>"!"         { Console::Log(yylineno, yytext, "operator !"); }
<INSERTED_PHP,PURE_PHP>"*"         { Console::Log(yylineno, yytext, "operator *"); }
<INSERTED_PHP,PURE_PHP>"/"         { Console::Log(yylineno, yytext, "operator /"); }
<INSERTED_PHP,PURE_PHP>"%"         { Console::Log(yylineno, yytext, "operator %"); }
<INSERTED_PHP,PURE_PHP>"+"         { Console::Log(yylineno, yytext, "operator +"); }
<INSERTED_PHP,PURE_PHP>"."         { Console::Log(yylineno, yytext, "operator ."); }
<INSERTED_PHP,PURE_PHP>"-"         { Console::Log(yylineno, yytext, "operator -"); }
<INSERTED_PHP,PURE_PHP>"<"         { Console::Log(yylineno, yytext, "operator <"); }
<INSERTED_PHP,PURE_PHP>">"         { Console::Log(yylineno, yytext, "operator >"); }
<INSERTED_PHP,PURE_PHP>"="         { Console::Log(yylineno, yytext, "operator ="); }
<INSERTED_PHP,PURE_PHP>"$"         { Console::Log(yylineno, yytext, "operator $"); }

<PURE_PHP>","					{ Console::Log(yylineno, yytext, "operator ,"); }
<PURE_PHP>";" 					{ Console::Log(yylineno, yytext, "operator ;"); }
<PURE_PHP>":" 					{ Console::Log(yylineno, yytext, "operator :"); }
<PURE_PHP>"?" 					{ Console::Log(yylineno, yytext, "operator ?"); }

<INSERTED_PHP,PURE_PHP>"(" 		{ Console::Log(yylineno, yytext, "operator (");}
<INSERTED_PHP,PURE_PHP>")" 		{ Console::Log(yylineno, yytext, "operator )"); }
<INSERTED_PHP,PURE_PHP>"{" 		{ Console::Log(yylineno, yytext, "operator {"); }
<INSERTED_PHP,PURE_PHP>"}" 		{ Console::Log(yylineno, yytext, "operator }"); }

<INSERTED_PHP,PURE_PHP>"["		{ Console::Log(yylineno, yytext, "operator ["); }
<INSERTED_PHP,PURE_PHP>"]"     	{ Console::Log(yylineno, yytext, "operator ]"); }

<INSERTED_PHP,PURE_PHP>{TRUE}		{ Console::Log(yylineno, yytext, "value true"); }
<INSERTED_PHP,PURE_PHP>{FALSE}		{ Console::Log(yylineno, yytext, "value false"); }
<INSERTED_PHP,PURE_PHP>{NULL}		{ Console::Log(yylineno, yytext, "value null"); }

<INSERTED_PHP,PURE_PHP>{NUMBER_DECIMAL}			{ Console::Log(yylineno, yytext, "value NUMBER_DECIMAL"); }
<INSERTED_PHP,PURE_PHP>{NUMBER_HEXADECIMAL}		{ Console::Log(yylineno, yytext, "value NUMBER_HEXADECIMAL"); }
<INSERTED_PHP,PURE_PHP>{NUMBER_OCTAL}			{ Console::Log(yylineno, yytext, "value NUMBER_OCTAL"); }
<INSERTED_PHP,PURE_PHP>{NUMBER_BINARY}			{ Console::Log(yylineno, yytext, "value NUMBER_BINARY"); }

<INSERTED_PHP,PURE_PHP>{NUMBER_FLOAT}			{ Console::Log(yylineno, yytext, "value NUMBER_FLOAT"); }
<INSERTED_PHP,PURE_PHP>{NUMBER_EXPONENT}		{ Console::Log(yylineno, yytext, "value NUMBER_EXPONENT"); }
<INSERTED_PHP,PURE_PHP>{ID}						{ Console::Log(yylineno, yytext, "value ID"); }

<INITIAL,PURE_PHP>[ \t\n]+						;
<INITIAL>.*										;

%%