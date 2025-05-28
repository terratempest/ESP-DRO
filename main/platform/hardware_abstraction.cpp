#include "hardware_abstraction.h"
#include <driver/gpio.h>
#include <esp_timer.h>
#include "esp_attr.h"
#include <vector>
#include <utility>

// Store trampoline data to avoid memory leaks (could use a map or pool for production)
static std::vector<std::pair<gpio_isr_handle_t, std::pair<InterruptCallback, void*>>> isr_handles;

HardwareAbstraction::HardwareAbstraction() {}

// Internal trampoline for ESP-IDF ISR
static void IRAM_ATTR isrTrampoline(void* arg) {
    auto* pair = static_cast<std::pair<InterruptCallback, void*>*>(arg);
    pair->first(pair->second);
}

void HardwareAbstraction::attachInterrupt(int pin, InterruptCallback cb, void* context, int mode) {
    // Set GPIO as input and configure
    gpio_config_t io_conf = {};
    io_conf.intr_type = (gpio_int_type_t)mode;
    io_conf.pin_bit_mask = 1ULL << pin;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);

    // Prepare the callback/context pair
    auto* pair = new std::pair<InterruptCallback, void*>(cb, context);

    // Register ISR
    gpio_isr_handler_add((gpio_num_t)pin, isrTrampoline, pair);

    // Track handles for later removal
    isr_handles.emplace_back(nullptr, *pair);
}

void HardwareAbstraction::detachInterrupt(int pin) {
    gpio_isr_handler_remove((gpio_num_t)pin);
}

// Return current time in milliseconds
uint32_t HardwareAbstraction::millis() const {
    return static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
}

// Return current time in microseconds
uint32_t HardwareAbstraction::micros() const {
    return static_cast<uint32_t>(esp_timer_get_time());
}
