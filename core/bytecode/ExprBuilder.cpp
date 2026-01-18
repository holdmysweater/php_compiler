#include "ExprBuilder.h"

#include <sstream>
#include <iomanip>
#include <vector>

#include "core/helpers/Console.h"
#include "jvm/descriptor-field.h"
#include "jvm/descriptor-method.h"

using namespace jvm;

static const ExprNode *childOrNull(const ExprNode *e, size_t i) {
    if (!e) return nullptr;
    if (i >= e->children.size()) return nullptr;
    return e->children[i];
}

static void emitBinary(
    Class *root,
    Method *method,
    AttributeCode *code,
    const ExprNode *left,
    const ExprNode *right,
    ConstantMethodref *op,
    void (*emitValue)(Class *, Method *, AttributeCode *, const ExprNode *)
) {
    emitValue(root, method, code, left); // push BasePhpValue
    emitValue(root, method, code, right); // push BasePhpValue
    *code << code->InvokeStatic(op); // pop2 push1
}

static void emitUnary(
    Class *root,
    Method *method,
    AttributeCode *code,
    const ExprNode *operand,
    ConstantMethodref *op,
    void (*emitValue)(Class *, Method *, AttributeCode *, const ExprNode *)
) {
    emitValue(root, method, code, operand); // push BasePhpValue
    *code << code->InvokeStatic(op); // pop1 push1
}

void ExprBuilder::EmitValue(Class *root, Method *method, AttributeCode *code, const ExprNode *expr) {
    if (!root) throw std::logic_error("ExprBuilder::EmitValue: root is null");
    if (!method) throw std::logic_error("ExprBuilder::EmitValue: method is null");
    if (!code) throw std::logic_error("ExprBuilder::EmitValue: code is null");

    // ---------- Runtime symbols ----------
    auto *nullValueField = root->getOrCreateFieldrefConstant(
        "com/phpjvm/BasePhpValue",
        "NULL_VALUE",
        DescriptorField("com/phpjvm/BasePhpValue")
    );

    auto *ofString = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "of",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("java/lang/String")}
        )
    );

    auto *ofBool = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "of",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("Z")} // boolean
        )
    );

    auto *ofLong = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "of",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("J")} // long
        )
    );

    auto *ofDouble = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "of",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("D")} // double
        )
    );

    // arithmetic / concat already exist in your RTL
    auto *add = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "add",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *sub = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "sub",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *mul = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "mul",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *div = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "div",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *mod = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "mod",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *concat = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "concat",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );

    // comparisons you already have:
    auto *eq = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "eq",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *identical = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "identical",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *lt = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "lt",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *le = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "le",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );

    // NEW helpers you add to RTL in step (1):
    auto *ne = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "ne",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *notIdentical = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "notIdentical",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *gt = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "gt",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *ge = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "ge",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *boolNot = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "not",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *boolAnd = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "and",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *boolOr = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "or",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );
    auto *spaceship = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "spaceship",
        DescriptorMethod(
            DescriptorField("com/phpjvm/BasePhpValue"),
            {DescriptorField("com/phpjvm/BasePhpValue"), DescriptorField("com/phpjvm/BasePhpValue")}
        )
    );

    if (expr == nullptr) {
        *code << code->GetStatic(nullValueField);
        return;
    }

    // ---------- Dispatch ----------
    switch (expr->type) {
        // ===== literals (already working) =====
        case ExprType::ET_STRING: {
            std::string s = (expr->value ? expr->value->stringValue : "");
            *code << code->PushString(s);
            *code << code->InvokeStatic(ofString);
            return;
        }

        case ExprType::ET_INT: {
            int64_t v = (expr->value ? (int64_t) expr->value->intValue : 0);
            *code << code->PushLong(v);
            *code << code->InvokeStatic(ofLong);
            return;
        }

        case ExprType::ET_FLOAT: {
            double v = (expr->value ? (double) expr->value->floatValue : 0.0);
            *code << code->PushDouble(v);
            *code << code->InvokeStatic(ofDouble);
            return;
        }

        case ExprType::ET_BOOL: {
            bool v = (expr->value ? expr->value->boolValue : false);
            *code << code->PushInt(v ? 1 : 0);
            *code << code->InvokeStatic(ofBool);
            return;
        }

        case ExprType::ET_NIL: {
            *code << code->GetStatic(nullValueField);
            return;
        }

        // ===== parentheses =====
        case ExprType::ET_PARENTHESIZED: {
            const ExprNode *inner = childOrNull(expr, 0);
            EmitValue(root, method, code, inner);
            return;
        }

        // ===== concat list (already in your code) =====
        case ExprType::ET_COMPLEX_STRING: {
            if (expr->children.empty() || expr->children[0] == nullptr) {
                *code << code->PushString("");
                *code << code->InvokeStatic(ofString);
                return;
            }

            const ExprNode *args = expr->children[0];

            std::vector<const ExprNode *> parts;
            if (args->type == ExprType::ET_EXPR_LIST) {
                for (auto *p: args->children) {
                    if (p) parts.push_back(p);
                }
            } else {
                parts.push_back(args);
            }

            if (parts.empty()) {
                *code << code->PushString("");
                *code << code->InvokeStatic(ofString);
                return;
            }

            EmitValue(root, method, code, parts[0]);
            for (size_t i = 1; i < parts.size(); i++) {
                EmitValue(root, method, code, parts[i]);
                *code << code->InvokeStatic(concat);
            }
            return;
        }

        // ===== arithmetic =====
        case ExprType::ET_ADD: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), add, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_SUBTRACT: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), sub, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_MULT: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), mul, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_DIV: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), div, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_MOD: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), mod, &ExprBuilder::EmitValue);
            return;
        }

        // ===== string concat "." =====
        case ExprType::ET_CONCAT: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), concat, &ExprBuilder::EmitValue);
            return;
        }

        // ===== comparisons =====
        case ExprType::ET_EQUAL: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), eq, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_NOT_EQUAL: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), ne, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_IDENTICALLY_EQUAL: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), identical,
                       &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_IDENTICALLY_NOT_EQUAL: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), notIdentical,
                       &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_LESS_THAN: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), lt, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_LESS_OR_EQUAL: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), le, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_GREATER_THAN: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), gt, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_GREAT_OR_EQUAL: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), ge, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_SPACESHIP: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), spaceship,
                       &ExprBuilder::EmitValue);
            return;
        }

        // ===== boolean ops (truthiness) =====
        case ExprType::ET_NOT: {
            emitUnary(root, method, code, childOrNull(expr, 0), boolNot, &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_AND: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), boolAnd,
                       &ExprBuilder::EmitValue);
            return;
        }
        case ExprType::ET_OR: {
            emitBinary(root, method, code, childOrNull(expr, 0), childOrNull(expr, 1), boolOr, &ExprBuilder::EmitValue);
            return;
        }

        default:
            Console::Warning("ExprBuilder::EmitValue not implemented for " + toString(expr->type) + " (pushing NULL)");
            *code << code->GetStatic(nullValueField);
            return;
    }
}
