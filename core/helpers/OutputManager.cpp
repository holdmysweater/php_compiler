#include "OutputManager.h"
#include "Config.h"
#include "Console.h"

#include <fstream>

fs::path OutputManager::EnsureOutputDir() {
    fs::path outputDir = Config::GetOutputDir();
    fs::create_directories(outputDir);
    return outputDir;
}

fs::path OutputManager::EnsureDotOutputDir() {
    fs::path dotDir = Config::GetDotOutputDir();
    fs::create_directories(dotDir);
    return dotDir;
}

void OutputManager::GenerateSvgFromDot(const fs::path &dotFilePath) {
    fs::path svgFile = fs::path(dotFilePath.string() + ".svg");
    std::string svgCommand = "dot -Tsvg \"" + dotFilePath.string() + "\" -o \"" + svgFile.string() + "\"";

    int result = system(svgCommand.c_str());

    if (result == 0) {
        Console::SystemLog("SVG generated: '" + svgFile.string() + "'");
    } else {
        Console::SystemError("Failed to generate SVG (make sure Graphviz is installed and in PATH)");
    }
}

void OutputManager::OutputJson(const std::string &jsonContent, const std::string &baseName) {
    fs::path outputDir = EnsureOutputDir();
    fs::path jsonPath = outputDir / (baseName + "_ast.json");

    std::ofstream jsonFile(jsonPath);
    if (jsonFile.is_open()) {
        jsonFile << jsonContent;
        jsonFile.close();
        Console::SystemLog("JSON AST written to: '" + jsonPath.string() + "'");
    } else {
        Console::SystemError("Could not write JSON file to '" + jsonPath.string() + "'");
    }
}

void OutputManager::OutputDot(const std::string &dotContent, const std::string &baseName) {
    fs::path dotDir = EnsureDotOutputDir();
    fs::path dotPath = dotDir / (baseName + "_tree.dot");

    std::ofstream dotFile(dotPath);
    if (dotFile.is_open()) {
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
        Console::SystemLog("DOT visualization written to: '" + dotPath.string() + "'");

        GenerateSvgFromDot(dotPath);
    } else {
        Console::SystemError("Could not write DOT file to '" + dotPath.string() + "'");
    }
}
