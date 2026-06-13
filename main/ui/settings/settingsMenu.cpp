#include "settingsMenu.h"
#include "config.h"

std::vector<SettingDef> makeSettingsTable(PreferencesWrapper& prefs, DroAxis* axes, int axisCount)
{
    std::vector<SettingDef> table;

    table.push_back({
        "Enable Auto Screen Off",
        AUTO_SCREEN_OFF_KEY,
        SettingType::Bool,
        0.0f,
        true,
        nullptr
    });

    table.push_back({
        "Screen Off Timeout (minutes)",
        SCREEN_OFF_MIN_KEY,
        SettingType::Number,
        10.0f,
        false,
        nullptr
    });

    for (int i = 0; i < axisCount; ++i) {
        std::string axisName = axes[i].name; // e.g. "x", "y", "z"
        float defaultStepUm = axes[i].getStepUm();

        // Calibration setting
        table.push_back({
            axisName + " Axis Calibration (um/unit)",
            axisName + "_calib",
            SettingType::Number,
            defaultStepUm,
            false,
            [&prefs, axis = &axes[i], key = axisName + "_calib", defaultStepUm] {
                axis->setStepUm(prefs.getFloat(key.c_str(), defaultStepUm));
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
