#include "Console.h"
#include "Color.h"

void Console::ShowSettings() {
	Log(Color::Bold() + Color::Blue() + "\n=========== MACROS ================");

	Log(Color::Bold() + Color::Cyan() + "\tSYSTEM DEBUG");
#ifdef CONSOLE_SYSTEM_LOG_ENABLED
	Log(Color::Bold() + Color::Blue() + "% CONSOLE_SYSTEM_LOG_ENABLED %");
#else
	Log(Color::Bold() + Color::Grey() + "% CONSOLE_SYSTEM_LOG_DISABLED %");
#endif

	Log(Color::Bold() + Color::Cyan() + "\n\tNODE DEBUG");
#ifdef CONSOLE_NODE_LOG_ENABLED
	Log(Color::Bold() + Color::Blue() + "% CONSOLE_NODE_LOG_ENABLED %");
#else
	Log(Color::Bold() + Color::Grey() + "% CONSOLE_NODE_LOG_ENABLED %");
#endif

#ifdef NODE_DOT_LABEL_DEBUG
	Log(Color::Bold() + Color::Blue() + "% NODE_DOT_LABEL_DEBUG %");
#else
	Log(Color::Bold() + Color::Grey() + "% NODE_DOT_LABEL_DEBUG %");
#endif

#ifdef NODE_FILE_GENERATION_DEBUG
	Log(Color::Bold() + Color::Blue() + "% NODE_FILE_GENERATION_DEBUG %");
#else
	Log(Color::Bold() + Color::Grey() + "% NODE_FILE_GENERATION_DEBUG %");
#endif

	Log(Color::Bold() + Color::Cyan() + "\n\tPARSER DEBUG");
#ifdef CONSOLE_PARSER_LOG_ENABLED
	Log(Color::Bold() + Color::Blue() + "% CONSOLE_PARSER_LOG_ENABLED %");
#else
	Log(Color::Bold() + Color::Grey() + "% CONSOLE_PARSER_LOG_ENABLED %");
#endif

	Log(Color::Bold() + Color::Cyan() + "\n\tLEXER DEBUG");
#ifdef CONSOLE_LEXER_LOG_ENABLED
	Log(Color::Bold() + Color::Blue() + "% CONSOLE_LEXER_LOG_ENABLED %");
#else
	Log(Color::Bold() + Color::Grey() + "% CONSOLE_LEXER_LOG_ENABLED %");
#endif

#ifdef CONSOLE_STATESTREE_LOG_ENABLED
	Log(Color::Bold() + Color::Blue() + "% CONSOLE_STATESTREE_LOG_ENABLED %");
#else
	Log(Color::Bold() + Color::Grey() + "% CONSOLE_STATESTREE_LOG_ENABLED %");
#endif

#ifdef CONSOLE_TOKENQUEUE_LOG_ENABLED
	Log(Color::Bold() + Color::Blue() + "% CONSOLE_TOKENQUEUE_LOG_ENABLED %");
#else
	Log(Color::Bold() + Color::Grey() + "% CONSOLE_TOKENQUEUE_LOG_ENABLED %");
#endif

#ifdef CONSOLE_INDENTTRACKER_LOG_ENABLED
	Log(Color::Bold() + Color::Blue() + "% CONSOLE_INDENTTRACKER_LOG_ENABLED %");
#else
	Log(Color::Bold() + Color::Grey() + "% CONSOLE_INDENTTRACKER_LOG_ENABLED %");
#endif

#ifdef CONSOLE_BUFFER_LOG_ENABLED
	Log(Color::Bold() + Color::Blue() + "% CONSOLE_BUFFER_LOG_ENABLED %");
#else
	Log(Color::Bold() + Color::Grey() + "% CONSOLE_BUFFER_LOG_ENABLED %");
#endif

	Log(Color::Bold() + Color::Blue() + "===================================\n");
}

void Console::SystemTitle(string title) {
	Log(Color::Bold() + Color::Blue() + "\n===================================");
	Log(Color::Bold() + Color::Blue() + "\t" + title);
	Log(Color::Bold() + Color::Blue() + "===================================\n");
}

void Console::Log(string message) {
	cout << message << "\n" << Color::Reset();
}

void Console::Warning(string message) {
	Log(Color::Bold() + Color::Yellow() + "\nWARNING: " + message + "\n");
}

void Console::Error(string message) {
	cout << Color::Bold() << Color::Red() << "\nERROR: " << message << "\n" << Color::Reset();
}

void Console::SystemLog(string message) {
#ifdef CONSOLE_SYSTEM_LOG_ENABLED
	Log(Color::Bold() + Color::Blue() + "\n%SYSTEM% " + message + "\n");
#endif
}

void Console::SystemError(string message) {
	cout << Color::Bold() << Color::Red() << "\n%SYSTEM% " << message << "\n\n" << Color::Reset();
}

void Console::LexLog(int line, const string content, string info) {
#ifdef CONSOLE_LEXER_LOG_ENABLED
	LexLog(line, content, info, false);
#endif
}

void Console::LexLog(int line, string content, string info, bool isSecondaryColor) {
#ifdef CONSOLE_LEXER_LOG_ENABLED
	cout << Color::Yellow() << "L = " << line << Color::Grey() << " \t'"
			<< (isSecondaryColor ? Color::Cyan() : Color::Green()) << content << Color::Grey() << "' (" << info << ")"
			<< "\n" << Color::Reset();
#endif
}

void Console::LexLog(int line, int content, string info) {
#ifdef CONSOLE_LEXER_LOG_ENABLED
	cout << Color::Yellow() << "L = " << line << Color::Grey() << " \t'" << Color::Green() << content << Color::Grey()
			<< "' (" << info << ")" << "\n" << Color::Reset();
#endif
}

void Console::LexLog(int line, double content, string info) {
#ifdef CONSOLE_LEXER_LOG_ENABLED
	cout << Color::Yellow() << "L = " << line << Color::Grey() << " \t'" << Color::Green() << content << Color::Grey()
			<< "' (" << info << ")" << "\n" << Color::Reset();
#endif
}

void Console::LexLog(int lineStart, int lineEnd, string content, string info) {
#ifdef CONSOLE_LEXER_LOG_ENABLED
	cout << Color::Yellow() << "L = " << lineStart << "-" << lineEnd << Color::Grey() << "\t'" << Color::Cyan() <<
			content << Color::Grey() << "' (" << info << ")" << "\n" << Color::Reset();
#endif
}

void Console::LexError(string message, int line) {
	cout << Color::Bold() << Color::Red() << "\n%LEXER% ERROR in line " << line << ": " << message << "\n" <<
			Color::Reset();
}

void Console::ParserLog(string message) {
#ifdef CONSOLE_PARSER_LOG_ENABLED
	Log(Color::Grey() + "%PARSER% State: '" + message + "'");
#endif
}

void Console::ParserError() {
	cout << Color::Bold() << Color::Red() << "\n%PARSER% In error state\n" << Color::Reset();
}

void Console::ParserError(string message) {
	cout << Color::Bold() << Color::Red() << "\n%PARSER% ERROR: " << message << "\n" << Color::Reset();
}

void Console::NodeLog(string message, string type, uint32_t id) {
#ifdef CONSOLE_NODE_LOG_ENABLED
	Log(Color::Grey() + "%NODE% " + std::to_string(id) + " (" + type + "): " + message);
#endif
}

void Console::NodeWarning(string message, string type, uint32_t id) {
	Log(Color::Bold() + Color::Yellow() + "\n%NODE% " + std::to_string(id) + " (" + type + "): " + message + "\n");
}

void Console::NodeError(string message, string type, uint32_t id) {
	cout << Color::Bold() << Color::Red() << "\n%NODE% " << std::to_string(id) << " (" << type << "): " << message <<
			"\n" << Color::Reset();
}
