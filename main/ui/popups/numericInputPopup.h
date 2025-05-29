#pragma once

#include <functional>
#include <string>

// Forward-declare LVGL wrapper types as needed:
class LVContainer;
class LVPanel;
class LVLabel;
class LVButton;
class LVTextarea;
class LVKeyboardNumeric;

class NumericInputPopup {
public:
    // The callback signature: float value entered, or 'cancelled'
    using Callback = std::function<void(float, bool cancelled)>;

    // Show the popup. Optional title, initial value.
    static void show(const std::string& title, Callback onComplete, float initialValue = 0.0f);
};
