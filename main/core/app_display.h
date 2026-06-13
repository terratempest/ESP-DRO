#pragma once
#include <stdint.h>

extern "C" {
    #include <lvgl.h>
}

void app_display_init();
uint32_t app_display_get_last_touch_ms();


