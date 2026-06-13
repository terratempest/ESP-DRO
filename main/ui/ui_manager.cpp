#include "ui_manager.h"
#include "config.h"
#include "app_display.h"
#include <driver/gpio.h>
#include <esp_timer.h>
#include <cmath>
#include <cstdio>
#include "lv_helpers/LVHelpers.h"
#include "screens/mainScreen.h"

namespace {
uint32_t currentMillis() {
    return static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
}
}

UIManager::UIManager(ToolManager& tm, DroAxis* axes, PreferencesWrapper& p)
    : toolManager(tm), axes(axes), prefs(p)
{
    currentToolIndex = 0;
}

void UIManager::init() {
    lv_obj_clean(lv_scr_act());
    loadUnitSettings();
    loadGlobalReferences();

    buildMainScreen(ui, axes, AXES_COUNT); 

    // Initialize incremental offsets vector
    incrOffsets.resize(AXES_COUNT, 0.0f);
    lastAxisTexts.resize(AXES_COUNT);
    lastRawAxisPositions.resize(AXES_COUNT, 0.0f);
    for (int i = 0; i < AXES_COUNT; ++i) {
        lastRawAxisPositions[i] = axes[i].getPositionUm();
    }
    lastActivityMs = currentMillis();
    lastSeenTouchMs = app_display_get_last_touch_ms();

    ////////////////////////////////////
    //         Event Handling         //
    ////////////////////////////////////

    // Attach per-axis event handlers
    for (int i = 0; i < AXES_COUNT; ++i) {
        // DRO click = set value popup
        ui.axisDROs[i]->onClick([this, i] {
            NumericInputPopup::show("Set DRO Value",
                [this, i](float value, bool cancelled) {
                    if (!cancelled) {
                        if(isIncremental) setIncrementalReference(i, value);
                        else setGlobalReference(i, value);
                    }
                },
                getDisplayedAxisValue(i)
            );
        });

        // Zero button: sets axis to 0 at current pos
        ui.axisZeroBtns[i]->onClick([this, i] {
            if (isIncremental) setIncrementalReference(i, 0.0f);
            else setGlobalReference(i, 0.0f);
        });

        // Tool offset (TLO) button: set tool offset for this axis
        ui.axisTLOBtns[i]->onClick([this, i] {
            NumericInputPopup::show("Set Tool Offset",
                [this, i](float value, bool cancelled) {
                    if (!cancelled) setToolOffsetValue(i, value);
                },
                getDisplayedAxisValue(i)
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

        if (!toolManager.addTool(name, offsets)) {
            showStatusMessage("Maximum tool count reached");
            return;
        }
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

    ui.funcBtnAbsIncr->onClick([this] {
        isIncremental = !isIncremental;
        ui.funcBtnAbsIncr->setText(isIncremental ? "INCR" : "ABS");
        if (isIncremental) {
            for (int i = 0; i < AXES_COUNT; ++i) {
                incrOffsets[i] = axes[i].getPositionUm() / 1000.0f;
            }
        }
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
    uint32_t nowMs = currentMillis();
    updateActivityState(nowMs);

    for (int i = 0; i < AXES_COUNT; ++i) {
        float axis_pos = getDisplayedAxisValue(i);

        char buf[32];
        snprintf(buf, sizeof(buf), "%.4f", axis_pos);
        if (lastAxisTexts[i] != buf) {
            ui.axisDROs[i]->setText(buf);
            lastAxisTexts[i] = buf;
        }
    }

    std::string unitsText = isMetric ? "mm" : "in";
    if (lastUnitsText != unitsText) {
        ui.funcBtnUnits->setText(unitsText);
        lastUnitsText = unitsText;
    }

    std::string diaText = isDiameterMode ? "DIA" : "RAD";
    if (lastDiaText != diaText) {
        ui.funcBtnDia->setText(diaText);
        lastDiaText = diaText;
    }

    std::string absIncrText = isIncremental ? "INCR" : "ABS";
    if (lastAbsIncrText != absIncrText) {
        ui.funcBtnAbsIncr->setText(absIncrText);
        lastAbsIncrText = absIncrText;
    }

    if (lastDropdownSelection != currentToolIndex) {
        ui.toolDropdown->setSelected(currentToolIndex);
        lastDropdownSelection = currentToolIndex;
    }

    if (screenAwake && isAutoScreenOffEnabled()) {
        uint32_t timeoutMs = getAutoScreenOffTimeoutMs();
        if (nowMs - lastActivityMs >= timeoutMs) {
            goToSleep();
        }
    }
}

void UIManager::updateToolDropdown() {
    std::vector<std::string> options;
    for (size_t i = 0; i < toolManager.toolCount(); ++i)
        options.push_back(toolManager.getTool(i).name);
    ui.toolDropdown->setOptions(options);
    ui.toolDropdown->setSelected(currentToolIndex);
    lastDropdownSelection = currentToolIndex;
}

void UIManager::loadOffsetsFromTool(uint16_t index) {
    if (index < toolManager.toolCount())
        currentToolIndex = index;
}

void UIManager::showStatusMessage(const char* message) {
    auto* overlay = new LVPanel(lv_scr_act(), [](auto& p) {
        p.setSize(320, 96);
        p.setAlign(LV_ALIGN_CENTER);
        p.setBgColor(COLOR_BLACK);
        p.setBgOpa(LV_OPA_COVER);
        p.setBorderColor(COLOR_GRAY);
        p.setBorderWidth(2);
        p.setFlag(LV_OBJ_FLAG_CLICKABLE);
    });

    new LVLabel(overlay->obj, message, [](auto& l) {
        l.setAlign(LV_ALIGN_CENTER);
        l.setTextColor(COLOR_YELLOW_GREEN);
    });

    overlay->onClick([=] {
        overlay->del();
    });
}

float UIManager::getDisplayedAxisValue(int axis) {
    const auto& tool = toolManager.getTool(currentToolIndex);
    float axis_pos = axes[axis].getPositionUm() / 1000.0f;

    if (isIncremental) {
        axis_pos -= incrOffsets[axis];
    } else {
        axis_pos -= axes[axis].getGlobalReference();
    }

    axis_pos -= tool.getOffset(axis);

    if (!isMetric) axis_pos *= 0.0393701f;
    if (isDiameterMode && axis == 0) axis_pos *= 2.0f;

    return axis_pos;
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

// --- Set global reference (work zero) for given axis ---
void UIManager::setIncrementalReference(int axis, float desiredDroValue) {
    auto& axisObj = axes[axis];
    auto& tool = toolManager.getTool(currentToolIndex);

    float userValue = desiredDroValue;

    // Convert from inches to mm if needed (store internally in mm)
    if (!isMetric) userValue /= 0.0393701f;

    // Convert from diameter mode to radius (store in axis units, applies to X axis only)
    if (axis == 0 && isDiameterMode) userValue /= 2.0f;

    float machine_pos = axisObj.getPositionUm() / 1000.0f;
    float tool_offset = tool.getOffset(axis);

    incrOffsets[axis] = machine_pos - userValue - tool_offset;
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
    if (!screenAwake) {
        return;
    }

    screenAwake = false;
    gpio_set_level(LCD_PIN_BK_LIGHT, 0);
    wakeLayer = new LVPanel(lv_scr_act(), [](auto& p) {
        p.setSize(LCD_H_RES, LCD_V_RES);
        p.setTransparentBg();
        p.clearFlag(LV_OBJ_FLAG_SCROLLABLE);
        p.setFlag(LV_OBJ_FLAG_CLICKABLE);
    });
    wakeLayer->onClick([this] {
        wakeScreen();
    });
}

void UIManager::wakeScreen() {
    gpio_set_level(LCD_PIN_BK_LIGHT, 1);
    screenAwake = true;

    if (wakeLayer) {
        LVPanel* layer = wakeLayer;
        wakeLayer = nullptr;
        layer->del();
    }

    markActivity(currentMillis());
}

void UIManager::markActivity(uint32_t nowMs) {
    lastActivityMs = nowMs;
}

void UIManager::updateActivityState(uint32_t nowMs) {
    bool axisMoved = false;
    for (int i = 0; i < AXES_COUNT; ++i) {
        float rawPosition = axes[i].getPositionUm();
        if (i >= (int)lastRawAxisPositions.size()) {
            lastRawAxisPositions.resize(AXES_COUNT, 0.0f);
        }
        if (std::fabs(rawPosition - lastRawAxisPositions[i]) > 0.0f) {
            axisMoved = true;
            lastRawAxisPositions[i] = rawPosition;
        }
    }

    if (axisMoved) {
        if (!screenAwake) {
            wakeScreen();
            return;
        }
        markActivity(nowMs);
    }

    uint32_t lastTouchMs = app_display_get_last_touch_ms();
    if (lastTouchMs != 0 && lastTouchMs != lastSeenTouchMs) {
        lastSeenTouchMs = lastTouchMs;
        markActivity(nowMs);
    }
}

bool UIManager::isAutoScreenOffEnabled() {
    return prefs.getBool(AUTO_SCREEN_OFF_KEY, true);
}

uint32_t UIManager::getAutoScreenOffTimeoutMs() {
    float timeoutMinutes = prefs.getFloat(SCREEN_OFF_MIN_KEY, 10.0f);
    if (timeoutMinutes <= 0.0f || !std::isfinite(timeoutMinutes)) {
        timeoutMinutes = 10.0f;
    }

    double timeoutMs = static_cast<double>(timeoutMinutes) * 60.0 * 1000.0;
    if (timeoutMs >= 4294967295.0) {
        return 4294967295UL;
    }
    if (timeoutMs < 1.0) {
        return 1;
    }
    return static_cast<uint32_t>(timeoutMs);
}
