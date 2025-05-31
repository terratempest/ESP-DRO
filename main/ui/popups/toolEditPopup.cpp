#include "toolEditPopup.h"
#include "lv_helpers/LVHelpers.h"

// The UI popup for renaming a tool.
void ToolEditPopup::show(ToolManager& toolManager, int toolIndex, std::function<void()> onDone) {
    std::string oldName = toolManager.getTool(toolIndex).name;

    // Full-screen overlay
    auto* overlay = new LVContainer(lv_scr_act(), [&](auto& p) {
        p.setSize(LCD_H_RES, LCD_V_RES);
        p.setBgColor(COLOR_BLACK);
        p.setBgOpa(LV_OPA_50);
        p.setAlign(LV_ALIGN_CENTER);
    });

    // Top Frame
    auto* topFrame = new LVContainer(overlay->obj, [&](auto& f){
        f.setSize(LCD_H_RES, LCD_V_RES*2/3);
        f.setAlign(LV_ALIGN_TOP_LEFT);
    });

    // Panel
    auto* panel = new LVPanel(topFrame->obj, [&](auto& p){
        p.setSize(320, 200);
        p.setBgColor(COLOR_BLACK);
        p.setRadius(10);
        p.setPadAll(0);
        p.setAlign(LV_ALIGN_CENTER);
        p.setScrollable(false);
    });

    // Title
    new LVLabel(panel->obj, "Rename Tool", [&](auto& l){
        l.setAlign(LV_ALIGN_TOP_MID, 0, 8);
    });

    // Textarea
    auto* ta = new LVTextarea(panel->obj, oldName, [&](auto& t){
        t.setWidthPercent(70);
        t.setHeight(40);
        t.setAlign(LV_ALIGN_CENTER, 0, 0);
        t.setRadius(0);
        t.setOneLine(true);
        t.focus();
    });

    // On-screen keyboard
    auto* kb = new LVKeyboard(overlay->obj, [&](auto& k){
        k.setTextarea(ta->obj);
        k.setWidthPercent(100);
        k.setHeight(LCD_V_RES/3);
        k.setAlign(LV_ALIGN_BOTTOM_MID, 0, 0);
        k.onEvent(LV_EVENT_READY, [&, toolIndex, ta, overlay, onDone]{
            std::string newName = ta->getText();
            toolManager.renameTool(toolIndex, newName);
            overlay->del();
            if (onDone) onDone();
        });
        k.onEvent(LV_EVENT_CANCEL, [=]{
            overlay->del();
            if (onDone) onDone();
        });
    });

    // Cancel Button
    new LVButton(panel->obj, "Cancel", [=](auto& b){
        b.setSize(80, 32);
        b.setAlign(LV_ALIGN_BOTTOM_RIGHT, -24, -12);
        b.onClick([=]{
            overlay->del();
            if (onDone) onDone();
        });
        b.setBgColor(COLOR_GREY);
    });
}
