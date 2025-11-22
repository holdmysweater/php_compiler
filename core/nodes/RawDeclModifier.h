#ifndef PHP_COMPILER_RAWDECLMODIFIER_H
#define PHP_COMPILER_RAWDECLMODIFIER_H

#include "enums/VisibilityType.h"

class RawDeclModifier {
public:
    VisibilityType visibility = VISIBILITY_PUBLIC;
    bool isStatic = false;

    RawDeclModifier(VisibilityType visibility, bool isStatic);

    explicit RawDeclModifier(bool isStatic);

    explicit RawDeclModifier(VisibilityType visibility);

    static RawDeclModifier *VisibilityMod(VisibilityType type);

    static RawDeclModifier *StaticMod(bool isStatic);

    static RawDeclModifier *StaticMod(RawDeclModifier *modifier, bool isStatic);
};

#endif //PHP_COMPILER_RAWDECLMODIFIER_H
