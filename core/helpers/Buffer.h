#ifndef PHP_COMPILER_BUFFER_H
#define PHP_COMPILER_BUFFER_H

#include <sstream>

using std::string;

class Buffer {
    string _buffer;
    string _docId;
    int _startLine = -1;

public:
    static string CutStringEnd(string str, int count);

    string current();

    void reset();

    void append(string text);

    void setStartLine(int line);

    int getStartLine();

    void setDocId(string id, int erase = 0);

    string getDocId();
};

#endif //PHP_COMPILER_BUFFER_H
