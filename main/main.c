#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define GPIO_BUTTON_PIN         GPIO_NUM_4
#define GPIO_LED_PIN            GPIO_NUM_2
#define ESP_INTR_FLAG_DEFAULT   0

#define COUNT_HIGH      4000000
#define COUNT_LOW       20

#define DEBOUNCE_DELAY_MS           50

bool slicing_flag = 0;
bool is_button_pressed = 0;
volatile long unsigned int task2_count = 0;
volatile long unsigned int task3_count = 0;

static QueueHandle_t count_evt_queue = NULL;

SemaphoreHandle_t binarysem;

void IRAM_ATTR gpio_isr_handler(void* arg)
{
    xSemaphoreGive(binarysem);
    is_button_pressed = 1;
    gpio_intr_disable(GPIO_BUTTON_PIN);
}

void task1(void* arg)
{
    uint32_t io_num;
    uint8_t led = 1;
    while(1) {
        if(xSemaphoreTake(binarysem, portMAX_DELAY) == pdTRUE) {
            printf("\n\nhello from task 1\n");
            gpio_intr_disable(GPIO_BUTTON_PIN);
            switch (led)
            {
            case 1:
                gpio_set_level(GPIO_LED_PIN, 0);
                printf("OFF led\n");
                led = 0;
                break;
            case 0:
                gpio_set_level(GPIO_LED_PIN, 1);
                printf("ON led\n");
                led = 1;
            default:
                break;
            }
            for (long i=0; i<COUNT_HIGH/2; i++) {
            }  
            gpio_intr_enable(GPIO_BUTTON_PIN);
        }
    }
}

void task2(void* arg) {
    uint32_t count;
    while(1) {
        if(xQueueReceive(count_evt_queue, &count, portMAX_DELAY)) {
            printf("\n\nhello from task 2 with count = %lu\n\n", count);
            for (task2_count=0; task2_count<COUNT_HIGH; task2_count++) {
            } 
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    } 
}

void task3(void* arg) {
    while(1) {
        printf("\n\nhello from task 3\n\n");
        for (task3_count=0; task3_count<COUNT_HIGH*2; task3_count++) {
            if (is_button_pressed == 1) {
                printf("button is pressed, task 1 unblock\n");
                is_button_pressed = 0;
            }
            if (task3_count == COUNT_HIGH/2) {
                printf("send to queue, task 2 unblock\n");
                xQueueSendFromISR(count_evt_queue, &task3_count, NULL);
            }
        }        
        taskYIELD();
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
    count_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    //Create semaphore
    binarysem = xSemaphoreCreateBinary();

    //Create and Add Task
    xTaskCreate(task1, "task1", 2048, NULL, 3, NULL);
    xTaskCreate(task2, "task2", 2048, NULL, 2, NULL);
    xTaskCreate(task3, "task3", 2048, NULL, 1, NULL);

    //Instal Interrupt Service Routine
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    gpio_isr_handler_add(GPIO_BUTTON_PIN, gpio_isr_handler, (void*) GPIO_BUTTON_PIN);
}