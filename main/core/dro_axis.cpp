#include "dro_axis.h"

#include "config.h"
#include <driver/gpio.h>
#include <esp_attr.h>
#include <esp_check.h>
#include <esp_log.h>

static const char* TAG = "DroAxis";

DroAxis::DroAxis()
    : name("init"), pinA(-1), pinB(-1), step_um(1.0f), position_um(0.0f),
      pcnt_unit(PCNT_UNIT_0), accumulated(0), invertAxis(false), configured(false) {}

void DroAxis::init(const char* axisName, int axisPinA, int axisPinB, float axisStepUm, pcnt_unit_t unit) {
    name = axisName;
    pinA = axisPinA;
    pinB = axisPinB;
    step_um = axisStepUm;
    pcnt_unit = unit;
    accumulated.store(0, std::memory_order_relaxed);
    invertAxis = false;
    position_um = 0.0f;
    configured = false;
}

void DroAxis::configureHardware() {
    pcnt_config_t cfg0 = {
        .pulse_gpio_num = pinA,
        .ctrl_gpio_num  = pinB,
        .lctrl_mode     = invertAxis ? PCNT_MODE_KEEP    : PCNT_MODE_REVERSE,
        .hctrl_mode     = invertAxis ? PCNT_MODE_REVERSE : PCNT_MODE_KEEP,
        .pos_mode       = PCNT_COUNT_INC,
        .neg_mode       = PCNT_COUNT_DEC,
        .counter_h_lim  = PCNT_H_LIM_VAL,
        .counter_l_lim  = PCNT_L_LIM_VAL,
        .unit           = pcnt_unit,
        .channel        = PCNT_CHANNEL_0,
    };
    ESP_ERROR_CHECK(pcnt_unit_config(&cfg0));

    pcnt_config_t cfg1 = {
        .pulse_gpio_num = pinB,
        .ctrl_gpio_num  = pinA,
        .lctrl_mode     = invertAxis ? PCNT_MODE_REVERSE : PCNT_MODE_KEEP,
        .hctrl_mode     = invertAxis ? PCNT_MODE_KEEP    : PCNT_MODE_REVERSE,
        .pos_mode       = PCNT_COUNT_INC,
        .neg_mode       = PCNT_COUNT_DEC,
        .counter_h_lim  = PCNT_H_LIM_VAL,
        .counter_l_lim  = PCNT_L_LIM_VAL,
        .unit           = pcnt_unit,
        .channel        = PCNT_CHANNEL_1,
    };
    ESP_ERROR_CHECK(pcnt_unit_config(&cfg1));

    ESP_ERROR_CHECK(pcnt_set_filter_value(pcnt_unit, 100));
    ESP_ERROR_CHECK(pcnt_filter_enable(pcnt_unit));
}

void DroAxis::begin() {
    configureHardware();

    ESP_ERROR_CHECK(pcnt_counter_pause(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_counter_clear(pcnt_unit));
    accumulated.store(0, std::memory_order_relaxed);
    ESP_ERROR_CHECK(pcnt_counter_resume(pcnt_unit));
    configured = true;

    ESP_LOGI(TAG, "PCNT unit %d configured for axis '%s' (pinA=%d, pinB=%d)",
             pcnt_unit, name, pinA, pinB);
}

void DroAxis::zero() {
    portENTER_CRITICAL(&counterMux);
    ESP_ERROR_CHECK(pcnt_counter_pause(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_counter_clear(pcnt_unit));
    accumulated.store(0, std::memory_order_relaxed);
    position_um = 0.0f;
    ESP_ERROR_CHECK(pcnt_counter_resume(pcnt_unit));
    portEXIT_CRITICAL(&counterMux);
}

void DroAxis::setStepUm(float step) {
    if (step > 0.0f) {
        step_um = step;
    }
}

float DroAxis::getStepUm() const {
    return step_um;
}

void DroAxis::setInvert(bool invert) {
    if (invertAxis == invert) {
        return;
    }

    float preservedPosition = 0.0f;
    if (configured) {
        preservedPosition = getPositionUm();
    }

    invertAxis = invert;

    if (configured) {
        ESP_ERROR_CHECK(pcnt_counter_pause(pcnt_unit));
        configureHardware();
        ESP_ERROR_CHECK(pcnt_counter_clear(pcnt_unit));
        accumulated.store(static_cast<int64_t>(preservedPosition / step_um), std::memory_order_relaxed);
        ESP_ERROR_CHECK(pcnt_counter_resume(pcnt_unit));
    }
}

bool DroAxis::getInvert() const {
    return invertAxis;
}

int64_t DroAxis::snapshotCounts() {
    int16_t hwCount = 0;
    portENTER_CRITICAL(&counterMux);
    ESP_ERROR_CHECK(pcnt_get_counter_value(pcnt_unit, &hwCount));
    if (hwCount != 0) {
        accumulated.fetch_add(hwCount, std::memory_order_relaxed);
        ESP_ERROR_CHECK(pcnt_counter_clear(pcnt_unit));
    }
    int64_t total = accumulated.load(std::memory_order_relaxed);
    portEXIT_CRITICAL(&counterMux);
    return total;
}

void DroAxis::setPositionUm(float newPos) {
    position_um = newPos;
    portENTER_CRITICAL(&counterMux);
    ESP_ERROR_CHECK(pcnt_counter_pause(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_counter_clear(pcnt_unit));
    accumulated.store(static_cast<int64_t>(newPos / step_um), std::memory_order_relaxed);
    ESP_ERROR_CHECK(pcnt_counter_resume(pcnt_unit));
    portEXIT_CRITICAL(&counterMux);
}

float DroAxis::getPositionUm() {
    return static_cast<float>(snapshotCounts()) * step_um;
}

void DroAxis::simulateStep(bool forward) {
    portENTER_CRITICAL(&counterMux);
    accumulated.fetch_add(forward ? 1 : -1, std::memory_order_relaxed);
    portEXIT_CRITICAL(&counterMux);
}
