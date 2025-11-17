#include "RawDeclModifier.h"

RawDeclModifier::RawDeclModifier(VisibilityType visibility, bool isStatic) {
    this->visibility = visibility;
    this->isStatic = isStatic;
}

RawDeclModifier::RawDeclModifier(bool isStatic) {
    this->isStatic = isStatic;
}

RawDeclModifier::RawDeclModifier(VisibilityType visibility) {
    this->visibility = visibility;
}

RawDeclModifier *RawDeclModifier::VisibilityMod(VisibilityType type) {
    return new RawDeclModifier(type);
}

RawDeclModifier *RawDeclModifier::StaticMod(bool isStatic) {
    return new RawDeclModifier(isStatic);
}

RawDeclModifier *RawDeclModifier::StaticMod(RawDeclModifier *modifier, bool isStatic) {
    modifier->isStatic = isStatic;
    return modifier;
}
