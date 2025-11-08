#include "IndentationTracker.h"
#include "Console.h"
#include "Color.h"

string IndentationTracker::current() const {
    return _indentation;
}

bool IndentationTracker::set(string indentation) {
#ifdef CONSOLE_LOG_ENABLED
    Console::Log(Color::Grey() + "%IndentationTracker% Set: '" + indentation + "'");
#endif

    if (indentation.empty()) {
        _indentation = indentation;
        return true;
    }

    bool all_tabs = true;
    bool all_spaces = true;

    for (char ch: indentation) {
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

    _indentation = indentation;
    return true;
}

void IndentationTracker::reset() {
    _indentation = "";

#ifdef CONSOLE_LOG_ENABLED
    Console::Log(Color::Grey() + "%IndentationTracker% Reset: ''");
#endif
}

bool IndentationTracker::isValid(string str) const {
    if (_indentation.empty()) {
#ifdef CONSOLE_LOG_ENABLED
        Console::Log(Color::Grey() + "%IndentationTracker% Valid: '" + str + "'");
#endif
        return true;
    }
    if (str.size() < _indentation.size()) {
#ifdef CONSOLE_LOG_ENABLED
        Console::Log(Color::Grey() + "%IndentationTracker% Invalid (too short): '" + str + "'");
#endif

        return false;
    }

    bool result = str.compare(0, _indentation.size(), _indentation) == 0;

#ifdef CONSOLE_LOG_ENABLED
    Console::Log(
        Color::Grey() + "%IndentationTracker% Validated (" + (result ? "true" : "false") + "): '" + str + "'");
#endif

    return result;
}

string IndentationTracker::getContent(string str) const {
    if (!isValid(str)) {
#ifdef CONSOLE_LOG_ENABLED
        Console::Log(Color::Grey() + "%IndentationTracker% Not valid: '" + str + "'");
#endif
        return "";
    }

    string result = str.substr(_indentation.size());

#ifdef CONSOLE_LOG_ENABLED
    Console::Log(Color::Grey() + "%IndentationTracker% Content: '" + result + "' (from '" + str + "')");
#endif

    return result;
}
