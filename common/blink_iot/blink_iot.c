/* LEDC (LED Controller) fade example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/ledc.h"
#include "esp_err.h"
#include "blink_iot.h"

/*
 * About this example
 *
 * 1. Start with initializing LEDC module:
 *    a. Set the timer of LEDC first, this determines the frequency
 *       and resolution of PWM.
 *    b. Then set the LEDC channel you want to use,
 *       and bind with one of the timers.
 *
 * 2. You need first to install a default fade function,
 *    then you can use fade APIs.
 *
 * 3. You can also set a target duty directly without fading.
 *
 * 4. On ESP32, GPIO18/19/4/5 are used as the LEDC outputs:
 *              GPIO18/19 are from the high speed channel group
 *              GPIO4/5 are from the low speed channel group
 *
 *    On other targets, GPIO8/9/4/5 are used as the LEDC outputs,
 *    and they are all from the low speed channel group.
 *
 * 5. All the LEDC outputs change the duty repeatedly.
 *
 */

/*
 * This callback function will be called when fade operation has ended
 * Use callback only if you are aware it is being called inside an ISR
 * Otherwise, you can use a semaphore to unblock tasks
 */
static IRAM_ATTR bool cb_ledc_fade_end_event(const ledc_cb_param_t *param, void *user_arg)
{
    BaseType_t taskAwoken = pdFALSE;

    if (param->event == LEDC_FADE_END_EVT) {
        SemaphoreHandle_t counting_sem = (SemaphoreHandle_t) user_arg;
        xSemaphoreGiveFromISR(counting_sem, &taskAwoken);
    }

    return (taskAwoken == pdTRUE);
}
void int_to_hex(uint32_t color, char *hex_string) {
    // Chuyển số nguyên thành chuỗi HEX dạng FFFFFF
    sprintf(hex_string, "%06lX", color & 0xFFFFFF); // Đảm bảo chỉ lấy 24 bit (mã màu RGB)
}
void set_rgb_duty(uint8_t r, uint8_t g, uint8_t b) {
    // Hàm này cần thiết lập PWM duty cho LED RGB
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, r);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, g);
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, b);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2);
}

void set_rgb_color(const char *hex_color)
{
    // Tách các giá trị màu từ mã hex
    int r, g, b;
    sscanf(hex_color, "%02x%02x%02x", &r, &g, &b);

    // Tính duty cycle cho từng màu
    uint32_t duty_r = (r * 7000) / 255;
    uint32_t duty_g = (g * 7000) / 255;
    uint32_t duty_b = (b * 7000) / 255;

    // Cập nhật duty cho từng kênh
    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0, duty_r); // Kênh đỏ
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_0);

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1, duty_g); // Kênh xanh lá
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_1);

    ledc_set_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2, duty_b); // Kênh xanh dương
    ledc_update_duty(LEDC_HIGH_SPEED_MODE, LEDC_CHANNEL_2);
}

void color_fade(uint32_t start_color, uint32_t end_color, uint32_t steps) {
    uint8_t start_r = (start_color >> 16) & 0xFF;
    uint8_t start_g = (start_color >> 8) & 0xFF;
    uint8_t start_b = start_color & 0xFF;

    uint8_t end_r = (end_color >> 16) & 0xFF;
    uint8_t end_g = (end_color >> 8) & 0xFF;
    uint8_t end_b = end_color & 0xFF;

    for (uint32_t i = 0; i <= steps; i++) {
        // Tính toán các giá trị màu trung gian
        uint8_t r = start_r + ((end_r - start_r) * i) / steps;
        uint8_t g = start_g + ((end_g - start_g) * i) / steps;
        uint8_t b = start_b + ((end_b - start_b) * i) / steps;

        // Cập nhật giá trị PWM cho RGB LED
        set_rgb_duty(r, g, b);

        // Độ trễ giữa mỗi bước để tạo hiệu ứng mượt mà
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms delay giữa các bước
    }
}

void blink_iot_init(void)
{
    /*
     * Prepare and set configuration of timers
     * that will be used by LED Controller
     */
    ledc_timer_config_t ledc_timer = { //cấu hình module ledc
        .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
        .freq_hz = 5000,                      // frequency of PWM signal
        .speed_mode = LEDC_HIGH_SPEED_MODE,           // timer mode
        .timer_num = LEDC_TIMER_1,            // timer index
        .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
    };
    // Set configuration of timer0 for high speed channels
    ledc_timer_config(&ledc_timer);

    /*
     * Prepare individual configuration
     * for each channel of LED Controller
     * by selecting:
     * - controller's channel number
     * - output duty cycle, set initially to 0
     * - GPIO number where LED is connected to
     * - speed mode, either high or low
     * - timer servicing selected channel
     *   Note: if different channels use one timer,
     *         then frequency and bit_num of these channels
     *         will be the same
     */
    ledc_channel_config_t ledc_channel_r =  //cáu hình channel
        {
            .channel    = LEDC_CHANNEL_0,
            .duty       = 0,
            .gpio_num   = GPIO_NUM_32,
            .speed_mode = LEDC_HIGH_SPEED_MODE,
            .hpoint     = 0, //ban dau pwm ở mức cao
            .timer_sel  = LEDC_TIMER_1,
            .flags.output_invert = 0
        };
        ledc_channel_config_t ledc_channel_g =  //cáu hình channel
        {
            .channel    = LEDC_CHANNEL_1,
            .duty       = 0,
            .gpio_num   = GPIO_NUM_33,
            .speed_mode = LEDC_HIGH_SPEED_MODE,
            .hpoint     = 0, //ban dau pwm ở mức cao
            .timer_sel  = LEDC_TIMER_1,
            .flags.output_invert = 0
        };
        ledc_channel_config_t ledc_channel_b =  //cáu hình channel
        {
            .channel    = LEDC_CHANNEL_2,
            .duty       = 0,
            .gpio_num   = GPIO_NUM_25,
            .speed_mode = LEDC_HIGH_SPEED_MODE,
            .hpoint     = 0, //ban dau pwm ở mức cao
            .timer_sel  = LEDC_TIMER_1,
            .flags.output_invert = 0
        };
    // Set LED Controller with previously prepared configuration

        ledc_channel_config(&ledc_channel_r);
        ledc_channel_config(&ledc_channel_g);
        ledc_channel_config(&ledc_channel_b);
}
