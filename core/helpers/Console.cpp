#include "Console.h"
#include "Color.h"

void Console::LogHeader(string title) {
#ifdef CONSOLE_LOG_ENABLED
	Log(Color::Bold() + Color::Blue() + "=============================");
	Log(Color::Bold() + Color::Blue() + title);
	Log(Color::Bold() + Color::Blue() + "\n% CONSOLE_LOG_ENABLED");
	Log(Color::Bold() + Color::Blue() + "=============================");
#endif
}

void Console::Log(string message) {
#ifdef CONSOLE_LOG_ENABLED
	cout << message << "\n" << Color::Reset();
#endif
}

void Console::Log(int line, const string content, string info) {
	Console::Log(line, content, info, false);
}

void Console::Log(int line, string content, string info, bool isSecondaryColor) {
#ifdef CONSOLE_LOG_ENABLED
	cout << Color::Yellow() << "L = " << line << Color::Grey() << " \t'" << (
				isSecondaryColor ? Color::Cyan() : Color::Green()) << content << Color::Grey() << "' (" << info << ")"
			<< "\n"
			<< Color::Reset();
#endif
}

void Console::Log(int line, int content, string info) {
#ifdef CONSOLE_LOG_ENABLED
	cout << Color::Yellow() << "L = " << line << Color::Grey() << " \t'" << Color::Green() << content << Color::Grey()
			<< "' (" << info << ")" << "\n" << Color::Reset();
#endif
}

void Console::Log(int line, double content, string info) {
#ifdef CONSOLE_LOG_ENABLED
	cout << Color::Yellow() << "L = " << line << Color::Grey() << " \t'" << Color::Green() << content << Color::Grey()
			<< "' (" << info << ")" << "\n" << Color::Reset();
#endif
}

void Console::Log(int lineStart, int lineEnd, string content, string info) {
#ifdef CONSOLE_LOG_ENABLED
	cout << Color::Yellow() << "L = " << lineStart << "-" << lineEnd << Color::Grey() << "\t'" << Color::Cyan() <<
			content << Color::Grey() << "' (" << info << ")" << "\n" << Color::Reset();
#endif
}

void Console::Error(string message, int line) {
#ifdef CONSOLE_LOG_ENABLED
	cout << Color::Bold() << Color::Red() << "ERROR in line " << line << ": " << message << "\n" << Color::Reset();
#endif
}
