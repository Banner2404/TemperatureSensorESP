#include "shadow.h"
#include "wifi.h"
#include "sensor.h"
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

#include "aws_iot_config.h"
#include "aws_iot_log.h"
#include "aws_iot_version.h"
#include "aws_iot_mqtt_client_interface.h"
#include "aws_iot_shadow_interface.h"

#define AWS_HOST "a3tbb06100h7yp-ats.iot.us-east-1.amazonaws.com"
#define AWS_PORT 8883
#define AWS_THING_NAME "TempSensor"
#define MAX_JSON_SIZE 200
#define TAG "SHADOW"

extern const uint8_t aws_root_ca_pem_start[] asm("_binary_aws_root_ca_pem_start");
extern const uint8_t aws_root_ca_pem_end[] asm("_binary_aws_root_ca_pem_end");
extern const uint8_t certificate_pem_crt_start[] asm("_binary_certificate_pem_crt_start");
extern const uint8_t certificate_pem_crt_end[] asm("_binary_certificate_pem_crt_end");
extern const uint8_t private_pem_key_start[] asm("_binary_private_pem_key_start");
extern const uint8_t private_pem_key_end[] asm("_binary_private_pem_key_end");

AWS_IoT_Client mqttClient;
bool isShadowConnected = false;
bool isUpdateNeeded = false;
bool isUpdateInProgress = false;

void initialize_shadow();
void connect_shadow();
void update_shadow();
void yield_shadow();
void shadow_update_callback(const char *thingName, ShadowActions_t action, Shadow_Ack_Status_t status, const char *jsonBuffer, void *pContextData);

void shadow_task(void *param) {
    initialize_shadow();
    while (1) {
        if (!isShadowConnected) {
            connect_shadow();
        } else if (isUpdateNeeded && !isUpdateInProgress) {
            update_shadow();
        } else {
            yield_shadow();
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
}

void initialize_shadow() {
    ShadowInitParameters_t shadowParameters = ShadowInitParametersDefault;
    shadowParameters.pHost = AWS_HOST;
    shadowParameters.port = AWS_PORT;
    
    shadowParameters.pClientCRT = (const char *)certificate_pem_crt_start;
    shadowParameters.pClientKey = (const char *)private_pem_key_start;
    shadowParameters.pRootCA = (const char *)aws_root_ca_pem_start;

    shadowParameters.enableAutoReconnect = true;
    shadowParameters.disconnectHandler = NULL;

    ESP_LOGI(TAG, "Waiting for internet connection");
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);

    ESP_LOGI(TAG, "Shadow init");
    IoT_Error_t error = aws_iot_shadow_init(&mqttClient, &shadowParameters);
    if (error != SUCCESS) {
        ESP_LOGE(TAG, "aws_iot_shadow_init failed %d", error);
    }
}

void connect_shadow() {
    ESP_LOGI(TAG, "Shadow connecting...");
    ShadowConnectParameters_t params = ShadowConnectParametersDefault;
    params.pMyThingName = AWS_THING_NAME;
    params.pMqttClientId = AWS_THING_NAME;
    params.mqttClientIdLen = (uint16_t) strlen(AWS_THING_NAME);
    IoT_Error_t error;
    error = aws_iot_shadow_connect(&mqttClient, &params);
    if (error != SUCCESS) {
        ESP_LOGE(TAG, "aws_iot_shadow_connect failed %d", error);
        return;
    }
    ESP_LOGI(TAG, "Shadow connected");

    error = aws_iot_shadow_set_autoreconnect_status(&mqttClient, true);
    if (error != SUCCESS) {
        ESP_LOGE(TAG, "aws_iot_shadow_set_autoreconnect_status failed %d", error);
        return;
    }
    isShadowConnected = true;
}

void update_shadow() {
    IoT_Error_t error;
    char jsonBuffer[MAX_JSON_SIZE];
    size_t jsonBufferSize = sizeof(jsonBuffer) / sizeof(jsonBuffer[0]);
    error = aws_iot_shadow_init_json_document(jsonBuffer, jsonBufferSize);
    if (error != SUCCESS) {
        ESP_LOGE(TAG, "aws_iot_shadow_init_json_document failed %d", error);
        return;
    }
    jsonStruct_t temperatureJson;
    temperatureJson.cb = NULL;
    temperatureJson.pData = &temperature;
    temperatureJson.pKey = "temperature";
    temperatureJson.type = SHADOW_JSON_FLOAT;
    temperatureJson.dataLength = sizeof(float);
    error = aws_iot_shadow_add_reported(jsonBuffer, jsonBufferSize, 1, &temperatureJson);
    if (error != SUCCESS) {
        ESP_LOGE(TAG, "aws_iot_shadow_add_reported failed %d", error);
        return;
    }
    error = aws_iot_finalize_json_document(jsonBuffer, jsonBufferSize);
    if (error != SUCCESS) {
        ESP_LOGE(TAG, "aws_iot_shadow_add_reported failed %d", error);
        return;
    }
    error = aws_iot_shadow_update(&mqttClient, AWS_THING_NAME, jsonBuffer, shadow_update_callback, NULL, 4, true);
    if (error != SUCCESS) {
        ESP_LOGE(TAG, "aws_iot_shadow_update failed %d", error);
        return;
    }
    ESP_LOGI(TAG, "Updating shadow %s", jsonBuffer);
    isUpdateNeeded = false;
    isUpdateInProgress = true;
}

void shadow_update_callback(const char *thingName, ShadowActions_t action, Shadow_Ack_Status_t status, const char *jsonBuffer, void *pContextData) {
    ESP_LOGI(TAG, "Update callback");
    isUpdateInProgress = false;
    switch (status) {
    case SHADOW_ACK_ACCEPTED:
        ESP_LOGI(TAG, "Update accepted");
        break;
    case SHADOW_ACK_TIMEOUT:
        ESP_LOGE(TAG, "Update timeout");
        set_need_shadow_update();
        break;
    case SHADOW_ACK_REJECTED:
        ESP_LOGE(TAG, "Update rejected");
        set_need_shadow_update();
        break;
    }
}

void yield_shadow() {
    IoT_Error_t error = aws_iot_shadow_yield(&mqttClient, 1000);
    if (error != SUCCESS) {
        ESP_LOGE(TAG, "aws_iot_shadow_yield failed %d", error);
    }
}

void set_need_shadow_update() {
    isUpdateNeeded = true;
}