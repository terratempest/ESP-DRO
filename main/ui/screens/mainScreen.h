#pragma once
#include "./lv_helpers/LVHelpers.h"
#include "popups/numericInputPopup.h"
#include "popups/settingsPopup.h"
#include "popups/toolEditPopup.h"
#include "settings/settingsMenu.h"
#include <vector>
#include "dro_axis.h"  

struct MainScreenPtrs {
    LVContainer* main = nullptr;
    LVContainer* title = nullptr;
    LVLabel*     titleLabel = nullptr;
    LVButton*    sleepBtn = nullptr;
    LVLabel*     sleepBtnLabel = nullptr;
    LVContainer* contents = nullptr;
    LVContainer* axisContainer = nullptr;

    // Per-axis containers
    std::vector<LVPanel*> axisPanels;
    std::vector<LVLabel*> axisLabels;
    std::vector<LVLabel*> axisGhosts;
    std::vector<LVLabel*> axisDROs;
    std::vector<LVButton*> axisZeroBtns;
    std::vector<LVButton*> axisTLOBtns;

    LVContainer* rightPanel = nullptr;
    LVFlexContainer* funcFlex = nullptr;
    LVButton*    funcBtnUnits = nullptr;
    LVButton*    funcBtnDia = nullptr;
    LVButton*    funcBtnAbsIncr = nullptr;
    LVButton*    funcBtnSettings = nullptr;

    LVFlexContainer* toolFlex = nullptr;
    LVDropdown*  toolDropdown = nullptr;
    LVButton*    toolBtnEdit = nullptr;
    LVButton*    toolBtnAdd = nullptr;
    LVButton*    toolBtnRemove = nullptr;
};

// Pass DroAxis* axes and axis_count!
inline void buildMainScreen(MainScreenPtrs& ui, DroAxis* axes, int axis_count) {
    ui.main = new LVContainer(lv_scr_act(), [&](auto& c) {
        c.setSize(LCD_H_RES, LCD_V_RES);
        c.clearFlag(LV_OBJ_FLAG_SCROLLABLE);
        c.setPadding(0,0,0,0);
    });

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

    ui.contents = new LVContainer(ui.main->obj, [&](auto& c) {
        c.setHeight(440);
        c.setWidth(LCD_H_RES);
        c.setPos(0, 40);
        c.setPadAll(0);
        c.clearFlag(LV_OBJ_FLAG_CLICKABLE);
        c.clearFlag(LV_OBJ_FLAG_SCROLLABLE);
    });

    ui.axisContainer = new LVContainer(ui.contents->obj, [&](auto& c){
        c.setWidthPercent(55);
        c.setHeight(440);
        c.setPadding(8,8,8,8);
    });

    // ---- DYNAMIC AXIS WIDGETS ----
    for (int i = 0; i < axis_count; ++i) {
        // Panel for this axis
        auto panel = new LVPanel(ui.axisContainer->obj, [&](auto& c){
            c.setHeight(84);
            c.setWidthPercent(100);
            c.setPos(0, i * 100);
            c.setBgColor(COLOR_BLACK);
            c.setScrollable(false);
        });

        // *** Get axis name directly from DroAxis ***
        const char* axis_name = axes[i].name; // Or axes[i].getName().c_str() if accessor

        auto label = new LVLabel(panel->obj, axis_name, [&](auto& lbl){
            lbl.setAlign(LV_ALIGN_LEFT_MID);
            lbl.setTextColor(COLOR_YELLOW_GREEN);
        });

        auto ghost = new LVLabel(panel->obj, "88888.8888", [&](auto& lbl){
            lbl.setAlign(LV_ALIGN_RIGHT_MID, -80, 0);
            lbl.setFont(&lv_font_7seg_64);
            lbl.setTextColor(COLOR_DARK_GRAY);
        });

        auto dro = new LVLabel(panel->obj, "0.0000", [&](auto& lbl){
            lbl.setAlign(LV_ALIGN_RIGHT_MID, -80, 0);
            lbl.setFont(&lv_font_7seg_64);
            lbl.setTextColor(COLOR_YELLOW_GREEN);
            lbl.setFlag(LV_OBJ_FLAG_CLICKABLE);
        });

        auto zeroBtn = new LVButton(panel->obj, "ZERO", [&](auto& btn){
            btn.setSize(64, 32);
            btn.setAlign(LV_ALIGN_RIGHT_MID, 0, 18);
            btn.setRadius(6);
            btn.setBgColor(COLOR_GRAY);
        });

        auto tloBtn = new LVButton(panel->obj, "TLO", [&](auto& btn){
            btn.setSize(64, 32);
            btn.setAlign(LV_ALIGN_RIGHT_MID, 0, -18);
            btn.setRadius(6);
            btn.setBgColor(COLOR_GRAY);
        });

        ui.axisPanels.push_back(panel);
        ui.axisLabels.push_back(label);
        ui.axisGhosts.push_back(ghost);
        ui.axisDROs.push_back(dro);
        ui.axisZeroBtns.push_back(zeroBtn);
        ui.axisTLOBtns.push_back(tloBtn);
    }

    ui.rightPanel = new LVContainer(ui.contents->obj, [&](auto& c){
        c.setWidth(LCD_H_RES * 0.45);
        c.setHeight(440);
        c.setAlign(LV_ALIGN_TOP_RIGHT);
        c.setPadding(8,8,8,8);
    });

    ui.funcFlex = new LVFlexContainer(ui.rightPanel->obj, [&](auto& flex){
        flex.setFlexFlow(LV_FLEX_FLOW_ROW_WRAP);
        flex.setFlexAlign(LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        flex.setSizePercent(100, 100);
        flex.setBgColor(COLOR_BLACK);
        ui.funcBtnUnits =    new LVButton(flex.obj, "IN",  [&](auto& b){ b.setSize(64,64); b.setBgColor(COLOR_GRAY);});
        ui.funcBtnDia =      new LVButton(flex.obj, "DIA", [&](auto& b){ b.setSize(64,64); b.setBgColor(COLOR_GRAY);});
        ui.funcBtnAbsIncr =  new LVButton(flex.obj, "ABS", [&](auto& b){ b.setSize(64,64); b.setBgColor(COLOR_GRAY);});
        ui.funcBtnSettings = new LVButton(flex.obj, LV_SYMBOL_SETTINGS, [&](auto& b){ b.setSize(64,64); b.setBgColor(COLOR_GRAY);});
    });

    ui.toolFlex = new LVFlexContainer(ui.funcFlex->obj, [&](auto& flex){
        flex.removeStyleAll();
        flex.setFlexFlow(LV_FLEX_FLOW_ROW);
        flex.setPadGap(8);
        flex.setAlign(LV_ALIGN_CENTER);
        flex.setSizeContent();
        ui.toolDropdown  = new LVDropdown(flex.obj, [&](auto& dd) {dd.setWidth(150);});
        ui.toolBtnEdit   = new LVButton(flex.obj, LV_SYMBOL_EDIT,  [&](auto& b){ b.setSize(40,40); b.setBgColor(COLOR_GRAY);});
        ui.toolBtnAdd    = new LVButton(flex.obj, LV_SYMBOL_PLUS,  [&](auto& b){ b.setSize(40,40); b.setBgColor(COLOR_GRAY);});
        ui.toolBtnRemove = new LVButton(flex.obj, LV_SYMBOL_TRASH, [&](auto& b){ b.setSize(40,40); b.setBgColor(COLOR_GRAY);});
    });
}
