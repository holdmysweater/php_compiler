#ifndef PHP_COMPILER_BUFFER_H
#define PHP_COMPILER_BUFFER_H

#include "../macros.h"
#include "IndentTracker.h"

#include <string>
#include <vector>

using std::vector;
using std::string;

class Buffer {
    vector<string> _text;
    IndentTracker _indentTracker = IndentTracker();
    string _docId;
    int _startLine = -1;

public:
    static string CutStringEnd(string str, int count);

    static string getLeftPropertyAccess(string str);

    static string getRightPropertyAccess(string str);

    void setStartLine(int line);

    int getStartLine() const;

    void setDocId(string id, int erase = 0);

    string getDocId() const;

    void append(string str);

    void appendNewLines(string newLines);

    string pop();

    void reset();

    int validateByLastLineAsIndent();

    string current() const;
};

#endif //PHP_COMPILER_BUFFER_H
