#include "preferences_wrapper.h"
#include <cstring>
#include "esp_log.h"
#include "config.h"

PreferencesWrapper::PreferencesWrapper(const char* ns, bool readOnly)
    : namespace_(ns), readOnly_(readOnly) {

    nvs_flash_init(); // Ensure NVS is initialized
    nvs_open(namespace_.c_str(), readOnly ? NVS_READONLY : NVS_READWRITE, &handle);
    
    if(getSchemaVersion() != NVS_VERSION){
        printf("NVS schema version mismatch, erasing NVS!\n");
        nvs_flash_deinit();
        nvs_flash_erase();
        nvs_flash_init(); // Ensure NVS is initialized
        nvs_open(namespace_.c_str(), readOnly ? NVS_READONLY : NVS_READWRITE, &handle);
        setSchemaVersion(NVS_VERSION);
    }
}
PreferencesWrapper::~PreferencesWrapper() {
    nvs_close(handle);
}
void PreferencesWrapper::putString(const std::string& key, const std::string& value) {
    esp_err_t err = nvs_set_str(handle, key.c_str(), value.c_str());
    nvs_commit(handle);
}
std::string PreferencesWrapper::getString(const std::string& key, const std::string& defaultValue) {
    size_t required_size = 0;
    esp_err_t err = nvs_get_str(handle, key.c_str(), NULL, &required_size);
    if (err != ESP_OK || required_size == 0) return defaultValue;
    std::string result(required_size, '\0');
    nvs_get_str(handle, key.c_str(), &result[0], &required_size);
    result.resize(strlen(result.c_str()));
    return result;
}
void PreferencesWrapper::putFloat(const std::string& key, float value) {
    esp_err_t err = nvs_set_blob(handle, key.c_str(), &value, sizeof(value));
    nvs_commit(handle);
}
float PreferencesWrapper::getFloat(const std::string& key, float defaultValue) {
    float value = defaultValue;
    size_t len = sizeof(value);
    esp_err_t err = nvs_get_blob(handle, key.c_str(), &value, &len);
    return (err == ESP_OK) ? value : defaultValue;
}
void PreferencesWrapper::putUShort(const std::string& key, uint16_t value) {
    esp_err_t err = nvs_set_u16(handle, key.c_str(), value);
    nvs_commit(handle);
}
uint16_t PreferencesWrapper::getUShort(const std::string& key, uint16_t defaultValue) {
    uint16_t value = defaultValue;
    esp_err_t err = nvs_get_u16(handle, key.c_str(), &value);
    return (err == ESP_OK) ? value : defaultValue;
}

void PreferencesWrapper::putBool(const std::string& key, bool value) {
    uint8_t val = value ? 1 : 0;
    esp_err_t err = nvs_set_u8(handle, key.c_str(), val);
    nvs_commit(handle);
}

bool PreferencesWrapper::getBool(const std::string& key, bool defaultValue) {
    uint8_t val = defaultValue ? 1 : 0;
    esp_err_t err = nvs_get_u8(handle, key.c_str(), &val);
    return val != 0;
}

uint32_t PreferencesWrapper::getSchemaVersion() {
    uint32_t version = 0;
    esp_err_t err = nvs_get_u32(handle, "schema_version", &version);
    return (err == ESP_OK) ? version : 0;
}
void PreferencesWrapper::setSchemaVersion(uint32_t version) {
    nvs_set_u32(handle, "schema_version", version);
    nvs_commit(handle);
}

