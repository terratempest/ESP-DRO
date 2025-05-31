#include "tool_manager.h"
#include <cstdio>
#include <algorithm>

ToolManager::ToolManager(PreferencesWrapper& prefs_ref)
    : prefs(prefs_ref)
{
    loadTools();
}

bool ToolManager::addTool(const std::string& name, const std::vector<float>& offsets) {
    if (tools.size() >= MAX_TOOLS) return false;
    tools.push_back(Tool{name, offsets});
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
    char key[64];

    // Save name
    snprintf(key, sizeof(key), "tool_name_%u", static_cast<unsigned>(index));
    prefs.putString(key, tools[index].name.c_str());

    // Save offsets for each axis
    const auto& offsets = tools[index].offsets;
    for (size_t ax = 0; ax < offsets.size(); ++ax) {
        snprintf(key, sizeof(key), "tool_axis%zu_%u", ax, static_cast<unsigned>(index));
        prefs.putFloat(key, offsets[ax]);
    }
}

void ToolManager::saveTools() const {
    saveToolCount();
    for (size_t i = 0; i < tools.size(); ++i) {
        saveTool(i);
    }
}

void ToolManager::loadTool(size_t index) {
    char key[64];
    snprintf(key, sizeof(key), "tool_name_%u", static_cast<unsigned>(index));
    std::string name = prefs.getString(key, ("Tool " + std::to_string(index + 1)).c_str()).c_str();

    // Figure out how many axes (assume AXES_COUNT is visible)
    std::vector<float> offsets;
    for (size_t ax = 0; ax < AXES_COUNT; ++ax) {
        snprintf(key, sizeof(key), "tool_axis%zu_%u", ax, static_cast<unsigned>(index));
        float val = prefs.getFloat(key, 0.0f);
        offsets.push_back(val);
    }

    if (index < tools.size()) {
        tools[index] = Tool{name, offsets};
    } else {
        tools.push_back(Tool{name, offsets});
    }
}

void ToolManager::loadTools() {
    tools.clear();
    size_t count = prefs.getUShort("tool_count", 0);
    if (count == 0) {
        // Default tool: all offsets zero for all axes
        std::vector<float> zero_offsets(AXES_COUNT, 0.0f);
        addTool("Tool 1", zero_offsets);
        count = 1;
    }
    for (size_t i = 0; i < count; ++i) {
        loadTool(i);
    }
}
