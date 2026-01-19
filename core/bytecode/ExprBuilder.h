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

    // ------------------------------------------------------------
    // Local scope support for function/method variables ($a, $b, ...)
    // ------------------------------------------------------------
    // Call before compiling a function/method body.
    // nextFreeLocal is the first free local slot index ExprBuilder can allocate for new locals.
    static void BeginLocalScope(jvm::Method *method, uint16_t nextFreeLocal);

    // Define a local slot for a variable name (without $ or with $ - both ok).
    static void DefineLocal(jvm::Method *method, const std::string &varName, uint16_t localIndex);

    // Look up a local slot; returns true if found.
    static bool TryGetLocal(jvm::Method *method, const std::string &varName, uint16_t &outIndex);

    // End local scope after compiling function/method body.
    static void EndLocalScope(jvm::Method *method);
};

#endif // PHP_COMPILER_EXPRBUILDER_H
