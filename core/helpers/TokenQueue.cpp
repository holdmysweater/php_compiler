#include "TokenQueue.h"
#include "Color.h"
#include "Console.h"

void TokenQueue::push(const Token &token) {
#ifdef CONSOLE_TOKENQUEUE_LOG_ENABLED
    Console::Log(Color::Grey() + "%TokenQueue% Old queue: " + this->toString());
#endif

    _queue.push(token);

#ifdef CONSOLE_TOKENQUEUE_LOG_ENABLED
    Console::Log(Color::Grey() + "%TokenQueue% Pushed: " + token.toString());
    Console::Log(Color::Grey() + "%TokenQueue% New queue: " + this->toString());
#endif
}

Token TokenQueue::pop() {
#ifdef CONSOLE_TOKENQUEUE_LOG_ENABLED
    Console::Log(Color::Grey() + "%TokenQueue% Old queue: " + this->toString());
#endif

    if (_queue.empty()) {
        Console::Error("%TokenQueue% Empty queue");
    }
    Token token = _queue.front();
    _queue.pop();

#ifdef CONSOLE_TOKENQUEUE_LOG_ENABLED
    Console::Log(Color::Grey() + "%TokenQueue% Popped: " + token.toString());
    Console::Log(Color::Grey() + "%TokenQueue% New queue: " + this->toString());
#endif

    return token;
}

bool TokenQueue::isEmpty() const {
    return _queue.empty();
}

string TokenQueue::toString() const {
    string result = "[ ";
    queue<Token> temp = _queue;

    while (!temp.empty()) {
        result += temp.front().toString();
        temp.pop();
        if (!temp.empty()) {
            result += ", ";
        }
    }

    result += " ]";
    return result;
}
