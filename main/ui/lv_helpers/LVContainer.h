#pragma once

#include "LVObjectBase.h"
#include <type_traits>
#include <functional>

// LVContainer: Universal LVGL container (panel/group/layout) C++ RAII wrapper
class LVContainer : public LVObjectBase {
public:
    // Basic constructor
    LVContainer(lv_obj_t* parent)
        : LVObjectBase(lv_obj_create(parent))
    {}

    // Lambda-based config constructor
    template<typename F>
    LVContainer(lv_obj_t* parent, F&& config)
        : LVContainer(parent)
    {
        this->removeStyleAll();
        if constexpr (!std::is_same_v<F, std::nullptr_t>)
            config(*this);
    }

    // ---- Container-specific helpers ----

    // Remove all children (clear container)
    void clear() { lv_obj_clean(obj); }

    // Flex/grid helpers (already available from base, but you can expose here for semantic clarity)
    void setFlexFlow(lv_flex_flow_t flow)          { LVObjectBase::setFlexFlow(flow); }
    void setFlexAlign(lv_flex_align_t main, lv_flex_align_t cross, lv_flex_align_t track=LV_FLEX_ALIGN_CENTER) {
        LVObjectBase::setFlexAlign(main, cross, track);
    }
    void setFlexGrow(uint8_t grow)                 { LVObjectBase::setFlexGrow(grow); }
    void setPadGap(int gap, int part = LV_PART_MAIN) { LVObjectBase::setPadGap(gap, part); }

    // ... add any container/panel-specific helpers you find useful!
};
