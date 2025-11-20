#include "BaseNode.h"
#include "core/helpers/OutputManager.h"
#include "core/helpers/Color.h"
#include "core/helpers/Console.h"

#include <filesystem>

namespace fs = std::filesystem;

uint32_t BaseNode::maxId = 0;

uint32_t BaseNode::GetId() const { return id; }

void BaseNode::WriteToJsonFile() const {
    Console::Log(Color::Grey() + "%NODE% Updating '" + std::to_string(this->GetId()) + "'");
    try {
        OutputManager::OutputJson(this->toJson(), std::to_string(this->GetId()));
        OutputManager::OutputDot(this->toDot(), std::to_string(this->GetId()));
    } catch (const std::exception &e) {
        Console::SystemError("Failed to generate output files: " + std::string(e.what()));
    }
}
