#include "dro_axis.h"

#include "config.h"
#include <driver/gpio.h>
#include <esp_check.h>
#include <esp_log.h>

static const char* TAG = "DroAxis";
static constexpr uint32_t PCNT_GLITCH_FILTER_NS = 1250;

DroAxis::DroAxis()
    : name("init"), pinA(-1), pinB(-1), step_um(1.0f), position_um(0.0f),
      unitIndex(0), accumulated(0), invertAxis(false), configured(false) {}

void DroAxis::init(const char* axisName, int axisPinA, int axisPinB, float axisStepUm, int axisUnitIndex) {
    releaseHardware();

    name = axisName;
    pinA = axisPinA;
    pinB = axisPinB;
    step_um = axisStepUm;
    unitIndex = axisUnitIndex;
    accumulated.store(0, std::memory_order_relaxed);
    invertAxis = false;
    position_um = 0.0f;
    configured = false;
}

void DroAxis::configureHardware() {
    pcnt_unit_config_t unitConfig = {
        .low_limit = PCNT_L_LIM_VAL,
        .high_limit = PCNT_H_LIM_VAL,
        .intr_priority = 0,
        .flags = {
            .accum_count = 0,
        },
    };
    ESP_ERROR_CHECK(pcnt_new_unit(&unitConfig, &pcntUnit));

    pcnt_glitch_filter_config_t filterConfig = {
        .max_glitch_ns = PCNT_GLITCH_FILTER_NS,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcntUnit, &filterConfig));

    pcnt_chan_config_t channelAConfig = {
        .edge_gpio_num = pinA,
        .level_gpio_num = pinB,
    };
    ESP_ERROR_CHECK(pcnt_new_channel(pcntUnit, &channelAConfig, &channelA));

    pcnt_chan_config_t channelBConfig = {
        .edge_gpio_num = pinB,
        .level_gpio_num = pinA,
    };
    ESP_ERROR_CHECK(pcnt_new_channel(pcntUnit, &channelBConfig, &channelB));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        channelA,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE,
        PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(
        channelA,
        invertAxis ? PCNT_CHANNEL_LEVEL_ACTION_INVERSE : PCNT_CHANNEL_LEVEL_ACTION_KEEP,
        invertAxis ? PCNT_CHANNEL_LEVEL_ACTION_KEEP : PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        channelB,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE,
        PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(
        channelB,
        invertAxis ? PCNT_CHANNEL_LEVEL_ACTION_KEEP : PCNT_CHANNEL_LEVEL_ACTION_INVERSE,
        invertAxis ? PCNT_CHANNEL_LEVEL_ACTION_INVERSE : PCNT_CHANNEL_LEVEL_ACTION_KEEP));

    ESP_ERROR_CHECK(pcnt_unit_enable(pcntUnit));
}

void DroAxis::releaseHardware() {
    if (!pcntUnit) {
        channelA = nullptr;
        channelB = nullptr;
        return;
    }

    if (configured) {
        ESP_ERROR_CHECK(pcnt_unit_stop(pcntUnit));
    }
    ESP_ERROR_CHECK(pcnt_unit_disable(pcntUnit));

    if (channelA) {
        ESP_ERROR_CHECK(pcnt_del_channel(channelA));
        channelA = nullptr;
    }
    if (channelB) {
        ESP_ERROR_CHECK(pcnt_del_channel(channelB));
        channelB = nullptr;
    }

    ESP_ERROR_CHECK(pcnt_del_unit(pcntUnit));
    pcntUnit = nullptr;
    configured = false;
}

void DroAxis::begin() {
    configureHardware();

    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcntUnit));
    accumulated.store(0, std::memory_order_relaxed);
    ESP_ERROR_CHECK(pcnt_unit_start(pcntUnit));
    configured = true;

    ESP_LOGI(TAG, "PCNT unit %d configured for axis '%s' (pinA=%d, pinB=%d)",
             unitIndex, name, pinA, pinB);
}

void DroAxis::zero() {
    if (configured) {
        ESP_ERROR_CHECK(pcnt_unit_stop(pcntUnit));
        ESP_ERROR_CHECK(pcnt_unit_clear_count(pcntUnit));
    }
    accumulated.store(0, std::memory_order_relaxed);
    position_um = 0.0f;
    if (configured) {
        ESP_ERROR_CHECK(pcnt_unit_start(pcntUnit));
    }
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
        releaseHardware();
        configureHardware();
        accumulated.store(static_cast<int64_t>(preservedPosition / step_um), std::memory_order_relaxed);
        ESP_ERROR_CHECK(pcnt_unit_clear_count(pcntUnit));
        ESP_ERROR_CHECK(pcnt_unit_start(pcntUnit));
        configured = true;
    }
}

bool DroAxis::getInvert() const {
    return invertAxis;
}

int64_t DroAxis::snapshotCounts() {
    if (!configured) {
        return accumulated.load(std::memory_order_relaxed);
    }

    int hwCount = 0;
    ESP_ERROR_CHECK(pcnt_unit_get_count(pcntUnit, &hwCount));
    if (hwCount != 0) {
        accumulated.fetch_add(hwCount, std::memory_order_relaxed);
        ESP_ERROR_CHECK(pcnt_unit_clear_count(pcntUnit));
    }
    return accumulated.load(std::memory_order_relaxed);
}

void DroAxis::setPositionUm(float newPos) {
    position_um = newPos;
    if (configured) {
        ESP_ERROR_CHECK(pcnt_unit_stop(pcntUnit));
        ESP_ERROR_CHECK(pcnt_unit_clear_count(pcntUnit));
    }
    accumulated.store(static_cast<int64_t>(newPos / step_um), std::memory_order_relaxed);
    if (configured) {
        ESP_ERROR_CHECK(pcnt_unit_start(pcntUnit));
    }
}

float DroAxis::getPositionUm() {
    return static_cast<float>(snapshotCounts()) * step_um;
}

void DroAxis::simulateStep(bool forward) {
    accumulated.fetch_add(forward ? 1 : -1, std::memory_order_relaxed);
}
