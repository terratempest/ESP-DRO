#pragma once
#include "preferences_wrapper.h"
#include "dro_axis.h"
#include <vector>
#include <functional>
#include <string>

enum class SettingType { Number, Bool };

struct SettingDef {
    std::string name;
    std::string key;
    SettingType type;
    float defaultNumber;
    bool defaultBool;
    std::function<void()> onChange;
};

std::vector<SettingDef> makeSettingsTable(PreferencesWrapper& prefs, DroAxis* axes, int axisCount);
