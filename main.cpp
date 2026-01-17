#include <iostream>
#include <fstream>
#include <filesystem>

#include "cmake-build-debug/generated/php_parser.hpp"

#include "core/nodes/ElementNode.h"
#include "core/helpers/Config.h"
#include "core/helpers/OutputManager.h"
#include "core/bytecode/ByteCodeHelper.h"

namespace fs = std::filesystem;

extern FILE *yyin;

extern int yyparse();

ElementNode *root;

int main(int argc, char *argv[]) {
    Console::SystemTitle("PHP TRANSLATOR");
    Console::ShowSettings();

    if (argc < 4) {
        Console::SystemError(
            "Not enough arguments!\n"
            "Usage: '" + std::string(argv[0]) + " <php_file> --output <dir>'\n"
            "Example: php_compiler.exe script.php --output ./output");
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputDir;
    bool outputFlagFound = false;

    for (int i = 2; i < argc - 1; i++) {
        if (std::string(argv[i]) == "--output" || std::string(argv[i]) == "-o") {
            outputDir = argv[i + 1];
            outputFlagFound = true;
            break;
        }
    }

    if (!outputFlagFound) {
        Console::SystemError(
            "Missing required --output flag!\n"
            "Usage: '" + std::string(argv[0]) + " <php_file> --output <dir>'\n"
            "Example: php_compiler.exe script.php --output ./output");
        return 1;
    }

    if (outputDir.empty()) {
        Console::SystemError("Output directory path cannot be empty!");
        return 1;
    }

    try {
        Config::SetOutputDir(outputDir);
    } catch (const std::exception &e) {
        Console::SystemError("Failed to set output directory: " + std::string(e.what()));
        return 1;
    }

    yyin = fopen(inputFile.c_str(), "r");
    if (!yyin) {
        Console::SystemError("Could not open input file: '" + inputFile + "'");
        return 1;
    }

    Console::SystemLog("Input file: '" + inputFile + "'");
    Console::SystemLog("Output directory: '" + Config::GetOutputDir().string() + "'");

    Console::SystemTitle("Parser initiation...");

    Config::SetOutputDir(outputDir + "/parser/debug");

    int parse_result = yyparse();

    fclose(yyin);

    if (parse_result != 0) {
        Console::SystemError("Parsing failed with code: '" + std::to_string(parse_result) + "'");
        return 1;
    }

    if (!root) {
        Console::SystemError("No parse tree generated");
        return 1;
    }

    Console::SystemLog("Parse successful!");
    Console::SystemLog("Total nodes created: " + std::to_string(root->GetId()));

    fs::path inputPath(inputFile);
    std::string baseName = inputPath.stem().string();

    Console::SystemLog("Generating parser output files...");

    try {
        Config::SetOutputDir(outputDir + "/parser");
        OutputManager::OutputJson(root->toJson(), "parser_" + baseName, true);
        OutputManager::OutputDot(root->toDot(), "parser_" + baseName, true);
    } catch (const std::exception &e) {
        Console::SystemError("Failed to generate output files: " + std::string(e.what()));
        return 1;
    }

    Console::SystemTitle("Semantics initiation...");

    Config::SetOutputDir(outputDir + "/semantics/debug");

    bool result = root->doSemantics();

    Console::SystemLog("Generating semantics output files...");

    try {
        Config::SetOutputDir(outputDir + "/semantics");
        OutputManager::OutputJson(root->toJson(), "semantics_" + baseName, true);
        OutputManager::OutputDot(root->toDot(), "semantics_" + baseName, true);
    } catch (const std::exception &e) {
        Console::SystemError("Failed to generate output files: " + std::string(e.what()));
        return 1;
    }

    if (!result) {
        Console::SystemError("Tree has errors, semantics were incomplete");
        return 1;
    }

    Console::SystemTitle("Bytecode initiation...");

    Config::SetOutputDir(outputDir + "/bytecode/");

    ByteCodeHelper::GenerateAndExecute(root, baseName);

    Console::SystemTitle("COMPLETED");

    return 0;
}
