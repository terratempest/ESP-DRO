#pragma once

#include "LVKeyboard.h"
#include <type_traits>
#include <functional>

// LVKeyboardNumeric: LVGL numeric keyboard C++ RAII wrapper
class LVKeyboardNumeric : public LVKeyboard {
public:
    // Basic constructor
    LVKeyboardNumeric(lv_obj_t* parent)
        : LVKeyboard(parent)
    {
        setMode(LV_KEYBOARD_MODE_NUMBER);
    }

    // Lambda-based config constructor
    template<typename F>
    LVKeyboardNumeric(lv_obj_t* parent, F&& config)
        : LVKeyboardNumeric(parent)
    {
        if constexpr (!std::is_same_v<F, std::nullptr_t>)
            config(*this);
    }
};
