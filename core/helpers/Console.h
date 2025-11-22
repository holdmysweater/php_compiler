#ifndef PHP_COMPILER_CONSOLE_H
#define PHP_COMPILER_CONSOLE_H

#define CONSOLE_LOG_ENABLED

#include <iostream>
#include <cstdint>

using std::string;

class Console {
public:
    static void SystemTitle(string title);

    static void Log(string message);

    static void Warning(string message);

    static void Error(string message);

    static void SystemLog(string message);

    static void SystemError(string message);

    static void LexLog(int line, string content, string info);

    static void LexLog(int line, int content, string info);

    static void LexLog(int line, double content, string info);

    static void LexLog(int line, string content, string info, bool isSecondaryColor);

    static void LexLog(int lineStart, int lineEnd, string content, string info);

    static void LexError(string message, int line);

    static void ParserLog(string message);

    static void ParserError();

    static void ParserError(string message);

    static void NodeLog(string message, string type, uint32_t id);

    static void NodeWarning(string message, string type, uint32_t id);

    static void NodeError(string message, string type, uint32_t id);
};

#endif //PHP_COMPILER_CONSOLE_H
