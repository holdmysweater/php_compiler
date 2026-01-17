#include "ByteCodeHelper.h"

#include <filesystem>
#include <fstream>

#include <jvm/class.h>
#include <jvm/method.h>
#include <jvm/descriptor-method.h>

#include "core/helpers/Config.h"
#include "core/helpers/Console.h"
#include "core/helpers/OutputManager.h"

using namespace jvm;


void ByteCodeHelper::GenerateAndExecute(ElementNode *root, const string &fileName) {
    Class byteCodeClass = GenerateClass(root, fileName);

    WriteClassToFile(fileName, byteCodeClass);

    ExecuteFile(fileName);
}


Class ByteCodeHelper::GenerateClass(ElementNode *root, const string &className) {
    Class byteCodeClass(className, "java/lang/Object");

    Method *mainMethod = byteCodeClass.getOrCreateMethod(
        "main",
        DescriptorMethod{
            std::nullopt,
            {{"java/lang/String", 1}} // String[] args
        }
    );

    byteCodeClass.addFlag(Class::ACC_SUPER);
    byteCodeClass.addFlag(Class::ACC_PUBLIC);

    mainMethod->addFlag(Method::ACC_PUBLIC);
    mainMethod->addFlag(Method::ACC_STATIC);

    // --- constants for System.out.println("Hello, world!") ---
    // Field: java/lang/System.out : Ljava/io/PrintStream;
    auto descriptor_field_out = DescriptorField("java/io/PrintStream");
    ConstantFieldref *systemOut = byteCodeClass.getOrCreateFieldrefConstant(
        "java/lang/System",
        "out",
        descriptor_field_out
    );

    auto descriptor_method_println = DescriptorMethod(
        std::nullopt,
        {{"java/lang/String"}}
    );

    // Method: java/io/PrintStream.println : (Ljava/lang/String;)V
    ConstantMethodref *println = byteCodeClass.getOrCreateMethodrefConstant(
        "java/io/PrintStream",
        "println",
        descriptor_method_println
    );

    AttributeCode *code = mainMethod->getCodeAttribute();
    *code
            << code->GetStatic(systemOut)
            << code->PushString("Hello, World!")
            << code->InvokeVirtual(println)
            << code->ReturnVoid();

    return byteCodeClass;
}


void ByteCodeHelper::WriteClassToFile(const string &fileName, Class byteCodeClass) {
    std::ofstream file = OutputManager::GetByteCodeFile(fileName);
    byteCodeClass.writeTo(file);
    file.close();
}


void ByteCodeHelper::ExecuteFile(const string &baseName) {
    Console::SystemLog("Executing verbose of bytecode...");

    std::string cmd = "javap -verbose \"" + Config::GetOutputDir().string() + baseName + ".class\"";

    Console::SystemLog(cmd);

    if (system(cmd.c_str()) != 0) {
        Console::SystemError("Bytecode verbose failed!");
        return;
    }

    Console::SystemLog("Bytecode verbose successful!");
    Console::SystemLog("Executing bytecode...");

    cmd += " > " + Config::GetOutputDir().string() + "output_verbose.txt";
    system(cmd.c_str());

    cmd = "java -cp \"" + Config::GetOutputDir().string() + "\\\" " + baseName;

    Console::SystemLog(cmd);

    if (system(cmd.c_str()) == 0) {
        Console::SystemLog("Bytecode execution successful!");
    } else {
        Console::SystemError("Bytecode execution failed!");
    }
}
