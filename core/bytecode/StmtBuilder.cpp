#include "StmtBuilder.h"

#include "core/helpers/Console.h"
#include "core/bytecode/ExprBuilder.h"
#include "jvm/descriptor-field.h"
#include "jvm/descriptor-method.h"

using namespace jvm;

static void emitEchoOne(Class *root, Method *method, AttributeCode *code, ExprNode *expr) {
    auto *systemOut = root->getOrCreateFieldrefConstant(
        "java/lang/System",
        "out",
        DescriptorField("java/io/PrintStream")
    );

    // echo has NO newline -> print(String)
    auto *print = root->getOrCreateMethodrefConstant(
        "java/io/PrintStream",
        "print",
        DescriptorMethod(
            std::nullopt,
            {DescriptorField("java/lang/String")}
        )
    );

    auto *toPhpString = root->getOrCreateMethodrefConstant(
        "com/phpjvm/BasePhpValue",
        "toPhpString",
        DescriptorMethod(
            DescriptorField("java/lang/String"),
            {}
        )
    );

    // EmitValue(expr) -> BasePhpValue
    // invokevirtual toPhpString -> String
    // getstatic System.out -> PrintStream
    // swap -> PrintStream, String
    // invokevirtual print -> void
    ExprBuilder::EmitValue(root, method, code, expr);
    *code << code->InvokeVirtual(toPhpString);
    *code << code->GetStatic(systemOut);
    *code << code->Swap();
    *code << code->InvokeVirtual(print);
}

void StmtBuilder::EmitEcho(Class *root, Method *method, AttributeCode *code, ExprNode *expr) {
    if (!root) throw std::logic_error("StmtBuilder::EmitEcho: root is null");
    if (!method) throw std::logic_error("StmtBuilder::EmitEcho: method is null");
    if (!code) throw std::logic_error("StmtBuilder::EmitEcho: code is null");

    if (!expr) {
        // echo null -> prints ""? In PHP echo NULL prints nothing.
        // Your BasePhpValue.NULL_VALUE.toPhpString() should presumably be "" anyway.
        emitEchoOne(root, method, code, nullptr);
        return;
    }

    // IMPORTANT: your AST stores echo args as ET_EXPR_LIST
    if (expr->type == ExprType::ET_EXPR_LIST) {
        for (auto *ch: expr->children) {
            if (!ch) continue;
            emitEchoOne(root, method, code, ch);
        }
        return;
    }

    // normal single expression
    emitEchoOne(root, method, code, expr);
}
