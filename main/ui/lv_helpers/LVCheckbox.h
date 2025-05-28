#pragma once

#include "LVObjectBase.h"
#include <string>
#include <type_traits>
#include <functional>

// LVCheckbox: Universal LVGL checkbox C++ RAII wrapper
class LVCheckbox : public LVObjectBase {
public:
    // Constructor with optional label text
    LVCheckbox(lv_obj_t* parent, const char* labelText = nullptr)
        : LVObjectBase(lv_checkbox_create(parent))
    {
        if(labelText && *labelText)
            lv_checkbox_set_text(obj, labelText);
    }

    // std::string overload
    LVCheckbox(lv_obj_t* parent, const std::string& labelText)
        : LVCheckbox(parent, labelText.c_str())
    {}

    // Lambda-based config constructor
    template<typename F>
    LVCheckbox(lv_obj_t* parent, const char* labelText, F&& config)
        : LVCheckbox(parent, labelText)
    {
        if constexpr (!std::is_same_v<F, std::nullptr_t>)
            config(*this);
    }
    template<typename F>
    LVCheckbox(lv_obj_t* parent, const std::string& labelText, F&& config)
        : LVCheckbox(parent, labelText)
    {
        if constexpr (!std::is_same_v<F, std::nullptr_t>)
            config(*this);
    }

    // ------ Checkbox-specific features ------

    // Set/get label text
    void setText(const char* text) override        { lv_checkbox_set_text(obj, text); }
    void setText(const std::string& text) override{ lv_checkbox_set_text(obj, text.c_str()); }
    std::string getText() const override          { return std::string(lv_checkbox_get_text(obj)); }

    // Checked state
    void setChecked(bool checked) override {
        if(checked) addState(LV_STATE_CHECKED);
        else        clearState(LV_STATE_CHECKED);
    }
    bool isChecked() const override {
        return hasState(LV_STATE_CHECKED);
    }

    // The label (for styling, font, etc)
    lv_obj_t* getLabel() override {
        // In LVGL, this is the first (and only) child
        return lv_obj_get_child(obj, 0);
    }
};
