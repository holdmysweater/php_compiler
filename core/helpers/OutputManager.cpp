#include "OutputManager.h"
#include "Config.h"
#include "Console.h"

#include <fstream>
#include "../bytecode/assets/runtime_all.h"


struct EmbeddedClassFile {
    const char *relPath;
    const unsigned char *bytes;
    unsigned int len;
};

static const EmbeddedClassFile kPhpRuntimeFiles[] = {
    {"com/phpjvm/BasePhpValue.class", BasePhpValue_class, BasePhpValue_class_len},
    {"com/phpjvm/BasePhpValue$PhpArray.class", BasePhpValue_PhpArray_class, BasePhpValue_PhpArray_class_len},
    {"com/phpjvm/BasePhpValue$PhpKey$K.class", BasePhpValue_PhpKey_K_class, BasePhpValue_PhpKey_K_class_len},
    {"com/phpjvm/BasePhpValue$PhpKey.class", BasePhpValue_PhpKey_class, BasePhpValue_PhpKey_class_len},
    {"com/phpjvm/BasePhpValue$PhpNumber.class", BasePhpValue_PhpNumber_class, BasePhpValue_PhpNumber_class_len},
    {
        "com/phpjvm/BasePhpValue$PhpRuntimeException.class",
        BasePhpValue_PhpRuntimeException_class,
        BasePhpValue_PhpRuntimeException_class_len
    },
    {
        "com/phpjvm/BasePhpValue$PhpTypeError.class",
        BasePhpValue_PhpTypeError_class,
        BasePhpValue_PhpTypeError_class_len
    },
    {"com/phpjvm/BasePhpValue$Type.class", BasePhpValue_Type_class, BasePhpValue_Type_class_len},
    {"com/phpjvm/PhpClass.class", PhpClass_class, PhpClass_class_len},
    {"com/phpjvm/PhpMethod.class", PhpMethod_class, PhpMethod_class_len},
    {"com/phpjvm/PhpObject.class", PhpObject_class, PhpObject_class_len},
    {"com/phpjvm/PhpRuntime.class", PhpRuntime_class, PhpRuntime_class_len},
    {"com/phpjvm/PhpStaticMethod.class", PhpStaticMethod_class, PhpStaticMethod_class_len},
};
static const std::size_t kPhpRuntimeFilesCount = sizeof(kPhpRuntimeFiles) / sizeof(kPhpRuntimeFiles[0]);


fs::path OutputManager::EnsureOutputDir() {
    fs::path outputDir = Config::GetOutputDir();
    fs::create_directories(outputDir);
    return outputDir;
}

void OutputManager::GenerateSvgFromDot(const fs::path &dotFilePath) {
#ifdef DISABLE_ALL_LOGS
    return;
#endif

    GenerateSvgFromDot(dotFilePath, false);
}

void OutputManager::GenerateSvgFromDot(const fs::path &dotFilePath, bool isLogEnabled) {
#ifdef DISABLE_ALL_LOGS
    return;
#endif

    fs::path svgFile = fs::path(dotFilePath.string() + ".svg");
    std::string svgCommand = "dot -Tsvg \"" + dotFilePath.string() + "\" -o \"" + svgFile.string() + "\"";

    int result = system(svgCommand.c_str());

    if (result == 0) {
        if (isLogEnabled) {
            Console::SystemLog("SVG generated: '" + svgFile.string() + "'");
        }
    } else {
        Console::SystemError("Failed to generate SVG (make sure Graphviz is installed and in PATH)");
    }
}

void OutputManager::OutputJson(const std::string &jsonContent, const std::string &baseName) {
#ifdef DISABLE_ALL_LOGS
    return;
#endif

    OutputJson(jsonContent, baseName, false);
}

void OutputManager::OutputJson(const std::string &jsonContent, const std::string &baseName, bool isLogEnabled) {
#ifdef DISABLE_ALL_LOGS
    return;
#endif
    fs::path jsonDir = EnsureOutputDir();
    fs::path jsonPath = jsonDir / (baseName + "_ast.json");

    if (std::ofstream jsonFile(jsonPath); jsonFile.is_open()) {
        jsonFile << jsonContent;
        jsonFile.close();

        if (isLogEnabled) {
            Console::SystemLog("JSON AST written to: '" + jsonPath.string() + "'");
        }
    } else {
        Console::SystemError("Could not write JSON file to '" + jsonPath.string() + "'");
    }
}

void OutputManager::OutputDot(const std::string &dotContent, const std::string &baseName) {
#ifdef DISABLE_ALL_LOGS
    return;
#endif

    OutputDot(dotContent, baseName, false);
}

void OutputManager::OutputDot(const std::string &dotContent, const std::string &baseName, bool isLogEnabled) {
#ifdef DISABLE_ALL_LOGS
    return;
#endif

    fs::path dotDir = EnsureOutputDir();
    fs::path dotPath = dotDir / (baseName + "_tree.dot");

    if (std::ofstream dotFile(dotPath); dotFile.is_open()) {
        dotFile << "digraph {\n";
        dotFile << "  rankdir=TB;\n";
        dotFile << "  node [shape=box, fontname=\"Arial\", fontsize=10];\n";
        dotFile << "  edge [fontname=\"Arial\", fontsize=9];\n";
        dotFile << "\n";

        dotFile << "  subgraph cluster_legend {\n";
        dotFile << "    label=\"Legend\";\n";
        dotFile << "    style=filled;\n";
        dotFile << "    fillcolor=\"#F0F0F0\";\n";
        dotFile << "    color=\"black\";\n";
        dotFile << "    fontname=\"Arial\";\n";
        dotFile << "    fontsize=11;\n";
        dotFile << "    margin=5;\n";
        dotFile << "\n";
        dotFile << "    legend_prog [label=\"Prog\", fillcolor=\"lightgrey\", style=filled];\n";
        dotFile << "    legend_decl [label=\"Decl\", fillcolor=\"#FFD580\", style=filled];\n";
        dotFile << "    legend_expr [label=\"Expr\", fillcolor=\"#90EE90\", style=filled];\n";
        dotFile << "    legend_stmt [label=\"Stmt\", fillcolor=\"#ADD8E6\", style=filled];\n";
        dotFile << "    legend_value [label=\"Value\", fillcolor=\"#FFFFE0\", style=filled];\n";
        dotFile << "  }\n";
        dotFile << "\n";
        dotFile << dotContent;
        dotFile << "}\n";

        dotFile.close();

        if (isLogEnabled) {
            Console::SystemLog("DOT visualization written to: '" + dotPath.string() + "'");
        }

        GenerateSvgFromDot(dotPath, isLogEnabled);
    } else {
        Console::SystemError("Could not write DOT file to '" + dotPath.string() + "'");
    }
}

std::ofstream OutputManager::GetByteCodeFile(const std::string &baseName) {
    fs::path bytecodeDir = EnsureOutputDir();
    fs::path classFilePath = bytecodeDir / (baseName + ".class");
    std::ofstream file(classFilePath, std::ios::binary);
    return file;
}

static void WriteBytesToFile(const fs::path &path, const unsigned char *data, unsigned int len) {
    fs::create_directories(path.parent_path());

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        throw std::runtime_error("Could not open file for writing: " + path.string());
    }

    out.write(reinterpret_cast<const char *>(data), static_cast<std::streamsize>(len));
    out.flush();

    if (!out.good()) {
        throw std::runtime_error("Failed writing bytes to: " + path.string());
    }
}

void OutputManager::EnsureEmbeddedPhpRuntime() {
    fs::path outputDir = EnsureOutputDir();

    try {
        for (std::size_t i = 0; i < kPhpRuntimeFilesCount; i++) {
            const auto &f = kPhpRuntimeFiles[i];

            fs::path outPath = outputDir / fs::path(f.relPath);

            WriteBytesToFile(outPath, f.bytes, f.len);
        }
    } catch (const std::exception &e) {
        Console::SystemError(std::string("Failed to ensure runtime classes: ") + e.what());
        throw;
    }
}
