#ifndef INPUT_IOT_H
#define INPUT_IOT_H
#include "esp_err.h"
#include "hal/gpio_types.h"

#define BUTTON0 GPIO_NUM_0

typedef void (*input_callback_t)(int, uint64_t); //tạo con trỏ hàm
typedef void (*TimeoutButton_t)(int); //tạo con trỏ hàm

typedef enum{
    LO_TO_HI = 1, // suon len
    HI_TO_LO = 2,  //suon xuong
    ANY_EDLE = 3 //ca suon len va suon xuong
}interrupt_type_edle_t; //bat suon nao

void input_io_create(gpio_num_t gpio_num, interrupt_type_edle_t type);
int input_io_get_level(gpio_num_t gpio_num);
void input_set_callback(void* cb);
void input_set_timeout_callback(void* cb);
#endif
