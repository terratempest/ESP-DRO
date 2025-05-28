#pragma once

#include "LVObjectBase.h"
#include <string>
#include <vector>
#include <type_traits>
#include <functional>

// LVDropdown: Universal LVGL dropdown C++ RAII wrapper
class LVDropdown : public LVObjectBase {
public:
    // Basic constructor
    LVDropdown(lv_obj_t* parent)
        : LVObjectBase(lv_dropdown_create(parent))
    {}

    // Lambda-based config constructor
    template<typename F>
    LVDropdown(lv_obj_t* parent, F&& config)
        : LVDropdown(parent)
    {
        if constexpr (!std::is_same_v<F, std::nullptr_t>)
            config(*this);
    }

    // ---- Dropdown-specific features ----

    // Set options from newline-separated string
    void setOptions(const char* options) {
        lv_dropdown_set_options(obj, options);
    }
    // Set options from vector of strings
    void setOptions(const std::vector<std::string>& opts) {
        std::string optStr;
        for (size_t i = 0; i < opts.size(); ++i) {
            optStr += opts[i];
            if (i + 1 < opts.size()) optStr += "\n";
        }
        setOptions(optStr.c_str());
    }
    // Set options from vector of strings
    void setOptions(std::vector<std::string>& opts) {
        std::string optStr;
        for (size_t i = 0; i < opts.size(); ++i) {
            optStr += opts[i];
            if (i + 1 < opts.size()) optStr += "\n";
        }
        setOptions(optStr.c_str());
    }

    // Get options as string
    std::string getOptions() const {
        return std::string(lv_dropdown_get_options(obj));
    }

    // Select by index
    void setSelected(uint16_t idx) {
        lv_dropdown_set_selected(obj, idx);
    }
    // Get selected index
    uint16_t getSelected() const {
        return lv_dropdown_get_selected(obj);
    }
    // Get selected text
    std::string getSelectedStr() const {
        char buf[128]{};
        lv_dropdown_get_selected_str(obj, buf, sizeof(buf));
        return std::string(buf);
    }

    // Set/get direction
    void setDir(lv_dir_t dir) {
        lv_dropdown_set_dir(obj, dir);
    }
    lv_dir_t getDir() const {
        return lv_dropdown_get_dir(obj);
    }

    // Set placeholder (LVGL 9+)
    void setPlaceholder(const std::string& txt) {
        setPlaceholder(txt.c_str());
    }

    // Event: On value changed (selection changed)
    void onChanged(std::function<void(uint16_t idx, std::string text)> fn) {
        struct Handler {
            static void callback(lv_event_t* e) {
                auto* fn = static_cast<std::function<void(uint16_t, std::string)>*>(lv_event_get_user_data(e));
                lv_obj_t* dd = (lv_obj_t*)lv_event_get_target(e);
                uint16_t idx = lv_dropdown_get_selected(dd);
                char buf[128]{};
                lv_dropdown_get_selected_str(dd, buf, sizeof(buf));
                (*fn)(idx, std::string(buf));
            }
        };
        lv_obj_add_event_cb(obj, Handler::callback, LV_EVENT_VALUE_CHANGED, new std::function<void(uint16_t, std::string)>(fn));
    }
    // Overload: no parameters
    void onChanged(std::function<void()> fn) {
        struct Handler {
            static void callback(lv_event_t* e) {
                auto* fn = static_cast<std::function<void()>*>(lv_event_get_user_data(e));
                (*fn)();
            }
        };
        lv_obj_add_event_cb(obj, Handler::callback, LV_EVENT_VALUE_CHANGED, new std::function<void()>(fn));
    }
};
