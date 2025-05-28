#include "settingsMenu.h"

std::vector<SettingDef> makeSettingsTable(PreferencesWrapper& prefs, DroAxis* axes, int axisCount)
{
    std::vector<SettingDef> table;

    for (int i = 0; i < axisCount; ++i) {
        std::string axisName = axes[i].name; // e.g. "x", "y", "z"

        // Calibration setting
        table.push_back({
            axisName + " Axis Calibration (um/unit)",
            axisName + "_calib",
            SettingType::Number,
            5.0f, // default
            false,
            [&prefs, axis = &axes[i], key = axisName + "_calib"] {
                axis->setStepUm(prefs.getFloat(key.c_str(), 5.0f));
            }
        });

        // Invert axis setting
        table.push_back({
            "Invert " + axisName + " Axis",
            axisName + "_invert",
            SettingType::Bool,
            0.0f,
            false,
            [&prefs, axis = &axes[i], key = axisName + "_invert"] {
                axis->setInvert(prefs.getBool(key.c_str(), false));
            }
        });
    }

    return table;
}
