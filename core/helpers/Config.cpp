#include "Config.h"

fs::path Config::customOutputDir;

fs::path Config::GetExecutableDir() {
    return fs::current_path();
}

fs::path Config::GetProjectRoot() {
    return fs::current_path();
}

fs::path Config::GetOutputDir() {
    if (!customOutputDir.empty()) {
        return customOutputDir;
    }

    return GetExecutableDir() / "output";
}

fs::path Config::GetDebugOutputDir() {
    return GetOutputDir() / "debug";
}

void Config::SetOutputDir(const fs::path &dir) {
    customOutputDir = fs::absolute(dir);
}
