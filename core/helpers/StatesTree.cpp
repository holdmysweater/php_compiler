#include "StatesTree.h"
#include "Console.h"
#include "Color.h"

#include <sstream>

int StatesTree::pushAndPeek(int state) {
    _states.push_back(state);

#ifdef CONSOLE_STATE_LOG_ENABLED
    Console::Log(Color::Magenta() + "S = " + StatesTree::getStateTree());
#endif

    return current();
}

int StatesTree::popAndPeek() {
    int prev = _states.back();
    _states.pop_back();

#ifdef CONSOLE_STATE_LOG_ENABLED
    Console::Log(
        Color::Magenta() + "S = " + StatesTree::getStateTree() + " (was " + StatesTree::StateCodeToString(prev) + ")");
#endif

    return _states.back();
}

int StatesTree::current() {
    return _states.back();
}

int StatesTree::previous() {
    return _states[_states.size() - 2];
}

string StatesTree::getStateTree() {
    if (_states.empty()) {
        return "";
    }

    std::ostringstream oss;
    oss << Color::Magenta();
    for (auto rit = _states.rbegin(); rit != _states.rend(); ++rit) {
        oss << StatesTree::StateCodeToString(*rit) << Color::Grey();
        if (rit + 1 != _states.rend()) {
            oss << " <- ";
        }
    }
    return oss.str();
}

string StatesTree::StateCodeToString(int state) {
    switch (state) {
        case 0: return "INITIAL";
        case 1: return "PURE_PHP";
        case 2: return "INTERPOLATED_PHP";
        case 3: return "COMMENT_HTML";
        case 4: return "COMMENT_MULTILINE";
        case 5: return "STRING_SINGLE_QUOTES";
        case 6: return "STRING_DOUBLE_QUOTES";
        case 7: return "STRING_HEREDOC_OR_NOWDOC";
        case 8: return "STRING_HEREDOC";
        case 9: return "STRING_NOWDOC";
        case 10: return "ARRAY_CONTENT";
        case 11: return "INDENT_HEREDOC_OR_NOWDOC";
        default: return "UNKNOWN";
    }
}
