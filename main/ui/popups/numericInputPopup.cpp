#include "numericInputPopup.h"
#include "lv_helpers/LVHelpers.h" 
#include "config.h"

void NumericInputPopup::show(const std::string& title, Callback onComplete, float initialValue) {
    auto* overlay = new LVContainer(lv_scr_act(), [](auto& p){
        p.setSize(LCD_H_RES, LCD_V_RES);
        p.setTransparentBg();
        p.setFlag(LV_OBJ_FLAG_CLICKABLE);
        p.setPadAll(0);
    });

    auto* rightCont = new LVContainer(overlay->obj, [&](auto& r) {
        r.setSize(LCD_H_RES*3/7, LCD_V_RES);
        r.setAlign(LV_ALIGN_TOP_RIGHT);
        r.setPadAll(16);
    });

    auto* panel = new LVPanel(rightCont->obj, [&](auto& p){
        p.setSizePercent(100, 100);
        p.setAlign(LV_ALIGN_CENTER);
        p.setBgColor(lv_color_black());
        p.setBgOpa(LV_OPA_COVER);
        p.setBorderColor(lv_color_white());
        p.setBorderWidth(3);
        p.setRadius(10);
        p.setPadAll(0);
    });

    auto* titleBar = new LVContainer(panel->obj, [](auto& p){
        p.setSizePercent(100, 10);
        p.setPadAll(8);
    });

    new LVLabel(titleBar->obj, title.c_str(), [](auto& l){
        l.setAlign(LV_ALIGN_CENTER);
        l.setTextColor(lv_color_white());
    });

    auto* closeBtn = new LVButton(titleBar->obj, LV_SYMBOL_CLOSE, [&](auto& b){
        b.setSize(30, 30);
        b.setAlign(LV_ALIGN_RIGHT_MID);
        b.setBgColor(COLOR_BUTTON);
        b.onClick([=]{
            onComplete(0.0f, true);
            overlay->del();
        });
    });

    auto* ta = new LVTextarea(panel->obj, "", [&](auto& t){
        t.setSizePercent(100, 10);
        t.setOneLine(true);
        t.setRadius(0);
        t.setAlign(LV_ALIGN_TOP_MID, 0, titleBar->getHeight());
    });

    auto* kb = new LVKeyboardNumeric(panel->obj, [&](auto& k){
        k.setSizePercent(100, 80);
        k.setAlign(LV_ALIGN_BOTTOM_MID, 0, 0);
        k.setTextarea(ta->obj);
        k.onEvent(LV_EVENT_READY, [=]{
            float val = strtof(ta->getText().c_str(), nullptr);
            onComplete(val, false);
            overlay->del();
        });
        k.onEvent(LV_EVENT_CANCEL, [=]{
            onComplete(0.0f, true);
            overlay->del();
        });
    });

    ta->focus();
}
