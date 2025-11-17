#ifndef PHP_COMPILER_VISIBILITYTYPE_H
#define PHP_COMPILER_VISIBILITYTYPE_H

#include <string>

using std::string;

enum VisibilityType {
    VISIBILITY_UNKNOWN,
    VISIBILITY_PUBLIC,
    VISIBILITY_PROTECTED,
    VISIBILITY_PRIVATE,
};

inline string toString(VisibilityType type) {
    switch (type) {
        case VisibilityType::VISIBILITY_UNKNOWN: return "VISIBILITY_UNKNOWN";
        case VisibilityType::VISIBILITY_PUBLIC: return "VISIBILITY_PUBLIC";
        case VisibilityType::VISIBILITY_PROTECTED: return "VISIBILITY_PROTECTED";
        case VisibilityType::VISIBILITY_PRIVATE: return "VISIBILITY_PRIVATE";
        default: return "ERROR";
    }
}

inline string toSymbol(VisibilityType type) {
    switch (type) {
        case VisibilityType::VISIBILITY_UNKNOWN: return "unknown";
        case VisibilityType::VISIBILITY_PUBLIC: return "public";
        case VisibilityType::VISIBILITY_PROTECTED: return "protected";
        case VisibilityType::VISIBILITY_PRIVATE: return "private";
        default: return "ERROR";
    }
}

#endif //PHP_COMPILER_VISIBILITYTYPE_H
