#include "Buffer.h"
#include "Console.h"
#include "Color.h"

string Buffer::current() {
    return _buffer;
}

void Buffer::reset() {
    _buffer.clear();
}

void Buffer::append(string text) {
#ifdef CONSOLE_LOG_ENABLED
    Console::Log(Color::Grey() + "%Buffer% Appended: '" + text + "'");
#endif
    _buffer += text;
}

void Buffer::setStartLine(int line) {
    _startLine = line;
}

int Buffer::getStartLine() {
    return _startLine;
}

void Buffer::setDocId(string id, int erase) {
    _docId = id;
    _docId.erase(_docId.size() - erase);
}

string Buffer::getDocId() {
    return _docId;
}

string Buffer::CutStringEnd(string str, int count) {
    if (count <= 0) return str;
    if ((size_t) count >= str.size()) return "";
    return str.substr(0, str.size() - count);
}
