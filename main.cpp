#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <filesystem>

#include "cmake-build-debug/generated/php_parser.hpp"

#include "core/nodes/ElementNode.h"
#include "core/helpers/Config.h"
#include "core/helpers/OutputManager.h"

namespace fs = std::filesystem;

extern FILE *yyin;

extern int yyparse();

ElementNode *root;

int main(int argc, char *argv[]) {
    Console::SystemTitle("PHP LEXER & PARSER");

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
    Console::SystemLog("Parser initiation...");

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

    Console::SystemLog("Generating output files...");

    try {
        OutputManager::OutputJson(root->toJson(), baseName, true);
        OutputManager::OutputDot(root->toDot(), baseName, true);
    } catch (const std::exception &e) {
        Console::SystemError("Failed to generate output files: " + std::string(e.what()));
        return 1;
    }

    Console::SystemTitle("COMPLETED! :)");

    return 0;
}
