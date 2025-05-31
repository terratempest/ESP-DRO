#include "ui_manager.h"
#include "config.h"
#include <driver/gpio.h>
#include <cstdio>
#include "lv_helpers/LVHelpers.h"
#include "screens/mainScreen.h"

UIManager::UIManager(ToolManager& tm, DroAxis* axes, PreferencesWrapper& p)
    : toolManager(tm), axes(axes), prefs(p)
{
    currentToolIndex = 0;
}

void UIManager::init() {
    lv_obj_clean(lv_scr_act());
    loadUnitSettings();
    loadGlobalReferences();

    buildMainScreen(ui, axes, AXES_COUNT); // You need to ensure your buildMainScreen takes axis count and fills vectors for DROs/buttons

    // Attach per-axis event handlers
    for (int i = 0; i < AXES_COUNT; ++i) {
        // DRO click = set value popup
        ui.axisDROs[i]->onClick([this, i] {
            NumericInputPopup::show("Set DRO Value",
                [this, i](float value, bool cancelled) {
                    if (!cancelled) setGlobalReference(i, value);
                }
            );
        });

        // Zero button: sets axis to 0 at current pos
        ui.axisZeroBtns[i]->onClick([this, i] {
            setGlobalReference(i, 0.0f);
        });

        // Tool offset (TLO) button: set tool offset for this axis
        ui.axisTLOBtns[i]->onClick([this, i] {
            NumericInputPopup::show("Set Tool Offset",
                [this, i](float value, bool cancelled) {
                    if (!cancelled) setToolOffsetValue(i, value);
                }
            );
        });
    }

    // Tool dropdown
    ui.toolDropdown->onChanged([this](uint16_t idx, std::string) {
        loadOffsetsFromTool(idx);
        updateDisplay();
    });

    // Tool buttons
    ui.toolBtnEdit->onClick([this] {
        ToolEditPopup::show(toolManager, currentToolIndex, [this] {
            updateToolDropdown();
            updateDisplay();
        });
    });

    ui.toolBtnAdd->onClick([this] {
        char name[32];
        snprintf(name, sizeof(name), "Tool %zu", toolManager.toolCount() + 1);

        // Offset vector for N axes (default 0)
        std::vector<float> offsets(AXES_COUNT, 0.0f);
        for (int i = 0; i < AXES_COUNT; ++i)
            offsets[i] = axes[i].getPositionUm() / 1000.0f - axes[i].getGlobalReference();

        toolManager.addTool(name, offsets);
        currentToolIndex = toolManager.toolCount() - 1;
        updateToolDropdown();
        updateDisplay();
    });

    ui.toolBtnRemove->onClick([this] {
        if (toolManager.toolCount() <= 1) return;
        toolManager.removeTool(currentToolIndex);
        if (currentToolIndex >= (int)toolManager.toolCount())
            currentToolIndex = toolManager.toolCount() - 1;
        updateToolDropdown();
        updateDisplay();
    });

    // Function buttons
    ui.funcBtnUnits->onClick([this] {
        isMetric = !isMetric;
        saveUnitSettings();
        updateDisplay();
    });

    ui.funcBtnDia->onClick([this] {
        isDiameterMode = !isDiameterMode;
        saveUnitSettings();
        updateDisplay();
    });

    ui.funcBtnSettings->onClick([this] {
        SettingsPopup::show(prefs, axes, AXES_COUNT, [this] { updateDisplay(); });
    });

    // Power button (sleep)
    ui.sleepBtn->onClick([this] { goToSleep(); });

    updateToolDropdown();
    loadOffsetsFromTool(currentToolIndex);
    updateDisplay();
}

// --- Display and Logic ---

void UIManager::updateDisplay() {
    const auto& tool = toolManager.getTool(currentToolIndex);

    for (int i = 0; i < AXES_COUNT; ++i) {
        float offset = tool.getOffset(i); // getOffset(i) must be provided by Tool (vector-based offsets)
        float val = (axes[i].getPositionUm() / 1000.0f) - axes[i].getGlobalReference() - offset;

        // Unit conversion
        if (!isMetric) val *= 0.0393701f;
        // Diameter mode usually applies only to X (axis 0), but generalize if wanted
        if (isDiameterMode && i == 0) val *= 2.0f;

        char buf[32];
        snprintf(buf, sizeof(buf), "%.4f", val);
        ui.axisDROs[i]->setText(buf);
    }

    ui.funcBtnUnits->setText(isMetric ? "mm" : "in");
    ui.funcBtnDia->setText(isDiameterMode ? "DIA" : "RAD");
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

// --- Set global reference (work zero) for given axis ---
void UIManager::setGlobalReference(int axis, float desiredDroValue) {
    auto& axisObj = axes[axis];
    auto& tool = toolManager.getTool(currentToolIndex);

    float userValue = desiredDroValue;

    // Convert from inches to mm if needed (store internally in mm)
    if (!isMetric) userValue /= 0.0393701f;

    // Convert from diameter mode to radius (store in axis units, applies to X axis only)
    if (axis == 0 && isDiameterMode) userValue /= 2.0f;

    float machine_pos = axisObj.getPositionUm() / 1000.0f;
    float tool_offset = tool.getOffset(axis);

    axisObj.setGlobalReference(machine_pos - userValue - tool_offset);

    saveGlobalReferences();
    updateDisplay();
}

// --- Set current tool's offset for selected axis (based on user value) ---
void UIManager::setToolOffsetValue(int axis, float userValue) {
    auto& axisObj = axes[axis];
    auto& tool = toolManager.getTool(currentToolIndex);

    float inputValue = userValue;

    // Convert from inches to mm if needed
    if (!isMetric) inputValue /= 0.0393701f;

    // Convert from diameter mode to radius if needed (X axis only)
    if (axis == 0 && isDiameterMode) inputValue /= 2.0f;

    float machine_pos = axisObj.getPositionUm() / 1000.0f;
    tool.setOffset(axis, machine_pos - axisObj.getGlobalReference() - inputValue);

    toolManager.saveTools();
    updateDisplay();
}

// --- Save/load global references for axes ---
void UIManager::saveGlobalReferences() {
    for (int i = 0; i < AXES_COUNT; ++i) {
        char key[32];
        snprintf(key, sizeof(key), "global_ref_%s", axes[i].name);
        prefs.putFloat(key, axes[i].getGlobalReference());
    }
}
void UIManager::loadGlobalReferences() {
    for (int i = 0; i < AXES_COUNT; ++i) {
        char key[32];
        snprintf(key, sizeof(key), "global_ref_%s", axes[i].name);
        axes[i].setGlobalReference(prefs.getFloat(key, 0.0f));
    }
}

// --- Utility, Sleep, and Persistence ---

void UIManager::saveUnitSettings() {
    prefs.putUShort("is_metric", isMetric ? 1 : 0);
    prefs.putUShort("is_diameter", isDiameterMode ? 1 : 0);
    // You may want to generalize axis inversion flags too
    for (int i = 0; i < AXES_COUNT; ++i) {
        char key[32];
        snprintf(key, sizeof(key), "%s_invert", axes[i].name);
        prefs.putBool(key, axes[i].getInvert());
    }
}

void UIManager::loadUnitSettings() {
    isMetric = prefs.getUShort("is_metric", 1) != 0;
    isDiameterMode = prefs.getUShort("is_diameter", 0) != 0;
    for (int i = 0; i < AXES_COUNT; ++i) {
        char key[32];
        snprintf(key, sizeof(key), "%s_invert", axes[i].name);
        axes[i].setInvert(prefs.getBool(key, false));
    }
}

void UIManager::goToSleep() {
    gpio_set_level(LCD_PIN_BK_LIGHT, 0);
    auto* wake_layer = new LVPanel(lv_scr_act(), [](auto& p) {
        p.setSize(LCD_H_RES, LCD_V_RES);
        p.setTransparentBg();
        p.clearFlag(LV_OBJ_FLAG_SCROLLABLE);
        p.setFlag(LV_OBJ_FLAG_CLICKABLE);
    });
    wake_layer->onClick([=] {
        gpio_set_level(LCD_PIN_BK_LIGHT, 1);
        wake_layer->del();
    });
}
