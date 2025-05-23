#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "driver/gpio.h"

#define LED_GPIO    GPIO_NUM_2
#define BTN_GPIO    GPIO_NUM_0

TimerHandle_t led_timer;

bool led_state = false;
bool long_mode = false;  // false = 2s, true = 4s

// Hàm điều khiển LED và cập nhật thời gian tiếp theo
void led_timer_callback(TimerHandle_t xTimer)
{
    led_state = !led_state;
    gpio_set_level(LED_GPIO, led_state);

    int next_delay = long_mode ? 4000 : 2000;
    xTimerChangePeriod(led_timer, pdMS_TO_TICKS(next_delay), 0);
}

// ISR xử lý nút nhấn
static void IRAM_ATTR button_isr_handler(void* arg)
{
    long_mode = !long_mode;  // đổi trạng thái
}

void app_main(void)
{
    // Cấu hình LED output
    gpio_config_t io_led = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_led);

    // Cấu hình nút nhấn input + interrupt
    gpio_config_t io_btn = {
        .pin_bit_mask = (1ULL << BTN_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE
    };
    gpio_config(&io_btn);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BTN_GPIO, button_isr_handler, NULL);

    // Tạo timer với chu kỳ ban đầu 2s
    led_timer = xTimerCreate("LEDTimer", pdMS_TO_TICKS(2000), pdFALSE, NULL, led_timer_callback);
    xTimerStart(led_timer, 0);
}
