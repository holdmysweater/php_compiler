#ifndef PHP_COMPILER_OUTPUTMANAGER_H
#define PHP_COMPILER_OUTPUTMANAGER_H

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

class OutputManager {
public:
    static void OutputJson(const std::string &jsonContent, const std::string &baseName);

    static void OutputDot(const std::string &dotContent, const std::string &baseName);

private:
    static fs::path EnsureOutputDir();

    static fs::path EnsureDotOutputDir();

    static void GenerateSvgFromDot(const fs::path &dotFilePath);
};

#endif //PHP_COMPILER_OUTPUTMANAGER_H
