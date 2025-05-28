#pragma once
#include <string>
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
    DroAxis& xAxis;
    DroAxis& zAxis;
    PreferencesWrapper& prefs;
    MainScreenPtrs ui;

    // UI state
    bool isMetric           = true;
    bool isDiameterMode     = false;
    int  currentToolIndex   = 0;
    int  numericInputAxis   = 0; // 0=X, 1=Z
    bool invertXAxis        = false;
    bool invertZAxis        = false;

    void showNumericInputPopup(int axis); // 0=X, 1=Z
    void onSetDroValue(int axis, float userValue);
    void updateToolDropdown();
    void loadOffsetsFromTool(uint16_t index);
    void saveUnitSettings();
    void loadUnitSettings();
    void goToSleep();
    void showSettingsPopup();
    void showToolEditPopup();

};
