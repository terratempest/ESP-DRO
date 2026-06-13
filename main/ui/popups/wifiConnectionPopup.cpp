#include "wifiConnectionPopup.h"

#include "app_services.h"
#include "config.h"
#include "ota_server.h"
#include "wifi_manager.h"
#include "lv_helpers/LVHelpers.h"

#include "esp_app_desc.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <memory>
#include <string>
#include <vector>

namespace {
struct StatusRefreshContext {
    lv_obj_t* overlay = nullptr;
    lv_obj_t* label = nullptr;
    lv_timer_t* timer = nullptr;
};

struct WifiScanContext {
    lv_obj_t* overlay = nullptr;
    lv_obj_t* dropdown = nullptr;
    lv_obj_t* statusLabel = nullptr;
    lv_obj_t* ssidInput = nullptr;
    lv_obj_t* scanButton = nullptr;
    std::shared_ptr<std::vector<WifiNetwork>> scannedNetworks;
    std::vector<WifiNetwork> results;
    esp_err_t result = ESP_FAIL;
};

std::string wifiDetailsText() {
    if (!gWifiManager || !gOtaServer) {
        return "Network services unavailable";
    }

    std::string text = gWifiManager->getStatusText();
    text += "\nSSID: " + (gWifiManager->getSsid().empty() ? std::string("none") : gWifiManager->getSsid());
    text += "\nIP: " + gWifiManager->getIpAddress();
    text += "\nmDNS: " + gWifiManager->getMdnsName();
    text += "\nOTA: " + gOtaServer->getStatusText();
    text += "\nUpload: " + gOtaServer->getUrl();
    return text;
}

void refreshStatusTimer(lv_timer_t* timer) {
    auto* ctx = static_cast<StatusRefreshContext*>(lv_timer_get_user_data(timer));
    if (!ctx || !lv_obj_is_valid(ctx->overlay) || !lv_obj_is_valid(ctx->label)) {
        lv_timer_delete(timer);
        delete ctx;
        return;
    }
    lv_label_set_text(ctx->label, wifiDetailsText().c_str());
}

void applyScanResult(void* userData) {
    auto* ctx = static_cast<WifiScanContext*>(userData);
    if (!ctx) {
        return;
    }

    if (lv_obj_is_valid(ctx->overlay) &&
        lv_obj_is_valid(ctx->dropdown) &&
        lv_obj_is_valid(ctx->statusLabel) &&
        lv_obj_is_valid(ctx->ssidInput) &&
        lv_obj_is_valid(ctx->scanButton)) {
        std::vector<std::string> options;
        ctx->scannedNetworks->clear();

        if (ctx->result != ESP_OK) {
            options.push_back("Scan failed");
            lv_obj_add_state(ctx->dropdown, LV_STATE_DISABLED);
        } else if (ctx->results.empty()) {
            options.push_back("No networks found");
            lv_obj_add_state(ctx->dropdown, LV_STATE_DISABLED);
        } else {
            *ctx->scannedNetworks = ctx->results;
            for (const auto& network : ctx->results) {
                options.push_back(
                    network.ssid + " (" + std::to_string(network.rssi) + " dBm" +
                    (network.secured ? ", locked)" : ", open)")
                );
            }
            lv_obj_clear_state(ctx->dropdown, LV_STATE_DISABLED);
            lv_textarea_set_text(ctx->ssidInput, ctx->results.front().ssid.c_str());
        }

        std::string optionText;
        for (size_t i = 0; i < options.size(); ++i) {
            optionText += options[i];
            if (i + 1 < options.size()) {
                optionText += "\n";
            }
        }
        lv_dropdown_set_options(ctx->dropdown, optionText.c_str());
        lv_dropdown_set_selected(ctx->dropdown, 0);
        lv_obj_clear_state(ctx->scanButton, LV_STATE_DISABLED);
        lv_label_set_text(ctx->statusLabel, wifiDetailsText().c_str());
    }

    delete ctx;
}

void scanTask(void* userData) {
    auto* ctx = static_cast<WifiScanContext*>(userData);
    if (ctx && gWifiManager) {
        ctx->result = gWifiManager->scanNetworks(ctx->results);
    }
    lv_async_call(applyScanResult, ctx);
    vTaskDelete(nullptr);
}
}

void WifiConnectionPopup::show(std::function<void()> onClose) {
    auto* overlay = new LVContainer(lv_scr_act(), [](auto& p){
        p.setSize(LCD_H_RES, LCD_V_RES);
        p.setTransparentBg();
        p.setFlag(LV_OBJ_FLAG_CLICKABLE);
    });

    auto* panel = new LVPanel(overlay->obj, [&](auto& p){
        p.setSize(LCD_H_RES - 120, LCD_V_RES - 70);
        p.setAlign(LV_ALIGN_CENTER);
        p.setBgColor(COLOR_BLACK);
        p.setRadius(12);
        p.setPadAll(18);
    });

    new LVLabel(panel->obj, "Wi-Fi Connection", [](auto& l){
        l.setAlign(LV_ALIGN_TOP_LEFT);
        l.setTextColor(lv_palette_main(LV_PALETTE_BLUE));
    });

    auto* form = new LVFlexContainer(panel->obj, [](auto& f){
        f.removeStyleAll();
        f.setWidthPercent(100);
        f.setHeightPercent(72);
        f.setAlign(LV_ALIGN_TOP_MID, 0, 40);
        f.setFlexFlow(LV_FLEX_FLOW_COLUMN);
        f.setFlexAlign(LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        f.setPadRow(10);
        f.setScrollable();
    });

    LVTextarea* ssidInput = nullptr;
    LVTextarea* passwordInput = nullptr;
    auto scannedNetworks = std::make_shared<std::vector<WifiNetwork>>();
    LVDropdown* networkDropdown = nullptr;

    new LVContainer(form->obj, [&](auto& row){
        row.setWidthPercent(92);
        row.setHeight(44);
        new LVLabel(row.obj, "SSID", [](auto& l){ l.setAlign(LV_ALIGN_LEFT_MID); });
        ssidInput = new LVTextarea(row.obj, gWifiManager ? gWifiManager->getSsid() : "", [](auto& t){
            t.setSize(360, 38);
            t.setAlign(LV_ALIGN_RIGHT_MID);
            t.setOneLine();
        });
    });

    new LVContainer(form->obj, [&](auto& row){
        row.setWidthPercent(92);
        row.setHeight(44);
        new LVLabel(row.obj, "Password", [](auto& l){ l.setAlign(LV_ALIGN_LEFT_MID); });
        passwordInput = new LVTextarea(row.obj, "", [](auto& t){
            t.setSize(360, 38);
            t.setAlign(LV_ALIGN_RIGHT_MID);
            t.setOneLine();
            lv_textarea_set_password_mode(t.obj, true);
        });
    });

    auto* statusLabel = new LVLabel(form->obj, wifiDetailsText(), [](auto& l){
        l.setWidthPercent(92);
    });

    new LVContainer(form->obj, [&](auto& row){
        row.setWidthPercent(92);
        row.setHeight(44);
        auto* scanButton = new LVButton(row.obj, "Scan", [](auto& b){
            b.setSize(95, 38);
            b.setAlign(LV_ALIGN_LEFT_MID);
            b.setBgColor(COLOR_GRAY);
        });
        networkDropdown = new LVDropdown(row.obj, [](auto& dd){
            dd.setSize(360, 38);
            dd.setAlign(LV_ALIGN_RIGHT_MID);
            dd.setOptions("Scan for networks");
            dd.setDisabled(true);
        });

        scanButton->onClick([=]{
            if (!gWifiManager || !networkDropdown) {
                return;
            }

            statusLabel->setText("Scanning...");
            scanButton->setDisabled(true);
            auto* ctx = new WifiScanContext();
            ctx->overlay = overlay->obj;
            ctx->dropdown = networkDropdown->obj;
            ctx->statusLabel = statusLabel->obj;
            ctx->ssidInput = ssidInput->obj;
            ctx->scanButton = scanButton->obj;
            ctx->scannedNetworks = scannedNetworks;
            BaseType_t taskStarted = xTaskCreate(scanTask, "wifi_scan", 4096, ctx, 5, nullptr);
            if (taskStarted != pdPASS) {
                delete ctx;
                scanButton->setDisabled(false);
                statusLabel->setText("Scan task failed");
            }
        });

        networkDropdown->onChanged([=](uint16_t idx, std::string text){
            if (idx < scannedNetworks->size()) {
                ssidInput->setText((*scannedNetworks)[idx].ssid);
            }
        });
    });

    const esp_app_desc_t* appDesc = esp_app_get_description();
    new LVLabel(form->obj,
        std::string("Firmware: ") + appDesc->version + "\nProject: " + appDesc->project_name,
        [](auto& l){
            l.setWidthPercent(92);
        });

    auto* keyboard = new LVKeyboard(overlay->obj, [&](auto& k){
        k.setSize(LCD_H_RES, LCD_V_RES / 3);
        k.setAlign(LV_ALIGN_BOTTOM_MID);
        k.setMode(LV_KEYBOARD_MODE_TEXT_LOWER);
        k.setVisible(false);
    });
    keyboard->onEvent(LV_EVENT_READY, [keyboard]{ keyboard->setVisible(false); });
    keyboard->onEvent(LV_EVENT_CANCEL, [keyboard]{ keyboard->setVisible(false); });

    ssidInput->onEvent(LV_EVENT_FOCUSED, [=]{
        keyboard->setTextarea(ssidInput->obj);
        keyboard->setVisible(true);
    });
    passwordInput->onEvent(LV_EVENT_FOCUSED, [=]{
        keyboard->setTextarea(passwordInput->obj);
        keyboard->setVisible(true);
    });

    auto* refreshContext = new StatusRefreshContext{
        .overlay = overlay->obj,
        .label = statusLabel->obj,
        .timer = nullptr
    };
    refreshContext->timer = lv_timer_create(refreshStatusTimer, 1000, refreshContext);

    new LVButton(panel->obj, "Connect", [&](auto& b){
        b.setSize(110, 38);
        b.setAlign(LV_ALIGN_BOTTOM_LEFT, 0, 0);
        b.setBgColor(COLOR_GRAY);
        b.onClick([=]{
            if (gWifiManager) {
                gWifiManager->connect(ssidInput->getText(), passwordInput->getText());
                statusLabel->setText(wifiDetailsText());
            }
        });
    });

    new LVButton(panel->obj, "Disconnect", [&](auto& b){
        b.setSize(135, 38);
        b.setAlign(LV_ALIGN_BOTTOM_LEFT, 125, 0);
        b.setBgColor(COLOR_GRAY);
        b.onClick([=]{
            if (gWifiManager) {
                gWifiManager->disconnect();
                statusLabel->setText(wifiDetailsText());
            }
        });
    });

    new LVButton(panel->obj, "Refresh", [&](auto& b){
        b.setSize(110, 38);
        b.setAlign(LV_ALIGN_BOTTOM_LEFT, 275, 0);
        b.setBgColor(COLOR_GRAY);
        b.onClick([=]{
            statusLabel->setText(wifiDetailsText());
        });
    });

    new LVButton(panel->obj, "Close", [&](auto& b){
        b.setSize(90, 38);
        b.setAlign(LV_ALIGN_BOTTOM_RIGHT, 0, 0);
        b.setBgColor(COLOR_GRAY);
        b.onClick([=]{
            overlay->del();
            if (onClose) {
                onClose();
            }
        });
    });
}
