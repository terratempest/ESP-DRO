#include "wifi_manager.h"

#include "config.h"

#include <algorithm>
#include <cstring>

#include "esp_check.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"

namespace {
constexpr const char* TAG = "WifiManager";
constexpr const char* WIFI_SSID_KEY = "wifi_ssid";
constexpr const char* WIFI_PASS_KEY = "wifi_pass";
constexpr const char* MDNS_NAME = "esp-dro";
constexpr uint8_t MAX_RECONNECT_ATTEMPTS = 4;
constexpr int64_t CONNECT_TIMEOUT_US = 15000000;

const char* disconnectReasonText(uint8_t reason) {
    switch (reason) {
        case WIFI_REASON_AUTH_EXPIRE: return "auth expired";
        case WIFI_REASON_AUTH_FAIL: return "auth failed";
        case WIFI_REASON_NO_AP_FOUND: return "network not found";
        case WIFI_REASON_ASSOC_FAIL: return "association failed";
        case WIFI_REASON_HANDSHAKE_TIMEOUT: return "password handshake timeout";
        case WIFI_REASON_BEACON_TIMEOUT: return "signal lost";
        default: return "disconnected";
    }
}
}

WifiManager::WifiManager(PreferencesWrapper& prefs) : prefs(prefs) {}

esp_err_t WifiManager::begin() {
    if (started.load()) {
        return ESP_OK;
    }

    loadCredentials();
    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "netif init failed");

    esp_err_t eventErr = esp_event_loop_create_default();
    if (eventErr != ESP_OK && eventErr != ESP_ERR_INVALID_STATE) {
        ESP_RETURN_ON_ERROR(eventErr, TAG, "event loop init failed");
    }

    stationNetif = esp_netif_create_default_wifi_sta();
    if (!stationNetif) {
        return ESP_FAIL;
    }

    wifi_init_config_t initConfig = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&initConfig), TAG, "wifi init failed");
    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WifiManager::handleEvent, this, nullptr),
                        TAG, "wifi event register failed");
    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WifiManager::handleEvent, this, nullptr),
                        TAG, "ip event register failed");
    const esp_timer_create_args_t timeoutArgs = {
        .callback = &WifiManager::handleConnectTimeout,
        .arg = this,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "wifi_connect_timeout",
        .skip_unhandled_events = true
    };
    ESP_RETURN_ON_ERROR(esp_timer_create(&timeoutArgs, &connectTimeoutTimer), TAG, "connect timeout timer failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_storage(WIFI_STORAGE_RAM), TAG, "wifi storage failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "wifi mode failed");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "wifi start failed");

    started.store(true);
    updateStatus(hasCredentials() ? "Wi-Fi connecting" : "Wi-Fi not configured");
    if (hasCredentials()) {
        return startStation();
    }
    return ESP_OK;
}

esp_err_t WifiManager::connect(const std::string& newSsid, const std::string& newPassword) {
    if (newSsid.empty()) {
        updateStatus("SSID required");
        return ESP_ERR_INVALID_ARG;
    }
    saveCredentials(newSsid, newPassword);
    intentionalDisconnect.store(false);
    reconnectAttempts = 0;
    if (!started.load()) {
        return begin();
    }
    return startStation();
}

void WifiManager::disconnect() {
    intentionalDisconnect.store(true);
    reconnectSuppressed.store(false);
    reconnectAttempts = 0;
    connected.store(false);
    connecting.store(false);
    ipAddress.clear();
    stopConnectTimeout();
    esp_wifi_disconnect();
    updateStatus("Wi-Fi disconnected");
}

void WifiManager::reconnect() {
    if (hasCredentials()) {
        intentionalDisconnect.store(false);
        reconnectAttempts = 0;
        startStation();
    }
}

esp_err_t WifiManager::scanNetworks(std::vector<WifiNetwork>& networks) {
    networks.clear();
    if (!started.load()) {
        ESP_RETURN_ON_ERROR(begin(), TAG, "wifi begin failed");
    }

    scanning.store(true);
    updateStatus("Wi-Fi scanning");

    wifi_scan_config_t scanConfig = {};
    esp_err_t err = esp_wifi_scan_start(&scanConfig, true);
    scanning.store(false);
    if (err != ESP_OK) {
        updateStatus(err == ESP_ERR_WIFI_STATE ? "Wi-Fi busy, retry scan" : "Wi-Fi scan failed");
        return err;
    }

    uint16_t apCount = 0;
    ESP_RETURN_ON_ERROR(esp_wifi_scan_get_ap_num(&apCount), TAG, "scan count failed");
    if (apCount == 0) {
        updateStatus("No networks found");
        return ESP_OK;
    }

    std::vector<wifi_ap_record_t> records(apCount);
    ESP_RETURN_ON_ERROR(esp_wifi_scan_get_ap_records(&apCount, records.data()), TAG, "scan records failed");

    std::sort(records.begin(), records.end(), [](const wifi_ap_record_t& a, const wifi_ap_record_t& b) {
        return a.rssi > b.rssi;
    });

    for (const auto& record : records) {
        std::string foundSsid(reinterpret_cast<const char*>(record.ssid));
        if (foundSsid.empty()) {
            continue;
        }
        auto duplicate = std::find_if(networks.begin(), networks.end(), [&](const WifiNetwork& network) {
            return network.ssid == foundSsid;
        });
        if (duplicate != networks.end()) {
            continue;
        }
        networks.push_back({
            foundSsid,
            record.rssi,
            record.authmode != WIFI_AUTH_OPEN
        });
    }

    updateStatus(networks.empty() ? "No networks found" : "Wi-Fi scan complete");
    return ESP_OK;
}

bool WifiManager::hasCredentials() const {
    return !ssid.empty();
}

bool WifiManager::isConnected() const {
    return connected.load();
}

bool WifiManager::isConnecting() const {
    return connecting.load();
}

std::string WifiManager::getSsid() const {
    return ssid;
}

std::string WifiManager::getIpAddress() const {
    return ipAddress.empty() ? "none" : ipAddress;
}

std::string WifiManager::getStatusText() const {
    return statusText;
}

std::string WifiManager::getMdnsName() const {
    return std::string(MDNS_NAME) + ".local";
}

uint8_t WifiManager::getReconnectAttempts() const {
    return reconnectAttempts;
}

void WifiManager::handleEvent(void* arg, esp_event_base_t eventBase, int32_t eventId, void* eventData) {
    auto* self = static_cast<WifiManager*>(arg);
    if (!self) {
        return;
    }

    if (eventBase == WIFI_EVENT && eventId == WIFI_EVENT_STA_DISCONNECTED) {
        auto* event = static_cast<wifi_event_sta_disconnected_t*>(eventData);
        self->connected.store(false);
        self->ipAddress.clear();

        if (self->reconnectSuppressed.exchange(false)) {
            return;
        }

        self->stopConnectTimeout();

        if (self->intentionalDisconnect.load()) {
            self->connecting.store(false);
            self->updateStatus("Wi-Fi disconnected");
            return;
        }

        if (!self->hasCredentials()) {
            self->connecting.store(false);
            self->updateStatus("Wi-Fi not configured");
            return;
        }

        const uint8_t reason = event ? event->reason : 0;
        if (reason == WIFI_REASON_AUTH_FAIL ||
            reason == WIFI_REASON_NO_AP_FOUND ||
            reason == WIFI_REASON_HANDSHAKE_TIMEOUT ||
            self->reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
            self->connecting.store(false);
            std::string status = "Wi-Fi failed: ";
            status += disconnectReasonText(reason);
            self->statusText = status;
            ESP_LOGW(TAG, "%s", status.c_str());
            return;
        }

        ++self->reconnectAttempts;
        if (self->hasCredentials()) {
            self->connecting.store(true);
            std::string status = "Wi-Fi reconnecting ";
            status += std::to_string(self->reconnectAttempts);
            status += "/";
            status += std::to_string(MAX_RECONNECT_ATTEMPTS);
            self->statusText = status;
            ESP_LOGW(TAG, "Reconnect after %s", disconnectReasonText(reason));
            esp_err_t err = esp_wifi_connect();
            if (err != ESP_OK) {
                self->connecting.store(false);
                self->statusText = std::string("Wi-Fi reconnect failed: ") + esp_err_to_name(err);
            } else {
                self->startConnectTimeout();
            }
        }
    } else if (eventBase == IP_EVENT && eventId == IP_EVENT_STA_GOT_IP) {
        auto* event = static_cast<ip_event_got_ip_t*>(eventData);
        char ip[16] = {};
        esp_ip4addr_ntoa(&event->ip_info.ip, ip, sizeof(ip));
        self->ipAddress = ip;
        self->connected.store(true);
        self->connecting.store(false);
        self->stopConnectTimeout();
        self->intentionalDisconnect.store(false);
        self->reconnectSuppressed.store(false);
        self->reconnectAttempts = 0;
        self->updateStatus("Wi-Fi connected");
        ESP_LOGI(TAG, "Connected to %s at %s", self->ssid.c_str(), self->ipAddress.c_str());
    }
}

void WifiManager::handleConnectTimeout(void* arg) {
    auto* self = static_cast<WifiManager*>(arg);
    if (!self || !self->connecting.load() || self->connected.load()) {
        return;
    }

    self->connecting.store(false);
    self->connected.store(false);
    self->intentionalDisconnect.store(true);
    self->reconnectSuppressed.store(false);
    self->reconnectAttempts = 0;
    self->ipAddress.clear();
    self->statusText = "Wi-Fi failed: connect timeout";
    ESP_LOGW(TAG, "Connection timed out for %s", self->ssid.c_str());
    esp_wifi_disconnect();
}

void WifiManager::loadCredentials() {
    ssid = prefs.getString(WIFI_SSID_KEY, "");
    password = prefs.getString(WIFI_PASS_KEY, "");
}

void WifiManager::saveCredentials(const std::string& newSsid, const std::string& newPassword) {
    ssid = newSsid;
    password = newPassword;
    prefs.putString(WIFI_SSID_KEY, ssid);
    prefs.putString(WIFI_PASS_KEY, password);
}

esp_err_t WifiManager::startStation() {
    if (!hasCredentials()) {
        updateStatus("Wi-Fi not configured");
        return ESP_ERR_INVALID_STATE;
    }

    wifi_config_t config = {};
    std::strncpy(reinterpret_cast<char*>(config.sta.ssid), ssid.c_str(), sizeof(config.sta.ssid));
    std::strncpy(reinterpret_cast<char*>(config.sta.password), password.c_str(), sizeof(config.sta.password));
    config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    if (password.empty()) {
        config.sta.threshold.authmode = WIFI_AUTH_OPEN;
    }
    config.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    connected.store(false);
    connecting.store(true);
    intentionalDisconnect.store(false);
    reconnectSuppressed.store(false);
    reconnectAttempts = 0;
    ipAddress.clear();
    updateStatus("Wi-Fi connecting");
    esp_err_t disconnectErr = esp_wifi_disconnect();
    reconnectSuppressed.store(disconnectErr == ESP_OK);
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &config), TAG, "wifi config failed");
    esp_err_t err = esp_wifi_connect();
    if (err != ESP_OK) {
        connecting.store(false);
        stopConnectTimeout();
        statusText = std::string("Wi-Fi connect failed: ") + esp_err_to_name(err);
    } else {
        startConnectTimeout();
    }
    return err;
}

void WifiManager::startConnectTimeout() {
    if (!connectTimeoutTimer) {
        return;
    }
    esp_timer_stop(connectTimeoutTimer);
    esp_timer_start_once(connectTimeoutTimer, CONNECT_TIMEOUT_US);
}

void WifiManager::stopConnectTimeout() {
    if (connectTimeoutTimer) {
        esp_timer_stop(connectTimeoutTimer);
    }
}

void WifiManager::updateStatus(const char* status) {
    statusText = status ? status : "";
}
