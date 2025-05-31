#pragma once

#include "tool_manager.h" // Or whatever header defines ToolManager
#include <functional>
#include <string>

class ToolEditPopup {
public:
    // Pops up a tool rename dialog
    // onDone: called after rename (success or cancel), for UI refresh
    static void show(
        ToolManager& toolManager,
        int toolIndex,
        std::function<void()> onDone
    );
};
