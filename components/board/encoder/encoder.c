#include "encoder.h"

#include "esp_log.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>

static const char *TAG = "encoder";

typedef enum {
    EC_DIR_L,
    EC_DIR_R,
}ec_direction_t;

#define active_L BIT(EC_DIR_L)
#define active_R BIT(EC_DIR_R)

typedef struct {
    uint8_t a_num;
    uint8_t b_num;
    ec_direction_t direction;
    EventGroupHandle_t event;
    void (*cb[2])(void *param);
}encoder_t;

static void IRAM_ATTR ec_r(void *param) {
    encoder_t *handle = (encoder_t*)param;
    xEventGroupSetBitsFromISR(handle->event, active_R, NULL);
}

static void IRAM_ATTR ec_l(void *param) {
    encoder_t *handle = (encoder_t*)param;
    xEventGroupSetBitsFromISR(handle->event, active_L, NULL);
}

void ec_isr(void *param) {
    static uint8_t intr_cnt = 0;
    static uint8_t last_level = 1;
    static uint8_t level = 1;
    encoder_t *handle = (encoder_t*)param;
    
    if ((gpio_get_level(handle->a_num) == 0) && intr_cnt == 0) {
        last_level = gpio_get_level(handle->b_num);
        intr_cnt = 1;
    }

    if ((gpio_get_level(handle->a_num) == 1) && intr_cnt == 1) {
        level = gpio_get_level(handle->b_num);
        if (level != last_level) {
            if (level == 1) {
                handle->direction = 1;
            } else if (level == 0) {
                handle->direction = 0;
            }
            if (handle->direction < 2 && handle->cb[handle->direction] != NULL) {
                handle->cb[handle->direction]((void*)handle);
            }
        }
        intr_cnt = 0;
    }
}

void init_ec_gpio(encoder_t *handle) {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = BIT(handle->a_num);
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    gpio_config(&io_conf);

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = BIT(handle->b_num);
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(handle->a_num, ec_isr, handle);
}

void task_ec_event(void *param) {
    encoder_t *handle = (encoder_t*)param;
    int cnt = 0;
    EventBits_t bits = 0;
    while (1) {
        bits = xEventGroupWaitBits(handle->event, active_L | active_R, 1, 0, portMAX_DELAY);
        if (bits & active_L) {
            cnt --;
        } else if (bits & active_R) {
            cnt ++;
        }
        ESP_LOGI(TAG, "direction: %d, %d\n", (int)handle->direction, cnt);
    }
    
}

static encoder_t sEC = {
    .a_num = EC_A_PIN_NUM,
    .b_num = EC_B_PIN_NUM,
    .cb[EC_DIR_L] = ec_l,
    .cb[EC_DIR_R] = ec_r,
};

void init_ec(void) {
    init_ec_gpio(&sEC);
    sEC.event = xEventGroupCreate();
    xTaskCreate(task_ec_event, "ec evt", 1024 / sizeof(StackType_t), &sEC, 13, NULL);
}