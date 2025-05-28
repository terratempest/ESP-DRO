#pragma once

#include "./lv_helpers/LVHelpers.h" // Import all your wrapper widgets

struct DebugScreenPtrs {
    LVPanel*         panel;
    LVFlexContainer* flex;
    LVLabel*         label;
    LVButton*        button;
    LVDropdown*      dropdown;
    LVCheckbox*      checkbox;
    LVSwitch*        sw;
    LVTextarea*      textarea;
    LVKeyboard*      keyboard;
    // Add more as needed
};

inline void buildDebugScreen(DebugScreenPtrs& ui) {
    // Main panel (fills screen)
    ui.panel = new LVPanel(lv_scr_act(), [](auto& p) {
        p.setSize(lv_pct(100), lv_pct(100));
        p.setPadAll(12);
    });

    // Flex container to hold widgets
    ui.flex = new LVFlexContainer(ui.panel->obj, [](auto& f) {
        f.setFlexFlow(LV_FLEX_FLOW_COLUMN);
        f.setPadGap(12);
        f.setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    });

    // Label
    ui.label = new LVLabel(ui.flex->obj, "Debug Label", [](auto& l) {
        l.setTextColor(lv_palette_main(LV_PALETTE_RED));
        l.setFont(LV_FONT_DEFAULT);
    });

    // Button
    ui.button = new LVButton(ui.flex->obj, "Test Button", [](auto& b) {
        b.setSize(120, 40);
        b.onClick([&]{
            ui.label->setText("Button Clicked!");
        });
    });

    // Dropdown
    ui.dropdown = new LVDropdown(ui.flex->obj, [](auto& dd) {
        dd.setWidth(150);
        dd.setOptions({"Item 1", "Item 2", "Item 3"});
        dd.onChanged([&](uint16_t idx, std::string text){
            ui.label->setText(("Dropdown: " + text).c_str());
        });
    });

    // Checkbox
    ui.checkbox = new LVCheckbox(ui.flex->obj, "Check me", [](auto& cb) {
        cb.onClick([&]{
            ui.label->setText(cb.isChecked() ? "Checked" : "Unchecked");
        });
    });

    // Switch
    ui.sw = new LVSwitch(ui.flex->obj, [](auto& sw) {
        sw.setChecked(false);
        sw.onClick([&]{
            ui.label->setText(sw.isChecked() ? "Switch ON" : "Switch OFF");
        });
    });

    // Textarea
    ui.textarea = new LVTextarea(ui.flex->obj, "", [](auto& ta) {
        ta.setWidth(200);
        ta.setHeight(40);
    });

    // Keyboard (optional: hidden by default, shown on textarea focus)
    ui.keyboard = new LVKeyboard(lv_scr_act(), [](auto& kb) {
        kb.setVisible(false);
    });

    // Show keyboard on textarea focus
    lv_obj_add_event_cb(ui.textarea->obj, [](lv_event_t* e) {
        DebugScreenPtrs* ui = static_cast<DebugScreenPtrs*>(lv_event_get_user_data(e));
        ui->keyboard->setVisible(true);
        ui->keyboard->setTextarea(ui->textarea->obj);
    }, LV_EVENT_FOCUSED, &ui);

    // Hide keyboard on "Ready" or "Cancel"
    lv_obj_add_event_cb(ui.keyboard->obj, [](lv_event_t* e) {
        DebugScreenPtrs* ui = static_cast<DebugScreenPtrs*>(lv_event_get_user_data(e));
        lv_event_code_t code = lv_event_get_code(e);
        if(code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
            ui->keyboard->setVisible(false);
        }
    }, LV_EVENT_ALL, &ui);
}
