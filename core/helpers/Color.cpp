#include "Color.h"

string Color::Reset() { return "\033[0m"; }
string Color::Red() { return "\033[31m"; }
string Color::Green() { return "\033[32m"; }
string Color::Yellow() { return "\033[33m"; }
string Color::Blue() { return "\033[94m"; }
string Color::Magenta() { return "\033[35m"; }
string Color::Cyan() { return "\033[36m"; }
string Color::Grey() { return "\033[90m"; }
string Color::White() { return "\033[37m"; }
string Color::Bold() { return "\033[1m"; }
string Color::Underline() { return "\033[4m"; }
