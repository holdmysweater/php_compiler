#ifndef PHP_COMPILER_BYTECODEHELPER_H
#define PHP_COMPILER_BYTECODEHELPER_H

#include "core/nodes/ElementNode.h"
#include "jvm/class.h"

class ByteCodeHelper {
public:
    static void GenerateAndExecute(ElementNode *root, const string &fileName);

protected:
    static vector<Class *> GenerateClasses(ElementNode *root, const string &className);

    static void WriteClassToFile(const string &fileName, Class byteCodeClass);

    static void ExecuteFile(const string &baseName);

    static void VerboseFile(const string &baseName);
};

#endif //PHP_COMPILER_BYTECODEHELPER_H
