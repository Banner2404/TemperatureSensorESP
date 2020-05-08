#include "sensor.h"
#include "shadow.h"

#include <stdio.h>
#include "sdkconfig.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "owb.h"
#include "owb_rmt.h"
#include "ds18b20.h"

#define SENSOR_GPIO 5
#define SENSOR_RESOLUTION DS18B20_RESOLUTION_12_BIT
#define TAG "SENSOR"
#define SAMPLE_PERIOD (5 * 60 * 1000) //miliseconds

OneWireBus *owb;
owb_rmt_driver_info driver_info;
DS18B20_Info *device;
float temperature = 0;

void initialize_sensor();
bool find_sensor();
void create_device();
void read_temperature();

void sensor_task(void *param) {
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    initialize_sensor();
    while (!find_sensor()) {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    create_device();
    while (1) {
        read_temperature();
        vTaskDelay(SAMPLE_PERIOD / portTICK_PERIOD_MS);
    }
    
}

void initialize_sensor() {
    ESP_LOGI(TAG, "Initializing OWB");
    owb = owb_rmt_initialize(&driver_info, SENSOR_GPIO, RMT_CHANNEL_1, RMT_CHANNEL_0);
    owb_use_crc(owb, true);
}

bool find_sensor() {
    ESP_LOGI(TAG, "Searching for sensor");
    OneWireBus_SearchState search_state = {0};
    bool found = false;

    owb_search_first(owb, &search_state, &found);
    if (found) {
        ESP_LOGI(TAG, "Sensor found");
        char rom_code_string[17];
        owb_string_from_rom_code(search_state.rom_code, rom_code_string, sizeof(rom_code_string));
        ESP_LOGI(TAG, "Sensor id %s", rom_code_string);
    } else {
        ESP_LOGE(TAG, "Sensor not found");
    }
    return found;
}

void create_device() {
    ESP_LOGI(TAG, "Creating device");
    device = ds18b20_malloc();
    ds18b20_init_solo(device, owb);
    ds18b20_use_crc(device, true);
    ds18b20_set_resolution(device, SENSOR_RESOLUTION);
}

void read_temperature() {
    ESP_LOGI(TAG, "Reading temperature");
    ds18b20_convert(device);
    ds18b20_wait_for_conversion(device);
    DS18B20_ERROR error = ds18b20_read_temp(device, &temperature);
    if (error != DS18B20_OK) {
        ESP_LOGE(TAG, "Reading error %d", error);
        return;
    }
    ESP_LOGI(TAG, "Temperature: %f", temperature);
    set_need_shadow_update();
}
