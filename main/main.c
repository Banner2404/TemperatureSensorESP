/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

#include "sensor.h"
#include "shadow.h"
#include "wifi.h"

#define STACK_SIZE 2048

void init_event_groups();

void app_main()
{
    nvs_flash_init();
    init_event_groups();
    xTaskCreate(&sensor_task, "sensor_task", STACK_SIZE, NULL, 5, NULL);
    xTaskCreate(&shadow_task, "shadow_task", 10 * STACK_SIZE, NULL, 5, NULL);
    xTaskCreate(&wifi_task, "wifi_task", 2 * STACK_SIZE, NULL, 5, NULL);
}

void init_event_groups() {
    wifi_event_group = xEventGroupCreate();
}
