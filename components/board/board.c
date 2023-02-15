#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/ringbuf.h"

#include "esp_log.h"

#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/i2c.h"

#include "esp_wifi.h"
#include "tcpip_adapter.h"

#include "button.h"
#include "encode.h"
#include "sht30.h"
#include "screen.h"
#include "DemoProc.h"
#include "shell.h"


RingbufHandle_t HMI_event_buffer;

static void init_pwrhold(void) {
    gpio_config_t iocfg;
    iocfg.intr_type = GPIO_INTR_DISABLE;
    iocfg.mode = GPIO_MODE_OUTPUT;
    iocfg.pin_bit_mask = BIT(GPIO_NUM_15);
    iocfg.pull_up_en = GPIO_PULLUP_DISABLE;
    iocfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&iocfg);
    gpio_set_level(GPIO_NUM_15, 1);
}

static void HMI_sync(void *param) {

    size_t item_size = 0;
    HMI_EVENT_BASE *item = NULL;
    HMI_event_buffer = xRingbufferCreate(512, RINGBUF_TYPE_NOSPLIT);
    InitializeHMIEngineObj();
    while (1) {
        item = xRingbufferReceive(HMI_event_buffer, &item_size, portMAX_DELAY);
        if (item) {
            HMI_ProcessEvent(item);
            vRingbufferReturnItem(HMI_event_buffer, (void*)item);
        }
    }
    vTaskDelete(NULL);
}

void board_init(void) {
    init_pwrhold();

    init_screen();
    xTaskCreate(HMI_sync, "HMI sync", 2048 / sizeof(StackType_t), NULL, 15, NULL);

    init_button();
    init_ec();
    init_sensor_interface();

    xTaskCreate(task_shell, "console", 4096 / sizeof(StackType_t), NULL, 7, NULL);

}