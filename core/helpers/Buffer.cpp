#include "Buffer.h"
#include "Console.h"
#include "Color.h"

string Buffer::CutStringEnd(string str, int count) {
    if (count <= 0) return str;
    if ((size_t) count >= str.size()) return "";
    return str.substr(0, str.size() - count);
}

string Buffer::getLeftPropertyAccess(string str) {
    size_t start = str.find('$') + 1;
    size_t end = str.find("->");
    return str.substr(start, end - start);
}

string Buffer::getRightPropertyAccess(string str) {
    size_t pos = str.find("->");
    return str.substr(pos + 2);
}

void Buffer::setStartLine(int line) {
    _startLine = line;
}

int Buffer::getStartLine() const {
    return _startLine;
}

void Buffer::setDocId(string id, int erase) {
    _docId = id;
    _docId.erase(_docId.size() - erase);
}

string Buffer::getDocId() const {
    return _docId;
}

void Buffer::append(string str) {
#ifdef CONSOLE_BUFFER_LOG_ENABLED
    Console::Log(Color::Grey() + "%Buffer% Appended: '" + str + "'");
#endif

    if (_text.empty()) {
#ifdef CONSOLE_BUFFER_LOG_ENABLED
        Console::Log(Color::Grey() + "%Buffer% Error: no lines");
#endif
    } else {
        _text.back() += str;
    }
}

void Buffer::appendNewLines(string newLines) {
    int count = 0;
    for (char ch: newLines) {
        if (ch == '\n') {
            count++;
        }
    }

    for (int i = 0; i < count; ++i) {
        _text.emplace_back("");
    }

#ifdef CONSOLE_BUFFER_LOG_ENABLED
    Console::Log(Color::Grey() + "%Buffer% New lines: " + to_string(count));
#endif
}

string Buffer::pop() {
    if (_text.empty()) {
        return "";
    }
    string lastLine = _text.back();
    _text.pop_back();
    return lastLine;
}

void Buffer::reset() {
    _indentTracker.reset();
    _text.clear();
    _text.emplace_back("");
}

int Buffer::validateByLastLineAsIndent() {
    string indent = pop();
    _indentTracker.set(indent);
    for (int i = 0; i < _text.size(); ++i) {
        if (!_indentTracker.isValid(_text[i])) {
            return i;
        }
    }
    return -1;
}

string Buffer::current() const {
    string result;
    for (int i = 0; i < _text.size(); ++i) {
        result += _indentTracker.getContent(_text[i]);
    }
    return CutStringEnd(result, 1);
}
