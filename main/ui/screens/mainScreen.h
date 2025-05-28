#pragma once
#include "./lv_helpers/LVHelpers.h"

// Forward declare global pointers if you need to access/update them elsewhere
struct MainScreenPtrs {
    LVContainer* main;
    LVContainer* title;
    LVLabel*     titleLabel;
    LVButton*    sleepBtn;
    LVLabel*     sleepBtnLabel;
    LVContainer* contents;
    LVContainer* axisContainer;
    LVPanel*     axis1;
    LVLabel*     axis1Label;
    LVLabel*     axis1Ghost;
    LVLabel*     axis1DRO;
    LVButton*    axis1Zero;
    LVLabel*     axis1ZeroLabel;
    LVPanel*     axis2;
    LVLabel*     axis2Label;
    LVLabel*     axis2Ghost;
    LVLabel*     axis2DRO;
    LVButton*    axis2Zero;
    LVLabel*     axis2ZeroLabel;
    LVContainer* rightPanel;
    LVFlexContainer* funcFlex;
    LVButton*    funcBtnUnits; 
    LVButton*    funcBtnDia; 
    LVButton*    funcBtn3; 
    LVButton*    funcBtnSettings; 
    LVFlexContainer* toolFlex;
    LVDropdown*  toolDropdown;
    LVButton*    toolBtnEdit; 
    LVButton*    toolBtnAdd; 
    LVButton*    toolBtnRemove; 
};

inline void buildMainScreen(MainScreenPtrs& ui) {
    // Main background container
    ui.main = new LVContainer(lv_scr_act(), [&](auto& c) {
        c.setSize(LCD_H_RES, LCD_V_RES);
        c.clearFlag(LV_OBJ_FLAG_SCROLLABLE);
        c.setPadding(0,0,0,0);
    });

    // Title bar
    ui.title = new LVContainer(ui.main->obj, [&](auto& bar) {
        bar.setHeight(40);
        bar.setWidth(LCD_H_RES);
        bar.clearFlag(LV_OBJ_FLAG_CLICKABLE);
        bar.clearFlag(LV_OBJ_FLAG_SCROLLABLE);
        bar.setBgColor(COLOR_BLACK);
        bar.setBgOpa(255);
        bar.setPadding(8,8,0,0);
    });

    ui.titleLabel = new LVLabel(ui.title->obj, "DRO", [&](auto& lbl){
        lbl.setAlign(LV_ALIGN_LEFT_MID);
        lbl.setTextColor(COLOR_YELLOW_GREEN);
    });

    ui.sleepBtn = new LVButton(ui.title->obj, LV_SYMBOL_POWER, [&](auto& btn){
        btn.setSize(32, 32);
        btn.setAlign(LV_ALIGN_RIGHT_MID);
        btn.setBgColor(COLOR_GRAY);
        btn.setRadius(16);
    });

    // Main contents panel
    ui.contents = new LVContainer(ui.main->obj, [&](auto& c) {
        c.setHeight(440);
        c.setWidth(LCD_H_RES);
        c.setPos(0, 40);
        c.setPadAll(0);
        c.clearFlag(LV_OBJ_FLAG_CLICKABLE);
        c.clearFlag(LV_OBJ_FLAG_SCROLLABLE);
    });

    // Axis/left panel (vertical split)
    ui.axisContainer = new LVContainer(ui.contents->obj, [&](auto& c){
        c.setWidthPercent(55);
        c.setHeight(440);
        c.setPadding(8,8,8,8);
    });

    // Axis 1 (X)
    ui.axis1 = new LVPanel(ui.axisContainer->obj, [&](auto& c){
        c.setHeight(84);
        c.setWidthPercent(100);
        c.setBgColor(COLOR_BLACK);
    });
    ui.axis1Label = new LVLabel(ui.axis1->obj, "X", [&](auto& lbl){
        lbl.setAlign(LV_ALIGN_LEFT_MID);
        lbl.setTextColor(COLOR_YELLOW_GREEN);
        //lbl.setFont(&LV_FONT_MONTSERRAT_32);
    });
    ui.axis1Ghost = new LVLabel(ui.axis1->obj, "88888.8888", [&](auto& lbl){
        lbl.setAlign(LV_ALIGN_RIGHT_MID, -80, 0);
        lbl.setFont(&lv_font_7seg_64);
        lbl.setTextColor(COLOR_DARK_GRAY);
    });
    ui.axis1DRO = new LVLabel(ui.axis1->obj, "167.2840", [&](auto& lbl){
        lbl.setAlign(LV_ALIGN_RIGHT_MID, -80, 0);
        lbl.setFont(&lv_font_7seg_64);
        lbl.setTextColor(COLOR_YELLOW_GREEN);
        lbl.setFlag(LV_OBJ_FLAG_CLICKABLE);
    });
    ui.axis1Zero = new LVButton(ui.axis1->obj, "ZERO", [&](auto& btn){
        btn.setSize(64, 32);
        btn.setAlign(LV_ALIGN_RIGHT_MID);
        btn.setRadius(6);
        btn.setBgColor(COLOR_GRAY);
    });

    // Axis 2 (Z)
    ui.axis2 = new LVPanel(ui.axisContainer->obj, [&](auto& c){
        c.setHeight(84);
        c.setWidthPercent(100);
        c.setPos(0, 100);
        c.setBgColor(COLOR_BLACK);
    });
    ui.axis2Label = new LVLabel(ui.axis2->obj, "Z", [&](auto& lbl){
        lbl.setAlign(LV_ALIGN_LEFT_MID);
        lbl.setTextColor(COLOR_YELLOW_GREEN);
        //lbl.setFont(&LV_FONT_MONTSERRAT_32);
    });
    ui.axis2Ghost = new LVLabel(ui.axis2->obj, "88888.8888", [&](auto& lbl){
        lbl.setAlign(LV_ALIGN_RIGHT_MID, -80, 0);
        lbl.setFont(&lv_font_7seg_64);
        lbl.setTextColor(COLOR_DARK_GRAY);
    });
    ui.axis2DRO = new LVLabel(ui.axis2->obj, "167.2840", [&](auto& lbl){
        lbl.setAlign(LV_ALIGN_RIGHT_MID, -80, 0);
        lbl.setFont(&lv_font_7seg_64);
        lbl.setTextColor(COLOR_YELLOW_GREEN);
        lbl.setFlag(LV_OBJ_FLAG_CLICKABLE);
    });
    ui.axis2Zero = new LVButton(ui.axis2->obj, "ZERO", [&](auto& btn){
        btn.setSize(64, 32);
        btn.setAlign(LV_ALIGN_RIGHT_MID);
        btn.setRadius(6);
        btn.setBgColor(COLOR_GRAY);
    });

    // Right panel container (function buttons)
    ui.rightPanel = new LVContainer(ui.contents->obj, [&](auto& c){
        c.setWidth(LCD_H_RES * 0.45);
        c.setHeight(440);
        c.setAlign(LV_ALIGN_TOP_RIGHT);
        c.setPadding(8,8,8,8);
    });

    // Flex layout for function buttons
    ui.funcFlex = new LVFlexContainer(ui.rightPanel->obj, [&](auto& flex){
        flex.setFlexFlow(LV_FLEX_FLOW_ROW_WRAP);
        flex.setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        flex.setSizePercent(100, 100);
        flex.setBgColor(COLOR_BLACK);
        // Add 4 function buttons
        ui.funcBtnUnits =    new LVButton(flex.obj, "IN", [&](auto& b){ b.setSize(64,64); b.setBgColor(COLOR_GRAY);});
        ui.funcBtnDia =      new LVButton(flex.obj, "DIA", [&](auto& b){ b.setSize(64,64); b.setBgColor(COLOR_GRAY);});
        ui.funcBtn3 =        new LVButton(flex.obj, "", [&](auto& b){ b.setSize(64,64); b.setBgColor(COLOR_GRAY);});
        ui.funcBtnSettings = new LVButton(flex.obj, LV_SYMBOL_SETTINGS, [&](auto& b){ b.setSize(64,64); b.setBgColor(COLOR_GRAY);});
    });

    // Tool area (flex row)
    ui.toolFlex = new LVFlexContainer(ui.funcFlex->obj, [&](auto& flex){
        flex.removeStyleAll();
        flex.setFlexFlow(LV_FLEX_FLOW_ROW);
        flex.setPadGap(8);
        flex.setAlign(LV_ALIGN_CENTER);
        flex.setSizeContent();
        // Tool dropdown + 3 tool buttons
        ui.toolDropdown  = new LVDropdown(flex.obj, [&](auto& dd) {dd.setWidth(150);});
        ui.toolBtnEdit   = new LVButton(flex.obj, LV_SYMBOL_EDIT,  [&](auto& b){ b.setSize(40,40); b.setBgColor(COLOR_GRAY);});
        ui.toolBtnAdd    = new LVButton(flex.obj, LV_SYMBOL_PLUS,  [&](auto& b){ b.setSize(40,40); b.setBgColor(COLOR_GRAY);});
        ui.toolBtnRemove = new LVButton(flex.obj, LV_SYMBOL_TRASH, [&](auto& b){ b.setSize(40,40); b.setBgColor(COLOR_GRAY);});
    });
}
