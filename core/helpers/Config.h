#ifndef PHP_COMPILER_CONFIG_H
#define PHP_COMPILER_CONFIG_H

#include <filesystem>

namespace fs = std::filesystem;

class Config {
    static fs::path customOutputDir;

public:
    static fs::path GetProjectRoot();

    static fs::path GetOutputDir();

    static fs::path GetDebugOutputDir();

    static void SetOutputDir(const fs::path &dir);

    static fs::path GetExecutableDir();
};

#endif //PHP_COMPILER_CONFIG_H
