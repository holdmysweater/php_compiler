#ifndef PHP_COMPILER_EXPRBUILDER_H
#define PHP_COMPILER_EXPRBUILDER_H

#include "core/nodes/ExprNode.h"
#include "jvm/attribute-code.h"
#include "jvm/class.h"
#include "jvm/method.h"

class ExprBuilder {
public:
    // Emits bytecode that leaves ONE com/phpjvm/BasePhpValue on the operand stack.
    static void EmitValue(Class *root, Method *method, AttributeCode *code, const ExprNode *expr);
};

#endif // PHP_COMPILER_EXPRBUILDER_H
