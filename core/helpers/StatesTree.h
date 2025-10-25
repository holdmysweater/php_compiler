#ifndef PHP_COMPILER_STATE_H
#define PHP_COMPILER_STATE_H

#define CONSOLE_STATE_LOG_ENABLED

#include <string>
#include <vector>

using std::vector;
using std::string;

class StatesTree {
    vector<int> _states;

public:
    static string StateCodeToString(int state);

    int pushAndPeek(int state);

    int popAndPeek();

    int current();

    int previous();

    string getStateTree();
};

#endif //PHP_COMPILER_STATE_H
