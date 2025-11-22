#include "BaseNode.h"
#include "core/helpers/OutputManager.h"
#include "core/helpers/Console.h"

#include <filesystem>

namespace fs = std::filesystem;

uint32_t BaseNode::maxId = 0;

uint32_t BaseNode::GetId() const { return id; }

void BaseNode::WriteToFiles() const {
    try {
        Log("updating debug files");
        OutputManager::OutputJson(this->toJson(), std::to_string(this->GetId()));
        OutputManager::OutputDot(this->toDot(), std::to_string(this->GetId()));
    } catch (const std::exception &e) {
        Console::SystemError("Failed to generate output files: " + std::string(e.what()));
    }
}

void BaseNode::Log(string message) const {
    Console::NodeLog(message, _getClassName(), GetId());
}

void BaseNode::Warn(string message) const {
    Console::NodeWarning(message, _getClassName(), GetId());
}

void BaseNode::Error(string message) const {
    Console::NodeError(message, _getClassName(), GetId());
}
