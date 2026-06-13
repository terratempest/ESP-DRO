#include "ota_server.h"

#include <algorithm>
#include <cstring>
#include <string>

#include "esp_app_desc.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "mdns.h"

namespace {
constexpr const char* TAG = "OtaServer";
constexpr size_t OTA_BUFFER_SIZE = 4096;

std::string htmlEscape(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            default: out += c; break;
        }
    }
    return out;
}
}

OtaServer::OtaServer(WifiManager& wifi) : wifi(wifi) {}

esp_err_t OtaServer::begin() {
    if (server) {
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.stack_size = 8192;
    ESP_RETURN_ON_ERROR(httpd_start(&server, &config), TAG, "http server start failed");

    httpd_uri_t indexUri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = &OtaServer::handleIndex,
        .user_ctx = this
    };
    httpd_uri_t updateUri = {
        .uri = "/update",
        .method = HTTP_POST,
        .handler = &OtaServer::handleUpdate,
        .user_ctx = this
    };
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &indexUri), TAG, "index handler failed");
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &updateUri), TAG, "update handler failed");
    ESP_RETURN_ON_ERROR(startMdns(), TAG, "mdns failed");

    setStatus("OTA server running");
    ESP_LOGI(TAG, "OTA server running at http://%s/", wifi.getMdnsName().c_str());
    return ESP_OK;
}

void OtaServer::stop() {
    if (server) {
        httpd_stop(server);
        server = nullptr;
    }
    mdns_free();
    setStatus("OTA server stopped");
}

bool OtaServer::isRunning() const {
    return server != nullptr;
}

std::string OtaServer::getStatusText() const {
    return statusText;
}

std::string OtaServer::getUrl() const {
    return std::string("http://") + wifi.getMdnsName() + "/";
}

esp_err_t OtaServer::handleIndex(httpd_req_t* req) {
    auto* self = static_cast<OtaServer*>(req->user_ctx);
    const esp_app_desc_t* app = esp_app_get_description();
    std::string ip = self ? self->wifi.getIpAddress() : "none";
    std::string url = self ? self->getUrl() : "";
    std::string status = self ? self->getStatusText() : "";

    std::string page =
        "<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>ESP-DRO OTA</title><style>"
        "body{font-family:system-ui;margin:2rem;max-width:42rem}button,input{font:inherit;margin:.5rem 0}"
        "progress{width:100%;height:1.5rem}.muted{color:#555}</style></head><body>"
        "<h1>ESP-DRO Firmware Update</h1>"
        "<p>Version: " + htmlEscape(app->version) + "</p>"
        "<p>Project: " + htmlEscape(app->project_name) + "</p>"
        "<p>IP: " + htmlEscape(ip) + "</p>"
        "<p>URL: " + htmlEscape(url) + "</p>"
        "<p>Status: " + htmlEscape(status) + "</p>"
        "<input id='file' type='file' accept='.bin,application/octet-stream'>"
        "<br><button onclick='upload()'>Upload firmware</button>"
        "<progress id='p' max='100' value='0'></progress><pre id='out'></pre>"
        "<p class='muted'>Upload build/LatheDRO_ESP.bin. Device reboots after a valid image.</p>"
        "<script>"
        "function upload(){const f=document.getElementById('file').files[0];if(!f){out.textContent='Choose firmware .bin';return;}"
        "const x=new XMLHttpRequest();x.open('POST','/update');x.upload.onprogress=e=>{if(e.lengthComputable)p.value=e.loaded*100/e.total};"
        "x.onload=()=>out.textContent=x.responseText;x.onerror=()=>out.textContent='Upload failed';x.send(f)}"
        "</script></body></html>";

    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, page.c_str(), page.size());
}

esp_err_t OtaServer::handleUpdate(httpd_req_t* req) {
    auto* self = static_cast<OtaServer*>(req->user_ctx);
    if (!self) {
        return ESP_FAIL;
    }

    const esp_partition_t* updatePartition = esp_ota_get_next_update_partition(nullptr);
    if (!updatePartition) {
        self->setStatus("No OTA partition");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No OTA partition");
        return ESP_FAIL;
    }

    esp_ota_handle_t otaHandle = 0;
    esp_err_t err = esp_ota_begin(updatePartition, OTA_SIZE_UNKNOWN, &otaHandle);
    if (err != ESP_OK) {
        self->setStatus("OTA begin failed");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, esp_err_to_name(err));
        return err;
    }

    std::string buffer;
    buffer.resize(OTA_BUFFER_SIZE);
    int remaining = req->content_len;
    int receivedTotal = 0;
    self->setStatus("OTA upload running");

    while (remaining > 0) {
        int received = httpd_req_recv(req, buffer.data(), std::min<int>(remaining, buffer.size()));
        if (received <= 0) {
            if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            esp_ota_abort(otaHandle);
            self->setStatus("OTA receive failed");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Upload receive failed");
            return ESP_FAIL;
        }
        err = esp_ota_write(otaHandle, buffer.data(), received);
        if (err != ESP_OK) {
            esp_ota_abort(otaHandle);
            self->setStatus("OTA write failed");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, esp_err_to_name(err));
            return err;
        }
        receivedTotal += received;
        remaining -= received;
    }

    err = esp_ota_end(otaHandle);
    if (err != ESP_OK) {
        self->setStatus("OTA image invalid");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, esp_err_to_name(err));
        return err;
    }

    esp_app_desc_t newApp = {};
    err = esp_ota_get_partition_description(updatePartition, &newApp);
    if (err != ESP_OK) {
        self->setStatus("OTA metadata invalid");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, esp_err_to_name(err));
        return err;
    }

    const esp_app_desc_t* currentApp = esp_app_get_description();
    if (std::strcmp(newApp.project_name, currentApp->project_name) != 0) {
        self->setStatus("OTA project mismatch");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Firmware project mismatch");
        return ESP_FAIL;
    }

    err = esp_ota_set_boot_partition(updatePartition);
    if (err != ESP_OK) {
        self->setStatus("OTA boot slot failed");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, esp_err_to_name(err));
        return err;
    }

    self->setStatus("OTA complete, rebooting");
    ESP_LOGI(TAG, "OTA received %d bytes, version %s, rebooting", receivedTotal, newApp.version);
    httpd_resp_sendstr(req, "Firmware accepted. Rebooting.");

    esp_timer_handle_t rebootTimer = nullptr;
    const esp_timer_create_args_t args = {
        .callback = &OtaServer::rebootTimerCallback,
        .arg = nullptr,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "ota_reboot",
        .skip_unhandled_events = false
    };
    if (esp_timer_create(&args, &rebootTimer) == ESP_OK) {
        esp_timer_start_once(rebootTimer, 1000000);
    } else {
        esp_restart();
    }
    return ESP_OK;
}

void OtaServer::rebootTimerCallback(void* arg) {
    esp_restart();
}

esp_err_t OtaServer::startMdns() {
    mdns_free();
    ESP_RETURN_ON_ERROR(mdns_init(), TAG, "mdns init failed");
    ESP_RETURN_ON_ERROR(mdns_hostname_set("esp-dro"), TAG, "mdns hostname failed");
    ESP_RETURN_ON_ERROR(mdns_instance_name_set("ESP-DRO Firmware Update"), TAG, "mdns instance failed");
    return mdns_service_add("ESP-DRO OTA", "_http", "_tcp", 80, nullptr, 0);
}

void OtaServer::setStatus(const char* status) {
    statusText = status ? status : "";
}
