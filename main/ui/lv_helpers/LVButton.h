#pragma once

#include "LVObjectBase.h"
#include <string>
#include <type_traits>
#include <functional>

// LVButton: Universal LVGL button C++ RAII wrapper
class LVButton : public LVObjectBase {
public:
    // Constructor (plain button, no label)
    LVButton(lv_obj_t* parent)
        : LVObjectBase(lv_btn_create(parent))
    {}

    // Constructor with label text (C-string)
    LVButton(lv_obj_t* parent, const char* labelText)
        : LVButton(parent)
    {
        if(labelText && *labelText) {
            lv_obj_t* lbl = lv_label_create(obj);
            lv_label_set_text(lbl, labelText);
            lv_obj_center(lbl);
        }
    }

    // Constructor with label text (std::string)
    LVButton(lv_obj_t* parent, const std::string& labelText)
        : LVButton(parent, labelText.c_str())
    {}

    // Lambda-based constructor for full configuration in-place
    template<typename F>
    LVButton(lv_obj_t* parent, const char* labelText, F&& config)
        : LVButton(parent, labelText)
    {
        this->setRadius(12);
        if constexpr (!std::is_same_v<F, std::nullptr_t>)
            config(*this);
    }
    template<typename F>
    LVButton(lv_obj_t* parent, const std::string& labelText, F&& config)
        : LVButton(parent, labelText)
    {
        this->setRadius(12);
        if constexpr (!std::is_same_v<F, std::nullptr_t>)
            config(*this);
    }

    // --------- Button-specific features ---------

    // Set the button label text
    void setText(const char* text) override {
        lv_obj_t* lbl = getLabel();
        if(lbl) lv_label_set_text(lbl, text);
    }
    void setText(const std::string& text) override { setText(text.c_str()); }

    // Get the button's internal label object (if any)
    lv_obj_t* getLabel() override {
        // Button's first child is typically the label (if created)
        return lv_obj_get_child(obj, 0);
    }

    // Get button's current label as string
    std::string getText() const override {
        lv_obj_t* lbl = lv_obj_get_child(obj, 0);
        return lbl ? std::string(lv_label_get_text(lbl)) : "";
    }

    // Set/get checked state (for toggle buttons)
    void setChecked(bool checked) override {
        if(checked) addState(LV_STATE_CHECKED);
        else        clearState(LV_STATE_CHECKED);
    }
    bool isChecked() const override {
        return hasState(LV_STATE_CHECKED);
    }

    // Set/get disabled state (optional, inherited but override for future extension)
    void setDisabled(bool dis = true) override {
        if(dis) addState(LV_STATE_DISABLED);
        else    clearState(LV_STATE_DISABLED);
    }
    bool isDisabled() const override {
        return hasState(LV_STATE_DISABLED);
    }
};
