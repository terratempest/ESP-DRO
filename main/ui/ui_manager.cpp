#include "ui_manager.h"
#include "config.h"
#include <driver/gpio.h>
#include <cstdio>
#include "lv_helpers/LVHelpers.h"
#include "screens/mainScreen.h"
#include "settings/settingsMenu.h" 

UIManager::UIManager(ToolManager& tm, DroAxis* axes, PreferencesWrapper& p)
    : toolManager(tm), axes(axes), xAxis(axes[0]), zAxis(axes[1]), prefs(p)
{}

// ---- UI Initialization ----
void UIManager::init() {
    lv_obj_clean(lv_scr_act());
    loadUnitSettings();

    buildMainScreen(ui);

    // ---- Event Hookups ----

    // DRO click (manual entry)
    ui.axis1DRO->onClick([this]{ showNumericInputPopup(0); });
    ui.axis2DRO->onClick([this]{ showNumericInputPopup(1); });

    // Zero buttons
    ui.axis1Zero->onClick([this]{
        toolManager.getTool(currentToolIndex).x_offset = xAxis.getPositionUm() / 1000.0f;
        toolManager.saveTools();
        updateDisplay();
    });
    ui.axis2Zero->onClick([this]{
        toolManager.getTool(currentToolIndex).z_offset = zAxis.getPositionUm() / 1000.0f;
        toolManager.saveTools();
        updateDisplay();
    });

    // Tool dropdown
    ui.toolDropdown->onChanged([this](uint16_t idx, std::string){
        loadOffsetsFromTool(idx);
        updateDisplay();
    });

    // Tool buttons
    ui.toolBtnEdit->onClick([this]{ showToolEditPopup(); });
    ui.toolBtnAdd->onClick([this]{
        char name[32];
        snprintf(name, sizeof(name), "Tool %zu", toolManager.toolCount() + 1);
        toolManager.addTool(name, xAxis.getPositionUm() / 1000.0f, zAxis.getPositionUm() / 1000.0f);
        currentToolIndex = toolManager.toolCount() - 1;
        updateToolDropdown();
        updateDisplay();
    });
    ui.toolBtnRemove->onClick([this]{
        if (toolManager.toolCount() <= 1) return;
        toolManager.removeTool(currentToolIndex);
        if (currentToolIndex >= (int)toolManager.toolCount())
            currentToolIndex = toolManager.toolCount() - 1;
        updateToolDropdown();
        updateDisplay();
    });

    // Function buttons
    ui.funcBtnUnits->onClick([this]{
        isMetric = !isMetric;
        saveUnitSettings();
        updateDisplay();
    });
    ui.funcBtnDia->onClick([this]{
        isDiameterMode = !isDiameterMode;
        saveUnitSettings();
        updateDisplay();
    });
    ui.funcBtnSettings->onClick([this]{
        showSettingsPopup();
    });

    // Power button (sleep)
    ui.sleepBtn->onClick([this]{ goToSleep(); });

    updateToolDropdown();
    loadOffsetsFromTool(currentToolIndex);
    updateDisplay();
}

// --- Display and Logic ---

void UIManager::updateDisplay() {
    // Calculate axis values
    float x_offset = toolManager.getTool(currentToolIndex).x_offset;
    float z_offset = toolManager.getTool(currentToolIndex).z_offset;
    float x = xAxis.getPositionUm() / 1000.0f - x_offset;
    float z = zAxis.getPositionUm() / 1000.0f - z_offset;
    if (!isMetric) { x *= 0.0393701f; z *= 0.0393701f; }
    if (isDiameterMode) x *= 2.0f;

    char buf[32];
    snprintf(buf, sizeof(buf), "%.4f", x);
    ui.axis1DRO->setText(buf);
    snprintf(buf, sizeof(buf), "%.4f", z);
    ui.axis2DRO->setText(buf);

    // Button texts
    ui.funcBtnUnits->setText(isMetric ? "mm" : "in");
    ui.funcBtnDia->setText(isDiameterMode ? "DIA" : "RAD");

    // Dropdown selection
    ui.toolDropdown->setSelected(currentToolIndex);
}
void UIManager::updateToolDropdown() {
    std::vector<std::string> options;
    for (size_t i = 0; i < toolManager.toolCount(); ++i)
        options.push_back(toolManager.getTool(i).name);
    ui.toolDropdown->setOptions(options);
    ui.toolDropdown->setSelected(currentToolIndex);
}
void UIManager::loadOffsetsFromTool(uint16_t index) {
    if (index < toolManager.toolCount())
        currentToolIndex = index;
}
void UIManager::onSetDroValue(int axis, float userValue) {
    Tool& tool = toolManager.getTool(currentToolIndex);
    if (axis == 0) {
        float pos_x = xAxis.getPositionUm() / 1000.0f;
        float val = userValue;
        if (!isMetric) val = val / 0.0393701f;
        if (isDiameterMode) val = val / 2.0f;
        tool.x_offset = pos_x - val;
    } else {
        float pos_z = zAxis.getPositionUm() / 1000.0f;
        float val = userValue;
        if (!isMetric) val = val / 0.0393701f;
        tool.z_offset = pos_z - val;
    }
    toolManager.saveTools();
    updateDisplay();
}

// --- Popup Windows ---

void UIManager::showNumericInputPopup(int axis) {
    // Overlay
    auto* overlay = new LVContainer(lv_scr_act(), [](auto& p){
        p.setSize(LCD_H_RES, LCD_V_RES);
        p.setTransparentBg();
        p.setFlag(LV_OBJ_FLAG_CLICKABLE);
        p.setPadAll(0);
        
    });

    auto* rightCont = new LVContainer(overlay->obj, [&](auto& r) {
        r.setSize(LCD_H_RES*3/7, LCD_V_RES);
        r.setAlign(LV_ALIGN_TOP_RIGHT);
        r.setPadAll(16);
    });

    // Centered panel
    auto* panel = new LVPanel(rightCont->obj, [&](auto& p){
        p.setSizePercent(100, 100);
        p.setAlign(LV_ALIGN_CENTER);
        p.setBgColor(lv_color_black());
        p.setBgOpa(LV_OPA_COVER);
        p.setBorderColor(lv_color_white());
        p.setBorderWidth(3);
        p.setRadius(10);
        p.setPadAll(0);
    });

    // Title
    auto* title = new LVContainer(panel->obj, [](auto& p){
        p.setSizePercent(100, 10);
        p.setPadAll(8);
    });
    
    new LVLabel(title->obj, "Set DRO Value", [](auto& l){
        l.setAlign(LV_ALIGN_CENTER);
        l.setTextColor(lv_color_white());
    });
    // Close button (top right)
    auto* closeBtn = new LVButton(title->obj, LV_SYMBOL_CLOSE, [&](auto& b){
        b.setSize(30, 30);
        b.setAlign(LV_ALIGN_RIGHT_MID);
        b.setBgColor(COLOR_BUTTON);
        b.onClick([=]{ overlay->del(); });
    });

    // Textarea for numeric input
    auto* ta = new LVTextarea(panel->obj, "", [&](auto& t){
        t.setSizePercent(100, 10);
        t.setOneLine(true);
        t.setRadius(0);
        t.setAlign(LV_ALIGN_TOP_MID, 0, title->getHeight());
    });

    // Numeric keyboard
    auto* kb = new LVKeyboardNumeric(panel->obj, [&](auto& k){
        k.setSizePercent(100,80);
        k.setAlign(LV_ALIGN_BOTTOM_MID, 0, 0);
        k.setTextarea(ta->obj);
        k.onEvent(LV_EVENT_READY, [=]{
            float val = strtof(ta->getText().c_str(), nullptr);
            this->onSetDroValue(axis, val);
            overlay->del();
        });
        k.onEvent(LV_EVENT_CANCEL, [=]{ overlay->del(); });
    });

    // Optionally: focus textarea for immediate input
    ta->focus();
}
void UIManager::showToolEditPopup() {
    int sel = ui.toolDropdown->getSelected();
    std::string oldName = toolManager.getTool(sel).name;

    // Full-screen panel overlay (semi-transparent)
    auto* overlay = new LVContainer(lv_scr_act(), [&](auto& p) {
        p.setSize(LCD_H_RES, LCD_V_RES);
        p.setBgColor(COLOR_BLACK);
        p.setBgOpa(LV_OPA_50);
        p.setAlign(LV_ALIGN_CENTER);
    });

    // Top Frame
    auto* topFrame = new LVContainer(overlay->obj, [&](auto& f){
        f.setSize(LCD_H_RES, LCD_V_RES*2/3);
        f.setAlign(LV_ALIGN_TOP_LEFT);
    });

    // Centered panel
    auto* panel = new LVPanel(topFrame->obj, [&](auto& p){
        p.setSize(320, 200);
        p.setBgColor(COLOR_BLACK);
        p.setRadius(10);
        p.setPadAll(0);
        p.setAlign(LV_ALIGN_CENTER);
        p.setScrollable(false);
    });

    // Title
    new LVLabel(panel->obj, "Rename Tool", [&](auto& l){
        l.setAlign(LV_ALIGN_TOP_MID, 0, 8);
    });

    // Textarea for editing name
    auto* ta = new LVTextarea(panel->obj, oldName, [&](auto& t){
        t.setWidthPercent(70);
        t.setHeight(40);
        t.setAlign(LV_ALIGN_CENTER, 0, 0);
        t.setRadius(0);
        t.setOneLine(true);
        t.focus();
    });

    // On-screen keyboard
    auto* kb = new LVKeyboard(overlay->obj, [&](auto& k){
        k.setTextarea(ta->obj);
        k.setWidthPercent(100);
        k.setHeight(LCD_V_RES/3);
        k.setAlign(LV_ALIGN_BOTTOM_MID, 0, 0);
        k.onEvent(LV_EVENT_READY, [=]{
            std::string newName = ta->getText();
            toolManager.renameTool(sel, newName);
            updateToolDropdown();
            updateDisplay();
            overlay->del();
        });
        k.onEvent(LV_EVENT_CANCEL, [=]{
            overlay->del();
        });
    });

    // Cancel Button
    auto* cancelBtn = new LVButton(panel->obj, "Cancel", [&](auto& b){
        b.setSize(80, 32);
        b.setAlign(LV_ALIGN_BOTTOM_RIGHT, -24, -12);
        b.onClick([=]{ overlay->del(); });
        b.setBgColor(COLOR_GREY);
    });
}

void UIManager::showSettingsPopup() {
    // Overlay (modal background)
    auto* overlay = new LVContainer(lv_scr_act(), [](auto& p){
        p.setSize(LCD_H_RES, LCD_V_RES);
        p.setTransparentBg();
        p.setFlag(LV_OBJ_FLAG_CLICKABLE);
    });

    // Centered panel
    auto* panel = new LVPanel(overlay->obj, [&](auto& p){
        p.setSize(LCD_H_RES-60, LCD_V_RES-30);
        p.setAlign(LV_ALIGN_CENTER);
        p.setBgColor(COLOR_BLACK);
        p.setRadius(12);
        p.setPadAll(16);
    });

    // Title
    auto* tabsView = new LVContainer(panel->obj, [](auto& l){
        l.setAlign(LV_ALIGN_TOP_LEFT);
        l.setHeightPercent(100);
        l.setWidthPercent(30);
        l.setPadAll(8);
        l.setBgColor(COLOR_GRAY);
        new LVLabel(l.obj, "Settings", [](auto& l){ l.setAlign(LV_ALIGN_LEFT_MID); });
    });

    // Settings fields container (right side)
    auto* settingsContainer = new LVFlexContainer(panel->obj, [&](auto& f){
        f.removeStyleAll();
        f.setHeightPercent(100);
        f.setWidthPercent(70);
        f.setAlign(LV_ALIGN_TOP_RIGHT);
        f.setFlexFlow(LV_FLEX_FLOW_COLUMN);
        f.setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        f.setPadRow(8);
        f.setScrollable();
    });

    // Bookkeeping for input fields
    std::vector<LVTextarea*> numInputs;
    std::vector<LVSwitch*> boolInputs;

    auto settingsTable = makeSettingsTable(prefs, axes, AXES_COUNT);

    // Loop through settingsTable, add UI fields
    for (size_t i = 0; i < settingsTable.size(); ++i) {
        const auto& s = settingsTable[i];
        new LVContainer(settingsContainer->obj, [&](auto& l){
            l.setWidthPercent(80);
            l.setHeight(32);
            new LVLabel(l.obj, s.name, [](auto& l){ l.setAlign(LV_ALIGN_LEFT_MID); });

            if (s.type == SettingType::Number) {
                float val = prefs.getFloat(s.key, s.defaultNumber);
                char buf[16];
                snprintf(buf, sizeof(buf), "%.3f", val);
                auto* ta = new LVTextarea(l.obj, buf, [](auto& t){
                    t.setSize(100, 32);
                    t.setAlign(LV_ALIGN_RIGHT_MID);
                    t.setOneLine();
                });
                numInputs.push_back(ta);
            } else if (s.type == SettingType::Bool) {
                bool val = prefs.getBool(s.key, s.defaultBool);
                auto* sw = new LVSwitch(l.obj, [&](auto& t){
                    t.setChecked(val);
                    t.setSize(75, 32);
                    t.setAlign(LV_ALIGN_RIGHT_MID);
                });
                boolInputs.push_back(sw);
            }
        });
    }

    // Shared numeric keyboard
    auto* kb = new LVKeyboardNumeric(overlay->obj, [&](auto& k){
        k.setSize(LCD_H_RES, LCD_V_RES/3);
        k.setAlign(LV_ALIGN_BOTTOM_MID, 0, 0);
        k.setVisible(false);
        k.onEvent(LV_EVENT_READY,  [&k]{ k.setVisible(false); });
        k.onEvent(LV_EVENT_CANCEL, [&k]{ k.setVisible(false); });
    });

    for (auto* ta : numInputs) {
        ta->onEvent(LV_EVENT_FOCUSED, [=]{
            kb->setTextarea(ta->obj);
            kb->setVisible(true);
        });
    }

    // Save button (writes all values to prefs, calls onChange if set)
    new LVButton(panel->obj, "Save", [&](auto& b){
        b.setSize(90, 36);
        b.setAlign(LV_ALIGN_BOTTOM_LEFT, 158, 0);
        b.setBgColor(COLOR_GRAY);
        b.onClick([=, this]{
            int numIdx = 0, boolIdx = 0;
            for (size_t i = 0; i < settingsTable.size(); ++i) {
                const auto& s = settingsTable[i];
                if (s.type == SettingType::Number && numIdx < (int)numInputs.size()) {
                    float v = strtof(numInputs[numIdx++]->getText().c_str(), nullptr);
                    prefs.putFloat(s.key, v);
                    if (s.onChange) s.onChange();
                } else if (s.type == SettingType::Bool && boolIdx < (int)boolInputs.size()) {
                    bool v = boolInputs[boolIdx++]->isChecked();
                    prefs.putBool(s.key, v);
                    if (s.onChange) s.onChange();
                }
            }
            overlay->del();
            updateDisplay();
        });
    });

    // Cancel button (just closes the menu, does not save)
    new LVButton(panel->obj, "Cancel", [&](auto& b){
        b.setSize(90, 36);
        b.setAlign(LV_ALIGN_BOTTOM_RIGHT, 0, 0);
        b.setBgColor(COLOR_GRAY);
        b.onClick([=]{ overlay->del(); });
    });
}

// --- Utility, Sleep, and Persistence as before ---

void UIManager::saveUnitSettings() { 
    prefs.putUShort("is_metric", isMetric ? 1 : 0); 
    prefs.putUShort("is_diameter", isDiameterMode ? 1 : 0);
    prefs.putBool("x_invert", invertXAxis);
    prefs.putBool("z_invert", invertZAxis);
}
void UIManager::loadUnitSettings() { 
    isMetric = prefs.getUShort("is_metric", 1) != 0; 
    isDiameterMode = prefs.getUShort("is_diameter", 0) != 0;
    invertXAxis = prefs.getBool("x_invert");
    invertZAxis = prefs.getBool("z_invert"); 
}
void UIManager::goToSleep() {
    gpio_set_level(LCD_PIN_BK_LIGHT, 0);
    auto* wake_layer = new LVPanel(lv_scr_act(), [](auto& p){
        p.setSize(LCD_H_RES, LCD_V_RES);
        p.setTransparentBg();
        p.clearFlag(LV_OBJ_FLAG_SCROLLABLE);
        p.setFlag(LV_OBJ_FLAG_CLICKABLE);
    });
    wake_layer->onClick([=]{
        gpio_set_level(LCD_PIN_BK_LIGHT, 1);
        wake_layer->del();
    });
}