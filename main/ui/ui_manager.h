#pragma once
#include <string>
#include <vector>
#include <stdint.h>
#include "tool_manager.h"
#include "dro_axis.h"
#include "preferences_wrapper.h"
#include "screens/mainScreen.h"

class LVPanel;

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
    std::vector<std::string> lastAxisTexts;
    std::string lastUnitsText;
    std::string lastDiaText;
    std::string lastAbsIncrText;
    int lastDropdownSelection = -1;
    bool screenAwake = true;
    LVPanel* wakeLayer = nullptr;
    uint32_t lastActivityMs = 0;
    uint32_t lastSeenTouchMs = 0;
    std::vector<float> lastRawAxisPositions;

    // ---- Methods ----

    // Update all UI displays (DROs, labels, etc.)
    void updateToolDropdown();
    void loadOffsetsFromTool(uint16_t index);
    void showStatusMessage(const char* message);
    void saveUnitSettings();
    void loadUnitSettings();
    void goToSleep();
    void wakeScreen();
    void markActivity(uint32_t nowMs);
    void updateActivityState(uint32_t nowMs);
    bool isAutoScreenOffEnabled();
    uint32_t getAutoScreenOffTimeoutMs();
    void loadGlobalReferences();
    void saveGlobalReferences();
    float getDisplayedAxisValue(int axisIndex);
    void setGlobalReference(int axisIndex, float desiredDroValue);
    void setToolOffsetValue(int axisIndex, float userValue);
    void setIncrementalReference(int axisIndex, float desiredDroValue);
};
