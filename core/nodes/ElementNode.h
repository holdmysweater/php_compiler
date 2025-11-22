#ifndef PHP_COMPILER_ELEMENTNODE_H
#define PHP_COMPILER_ELEMENTNODE_H

#include "DeclNode.h"
#include "enums/ElementType.h"

class ElementNode : public BaseNode {
public:
    ElementType type = ELEMENT_UNKNOWN;

    vector<ElementNode *> children;

    DeclNode *decl = nullptr;
    StmtNode *stmt = nullptr;

    string _getClassName() const override;

    string toJson() const override;

    string toDot() const override;

    bool doSemantics() override;

    // List
    static ElementNode *EmptyElement();

    static ElementNode *ElementList(ElementNode *element);

    static ElementNode *AppendToElementList(ElementNode *elementList, ElementNode *newElement);

    // Base
    static ElementNode *PhpClassDecl(DeclNode *declList);

    static ElementNode *PhpFuncDecl(DeclNode *declList);

    static ElementNode *PhpStmt(StmtNode *stmt);
};

#endif //PHP_COMPILER_ELEMENTNODE_H
