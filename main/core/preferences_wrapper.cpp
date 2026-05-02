#include "preferences_wrapper.h"

#include "config.h"
#include <esp_check.h>
#include <esp_log.h>
#include <inttypes.h>
#include <cstring>

static const char* TAG = "Preferences";

PreferencesWrapper::PreferencesWrapper(const char* ns, bool readOnly)
    : namespace_(ns), readOnly_(readOnly) {}

PreferencesWrapper::~PreferencesWrapper() {
    if (opened_) {
        nvs_close(handle);
    }
}

void PreferencesWrapper::begin() {
    if (opened_) {
        return;
    }

    ESP_ERROR_CHECK(nvs_open(namespace_.c_str(), readOnly_ ? NVS_READONLY : NVS_READWRITE, &handle));
    opened_ = true;

    uint32_t version = getSchemaVersion();
    if (version == 0) {
        if (!readOnly_) {
            setSchemaVersion(NVS_VERSION);
        }
        ESP_LOGI(TAG, "Initialized NVS schema version %" PRIu32, static_cast<uint32_t>(NVS_VERSION));
    } else if (version < NVS_VERSION) {
        ESP_LOGW(TAG, "Migrating NVS schema from %" PRIu32 " to %" PRIu32,
                 version, static_cast<uint32_t>(NVS_VERSION));
        if (!readOnly_) {
            setSchemaVersion(NVS_VERSION);
        }
    } else if (version > NVS_VERSION) {
        ESP_LOGW(TAG, "NVS schema version %" PRIu32 " is newer than firmware schema %" PRIu32,
                 version, static_cast<uint32_t>(NVS_VERSION));
    }
}

void PreferencesWrapper::putString(const std::string& key, const std::string& value) {
    ESP_ERROR_CHECK(nvs_set_str(handle, key.c_str(), value.c_str()));
    ESP_ERROR_CHECK(nvs_commit(handle));
}

std::string PreferencesWrapper::getString(const std::string& key, const std::string& defaultValue) {
    size_t required_size = 0;
    esp_err_t err = nvs_get_str(handle, key.c_str(), NULL, &required_size);
    if (err != ESP_OK || required_size == 0) {
        return defaultValue;
    }

    std::string result(required_size, '\0');
    ESP_ERROR_CHECK(nvs_get_str(handle, key.c_str(), &result[0], &required_size));
    result.resize(strlen(result.c_str()));
    return result;
}

void PreferencesWrapper::putFloat(const std::string& key, float value) {
    ESP_ERROR_CHECK(nvs_set_blob(handle, key.c_str(), &value, sizeof(value)));
    ESP_ERROR_CHECK(nvs_commit(handle));
}

float PreferencesWrapper::getFloat(const std::string& key, float defaultValue) {
    float value = defaultValue;
    size_t len = sizeof(value);
    esp_err_t err = nvs_get_blob(handle, key.c_str(), &value, &len);
    return (err == ESP_OK && len == sizeof(value)) ? value : defaultValue;
}

void PreferencesWrapper::putUShort(const std::string& key, uint16_t value) {
    ESP_ERROR_CHECK(nvs_set_u16(handle, key.c_str(), value));
    ESP_ERROR_CHECK(nvs_commit(handle));
}

uint16_t PreferencesWrapper::getUShort(const std::string& key, uint16_t defaultValue) {
    uint16_t value = defaultValue;
    esp_err_t err = nvs_get_u16(handle, key.c_str(), &value);
    return (err == ESP_OK) ? value : defaultValue;
}

void PreferencesWrapper::putBool(const std::string& key, bool value) {
    uint8_t val = value ? 1 : 0;
    ESP_ERROR_CHECK(nvs_set_u8(handle, key.c_str(), val));
    ESP_ERROR_CHECK(nvs_commit(handle));
}

bool PreferencesWrapper::getBool(const std::string& key, bool defaultValue) {
    uint8_t val = defaultValue ? 1 : 0;
    esp_err_t err = nvs_get_u8(handle, key.c_str(), &val);
    return (err == ESP_OK) ? (val != 0) : defaultValue;
}

uint32_t PreferencesWrapper::getSchemaVersion() {
    uint32_t version = 0;
    esp_err_t err = nvs_get_u32(handle, "schema_version", &version);
    return (err == ESP_OK) ? version : 0;
}

void PreferencesWrapper::setSchemaVersion(uint32_t version) {
    ESP_ERROR_CHECK(nvs_set_u32(handle, "schema_version", version));
    ESP_ERROR_CHECK(nvs_commit(handle));
}
