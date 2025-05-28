#pragma once

#include "LVObjectBase.h"
#include <string>
#include <type_traits>
#include <functional>

// LVTextarea: Universal LVGL textarea C++ RAII wrapper
class LVTextarea : public LVObjectBase {
public:
    // Basic constructor with optional initial text
    LVTextarea(lv_obj_t* parent, const char* text = nullptr)
        : LVObjectBase(lv_textarea_create(parent))
    {
        if(text && *text) lv_textarea_set_text(obj, text);
    }

    LVTextarea(lv_obj_t* parent, const std::string& text)
        : LVTextarea(parent, text.c_str())
    {}

    // Lambda-based config constructor (C-string)
    template<typename F>
    LVTextarea(lv_obj_t* parent, const char* text, F&& config)
        : LVTextarea(parent, text)
    {
        if constexpr (!std::is_same_v<F, std::nullptr_t>)
            config(*this);
    }

    // Lambda-based config constructor (std::string)
    template<typename F>
    LVTextarea(lv_obj_t* parent, const std::string& text, F&& config)
        : LVTextarea(parent, text)
    {
        if constexpr (!std::is_same_v<F, std::nullptr_t>)
            config(*this);
    }

    // Set textarea text
    void setText(const char* text) override {
        lv_textarea_set_text(obj, text);
    }
    void setText(const std::string& text) override {
        lv_textarea_set_text(obj, text.c_str());
    }

    // Get textarea text
    std::string getText() const override {
        return std::string(lv_textarea_get_text(obj));
    }

    void focus(int part = LV_PART_MAIN) {
        lv_obj_scroll_to_view(obj, LV_ANIM_ON);
        lv_obj_add_state(obj, LV_STATE_FOCUSED);
    }

    void setOneLine(bool enable = true) {
        lv_textarea_set_one_line(obj, enable);
    }
};
