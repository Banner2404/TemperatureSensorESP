#include "wifi.h"

#include "esp_system.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"

#define WIFI_SSID "AirPort Wi-Fi"
#define WIFI_PASSWORD "24041998"
#define TAG "WIFI"

void initialize_wifi();
void connect_wifi();

static esp_err_t event_handler(void *ctx, system_event_t *event) {
    switch (event->event_id) {
    case SYSTEM_EVENT_STA_START:
        connect_wifi();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "Wifi connected");
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGE(TAG, "Wifi disconnected");
        xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
        connect_wifi();
        break;
    default:
        break;
    }
    return ESP_OK;
}

void wifi_task(void *param) {
    initialize_wifi();
    while (1) {
        vTaskDelay(1200 / portTICK_PERIOD_MS);
    }
}

void initialize_wifi() {
    ESP_LOGI(TAG, "Init wifi");
    tcpip_adapter_init();
    ESP_LOGI(TAG, "Init gr");
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&init_config) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };
    ESP_LOGI(TAG, "Update configuration");
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

void connect_wifi() {
    ESP_LOGI(TAG, "Connecting to wifi...");
    esp_wifi_connect();
}