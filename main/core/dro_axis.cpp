#include "dro_axis.h"

#include "config.h"
#include <driver/gpio.h>
#include <esp_check.h>
#include <esp_err.h>
#include <esp_log.h>
#include <cmath>

static const char* TAG = "DroAxis";
static constexpr uint32_t PCNT_GLITCH_FILTER_NS = 1250;

DroAxis::DroAxis()
    : name("init"), pinA(-1), pinB(-1), step_um(1.0f), position_um(0.0f),
      unitIndex(0), countOffset(0), cachedCounts(0), invertAxis(false), configured(false) {}

void DroAxis::init(const char* axisName, int axisPinA, int axisPinB, float axisStepUm, int axisUnitIndex) {
    releaseHardware();

    name = axisName;
    pinA = axisPinA;
    pinB = axisPinB;
    step_um = axisStepUm;
    unitIndex = axisUnitIndex;
    countOffset.store(0, std::memory_order_relaxed);
    cachedCounts.store(0, std::memory_order_relaxed);
    invertAxis = false;
    position_um = 0.0f;
    configured = false;
}

bool DroAxis::applyChannelActions(bool desiredInvert) {
    esp_err_t err = pcnt_channel_set_level_action(
        channelA,
        desiredInvert ? PCNT_CHANNEL_LEVEL_ACTION_INVERSE : PCNT_CHANNEL_LEVEL_ACTION_KEEP,
        desiredInvert ? PCNT_CHANNEL_LEVEL_ACTION_KEEP : PCNT_CHANNEL_LEVEL_ACTION_INVERSE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set channel A direction for axis '%s': %s",
                 name, esp_err_to_name(err));
        return false;
    }

    err = pcnt_channel_set_level_action(
        channelB,
        desiredInvert ? PCNT_CHANNEL_LEVEL_ACTION_KEEP : PCNT_CHANNEL_LEVEL_ACTION_INVERSE,
        desiredInvert ? PCNT_CHANNEL_LEVEL_ACTION_INVERSE : PCNT_CHANNEL_LEVEL_ACTION_KEEP);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set channel B direction for axis '%s': %s",
                 name, esp_err_to_name(err));
        return false;
    }

    return true;
}

void DroAxis::configureHardware() {
    pcnt_unit_config_t unitConfig = {
        .low_limit = PCNT_L_LIM_VAL,
        .high_limit = PCNT_H_LIM_VAL,
        .flags = {
            .accum_count = 1,
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

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
        channelB,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE,
        PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(applyChannelActions(invertAxis) ? ESP_OK : ESP_FAIL);

    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcntUnit, PCNT_H_LIM_VAL));
    ESP_ERROR_CHECK(pcnt_unit_add_watch_point(pcntUnit, PCNT_L_LIM_VAL));
    ESP_ERROR_CHECK(pcnt_unit_enable(pcntUnit));
}

void DroAxis::releaseHardware() {
    if (!pcntUnit) {
        channelA = nullptr;
        channelB = nullptr;
        return;
    }

    if (configured) {
        esp_err_t err = pcnt_unit_stop(pcntUnit);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to stop PCNT unit for axis '%s': %s",
                     name, esp_err_to_name(err));
        }
    }
    esp_err_t err = pcnt_unit_disable(pcntUnit);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to disable PCNT unit for axis '%s': %s",
                 name, esp_err_to_name(err));
    }

    if (channelA) {
        err = pcnt_del_channel(channelA);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to delete channel A for axis '%s': %s",
                     name, esp_err_to_name(err));
        }
        channelA = nullptr;
    }
    if (channelB) {
        err = pcnt_del_channel(channelB);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to delete channel B for axis '%s': %s",
                     name, esp_err_to_name(err));
        }
        channelB = nullptr;
    }

    err = pcnt_del_unit(pcntUnit);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to delete PCNT unit for axis '%s': %s",
                 name, esp_err_to_name(err));
    }
    pcntUnit = nullptr;
    configured = false;
}

void DroAxis::begin() {
    configureHardware();

    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcntUnit));
    countOffset.store(0, std::memory_order_relaxed);
    cachedCounts.store(0, std::memory_order_relaxed);
    ESP_ERROR_CHECK(pcnt_unit_start(pcntUnit));
    configured = true;

    ESP_LOGI(TAG, "PCNT unit %d configured for axis '%s' (pinA=%d, pinB=%d)",
             unitIndex, name, pinA, pinB);
}

void DroAxis::zero() {
    if (configured) {
        esp_err_t err = pcnt_unit_stop(pcntUnit);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to stop PCNT unit while zeroing axis '%s': %s",
                     name, esp_err_to_name(err));
            return;
        }
        err = pcnt_unit_clear_count(pcntUnit);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to clear PCNT unit while zeroing axis '%s': %s",
                     name, esp_err_to_name(err));
            pcnt_unit_start(pcntUnit);
            return;
        }
    }
    countOffset.store(0, std::memory_order_relaxed);
    cachedCounts.store(0, std::memory_order_relaxed);
    position_um = 0.0f;
    if (configured) {
        esp_err_t err = pcnt_unit_start(pcntUnit);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to restart PCNT unit after zeroing axis '%s': %s",
                     name, esp_err_to_name(err));
        }
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

    if (!configured) {
        invertAxis = invert;
        return;
    }

    esp_err_t err = pcnt_unit_stop(pcntUnit);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to stop PCNT unit before inverting axis '%s': %s",
                 name, esp_err_to_name(err));
        return;
    }

    err = pcnt_unit_disable(pcntUnit);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to disable PCNT unit before inverting axis '%s': %s",
                 name, esp_err_to_name(err));
        pcnt_unit_start(pcntUnit);
        return;
    }

    if (!applyChannelActions(invert)) {
        applyChannelActions(invertAxis);
        pcnt_unit_enable(pcntUnit);
        pcnt_unit_start(pcntUnit);
        return;
    }

    err = pcnt_unit_enable(pcntUnit);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable PCNT unit after inverting axis '%s': %s",
                 name, esp_err_to_name(err));
        applyChannelActions(invertAxis);
        pcnt_unit_enable(pcntUnit);
        pcnt_unit_start(pcntUnit);
        return;
    }

    err = pcnt_unit_start(pcntUnit);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to restart PCNT unit after inverting axis '%s': %s",
                 name, esp_err_to_name(err));
        pcnt_unit_disable(pcntUnit);
        applyChannelActions(invertAxis);
        pcnt_unit_enable(pcntUnit);
        pcnt_unit_start(pcntUnit);
        return;
    }

    invertAxis = invert;
}

bool DroAxis::getInvert() const {
    return invertAxis;
}

int64_t DroAxis::snapshotCounts() {
    if (!configured) {
        return countOffset.load(std::memory_order_relaxed);
    }

    int hwCount = 0;
    esp_err_t err = pcnt_unit_get_count(pcntUnit, &hwCount);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read PCNT count for axis '%s': %s",
                 name, esp_err_to_name(err));
        return cachedCounts.load(std::memory_order_relaxed);
    }

    int64_t counts = countOffset.load(std::memory_order_relaxed) + hwCount;
    cachedCounts.store(counts, std::memory_order_relaxed);
    return counts;
}

void DroAxis::setPositionUm(float newPos) {
    int64_t newCount = static_cast<int64_t>(std::llround(newPos / step_um));
    if (configured) {
        esp_err_t err = pcnt_unit_stop(pcntUnit);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to stop PCNT unit while setting axis '%s': %s",
                     name, esp_err_to_name(err));
            return;
        }
        err = pcnt_unit_clear_count(pcntUnit);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to clear PCNT unit while setting axis '%s': %s",
                     name, esp_err_to_name(err));
            pcnt_unit_start(pcntUnit);
            return;
        }
    }
    position_um = newPos;
    countOffset.store(newCount, std::memory_order_relaxed);
    cachedCounts.store(newCount, std::memory_order_relaxed);
    if (configured) {
        esp_err_t err = pcnt_unit_start(pcntUnit);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to restart PCNT unit after setting axis '%s': %s",
                     name, esp_err_to_name(err));
        }
    }
}

float DroAxis::getPositionUm() {
    return static_cast<float>(snapshotCounts()) * step_um;
}

void DroAxis::simulateStep(bool forward) {
    int64_t counts = countOffset.fetch_add(forward ? 1 : -1, std::memory_order_relaxed) + (forward ? 1 : -1);
    cachedCounts.store(counts, std::memory_order_relaxed);
}
