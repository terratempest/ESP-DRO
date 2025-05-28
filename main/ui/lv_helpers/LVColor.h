#pragma once
extern "C" {
    #include <lvgl.h>
}

static const lv_color_t COLOR_BLACK       = lv_color_black();
static const lv_color_t COLOR_YELLOW_GREEN= lv_color_make(0x39, 0xff, 0x14); // Green
static const lv_color_t COLOR_DARK_GRAY   = lv_color_make(0x08, 0x08, 0x08); // Dark gray
static const lv_color_t COLOR_GRAY        = lv_color_make(0x20, 0x20, 0x20); // Green (same as label)
static const lv_color_t COLOR_GREY        = COLOR_GRAY;
static const lv_color_t COLOR_WHITE       = lv_color_white();

static const lv_color_t COLOR_BUTTON = COLOR_GRAY;
static const lv_color_t COLOR_PANEL  = COLOR_BLACK;