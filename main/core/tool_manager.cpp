#include "tool_manager.h"
#include <cstdio> 
#include <algorithm>

ToolManager::ToolManager(PreferencesWrapper& prefs_ref)
    : prefs(prefs_ref)
{
    loadTools();
}

bool ToolManager::addTool(const std::string& name, float x_offset, float z_offset) {
    if (tools.size() >= MAX_TOOLS) return false;
    tools.push_back(Tool{name, x_offset, z_offset});
    saveTools();
    return true;
}

bool ToolManager::removeTool(size_t index) {
    if (tools.size() <= 1) return false; // Don't remove last tool
    if (index >= tools.size()) return false;
    tools.erase(tools.begin() + index);
    saveTools();
    return true;
}

bool ToolManager::renameTool(size_t index, const std::string& newName) {
    if (index >= tools.size()) return false;
    tools[index].name = newName;
    saveTools();
    return true;
}

Tool& ToolManager::getTool(size_t index) {
    return tools.at(index);
}

const Tool& ToolManager::getTool(size_t index) const {
    return tools.at(index);
}

size_t ToolManager::toolCount() const {
    return tools.size();
}

void ToolManager::saveToolCount() const {
    prefs.putUShort("tool_count", tools.size());
}

void ToolManager::loadToolCount() {
    // Not strictly needed; handled in loadTools
}

void ToolManager::saveTool(size_t index) const {
    if (index >= tools.size()) return;
    char key[32];
    snprintf(key, sizeof(key), "tool_name_%u", static_cast<unsigned>(index));
    prefs.putString(key, tools[index].name.c_str());

    snprintf(key, sizeof(key), "tool_x_%u", static_cast<unsigned>(index));
    prefs.putFloat(key, tools[index].x_offset);

    snprintf(key, sizeof(key), "tool_z_%u", static_cast<unsigned>(index));
    prefs.putFloat(key, tools[index].z_offset);
}

void ToolManager::saveTools() const {
    saveToolCount();
    for (size_t i = 0; i < tools.size(); ++i) {
        saveTool(i);
    }
}

void ToolManager::loadTool(size_t index) {
    char key[32];
    snprintf(key, sizeof(key), "tool_name_%u", static_cast<unsigned>(index));
    std::string name = prefs.getString(key, ("Tool " + std::to_string(index + 1)).c_str()).c_str();

    snprintf(key, sizeof(key), "tool_x_%u", static_cast<unsigned>(index));
    float x = prefs.getFloat(key, 0);

    snprintf(key, sizeof(key), "tool_z_%u", static_cast<unsigned>(index));
    float z = prefs.getFloat(key, 0);

    if (index < tools.size()) {
        tools[index] = Tool{name, x, z};
    } else {
        tools.push_back(Tool{name, x, z});
    }
}

void ToolManager::loadTools() {
    tools.clear();
    size_t count = prefs.getUShort("tool_count", 0);
    if (count == 0) {
        addTool("Tool 1", 0.0f, 0.0f);
        count = 1;
    }
    //if (count > MAX_TOOLS) count = MAX_TOOLS;
    for (size_t i = 0; i < count; ++i) {
        loadTool(i);
    }
}
