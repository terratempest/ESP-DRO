#include "config.h"

// Core modules
#include "dro_axis.h"
#include "tool_manager.h"
#include "preferences_wrapper.h"
#include "app_display.h"

// UI
#include "ui_manager.h"

// LVGL
extern "C" {
    #include <lvgl.h>
}

#include "esp_log.h"
#include "esp_check.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include <string>
#include <iostream>
#include <driver/gpio.h>

// ------------ Hardware/Global Objects ------------

DroAxis DROAxes[AXES_COUNT];

PreferencesWrapper prefsWrapper(NVS_NAMESPACE);
ToolManager toolManager(prefsWrapper);

UIManager* uiManager = nullptr;

// Forward: LVGL tick glue
void lv_tick_handler(void* arg);

// ------------- Main Entry Point -------------

extern "C" void app_main() {
    // Logging
    esp_log_level_set("*", ESP_LOG_INFO);
    ESP_LOGI("MAIN", "DRO Firmware Starting...");

    esp_err_t nvsErr = nvs_flash_init();
    if (nvsErr == ESP_ERR_NVS_NO_FREE_PAGES || nvsErr == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvsErr = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvsErr);
    prefsWrapper.begin();
    toolManager.loadTools();

    // Setup Axes
    // In app_main, replace the axis init loop:
    for (int i = 0; i < AXES_COUNT; ++i) {
        DROAxes[i].init(
            AXES[i].name,
            AXES[i].pinA,
            AXES[i].pinB,
            AXES[i].resolution_um,
            static_cast<pcnt_unit_t>(i)  // PCNT_UNIT_0, PCNT_UNIT_1, ...
        );
        DROAxes[i].setStepUm(prefsWrapper.getFloat(std::string(DROAxes[i].name) + "_calib", AXES[i].resolution_um));
        DROAxes[i].setInvert(prefsWrapper.getBool(std::string(DROAxes[i].name) + "_invert", false));
        DROAxes[i].begin();
    }

    // LVGL init
    lv_init();
    app_display_init();

    // Setup UI
    uiManager = new UIManager(toolManager, DROAxes, prefsWrapper); 
    uiManager->init();

    // Set up LVGL tick (every 5ms) using esp_timer
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &lv_tick_handler,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "lv_tick"
    };
    esp_timer_handle_t lvgl_tick_timer;
    esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
    esp_timer_start_periodic(lvgl_tick_timer, 5000); // 5000us = 5ms

    ESP_LOGI("MAIN", "Setup complete!");

    uint32_t lastUiUpdateMs = 0;

    // Main loop (run forever)
    while (1) {
        lv_timer_handler(); // Handles all LVGL tasks
        vTaskDelay(pdMS_TO_TICKS(5)); // 5ms delay

        uint32_t nowMs = static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
        if (uiManager && nowMs - lastUiUpdateMs >= 33) {
            uiManager->updateDisplay();
            lastUiUpdateMs = nowMs;
        }

    }
}

// LVGL tick handler for periodic timer
void lv_tick_handler(void* arg) {
    lv_tick_inc(5);
}
