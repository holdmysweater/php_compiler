#ifndef PHP_COMPILER_STMTBUILDER_H
#define PHP_COMPILER_STMTBUILDER_H

#include "core/nodes/ExprNode.h"
#include "jvm/attribute-code.h"


class StmtBuilder {
public:
    static void EmitEcho(Class *root, Method *method, AttributeCode *code, ExprNode *expr);
};

#endif //PHP_COMPILER_STMTBUILDER_H
