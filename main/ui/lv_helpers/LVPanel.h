#pragma once

#include "LVObjectBase.h"
#include <type_traits>
#include <functional>

// LVPanel: LVGL styled panel C++ RAII wrapper
class LVPanel : public LVObjectBase {
public:
    // Basic constructor
    LVPanel(lv_obj_t* parent)
        : LVObjectBase(lv_obj_create(parent))
    {}

    // Lambda-based config constructor
    template<typename F>
    LVPanel(lv_obj_t* parent, F&& config)
        : LVPanel(parent)
    {
        this->setRadius(12);
        this->setPadding(8,8,8,8);
        this->setBgOpa(LV_OPA_COVER); 
        if constexpr (!std::is_same_v<F, std::nullptr_t>)
            config(*this);
    }

    // Set panel background color
    void setPanelColor(lv_color_t color, int part = LV_PART_MAIN) {
        setBgColor(color, part);
    }

    // Set panel border properties
    void setPanelBorder(lv_color_t color, int width, int part = LV_PART_MAIN) {
        setBorderColor(color, part);
        setBorderWidth(width, part);
    }

    // Set panel radius
    void setPanelRadius(int radius, int part = LV_PART_MAIN) {
        setRadius(radius, part);
    }

    // Set panel shadow
    void setPanelShadow(lv_color_t color, int width, int offsetX = 0, int offsetY = 0, int part = LV_PART_MAIN) {
        setShadowColor(color, part);
        setShadowWidth(width, part);
        setShadowOfsX(offsetX, part);
        setShadowOfsY(offsetY, part);
    }
};
