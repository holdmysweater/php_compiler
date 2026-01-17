#include "ByteCodeHelper.h"

#include <filesystem>
#include <fstream>
#include <optional>

#include <jvm/class.h>
#include <jvm/method.h>

#include "core/helpers/Config.h"
#include "core/helpers/Console.h"
#include "core/helpers/OutputManager.h"
#include "jvm/descriptor-method.h"

using namespace jvm;


void ByteCodeHelper::GenerateAndExecute(ElementNode *root, const string &fileName) {
    vector<Class *> classes = GenerateClasses(root, fileName);

    for (auto byteCodeClass: classes) {
        WriteClassToFile(byteCodeClass->getClassName(), *byteCodeClass);
        VerboseFile(byteCodeClass->getClassName());
    }

    ExecuteFile(fileName);
}


vector<Class *> ByteCodeHelper::GenerateClasses(ElementNode *root, const string &className) {
    Class *byteCodeClass = new Class(className, "java/lang/Object");

    byteCodeClass->addFlag(Class::ACC_SUPER);
    byteCodeClass->addFlag(Class::ACC_PUBLIC);

    Method *mainMethod = byteCodeClass->getOrCreateMethod(
        "main",
        DescriptorMethod{
            std::nullopt,
            {{"java/lang/String", 1}}
        }
    );

    mainMethod->addFlag(Method::ACC_PUBLIC);
    mainMethod->addFlag(Method::ACC_STATIC);

    AttributeCode *baseCode = mainMethod->getCodeAttribute();
    *baseCode << baseCode->ReturnVoid();

    std::vector<Class *> classes;
    byteCodeClass = root->processClass(byteCodeClass, classes);
    classes.push_back(byteCodeClass);

    return classes;
}


void ByteCodeHelper::WriteClassToFile(const string &fileName, Class byteCodeClass) {
    std::ofstream file = OutputManager::GetByteCodeFile(fileName);
    byteCodeClass.writeTo(file);
    file.close();
}


void ByteCodeHelper::VerboseFile(const string &baseName) {
    Console::SystemLog("Executing verbose of " + baseName + "...");

    std::string cmd = "javap -verbose \"" + Config::GetOutputDir().string() + baseName + ".class\"";

    Console::SystemLog(cmd);

    if (system(cmd.c_str()) != 0) {
        Console::SystemError("Bytecode verbose failed!");
        return;
    }

    cmd += " > " + Config::GetOutputDir().string() + "output_verbose.txt";
    system(cmd.c_str());

    Console::SystemLog("Bytecode verbose successful!");
}


void ByteCodeHelper::ExecuteFile(const string &baseName) {
    std::string cmd;
    Console::SystemLog("Executing bytecode...");

    cmd = "java -cp \"" + Config::GetOutputDir().string() + "\\\" " + baseName;

    Console::SystemLog(cmd);

    if (system(cmd.c_str()) == 0) {
        Console::SystemLog("Bytecode execution successful!");
    } else {
        Console::SystemError("Bytecode execution failed!");
    }
}
