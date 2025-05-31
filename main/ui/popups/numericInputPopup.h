#pragma once

#include <functional>
#include <string>
#include "lv_helpers/LVHelpers.h"

class NumericInputPopup {
public:
    // The callback signature: float value entered, or 'cancelled'
    using Callback = std::function<void(float, bool cancelled)>;

    // Show the popup. Optional title, initial value.
    static void show(const std::string& title, Callback onComplete, float initialValue = 0.0f);
};
