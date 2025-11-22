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
    ExprNode *expr = nullptr;
    string content;

    string toJson() const override;

    string toDot() const override;

    bool doSemantics() const override;

    // List
    static ElementNode *EmptyElement();

    static ElementNode *ElementList(ElementNode *element, ElementType type);

    static ElementNode *AppendToElementList(ElementNode *elementList, ElementNode *newElement);

    // Base
    static ElementNode *PhpDecl(DeclNode *declList);

    static ElementNode *PhpStmt(StmtNode *stmt);

    static ElementNode *PhpEchoContent(ExprNode *expr);

    static ElementNode *HtmlContent(string *content);
};

#endif //PHP_COMPILER_ELEMENTNODE_H
