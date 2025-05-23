#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_system.h"
#include "driver/uart.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_event.h"
#include "tcpip_adapter.h"

#define BUF_SIZE (256)

void uart_write(const char* msg) {
    uart_write_bytes(0, msg, strlen(msg));
}

void uart_read_line(char* buffer, int max_len) {
    int i = 0;
    while (i < max_len - 1) {
        uint8_t byte;
        int len = uart_read_bytes(0, &byte, 1, portMAX_DELAY);
        if (len > 0) {
            if (byte == '\r' || byte == '\n') {
                if (i > 0) break;
            } else {
                buffer[i++] = byte;
                uart_write_bytes(0, (const char*)&byte, 1);
            }
        }
    }
    buffer[i] = '\0';
    uart_write("\r\n");
}

int get_wifi_count(nvs_handle handle) {
    int count = 0;
    char key[16];
    char dummy[64];
    size_t length;

    while (1) {
        sprintf(key, "ssid%d", count);
        length = sizeof(dummy);
        esp_err_t err = nvs_get_str(handle, key, dummy, &length);
        if (err != ESP_OK) break;
        count++;
    }
    return count;
}

void save_wifi_config(const char* ssid, const char* pass) {
    nvs_handle handle;
    if (nvs_open("wifi_config", NVS_READWRITE, &handle) != ESP_OK) {
        uart_write("Lỗi mở NVS!\r\n");
        return;
    }

    int count = get_wifi_count(handle);
    char key_ssid[16], key_pass[16];

    sprintf(key_ssid, "ssid%d", count);
    sprintf(key_pass, "pass%d", count);

    nvs_set_str(handle, key_ssid, ssid);
    nvs_set_str(handle, key_pass, pass);

    nvs_commit(handle);
    nvs_close(handle);

    uart_write("✅ Đã lưu cấu hình WiFi mới vào NVS.\r\n");
}

void show_all_wifi_configs() {
    nvs_handle handle;
    if (nvs_open("wifi_config", NVS_READONLY, &handle) != ESP_OK) {
        uart_write("Lỗi mở NVS!\r\n");
        return;
    }

    int count = get_wifi_count(handle);
    if (count == 0) {
        uart_write(" Chưa có cấu hình WiFi nào.\r\n");
        nvs_close(handle);
        return;
    }

    char key[16], val[64];
    size_t len;

    uart_write("Các cấu hình WiFi đã lưu:\r\n");
    for (int i = 0; i < count; i++) {
        sprintf(key, "ssid%d", i);
        len = sizeof(val);
        nvs_get_str(handle, key, val, &len);
        uart_write("SSID: "); uart_write(val); uart_write("\r\n");

        sprintf(key, "pass%d", i);
        len = sizeof(val);
        nvs_get_str(handle, key, val, &len);
        uart_write("PASS: "); uart_write(val); uart_write("\r\n---\r\n");
    }

    nvs_close(handle);
}

void clear_wifi_config() {
    nvs_handle handle;
    if (nvs_open("wifi_config", NVS_READWRITE, &handle) != ESP_OK) {
        uart_write("Lỗi mở NVS!\r\n");
        return;
    }
    nvs_erase_all(handle);
    nvs_commit(handle);
    nvs_close(handle);
    uart_write("Đã xóa tất cả cấu hình WiFi.\r\n");
}

esp_err_t event_handler(void* ctx, system_event_t* event) {
    return ESP_OK;
}

void try_connect_wifi(const char* ssid, const char* password) {
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);

    wifi_config_t wifi_config = { 0 };
    strncpy((char*)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);

    esp_wifi_start();
    esp_wifi_connect();

    uart_write("Đang kết nối WiFi...\r\n");

    for (int i = 0; i < 20; i++) { // 10s
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            uart_write("✅ Đã kết nối WiFi!\r\n");
            return;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    uart_write("WiFi không khả dụng!\r\n");
}

void show_menu() {
    uart_write("\r\n=== MENU ===\r\n");
    uart_write("r - Nhập cấu hình WiFi mới và lưu\r\n");
    uart_write("d - Hiển thị tất cả cấu hình WiFi đã lưu\r\n");
    uart_write("c - Xóa tất cả cấu hình WiFi\r\n");
    uart_write("=============\r\n");
}

void app_main() {
    nvs_flash_init();
    uart_driver_install(0, BUF_SIZE * 2, 0, 0, NULL, 0);

    tcpip_adapter_init();
    esp_event_loop_init(event_handler, NULL);

    char ssid[64];
    char pass[64];
    char input[8];

    while (1) {
        show_menu();
        uart_read_line(input, sizeof(input));
        char cmd = input[0];

        if (cmd == 'r') {
            uart_write("Nhập SSID: ");
            uart_read_line(ssid, sizeof(ssid));
            uart_write("Nhập Password: ");
            uart_read_line(pass, sizeof(pass));
            save_wifi_config(ssid, pass);
            try_connect_wifi(ssid, pass);
        } else if (cmd == 'd') {
            show_all_wifi_configs();
        } else if (cmd == 'c') {
            clear_wifi_config();
        } else {
            uart_write("Lệnh không hợp lệ.\r\n");
        }
    }
}
