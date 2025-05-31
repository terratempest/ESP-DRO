#pragma once

#include "preferences_wrapper.h"
#include "dro_axis.h"
#include <vector>
#include <functional>
#include "settings/settingsMenu.h"

class SettingsPopup {
public:
    // Pops up the settings menu. (Caller handles memory and references)
    static void show(PreferencesWrapper& prefs, DroAxis* axes, int axesCount, std::function<void()> onClose = nullptr);
};
