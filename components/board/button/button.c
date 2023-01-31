#include "button.h"

#include <esp_log.h>

#include <driver/gpio.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/ringbuf.h>

#include "DemoProc.h"

extern RingbufHandle_t HMI_event_buffer;

static const char *TAG = "button";

#define BTN0_PIN_NUM GPIO_NUM_16

struct Button btn0;

void init_button_gpio(void) {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = BIT(BTN0_PIN_NUM);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);
}

static uint8_t read_btn(uint8_t btn_id) {
    return gpio_get_level(btn_id);
}

static void PRESS_UP_Handler(void *btn) {
    ESP_LOGI(TAG, "press up!\n");
}

static void PRESS_DOWN_Handler(void *btn) {
    ESP_LOGI(TAG, "press down!\n");
}

static void PRESS_REPEAT_Handler(void *btn) {
    ESP_LOGI(TAG, "press repeat!\n");
}

static void SINGLE_CLICK_Handler(void *btn) {
    KEY_PRESS_EVENT evt;
    evt.Head.iSize = sizeof(evt);
    evt.Head.iID = EVENT_ID_KEY_PRESS;
    evt.Data.uiKeyValue = KEY_VALUE_ENTER;
    xRingbufferSend(HMI_event_buffer, &evt, sizeof(evt), portMAX_DELAY);
    ESP_LOGI(TAG, "single click!\n");
}

static void DOUBLE_CLICK_Handler(void *btn) {
    KEY_PRESS_EVENT evt;
    evt.Head.iSize = sizeof(evt);
    evt.Head.iID = EVENT_ID_KEY_PRESS;
    evt.Data.uiKeyValue = KEY_VALUE_ESC;
    xRingbufferSend(HMI_event_buffer, &evt, sizeof(evt), portMAX_DELAY);
    ESP_LOGI(TAG, "double click!\n");
}

static void LONG_RRESS_START_Handler(void *btn) {
    ESP_LOGI(TAG, "long press start!\n");
}

static void button_loops(void* param) {
    // (void*)param;
    while (1) {
        button_ticks();
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    
}

void init_button(void) {
    init_button_gpio();
    button_init(&btn0, read_btn, 0, BTN0_PIN_NUM);
    
    // button_attach(&btn0, PRESS_UP, PRESS_UP_Handler);
    // button_attach(&btn0, PRESS_DOWN, PRESS_DOWN_Handler);
    // button_attach(&btn0, PRESS_REPEAT, PRESS_REPEAT_Handler);
    button_attach(&btn0, SINGLE_CLICK, SINGLE_CLICK_Handler);
    button_attach(&btn0, DOUBLE_CLICK, DOUBLE_CLICK_Handler);
    button_attach(&btn0, LONG_PRESS_START, LONG_RRESS_START_Handler);
    button_start(&btn0);

    xTaskCreate(button_loops, "button ticks", 2048 / sizeof(StackType_t), NULL, 10, NULL);
}