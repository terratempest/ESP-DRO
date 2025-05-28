#pragma once

extern "C" {
    #include <lvgl.h>
}

#include <cstdint>
#include <string>
#include <functional>
#include <type_traits>

// Universal LVGL UI object wrapper
class LVObjectBase {
public:
    lv_obj_t* obj;

    LVObjectBase(lv_obj_t* o) : obj(o) {}

    // ---- Layout & Sizing ----
    void setSize(int w, int h)                      { lv_obj_set_size(obj, w, h); }
    void setWidth(int w)                            { lv_obj_set_width(obj, w); }
    void setHeight(int h)                           { lv_obj_set_height(obj, h); }
    void setSizePercent(int w_pct, int h_pct)       { lv_obj_set_width(obj, lv_pct(w_pct)); lv_obj_set_height(obj, lv_pct(h_pct)); }
    void setWidthPercent(int w_pct)                 { lv_obj_set_width(obj, lv_pct(w_pct)); }
    void setHeightPercent(int h_pct)                { lv_obj_set_height(obj, lv_pct(h_pct)); }
    void setSizeContent()                           { lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT); }
    void setWidthContent()                          { lv_obj_set_width(obj, LV_SIZE_CONTENT); }
    void setHeightContent()                         { lv_obj_set_height(obj, LV_SIZE_CONTENT); }
    void setPos(int x, int y)                       { lv_obj_set_pos(obj, x, y); }
    void setAlign(lv_align_t align, int x=0, int y=0) { lv_obj_align(obj, align, x, y); }

    // ---- Flags & State ----
    void setFlag(lv_obj_flag_t flag)                { lv_obj_add_flag(obj, flag); }
    void clearFlag(lv_obj_flag_t flag)              { lv_obj_clear_flag(obj, flag); }
    bool hasFlag(lv_obj_flag_t flag) const          { return lv_obj_has_flag(obj, flag); }
    void addState(lv_state_t state)                 { lv_obj_add_state(obj, state); }
    void clearState(lv_state_t state)               { lv_obj_clear_state(obj, state); }
    bool hasState(lv_state_t state) const           { return lv_obj_has_state(obj, state); }
    void setScrollable(bool s = true)               { if (s){lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);} else {lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLLABLE);}}

    // ---- Visibility & Focus ----
    void setVisible(bool visible=true)              { if(visible) lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN); }
    void setHidden(bool hide=true)                  { if(hide) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN); else lv_obj_clear_flag(obj, LV_OBJ_FLAG_HIDDEN); }
    bool isHidden() const                           { return lv_obj_has_flag(obj, LV_OBJ_FLAG_HIDDEN); }
    void setFocusable(bool enable=true)             { enable ? lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE) : lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICK_FOCUSABLE); }
    bool isVisible() const                          { return lv_obj_is_visible(obj); }

    // ---- Style (Color, Border, Padding, etc.) ----
    void setBgColor(lv_color_t color, int part = LV_PART_MAIN)         { lv_obj_set_style_bg_color(obj, color, part); }
    void setBgOpa(lv_opa_t opa, int part = LV_PART_MAIN)               { lv_obj_set_style_bg_opa(obj, opa, part); }
    void setTransparentBg(int part = LV_PART_MAIN)                     { lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, part);}
    void setRadius(int r, int part = LV_PART_MAIN)                     { lv_obj_set_style_radius(obj, r, part); }
    void setBorderColor(lv_color_t c, int part = LV_PART_MAIN)         { lv_obj_set_style_border_color(obj, c, part); }
    void setBorderWidth(int w, int part = LV_PART_MAIN)                { lv_obj_set_style_border_width(obj, w, part); }
    void setBorderOpa(lv_opa_t opa, int part = LV_PART_MAIN)           { lv_obj_set_style_border_opa(obj, opa, part); }
    void setPadAll(int pad, int part = LV_PART_MAIN)                   { lv_obj_set_style_pad_all(obj, pad, part); }
    void setPadding(int left, int right, int top, int bottom, int part = LV_PART_MAIN) {
        lv_obj_set_style_pad_left(obj, left, part);
        lv_obj_set_style_pad_right(obj, right, part);
        lv_obj_set_style_pad_top(obj, top, part);
        lv_obj_set_style_pad_bottom(obj, bottom, part);
    }
    void setPadRow(int pad, int part = LV_PART_MAIN)                   { lv_obj_set_style_pad_row(obj, pad, part); }
    void setPadColumn(int pad, int part = LV_PART_MAIN)                { lv_obj_set_style_pad_column(obj, pad, part); }
    void setPadTop(int pad, int part = LV_PART_MAIN)                   { lv_obj_set_style_pad_top(obj, pad, part); }
    void setPadBottom(int pad, int part = LV_PART_MAIN)                { lv_obj_set_style_pad_bottom(obj, pad, part); }
    void setPadLeft(int pad, int part = LV_PART_MAIN)                  { lv_obj_set_style_pad_left(obj, pad, part); }
    void setPadRight(int pad, int part = LV_PART_MAIN)                 { lv_obj_set_style_pad_right(obj, pad, part); }
    void setShadowColor(lv_color_t c, int part = LV_PART_MAIN)         { lv_obj_set_style_shadow_color(obj, c, part); }
    void setShadowWidth(int w, int part = LV_PART_MAIN)                { lv_obj_set_style_shadow_width(obj, w, part); }
    void setShadowOpa(lv_opa_t opa, int part = LV_PART_MAIN)           { lv_obj_set_style_shadow_opa(obj, opa, part); }
    void setShadowSpread(int spread, int part = LV_PART_MAIN)          { lv_obj_set_style_shadow_spread(obj, spread, part); }
    void setShadowOfsX(int ofs, int part = LV_PART_MAIN)               { lv_obj_set_style_shadow_ofs_x(obj, ofs, part); }
    void setShadowOfsY(int ofs, int part = LV_PART_MAIN)               { lv_obj_set_style_shadow_ofs_y(obj, ofs, part); }
    void clearFormatting(int part = LV_PART_MAIN) {
        lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_row(obj, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_column(obj, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_gap(obj, 0, LV_PART_MAIN);  // For flex/grid layouts
        lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
        lv_obj_set_style_outline_width(obj, 0, LV_PART_MAIN);
        lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN);
    }

    // ---- Text and Font ----
    virtual void setText(const char* text)          { /* No-op by default. Derived classes can override. */ }
    virtual void setText(const std::string& text)   { setText(text.c_str()); }
    virtual std::string getText() const             { return ""; }
    void setTextColor(lv_color_t color, int part = LV_PART_MAIN)       { lv_obj_set_style_text_color(obj, color, part); }
    void setFont(const lv_font_t* font, int part = LV_PART_MAIN)       { lv_obj_set_style_text_font(obj, font, part); }
    void setTextOpa(lv_opa_t opa, int part = LV_PART_MAIN)             { lv_obj_set_style_text_opa(obj, opa, part); }
    void setTextLetterSpace(int space, int part = LV_PART_MAIN)        { lv_obj_set_style_text_letter_space(obj, space, part); }
    void setTextLineSpace(int space, int part = LV_PART_MAIN)          { lv_obj_set_style_text_line_space(obj, space, part); }

    // ---- Label Access (for widgets with a child label) ----
    virtual lv_obj_t* getLabel() { return nullptr; } // Override if widget uses a label

    // ---- Checkable (buttons, checkbox, switch, etc.) ----
    virtual void setChecked(bool checked)           { /* No-op by default. Override if applicable. */ }
    virtual bool isChecked() const                  { return false; }

    // ---- Disabled State (applies to all) ----
    virtual void setDisabled(bool dis = true)       { if(dis) addState(LV_STATE_DISABLED); else clearState(LV_STATE_DISABLED); }
    virtual bool isDisabled() const                 { return hasState(LV_STATE_DISABLED); }

    // ---- Opacity ----
    void setOpa(lv_opa_t opa, int part = LV_PART_MAIN)                 { lv_obj_set_style_opa(obj, opa, part); }

    // ---- Scroll ----
    void setScrollSnapX(lv_scroll_snap_t s)                            { lv_obj_set_scroll_snap_x(obj, s); }
    void setScrollSnapY(lv_scroll_snap_t s)                            { lv_obj_set_scroll_snap_y(obj, s); }
    void setScrollDir(lv_dir_t dir)                                    { lv_obj_set_scroll_dir(obj, dir); }
    void setScrollbarMode(lv_scrollbar_mode_t m)                       { lv_obj_set_scrollbar_mode(obj, m); }
    void scrollToX(lv_coord_t x, lv_anim_enable_t anim)                { lv_obj_scroll_to_x(obj, x, anim); }
    void scrollToY(lv_coord_t y, lv_anim_enable_t anim)                { lv_obj_scroll_to_y(obj, y, anim); }

    // ---- User Data ----
    void setUserData(void* data)                                       { lv_obj_set_user_data(obj, data); }
    void* getUserData() const                                          { return lv_obj_get_user_data(obj); }

    // ---- Event Callbacks ----
    void addEventCb(lv_event_cb_t cb, lv_event_code_t code, void* user_data = nullptr) {
        lv_obj_add_event_cb(obj, cb, code, user_data);
    }
    void removeEventCb(lv_event_cb_t cb) {
        lv_obj_remove_event_cb(obj, cb);
    }

    // Generic lambda event
    template<typename F>
    void onEvent(lv_event_code_t code, F&& fn) {
        struct Handler {
            static void callback(lv_event_t* e) {
                auto* fn = static_cast<std::function<void()>*>(lv_event_get_user_data(e));
                if(fn) (*fn)();
            }
        };
        lv_obj_add_event_cb(obj, Handler::callback, code, new std::function<void()>(fn));
    }

    // Click helper
    template<typename F>
    void onClick(F&& fn) { onEvent(LV_EVENT_CLICKED, std::forward<F>(fn)); }

    // ---- Parent/Child ----
    void setParent(lv_obj_t* parent)                                   { lv_obj_set_parent(obj, parent); }
    lv_obj_t* getParent() const                                        { return lv_obj_get_parent(obj); }

    // ---- Flex and Grid ----
    void setFlexFlow(lv_flex_flow_t flow)                              { lv_obj_set_flex_flow(obj, flow); }
    void setFlexAlign(lv_flex_align_t main, lv_flex_align_t cross, lv_flex_align_t track=LV_FLEX_ALIGN_CENTER) {
        lv_obj_set_flex_align(obj, main, cross, track);
    }
    void setFlexGrow(uint8_t grow)                                     { lv_obj_set_flex_grow(obj, grow); }
    void setPadGap(int gap, int part = LV_PART_MAIN)                   { lv_obj_set_style_pad_gap(obj, gap, part); }

    // ---- Misc ----
    void removeStyleAll()                                              { lv_obj_remove_style_all(obj); }
    void del()                                                         { lv_obj_del(obj); }
    void clearChildren()                                               { lv_obj_clean(obj); }

    // ---- Utility ----
    void bringToFront()                                                { lv_obj_move_foreground(obj); }
    void sendToBack()                                                  { lv_obj_move_background(obj); }
    void invalidate()                                                  { lv_obj_invalidate(obj); }
    lv_coord_t getWidth() const                                        { return lv_obj_get_width(obj); }
    lv_coord_t getHeight() const                                       { return lv_obj_get_height(obj); }
    lv_coord_t getX() const                                            { return lv_obj_get_x(obj); }
    lv_coord_t getY() const                                            { return lv_obj_get_y(obj); }

    // ---- Type conversion ----
    operator lv_obj_t*() { return obj; }
    operator const lv_obj_t*() const { return obj; }
};
