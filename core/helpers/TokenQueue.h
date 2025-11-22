#ifndef PHP_COMPILER_TOKENQUEUE_H
#define PHP_COMPILER_TOKENQUEUE_H

#include "../macros.h"

#include "Token.h"

#include <queue>

using std::queue;

class TokenQueue {
    queue<Token> _queue;

public:
    void push(const Token &token);

    Token pop();

    bool isEmpty() const;

    string toString() const;
};

#endif //PHP_COMPILER_TOKENQUEUE_H
