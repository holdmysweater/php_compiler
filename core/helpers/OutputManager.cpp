#include "OutputManager.h"
#include "Config.h"
#include "Console.h"

#include <fstream>

fs::path OutputManager::EnsureOutputDir() {
    fs::path outputDir = Config::GetOutputDir();
    fs::create_directories(outputDir);
    return outputDir;
}

void OutputManager::GenerateSvgFromDot(const fs::path &dotFilePath) {
    GenerateSvgFromDot(dotFilePath, false);
}

void OutputManager::GenerateSvgFromDot(const fs::path &dotFilePath, bool isLogEnabled) {
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
    OutputJson(jsonContent, baseName, false);
}

void OutputManager::OutputJson(const std::string &jsonContent, const std::string &baseName, bool isLogEnabled) {
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
    OutputDot(dotContent, baseName, false);
}

void OutputManager::OutputDot(const std::string &dotContent, const std::string &baseName, bool isLogEnabled) {
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
