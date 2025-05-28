#include "config.h"

// Core modules
#include "dro_axis.h"
#include "tool_manager.h"
#include "preferences_wrapper.h"
#include "app_display.h"

// UI
#include "ui_manager.h"

// Platform/HAL
#include "hardware_abstraction.h"
#include "interrupts.h"

// LVGL
extern "C" {
    #include <lvgl.h>
}

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include <string>
#include <iostream>

// ------------ Hardware/Global Objects ------------

HardwareAbstraction hw;

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

    // Setup Axes
    for (int i = 0; i < AXES_COUNT; ++i) {
        DROAxes[i].init(AXES[i].name, AXES[i].pinA, AXES[i].pinB, AXES[i].resolution_um);
        // Grab preferences if available
        DROAxes[i].setStepUm(prefsWrapper.getFloat(std::string(DROAxes[i].name) + "_calib"));
        DROAxes[i].setInvert(prefsWrapper.getBool(std::string(DROAxes[i].name) + "_invert"));
    }
    setupAxisInterrupts(hw, DROAxes, AXES_COUNT);

    // LVGL init
    lv_init();
    app_display_init();
    
    toolManager.loadTools(); // Load tools and offsets from preferences   

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

    // Main loop (run forever)
    while (1) {
        lv_timer_handler(); // Handles all LVGL tasks
        vTaskDelay(pdMS_TO_TICKS(5)); // 5ms delay

        // Update UI with latest axis/tool states
        if (uiManager) {
            uiManager->updateDisplay();
        }

    }
}

// LVGL tick handler for periodic timer
void lv_tick_handler(void* arg) {
    lv_tick_inc(5);
}
