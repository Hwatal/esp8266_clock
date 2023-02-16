#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_smartconfig.h"
#include "esp_console.h"

#include "argtable3/argtable3.h"
#include "tcpip_adapter.h"
#include "smartconfig_ack.h"

static const char *TAG = "BSP_WIFI";

static EventGroupHandle_t wifi_event_group;
static const int STA_START_BIT = BIT0;
static const int CONNECTED_BIT = BIT1;
static const int ESPTOUCH_DONE_BIT = BIT1;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {

    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                xEventGroupSetBits(wifi_event_group, STA_START_BIT);
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                esp_wifi_connect();
                xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
                break;
        }
        
    } else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP:
                xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
                break;
        }

    } else if (event_base == SC_EVENT) {
        switch (event_id) {
            case SC_EVENT_SCAN_DONE:
                ESP_LOGI(TAG, "Scan done");
                break;
            case SC_EVENT_FOUND_CHANNEL:
                ESP_LOGI(TAG, "Found channel");
                break;
            case SC_EVENT_GOT_SSID_PSWD: {
                ESP_LOGI(TAG, "Got SSID and password");

                smartconfig_event_got_ssid_pswd_t* evt = (smartconfig_event_got_ssid_pswd_t*)event_data;
                wifi_config_t wifi_config;
                bzero(&wifi_config, sizeof(wifi_config_t));
                memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
                memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
                wifi_config.sta.bssid_set = evt->bssid_set;
                if (wifi_config.sta.bssid_set == true) {
                    memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
                }

                ESP_LOGI(TAG, "SSID:%s", wifi_config.sta.ssid);
                ESP_LOGI(TAG, "PASSWORD:%s", wifi_config.sta.password);
                if (evt->type == SC_TYPE_ESPTOUCH_V2) {
                    uint8_t rvd_data[33] = { 0 };
                    ESP_ERROR_CHECK( esp_smartconfig_get_rvd_data(rvd_data, sizeof(rvd_data)) );
                    ESP_LOGI(TAG, "RVD_DATA:%s", rvd_data);
                }

                ESP_ERROR_CHECK(esp_wifi_disconnect());
                ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
                ESP_ERROR_CHECK(esp_wifi_connect());
                break;
            }
            case SC_EVENT_SEND_ACK_DONE:
                xEventGroupSetBits(wifi_event_group, ESPTOUCH_DONE_BIT);
                break;
        }
    }
}

void init_wifi(void) {
    wifi_event_group = xEventGroupCreate();
    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_ps(WIFI_PS_NONE));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static int smartconfig_cmd(int argc, char** argv) {

    ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH));
    smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_smartconfig_start(&cfg));

    EventBits_t uxBits;
    while (1) {
        uxBits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, false, portMAX_DELAY);
        if (uxBits & CONNECTED_BIT) {
            ESP_LOGI(TAG, "WiFi Connected to ap");
        }

        if (uxBits & ESPTOUCH_DONE_BIT) {
            ESP_LOGI(TAG, "smartconfig over");
            esp_smartconfig_stop();
            break;
        }
    }
    
    return 0;
}

void register_smartconfig(void) {

    const esp_console_cmd_t cmd = {
        .command = "smartconfig",
        .help = "Smartconfig connect a new Wi-Fi",
        .hint = NULL,
        .func = &smartconfig_cmd,
    };

    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}