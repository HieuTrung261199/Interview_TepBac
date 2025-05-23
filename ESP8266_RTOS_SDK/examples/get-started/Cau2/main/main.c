#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "mqtt_client.h"
#include "esp_log.h"

#define WIFI_SSID       "Bich Lua"
#define WIFI_PASS       "12341234"
#define WIFI_CONNECTED_BIT BIT0

static EventGroupHandle_t wifi_event_group;
static const char *TAG = "MQTT_DEMO";

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event) {
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
            ESP_LOGI(TAG, "Wi-Fi connected");
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
            ESP_LOGW(TAG, "Wi-Fi disconnected, retrying...");
            break;
        default:
            break;
    }
    return ESP_OK;
}

void wifi_init_sta() {
    wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    esp_event_loop_init(wifi_event_handler, NULL);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();
}

static void mqtt_event_handler_cb(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;

    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT disconnected");
            break;
        default:
            break;
    }
}

esp_mqtt_client_handle_t mqtt_client;

void mqtt_app_start() {
    esp_mqtt_client_config_t mqtt_cfg = {
        .uri = "mqtt://test.mosquitto.org",  // Broker công cộng
        .port = 1883,
        .client_id = "esp8266client",
        .username = NULL,
        .password = NULL,
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler_cb, NULL);
    esp_mqtt_client_start(mqtt_client);
}

void app_main(void) {
    nvs_flash_init();
    wifi_init_sta();

    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);
    mqtt_app_start();

    while (1) {
        int random_value = rand() % 100;
        char msg[64];
        snprintf(msg, sizeof(msg), "{\"value\": %d}", random_value);

        esp_mqtt_client_publish(mqtt_client, "esp8266/random", msg, 0, 1, 0);
        ESP_LOGI(TAG, "Published: %s", msg);

        vTaskDelay(pdMS_TO_TICKS(10000)); 
    }
}
