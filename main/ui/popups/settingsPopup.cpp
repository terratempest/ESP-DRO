#include "settingsPopup.h"
#include "lv_helpers/LVHelpers.h"
#include <vector>
#include <string>
#include "config.h"

void SettingsPopup::show(PreferencesWrapper& prefs, DroAxis* axes, int axesCount, std::function<void()> onClose) {
    auto* overlay = new LVContainer(lv_scr_act(), [](auto& p){
        p.setSize(LCD_H_RES, LCD_V_RES);
        p.setTransparentBg();
        p.setFlag(LV_OBJ_FLAG_CLICKABLE);
    });

    auto* panel = new LVPanel(overlay->obj, [&](auto& p){
        p.setSize(LCD_H_RES-60, LCD_V_RES-30);
        p.setAlign(LV_ALIGN_CENTER);
        p.setBgColor(COLOR_BLACK);
        p.setRadius(12);
        p.setPadAll(16);
    });

    auto* tabsView = new LVContainer(panel->obj, [](auto& l){
        l.setAlign(LV_ALIGN_TOP_LEFT);
        l.setHeightPercent(100);
        l.setWidthPercent(30);
        l.setPadAll(8);
        l.setBgColor(COLOR_GRAY);
        new LVLabel(l.obj, "Settings", [](auto& l){ l.setAlign(LV_ALIGN_LEFT_MID); });
    });

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

    std::vector<LVTextarea*> numInputs;
    std::vector<LVSwitch*> boolInputs;
    auto settingsTable = makeSettingsTable(prefs, axes, axesCount);

    for (size_t i = 0; i < settingsTable.size(); ++i) {
        const auto& s = settingsTable[i];
        new LVContainer(settingsContainer->obj, [&](auto& l){
            l.setWidthPercent(80);
            l.setHeight(32);
            new LVLabel(l.obj, s.name.c_str(), [](auto& l){ l.setAlign(LV_ALIGN_LEFT_MID); });

            if (s.type == SettingType::Number) {
                float val = prefs.getFloat(s.key.c_str(), s.defaultNumber);
                char buf[16];
                snprintf(buf, sizeof(buf), "%.3f", val);
                auto* ta = new LVTextarea(l.obj, buf, [](auto& t){
                    t.setSize(100, 32);
                    t.setAlign(LV_ALIGN_RIGHT_MID);
                    t.setOneLine();
                });
                numInputs.push_back(ta);
            } else if (s.type == SettingType::Bool) {
                bool val = prefs.getBool(s.key.c_str(), s.defaultBool);
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

    // Save button
    new LVButton(panel->obj, "Save", [&](auto& b){
        b.setSize(90, 36);
        b.setAlign(LV_ALIGN_BOTTOM_LEFT, 158, 0);
        b.setBgColor(COLOR_GRAY);
        // Capture settingsTable, numInputs, boolInputs **by value**
        b.onClick([=, &prefs]{
            int numIdx = 0, boolIdx = 0;
            for (size_t i = 0; i < settingsTable.size(); ++i) {
                const auto& s = settingsTable[i];
                if (s.type == SettingType::Number && numIdx < (int)numInputs.size()) {
                    float v = strtof(numInputs[numIdx++]->getText().c_str(), nullptr);
                    prefs.putFloat(s.key.c_str(), v);
                    if (s.onChange) s.onChange();
                } else if (s.type == SettingType::Bool && boolIdx < (int)boolInputs.size()) {
                    bool v = boolInputs[boolIdx++]->isChecked();
                    prefs.putBool(s.key.c_str(), v);
                    if (s.onChange) s.onChange();
                }
            }
            overlay->del();
            if (onClose) onClose();
        });
    });

    // Cancel button
    new LVButton(panel->obj, "Cancel", [&](auto& b){
        b.setSize(90, 36);
        b.setAlign(LV_ALIGN_BOTTOM_RIGHT, 0, 0);
        b.setBgColor(COLOR_GRAY);
        b.onClick([=]{
            overlay->del();
            if (onClose) onClose();
        });
    });
}
