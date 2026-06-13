#include "settingsPopup.h"
#include "lv_helpers/LVHelpers.h"
#include "app_services.h"
#include "wifi_manager.h"
#include "ota_server.h"
#include "wifiConnectionPopup.h"
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cctype>
#include <vector>
#include <string>
#include "config.h"

namespace {
bool parseSettingNumber(const std::string& text, float& value) {
    errno = 0;
    char* end = nullptr;
    value = strtof(text.c_str(), &end);
    if (end == text.c_str() || errno == ERANGE || !std::isfinite(value)) {
        return false;
    }
    while (*end != '\0') {
        if (!std::isspace(static_cast<unsigned char>(*end))) {
            return false;
        }
        ++end;
    }
    return true;
}

bool settingRequiresPositiveValue(const SettingDef& setting) {
    return setting.key.find("_calib") != std::string::npos ||
           setting.key == SCREEN_OFF_MIN_KEY;
}

std::string networkStatusText() {
    if (!gWifiManager) {
        return "Unavailable";
    }
    return gWifiManager->getSsid().empty()
        ? gWifiManager->getStatusText()
        : gWifiManager->getStatusText() + " - " + gWifiManager->getSsid();
}
}

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

    LVLabel* wifiValueLabel = nullptr;
    auto* wifiRow = new LVContainer(settingsContainer->obj, [&](auto& l){
        l.setWidthPercent(80);
        l.setHeight(38);
        l.setFlag(LV_OBJ_FLAG_CLICKABLE);
        l.setBgColor(COLOR_GRAY);
        l.setBgOpa(LV_OPA_30);
        l.setRadius(6);
        l.setPadAll(8);
        new LVLabel(l.obj, "Wi-Fi", [](auto& label){ label.setAlign(LV_ALIGN_LEFT_MID); });
        wifiValueLabel = new LVLabel(l.obj, networkStatusText(), [](auto& label){
            label.setAlign(LV_ALIGN_RIGHT_MID, -24, 0);
        });
        new LVLabel(l.obj, ">", [](auto& label){ label.setAlign(LV_ALIGN_RIGHT_MID); });
    });
    wifiRow->onClick([=]{
        WifiConnectionPopup::show([=]{
            if (wifiValueLabel) {
                wifiValueLabel->setText(networkStatusText());
            }
        });
    });

    auto* errorLabel = new LVLabel(panel->obj, "", [&](auto& l){
        l.setAlign(LV_ALIGN_BOTTOM_MID, 0, 0);
        l.setTextColor(lv_palette_main(LV_PALETTE_RED));
        l.setHidden(true);
    });

    // Shared numeric keyboard
    auto* kb = new LVKeyboardNumeric(overlay->obj, [&](auto& k){
        k.setSize(LCD_H_RES, LCD_V_RES/3);
        k.setAlign(LV_ALIGN_BOTTOM_MID, 0, 0);
        k.setVisible(false);
    });
    kb->onEvent(LV_EVENT_READY,  [kb]{ kb->setVisible(false); });
    kb->onEvent(LV_EVENT_CANCEL, [kb]{ kb->setVisible(false); });

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
            int numIdx = 0;
            std::vector<float> parsedNumbers;
            parsedNumbers.reserve(numInputs.size());

            for (size_t i = 0; i < settingsTable.size(); ++i) {
                const auto& s = settingsTable[i];
                if (s.type != SettingType::Number) {
                    continue;
                }
                if (numIdx >= (int)numInputs.size()) {
                    errorLabel->setText("Missing numeric setting");
                    errorLabel->setHidden(false);
                    return;
                }

                float v = 0.0f;
                if (!parseSettingNumber(numInputs[numIdx++]->getText(), v)) {
                    errorLabel->setText((s.name + " must be a valid number").c_str());
                    errorLabel->setHidden(false);
                    return;
                }
                if (settingRequiresPositiveValue(s) && v <= 0.0f) {
                    errorLabel->setText((s.name + " must be greater than zero").c_str());
                    errorLabel->setHidden(false);
                    return;
                }
                parsedNumbers.push_back(v);
            }

            numIdx = 0;
            int boolIdx = 0;
            std::vector<std::function<void()>> pendingChanges;
            prefs.beginBatch();
            for (size_t i = 0; i < settingsTable.size(); ++i) {
                const auto& s = settingsTable[i];
                if (s.type == SettingType::Number && numIdx < (int)parsedNumbers.size()) {
                    prefs.putFloat(s.key.c_str(), parsedNumbers[numIdx++]);
                    if (s.onChange) pendingChanges.push_back(s.onChange);
                } else if (s.type == SettingType::Bool && boolIdx < (int)boolInputs.size()) {
                    bool v = boolInputs[boolIdx++]->isChecked();
                    prefs.putBool(s.key.c_str(), v);
                    if (s.onChange) pendingChanges.push_back(s.onChange);
                }
            }
            prefs.commitBatch();
            for (const auto& onChange : pendingChanges) {
                onChange();
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
