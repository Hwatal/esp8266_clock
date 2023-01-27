/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_spi_flash.h"

#include "driver/gpio.h"
#include "driver/uart.h"

#include "button.h"
#include "encoder.h"
#include "lcd12864.h"

#include "screen.h"
#include "DemoProc.h"

static const char *UART0_TAG = "UART0";
static QueueHandle_t uart0_queue;
#define UART0_BUF_SIZE (1024)
#define RD_BUF_SIZE (UART0_BUF_SIZE)
static int ukey_flags;
static uint32_t keyvalue;


void init_pwrhold(void) {
    gpio_config_t iocfg;
    iocfg.intr_type = GPIO_INTR_DISABLE;
    iocfg.mode = GPIO_MODE_OUTPUT;
    iocfg.pin_bit_mask = BIT(GPIO_NUM_15);
    iocfg.pull_up_en = GPIO_PULLUP_DISABLE;
    iocfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&iocfg);
    gpio_set_level(GPIO_NUM_15, 1);
}

void KeyPressEventProc(uint16_t key) {
    KEY_PRESS_EVENT     key_event;

    HMI_EVENT_INIT(key_event);

    // key_event.Head.iType = EVENT_TYPE_ACTION;
    key_event.Head.iID = EVENT_ID_KEY_PRESS;
    key_event.Data.uiKeyValue = key;
    // Post key press event.
    HMI_ProcessEvent((HMI_EVENT_BASE*)(&key_event));
}

static void uart_event_task(void *pvParameters)
{
#define EX_UART_NUM UART_NUM_0
    uart_event_t event;
    uint8_t *dtmp = (uint8_t *) malloc(RD_BUF_SIZE);

    for (;;) {
        // Waiting for UART event.
        if (xQueueReceive(uart0_queue, (void *)&event, (portTickType)portMAX_DELAY)) {
            bzero(dtmp, RD_BUF_SIZE);
            ESP_LOGI(UART0_TAG, "uart[%d] event:", EX_UART_NUM);

            switch (event.type) {
                // Event of UART receving data
                // We'd better handler data event fast, there would be much more data events than
                // other types of events. If we take too much time on data event, the queue might be full.
                case UART_DATA:
                    ESP_LOGI(UART0_TAG, "[UART DATA]: %d", event.size);
                    uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);
                    if (event.size == 2) {
                        uint16_t key = 0;
                        key = *(uint16_t*)dtmp;
                        KeyPressEventProc(key);
                    }
                    ESP_LOGI(UART0_TAG, "[DATA EVT]:");
                    uart_write_bytes(EX_UART_NUM, (const char *) dtmp, event.size);
                    break;

                // Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    ESP_LOGI(UART0_TAG, "hw fifo overflow");
                    // If fifo overflow happened, you should consider adding flow control for your application.
                    // The ISR has already reset the rx FIFO,
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart0_queue);
                    break;

                // Event of UART ring buffer full
                case UART_BUFFER_FULL:
                    ESP_LOGI(UART0_TAG, "ring buffer full");
                    // If buffer full happened, you should consider encreasing your buffer size
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart0_queue);
                    break;

                case UART_PARITY_ERR:
                    ESP_LOGI(UART0_TAG, "uart parity error");
                    break;

                // Event of UART frame error
                case UART_FRAME_ERR:
                    ESP_LOGI(UART0_TAG, "uart frame error");
                    break;

                // Others
                default:
                    ESP_LOGI(UART0_TAG, "uart event type: %d", event.type);
                    break;
            }
        }
    }

    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}

void app_main()
{
    init_pwrhold();
    
    uart_driver_install(UART_NUM_0, UART0_BUF_SIZE, UART0_BUF_SIZE, 10, &uart0_queue, 0);
    // Create a task to handler UART event from ISR
    xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 12, NULL);

    printf("Hello world!\n");
    /* Print chip information */
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP8266 chip with %d CPU cores, WiFi, ",
            chip_info.cores);

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    init_button();
    init_ec();
    screen_init();
    // Initialize HMI Engine.
    KEY_PRESS_EVENT     stEvent;

    InitializeHMIEngineObj();
    while(1) {

        vTaskDelay(250);
    }
}
