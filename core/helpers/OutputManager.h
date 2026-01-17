#ifndef PHP_COMPILER_OUTPUTMANAGER_H
#define PHP_COMPILER_OUTPUTMANAGER_H

#include <string>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

class OutputManager {
public:
    static void OutputJson(const std::string &jsonContent, const std::string &baseName);

    static void OutputJson(const std::string &jsonContent, const std::string &baseName, bool isLogEnabled);

    static void OutputDot(const std::string &dotContent, const std::string &baseName);

    static void OutputDot(const std::string &dotContent, const std::string &baseName, bool isLogEnabled);

    static std::ofstream GetByteCodeFile(const std::string &baseName);

private:
    static fs::path EnsureOutputDir();

    static void GenerateSvgFromDot(const fs::path &dotFilePath);

    static void GenerateSvgFromDot(const fs::path &dotFilePath, bool isLogEnabled);
};

#endif //PHP_COMPILER_OUTPUTMANAGER_H
