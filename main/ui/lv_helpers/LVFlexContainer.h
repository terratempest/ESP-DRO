#pragma once

#include "LVObjectBase.h"
#include <type_traits>
#include <functional>

// LVFlexContainer: LVGL flexbox container C++ RAII wrapper
class LVFlexContainer : public LVObjectBase {
public:
    // Basic constructor
    LVFlexContainer(lv_obj_t* parent)
        : LVObjectBase(lv_obj_create(parent))
    {
        setFlexFlow(LV_FLEX_FLOW_ROW);
    }

    // Lambda-based config constructor
    template<typename F>
    LVFlexContainer(lv_obj_t* parent, F&& config)
        : LVFlexContainer(parent)
    {   
        if constexpr (!std::is_same_v<F, std::nullptr_t>)
            config(*this);
    }

    // Set flex flow direction (row, column, wrap, etc.)
    void setFlexFlow(lv_flex_flow_t flow) {
        lv_obj_set_flex_flow(obj, flow);
    }

    // Set flex alignment (main, cross, track axes)
    void setFlexAlign(lv_flex_align_t main, lv_flex_align_t cross, lv_flex_align_t track = LV_FLEX_ALIGN_CENTER) {
        lv_obj_set_flex_align(obj, main, cross, track);
    }

    // Set the gap between children
    void setPadGap(int gap, int part = LV_PART_MAIN) {
        lv_obj_set_style_pad_gap(obj, gap, part);
    }

    // Set flex grow for this container
    void setFlexGrow(uint8_t grow) {
        lv_obj_set_flex_grow(obj, grow);
    }
};
