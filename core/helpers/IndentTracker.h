#ifndef PHP_COMPILER_INDENTTRACKER_H
#define PHP_COMPILER_INDENTTRACKER_H

#define CONSOLE_INDENTTRACKER_LOG_ENABLED

#include <string>

using std::string;

class IndentTracker {
    string _indent;

public:
    string current() const;

    bool set(string indent);

    void reset();

    bool isValid(string str) const;

    string getContent(string str) const;
};

#endif //PHP_COMPILER_INDENTTRACKER_H
