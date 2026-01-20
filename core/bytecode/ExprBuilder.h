#pragma once

#include <cstdint>
#include <vector>
#include <string>

#include "core/nodes/ExprNode.h"
#include "core/nodes/DeclNode.h"
#include "jvm/class.h"
#include "jvm/attribute-code.h"

class DeclNode;
class ValueNode;

class ExprBuilder {
public:
    struct ByRefPair {
        uint16_t refKeySlot = 0;
        uint16_t valueSlot = 0;
    };

    // Local-scope management (already used by DeclNode)
    static void BeginLocalScope(jvm::Method *method, uint16_t nextFreeLocal);

    static void EndLocalScope(jvm::Method *method);

    static void DefineLocal(jvm::Method *method, const std::string &varName, uint16_t localIndex);

    static bool TryGetLocal(jvm::Method *method, const std::string &varName, uint16_t &outIndex);

    static void ReserveNextLocal(jvm::Method *method, uint16_t nextFreeLocal);

    static uint16_t AllocTempLocal(jvm::Method *method);

    // By-ref copy-out layout
    static void SetByRefLayout(jvm::Method *method, const std::vector<ByRefPair> &pairs);

    static const std::vector<ByRefPair> *GetByRefLayout(jvm::Method *method);

    static void EmitFlushByRefIfNeeded(jvm::Class *root, jvm::Method *method, jvm::AttributeCode *code);

    // Signature registry
    static void RegisterFunctionSignature(const std::string &fnLower, DeclNode *params, ValueNode *retType);

    static void RegisterMethodSignature(const std::string &classLower, const std::string &methodLower,
                                        DeclNode *params, ValueNode *retType);

    // Main expression emitter
    static void EmitValue(jvm::Class *root, jvm::Method *method, jvm::AttributeCode *code, const ExprNode *expr);

    // Method context helpers (needed for $this + static::)
    static void SetPhpMethodHasThis(jvm::Method *method, bool hasThis);

    static bool PhpMethodHasThis(jvm::Method *method);

    static uint16_t PhpThisLocalSlot(); // currently always 0 in your calling convention

    // NEW: caller-scope (lexical class) for PHP visibility checks
    // Empty string = global scope / function scope (no class context)
    static void SetPhpCallerClass(jvm::Method *method, const std::string &callerClassLower);

    static std::string PhpCallerClass(jvm::Method *method);
};
