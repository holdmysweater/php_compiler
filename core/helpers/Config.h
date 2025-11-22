#ifndef PHP_COMPILER_CONFIG_H
#define PHP_COMPILER_CONFIG_H

#include <filesystem>

namespace fs = std::filesystem;

class Config {
    static fs::path customOutputDir;

public:
    static fs::path GetExecutableDir();

    static fs::path GetOutputDir();

    static void SetOutputDir(const fs::path &dir);
};

#endif //PHP_COMPILER_CONFIG_H
