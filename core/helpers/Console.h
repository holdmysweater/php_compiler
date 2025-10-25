#ifndef PHP_COMPILER_CONSOLE_H
#define PHP_COMPILER_CONSOLE_H

#define CONSOLE_LOG_ENABLED

#include <iostream>

using std::string;

class Console {
public:
    static void LogHeader(string title);

    static void Log(string message);

    static void Log(int line, string content, string info);

    static void Log(int line, int content, string info);

    static void Log(int line, double content, string info);

    static void Log(int line, string content, string info, bool isSecondaryColor);

    static void Log(int lineStart, int lineEnd, string content, string info);

    static void Error(string message, int line);
};

#endif //PHP_COMPILER_CONSOLE_H
