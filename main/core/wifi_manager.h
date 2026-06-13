#pragma once

#include "preferences_wrapper.h"

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

#include "esp_err.h"
#include "esp_netif.h"
#include "esp_timer.h"

struct WifiNetwork {
    std::string ssid;
    int8_t rssi = 0;
    bool secured = false;
};

class WifiManager {
public:
    explicit WifiManager(PreferencesWrapper& prefs);

    esp_err_t begin();
    esp_err_t connect(const std::string& ssid, const std::string& password);
    void disconnect();
    void reconnect();
    esp_err_t scanNetworks(std::vector<WifiNetwork>& networks);

    bool hasCredentials() const;
    bool isConnected() const;
    bool isConnecting() const;
    std::string getSsid() const;
    std::string getIpAddress() const;
    std::string getStatusText() const;
    std::string getMdnsName() const;
    uint8_t getReconnectAttempts() const;

private:
    static void handleEvent(void* arg, esp_event_base_t eventBase, int32_t eventId, void* eventData);
    static void handleConnectTimeout(void* arg);
    void loadCredentials();
    void saveCredentials(const std::string& ssid, const std::string& password);
    esp_err_t startStation();
    void startConnectTimeout();
    void stopConnectTimeout();
    void updateStatus(const char* status);

    PreferencesWrapper& prefs;
    esp_netif_t* stationNetif = nullptr;
    esp_timer_handle_t connectTimeoutTimer = nullptr;
    std::string ssid;
    std::string password;
    std::string ipAddress;
    std::string statusText = "Wi-Fi idle";
    uint8_t reconnectAttempts = 0;
    std::atomic<bool> started{false};
    std::atomic<bool> connected{false};
    std::atomic<bool> connecting{false};
    std::atomic<bool> scanning{false};
    std::atomic<bool> intentionalDisconnect{false};
    std::atomic<bool> reconnectSuppressed{false};
};
