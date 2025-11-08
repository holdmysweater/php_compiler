#ifndef PHP_COMPILER_INDENTATIONTRACKER_H
#define PHP_COMPILER_INDENTATIONTRACKER_H

#define CONSOLE_LOG_ENABLED

#include <string>

using std::string;

class IndentationTracker {
    string _indentation;

public:
    string current() const;

    bool set(string indentation);

    void reset();

    bool isValid(string str) const;

    string getContent(string str) const;
};

#endif //PHP_COMPILER_INDENTATIONTRACKER_H
