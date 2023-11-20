#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define GPIO_BUTTON_PIN         GPIO_NUM_4
#define GPIO_LED_PIN            GPIO_NUM_2
#define ESP_INTR_FLAG_DEFAULT   0

#define COUNT_HIGH      4000000
#define COUNT_LOW       20

#define DEBOUNCE_DELAY_MS       50

int slicing_flag = 0;
volatile long unsigned int periodic_count = 0;
volatile long unsigned int idle_count = 0;

static QueueHandle_t gpio_evt_queue = NULL;

void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

void task1(void* arg)
{
    uint32_t io_num;
    uint32_t current_state;
    uint32_t last_state = 0;
    bool led = 1;
    while(1) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("\n\nhello from task 1\n");
            gpio_intr_disable(GPIO_BUTTON_PIN);
            current_state = gpio_get_level(io_num);
            if (current_state != last_state && current_state != 1) {
                vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_DELAY_MS));
                gpio_set_level(GPIO_LED_PIN, led);
                led = !led;
                printf("button is pressed\n");
            }
            last_state = current_state;
            gpio_intr_enable(GPIO_BUTTON_PIN);
        }
    }
}

void task2(void* arg) {
    while(1) {
        printf("\n\nhello from task 2\n\n");
        slicing_flag = 0;
        for (periodic_count=0; periodic_count<COUNT_HIGH; periodic_count++) {
            if (slicing_flag == 1) {
                printf("switch back periodic task with periodic count = %lu\n", periodic_count);
                slicing_flag = 0;
            }
        }        
    } 
}

void vApplicationIdleHook(void) {
    printf("hello from idle task\n");
    if (slicing_flag == 0) {
        printf("switch to idle task with periodic count = %lu\n", periodic_count);
        slicing_flag = 1;
    }

    for (idle_count=0; idle_count<COUNT_LOW; idle_count++) {

    }
}

void app_main(void)
{
    gpio_config_t io_conf = {};

    //Config for LED PIN
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1ULL<<GPIO_LED_PIN;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    //Config for INTERRUPT PIN
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.pin_bit_mask = 1ULL<<GPIO_BUTTON_PIN;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    //Create Queue
    gpio_evt_queue = xQueueCreate(1, sizeof(uint32_t));
    
    //Create and Add Task
    xTaskCreate(task2, "task2", 2048, NULL, 0, NULL);
    xTaskCreate(task1, "task1", 2048, NULL, 2, NULL);

    //Instal Interrupt Service Routine
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(GPIO_BUTTON_PIN, gpio_isr_handler, (void*) GPIO_BUTTON_PIN);
}