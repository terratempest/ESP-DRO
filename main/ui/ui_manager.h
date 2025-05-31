#pragma once
#include <string>
#include <vector>
#include "tool_manager.h"
#include "dro_axis.h"
#include "preferences_wrapper.h"
#include "screens/mainScreen.h"

extern "C" {
    #include <lvgl.h>
}

class UIManager {
public:
    UIManager(ToolManager& toolManager, DroAxis* axes, PreferencesWrapper& prefs);

    void init();
    void updateDisplay();

private:
    ToolManager& toolManager;
    DroAxis* axes;
    PreferencesWrapper& prefs;
    MainScreenPtrs ui;

    // UI state
    bool isMetric           = true;
    bool isDiameterMode     = false;
    bool isIncremental      = false;
    int  currentToolIndex   = 0;

    std::vector<float> incrOffsets;

    // ---- Methods ----

    // Update all UI displays (DROs, labels, etc.)
    void updateToolDropdown();
    void loadOffsetsFromTool(uint16_t index);
    void saveUnitSettings();
    void loadUnitSettings();
    void goToSleep();
    void loadGlobalReferences();
    void saveGlobalReferences();
    void setGlobalReference(int axisIndex, float desiredDroValue);
    void setToolOffsetValue(int axisIndex, float userValue);
    void setIncrementalReference(int axisIndex, float desiredDroValue);
};
