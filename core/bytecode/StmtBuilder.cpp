#include "StmtBuilder.h"

#include "core/helpers/Console.h"
#include "core/bytecode/ExprBuilder.h"
#include "jvm/descriptor-field.h"
#include "jvm/descriptor-method.h"

using namespace jvm;

void StmtBuilder::EmitEcho(Class *root, Method *method, AttributeCode *code, ExprNode *expr) {
    if (!root) throw std::logic_error("StmtBuilder::EmitEcho: root is null");
    if (!method) throw std::logic_error("StmtBuilder::EmitEcho: method is null");
    if (!code) throw std::logic_error("StmtBuilder::EmitEcho: code is null");

    auto *systemOut = root->getOrCreateFieldrefConstant(
        "java/lang/System",
        "out",
        DescriptorField("java/io/PrintStream")
    );

    // echo in PHP has NO newline, so prefer print(...) not println(...)
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

    // Stack:
    // EmitValue(expr)                   -> [BasePhpValue]
    // invokevirtual toPhpString         -> [String]
    // getstatic System.out              -> [String, PrintStream]
    // swap                              -> [PrintStream, String]
    // invokevirtual print(String)       -> []
    ExprBuilder::EmitValue(root, method, code, expr); // push BasePhpValue
    *code << code->InvokeVirtual(toPhpString); // pop BasePhpValue, push String
    *code << code->GetStatic(systemOut); // push PrintStream
    *code << code->Swap(); // reorder to [PrintStream, String]
    *code << code->InvokeVirtual(print); // consume both
}
