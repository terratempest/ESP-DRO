#pragma once

#include "LVObjectBase.h"
#include <string>
#include <type_traits>
#include <functional>

// LVLabel: Universal LVGL label C++ RAII wrapper
class LVLabel : public LVObjectBase {
public:
    // Basic constructor with text (C-string)
    LVLabel(lv_obj_t* parent, const char* text = nullptr)
        : LVObjectBase(lv_label_create(parent))
    {
        if(text && *text) lv_label_set_text(obj, text);
    }

    // Basic constructor with text (std::string)
    LVLabel(lv_obj_t* parent, const std::string& text)
        : LVLabel(parent, text.c_str())
    {}

    // Lambda-based config constructor (C-string)
    template<typename F>
    LVLabel(lv_obj_t* parent, const char* text, F&& config)
        : LVLabel(parent, text)
    {
        if constexpr (!std::is_same_v<F, std::nullptr_t>)
            config(*this);
    }

    // Lambda-based config constructor (std::string)
    template<typename F>
    LVLabel(lv_obj_t* parent, const std::string& text, F&& config)
        : LVLabel(parent, text)
    {
        if constexpr (!std::is_same_v<F, std::nullptr_t>)
            config(*this);
    }

    // Set label text
    void setText(const char* text) override {lv_label_set_text(obj, text);}
    void setText(const std::string& text) override {lv_label_set_text(obj, text.c_str());}

    // Get label text
    std::string getText() const override {return std::string(lv_label_get_text(obj));}

    // Get label object (self)
    lv_obj_t* getLabel() override {return obj;}
};
