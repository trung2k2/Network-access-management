#include <stdio.h>
#include <esp_log.h>
#include <driver/gpio.h>
#include "input_iot.h"
#include "esp_attr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/timers.h"

input_callback_t input_callback = NULL; //tạo biến kiểu con trỏ hàm
TimeoutButton_t timeoutButton_callback = NULL; //tạo biến kiểu con trỏ hàm
static uint64_t _start, _stop, _pressTick;
static TimerHandle_t xTimers;


static void IRAM_ATTR gpio_input_handler(void* arg) { //khi co ngat thi check
    int gpio_num = (uint32_t) arg;
    uint32_t rtc = xTaskGetTickCountFromISR();
    if(gpio_get_level(gpio_num) == 0) //1 xong 0 - hi to lo
    {
        _start = rtc;
        xTimerStart(xTimers, 0);
    }
    else //0 to 1
    {
        xTimerStop(xTimers, 0);
        _stop = rtc;
        _pressTick = _stop - _start;
        input_callback(gpio_num, _pressTick);
    }
   
}

static void vTimerCallback( TimerHandle_t xTimer )
 {
 uint32_t ulCount;

    /* Optionally do something if the pxTimer parameter is NULL. */
    configASSERT( xTimer );

    /* The number of times this timer has expired is saved as the
       timer's ID. Obtain the count. */
    ulCount = ( uint32_t ) pvTimerGetTimerID( xTimer ); 
    if (ulCount == 0) //bang id da tao trong xTimerCreate
    {
        timeoutButton_callback(BUTTON0);
    }
 }

void input_io_create(gpio_num_t gpio_num, interrupt_type_edle_t type) //ham khoi tạo, bat sườn lên hay sườn xuống hay cả hai
{
    esp_rom_gpio_pad_select_gpio(gpio_num);
    gpio_set_direction(gpio_num, GPIO_MODE_INPUT);
    gpio_set_pull_mode(gpio_num, GPIO_PULLUP_ONLY);
    gpio_set_intr_type(gpio_num, type);
    gpio_install_isr_service(0); //install server ngat
    gpio_isr_handler_add(gpio_num, gpio_input_handler, (void *) gpio_num); //add ham xu ly ngat gpio_input_handler

    xTimers = xTimerCreate
                   ( /* Just a text name, not used by the RTOS kernel. */
                     "TimerFourTimeout",
                     /* The timer period in ticks, must be greater than 0. */
                     5000/portTICK_PERIOD_MS,
                     /* The timers will auto-reload themselves when they expire. */
                     pdFALSE,
                     /* The ID is used to store a count of the number of times the
                        timer has expired, which is initialised to 0. */
                     ( void * ) 0,
                     /* Each timer calls the same callback when it expires. */
                     vTimerCallback
                   );
}

int input_io_get_level(gpio_num_t gpio_num) //get mức, đọc mức logic của input
{
    return gpio_get_level(gpio_num);
}

void input_set_callback(void* cb) //gọi từ ngoài main truyền hàm cb vào con trỏ hàm
{
    input_callback = cb; //truyền hàm vào con trỏ hàm
}

void input_set_timeout_callback(void* cb) //gọi từ ngoài main truyền hàm cb vào con trỏ hàm
{   
    timeoutButton_callback = cb; //truyền hàm vào con trỏ hàm
}