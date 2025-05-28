#pragma once

#include "LVObjectBase.h"
#include <type_traits>
#include <functional>

// LVKeyboard: LVGL on-screen keyboard C++ RAII wrapper
class LVKeyboard : public LVObjectBase {
public:
    // Basic constructor
    LVKeyboard(lv_obj_t* parent)
        : LVObjectBase(lv_keyboard_create(parent))
    {}

    // Lambda-based config constructor
    template<typename F>
    LVKeyboard(lv_obj_t* parent, F&& config)
        : LVKeyboard(parent)
    {
        if constexpr (!std::is_same_v<F, std::nullptr_t>)
            config(*this);
    }

    // Set the keyboard mode (LV_KEYBOARD_MODE_TEXT, LV_KEYBOARD_MODE_NUM, etc.)
    void setMode(lv_keyboard_mode_t mode) {
        lv_keyboard_set_mode(obj, mode);
    }

    // Assign the target textarea
    void setTextarea(lv_obj_t* textarea) {
        lv_keyboard_set_textarea(obj, textarea);
    }

    // Set map (custom key map)
    void setMap(lv_keyboard_mode_t mode, const char *const *map, const lv_buttonmatrix_ctrl_t *ctrl_map) {
        lv_keyboard_set_map(obj, mode, map, ctrl_map);
    }

    // Set popovers enabled/disabled
    void setPopovers(bool enabled) {
        lv_keyboard_set_popovers(obj, enabled);
    }
};
