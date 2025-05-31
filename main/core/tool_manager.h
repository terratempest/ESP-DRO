#pragma once
#include <vector>
#include <string>
#include <cstddef>
#include "tool.h"
#include "preferences_wrapper.h"
#include "config.h"

class ToolManager {
public:
    ToolManager(PreferencesWrapper& prefs);

    // Tool CRUD (now with vector offsets)
    bool addTool(const std::string& name, const std::vector<float>& offsets);
    bool removeTool(size_t index);
    bool renameTool(size_t index, const std::string& newName);

    // Accessors
    Tool& getTool(size_t index);
    const Tool& getTool(size_t index) const;
    size_t toolCount() const;

    // Persistence
    void saveTools() const;
    void loadTools();

private:
    std::vector<Tool> tools;
    PreferencesWrapper& prefs;
    static constexpr size_t MAX_TOOLS = 32;

    void saveTool(size_t index) const;
    void loadTool(size_t index);
    void saveToolCount() const;
    void loadToolCount();
};
