#include "IndentTracker.h"
#include "Console.h"
#include "Color.h"

string IndentTracker::current() const {
    return _indent;
}

bool IndentTracker::set(string indent) {
#ifdef CONSOLE_INDENTTRACKER_LOG_ENABLED
    Console::Log(Color::Grey() + "%IndentTracker% Set: '" + indent + "'");
#endif

    if (indent.empty()) {
        _indent = indent;
        return true;
    }

    bool all_tabs = true;
    bool all_spaces = true;

    for (char ch: indent) {
        if (ch != '\t') {
            all_tabs = false;
        }

        if (ch != ' ') {
            all_spaces = false;
        }
    }

    if (!all_tabs && !all_spaces) {
        return false;
    }

    _indent = indent;
    return true;
}

void IndentTracker::reset() {
    _indent = "";

#ifdef CONSOLE_INDENTTRACKER_LOG_ENABLED
    Console::Log(Color::Grey() + "%IndentTracker% Reset: ''");
#endif
}

bool IndentTracker::isValid(string str) const {
    if (_indent.empty() || str.empty()) {
#ifdef CONSOLE_INDENTTRACKER_LOG_ENABLED
        Console::Log(Color::Grey() + "%IndentTracker% Valid: '" + str + "'");
#endif
        return true;
    }
    if (str.size() < _indent.size()) {
#ifdef CONSOLE_INDENTTRACKER_LOG_ENABLED
        Console::Log(Color::Grey() + "%IndentTracker% Invalid (too short): '" + str + "'");
#endif

        return false;
    }

    bool result = str.compare(0, _indent.size(), _indent) == 0;

#ifdef CONSOLE_INDENTTRACKER_LOG_ENABLED
    Console::Log(
        Color::Grey() + "%IndentTracker% Validated (" + (result ? "true" : "false") + "): '" + str + "'");
#endif

    return result;
}

string IndentTracker::getContent(string str) const {
    if (!isValid(str)) {
#ifdef CONSOLE_INDENTTRACKER_LOG_ENABLED
        Console::Log(Color::Grey() + "%IndentTracker% Not valid: '" + str + "'");
#endif
        return "";
    }

    if (str.empty()) {
#ifdef CONSOLE_INDENTTRACKER_LOG_ENABLED
        Console::Log(Color::Grey() + "%IndentTracker% Content: empty");
#endif
        return "\n";
    }

    string result = str.substr(_indent.size()) + "\n";

#ifdef CONSOLE_INDENTTRACKER_LOG_ENABLED
    Console::Log(Color::Grey() + "%IndentTracker% Content: '" + result + "' (from '" + str + "')");
#endif

    return result;
}
