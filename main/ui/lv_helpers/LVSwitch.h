#pragma once

#include "LVObjectBase.h"
#include <type_traits>
#include <functional>

// LVSwitch: LVGL switch (toggle) C++ RAII wrapper
class LVSwitch : public LVObjectBase {
public:
    // Basic constructor
    LVSwitch(lv_obj_t* parent)
        : LVObjectBase(lv_switch_create(parent))
    {}

    // Lambda-based config constructor
    template<typename F>
    LVSwitch(lv_obj_t* parent, F&& config)
        : LVSwitch(parent)
    {
        if constexpr (!std::is_same_v<F, std::nullptr_t>)
            config(*this);
    }

    // Set checked (on/off) state
    void setChecked(bool checked) override {
        if(checked) lv_obj_add_state(obj, LV_STATE_CHECKED);
        else        lv_obj_clear_state(obj, LV_STATE_CHECKED);
    }

    // Get checked state
    bool isChecked() const override {
        return lv_obj_has_state(obj, LV_STATE_CHECKED);
    }
};
