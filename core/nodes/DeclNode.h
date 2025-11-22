#ifndef PHP_COMPILER_DECLNODE_H
#define PHP_COMPILER_DECLNODE_H

#include "BaseNode.h"
#include "ValueNode.h"
#include "ExprNode.h"
#include "RawDeclModifier.h"
#include "enums/DeclType.h"
#include "enums/VisibilityType.h"

#include <vector>

#include "StmtNode.h"

using std::vector;

class DeclNode : public BaseNode {
public:
    DeclType type = DT_UNKNOWN;
    string name;

    vector<DeclNode *> children;

    DeclNode *declList = nullptr;
    ExprNode *expr = nullptr;
    StmtNode *stmt = nullptr;
    ValueNode *valueType = nullptr;

    string className;
    string classNameExtended;

    int isStatic = -1;
    VisibilityType visibilityType = VisibilityType::VISIBILITY_UNKNOWN;

    string toJson() const override;

    string toDot() const override;

    bool doSemantics() const override;

    // List
    static DeclNode *DeclList(DeclNode *decl);

    static DeclNode *AppendToDeclList(DeclNode *declList, DeclNode *newDecl);

    static DeclNode *SetModsToDecl(DeclNode *decl, RawDeclModifier *modifier);

    // Declarations
    static DeclNode *ClassDecl(string *className, DeclNode *declList);

    static DeclNode *ClassDecl(string *className, string *extendedClassName, DeclNode *declList);

    static DeclNode *PropertyDecl(string *name);

    static DeclNode *PropertyDecl(string *name, ExprNode *expr);

    static DeclNode *ConstDecl(string *name, ExprNode *expr);

    static DeclNode *ParamDecl(string *name);

    static DeclNode *ParamDeclType(string *name, ValueNode *type);

    static DeclNode *ParamDeclExpr(string *name, ExprNode *expr);

    static DeclNode *ParamDeclExprType(string *name, ExprNode *expr, ValueNode *type);

    static DeclNode *FunctionDecl(string *name, DeclNode *params);

    static DeclNode *FunctionDecl(string *name, DeclNode *params, ValueNode *type);

    static DeclNode *FunctionAddBody(DeclNode *func, StmtNode *body);
};

#endif //PHP_COMPILER_DECLNODE_H
