#pragma once

#include "wifi_manager.h"

#include <string>

#include "esp_err.h"
#include "esp_http_server.h"

class OtaServer {
public:
    explicit OtaServer(WifiManager& wifi);

    esp_err_t begin();
    void stop();
    bool isRunning() const;
    std::string getStatusText() const;
    std::string getUrl() const;

private:
    static esp_err_t handleIndex(httpd_req_t* req);
    static esp_err_t handleUpdate(httpd_req_t* req);
    static void rebootTimerCallback(void* arg);

    esp_err_t startMdns();
    void setStatus(const char* status);

    WifiManager& wifi;
    httpd_handle_t server = nullptr;
    std::string statusText = "OTA server stopped";
};
