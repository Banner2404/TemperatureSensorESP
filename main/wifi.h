#ifndef __WIFI_H__
#define __WIFI_H__
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#define WIFI_CONNECTED_BIT BIT0

EventGroupHandle_t wifi_event_group;

void wifi_task(void *param);

#endif