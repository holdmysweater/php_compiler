#include "StmtBuilder.h"

#include "core/helpers/Console.h"
#include "core/bytecode/ExprBuilder.h"
#include "jvm/descriptor-field.h"
#include "jvm/descriptor-method.h"

using namespace jvm;

void StmtBuilder::EmitEcho(Class *root, Method *method, AttributeCode *code, ExprNode *expr) {
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
    // getstatic out                -> [PrintStream]
    // EmitValue(expr)              -> [PrintStream, BasePhpValue]
    // invokevirtual toPhpString    -> [PrintStream, String]
    // invokevirtual print(String)  -> []
    *code << code->GetStatic(systemOut);

    ExprBuilder::EmitValue(root, method, code, expr);

    *code << code->InvokeVirtual(toPhpString);
    *code << code->InvokeVirtual(print);
}
