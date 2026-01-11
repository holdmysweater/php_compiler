#ifndef PHP_COMPILER_STMTNODE_H
#define PHP_COMPILER_STMTNODE_H

#include "BaseNode.h"
#include "ExprNode.h"
#include "enums/StmtType.h"

#include <vector>

using std::vector;

class StmtNode : public BaseNode {
public:
    StmtType type = ST_UNKNOWN;

    vector<StmtNode *> children;

    ExprNode *condition = nullptr;

    ExprNode *expr = nullptr;
    StmtNode *stmt = nullptr;

    ExprNode *loopInitializer = nullptr;
    ExprNode *loopEndAction = nullptr;

    ExprNode *foreachCollection = nullptr;
    ExprNode *foreachKey = nullptr;
    ExprNode *foreachValue = nullptr;

    StmtNode *elseIfStmt = nullptr;
    StmtNode *elseStmt = nullptr;

    StmtNode *catchStmt = nullptr;
    string catchId;
    ValueNode *catchType = nullptr;
    StmtNode *finallyStmt = nullptr;

    string _getClassName() const override;

    string toJson() const override;

    string toDot() const override;

    bool doSemantics() override;

    // List
    static StmtNode *StmtList(StmtNode *stmt);

    static StmtNode *AppendToStmtList(StmtNode *stmtList, StmtNode *newStmt);

    // Statements
    static StmtNode *ExprStmt(ExprNode *expr);

    static StmtNode *While(ExprNode *condition, StmtNode *stmt);

    static StmtNode *DoWhile(ExprNode *condition, StmtNode *stmt);

    static StmtNode *For(ExprNode *initializer, ExprNode *condition, ExprNode *endAction, StmtNode *stmt);

    static StmtNode *ForEachSimple(ExprNode *collection, ExprNode *value, StmtNode *stmt);

    static StmtNode *ForEachKeyValue(ExprNode *collection, ExprNode *key, ExprNode *value, StmtNode *stmt);

    static StmtNode *If(ExprNode *condition, StmtNode *stmt);

    static StmtNode *If_ElifElse(ExprNode *condition, StmtNode *stmt, StmtNode *elseIfStmt, StmtNode *elseStmt);

    static StmtNode *If_Elif(ExprNode *condition, StmtNode *stmt, StmtNode *elseIfStmt);

    static StmtNode *If_Else(ExprNode *condition, StmtNode *stmt, StmtNode *elseStmt);

    static StmtNode *ElseIf(ExprNode *condition, StmtNode *stmt);

    static StmtNode *Else(StmtNode *stmt);

    static StmtNode *Switch(ExprNode *expr, StmtNode *stmt);

    static StmtNode *Case(ExprNode *expr, StmtNode *stmt);

    static StmtNode *CaseDefault(StmtNode *stmt);

    static StmtNode *Echo(ExprNode *expr);

    static StmtNode *ReturnStmt();

    static StmtNode *ReturnStmt(ExprNode *expr);

    static StmtNode *ThrowStmt(ExprNode *expr);

    static StmtNode *CatchStmt(StmtNode *stmt, ValueNode *catchType, string *catchId);

    static StmtNode *FinallyStmt(StmtNode *stmt);

    static StmtNode *TryCatchStmt(StmtNode *stmt, StmtNode *catchStmt);

    static StmtNode *TryFinallyStmt(StmtNode *stmt, StmtNode *finallyStmt);

    static StmtNode *TryCatchFinallyStmt(StmtNode *stmt, StmtNode *catchStmt, StmtNode *finallyStmt);

    static StmtNode *BreakStmt();

    static StmtNode *ContinueStmt();
};

#endif //PHP_COMPILER_STMTNODE_H
