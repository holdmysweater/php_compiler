#ifndef PHP_COMPILER_COLOR_H
#define PHP_COMPILER_COLOR_H

#include <string>

using namespace std;

class Color {
public:
    static string Reset();

    static string Red();

    static string Green();

    static string Yellow();

    static string Blue();

    static string Magenta();

    static string Cyan();

    static string Grey();

    static string White();

    static string Bold();

    static string Underline();
};

#endif //PHP_COMPILER_COLOR_H
