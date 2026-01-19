#ifndef PHP_COMPILER_EXPRBUILDER_H
#define PHP_COMPILER_EXPRBUILDER_H

#include "core/nodes/DeclNode.h"
#include "core/nodes/ExprNode.h"
#include "jvm/attribute-code.h"
#include "jvm/class.h"
#include "jvm/method.h"

class ExprBuilder {
public:
    static void RegisterFunctionSignature(const std::string &fnLower, DeclNode *params, ValueNode *retType);

    static void RegisterMethodSignature(const std::string &classLower, const std::string &methodLower, DeclNode *params,
                                        ValueNode *retType);

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

    // --- NEW: support updating nextFree without wiping the map ---
    static void ReserveNextLocal(jvm::Method *method, uint16_t nextFreeLocal);

    // --- NEW: allocate a temporary local slot (for return-value storing) ---
    static uint16_t AllocTempLocal(jvm::Method *method);

    // --- NEW: remember which locals are by-ref (valueSlot + refKeySlot) ---
    struct ByRefPair {
        uint16_t valueSlot;
        uint16_t refKeySlot;
    };

    static void SetByRefLayout(jvm::Method *method, const std::vector<ByRefPair> &pairs);

    static const std::vector<ByRefPair> *GetByRefLayout(jvm::Method *method);

    // --- NEW: emit writeback for by-ref params (stack-neutral; call with empty stack) ---
    static void EmitFlushByRefIfNeeded(jvm::Class *root, jvm::Method *method, jvm::AttributeCode *code);
};

#endif // PHP_COMPILER_EXPRBUILDER_H
