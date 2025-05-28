#pragma once
#include <string>
#include <nvs_flash.h>
#include <nvs.h>

class PreferencesWrapper {
public:
    PreferencesWrapper(const char* ns, bool readOnly = false);
    ~PreferencesWrapper();

    void        putString(const std::string& key, const std::string& value);
    std::string getString(const std::string& key, const std::string& defaultValue = "");

    void        putFloat(const std::string& key, float value);
    float       getFloat(const std::string& key, float defaultValue = 0.0f);

    void        putUShort(const std::string& key, uint16_t value);
    uint16_t    getUShort(const std::string& key, uint16_t defaultValue = 0);

    void        putBool(const std::string& key, bool value);
    bool        getBool(const std::string& key, bool defaultValue = false);

    uint32_t    getSchemaVersion();
    void        setSchemaVersion(uint32_t version);

private:
    bool schemaIsOutdated();
    nvs_handle_t handle = 0;
    std::string namespace_;
    bool readOnly_;
};
