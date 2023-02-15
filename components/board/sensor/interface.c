#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_err.h"
#include "esp_log.h"

#include "driver/i2c.h"

#include "interface.h"
#include "sht30.h"

void task_sht30_refresh(void *param) {
    sht3x_sensor_t *sensor = (sht3x_sensor_t*)param;
    float measure[2] = {0};
    while (1) {
        sht3x_read(sensor, 0, measure, sizeof(measure));
        // printf("temperature : %f, humidity : %f\n", measure[0], measure[1]);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

esp_err_t init_sensor_interface(void) {
    i2c_port_t i2c_master_port = SENSOR_SHT30_BUS;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = 1;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = 1;
    conf.clk_stretch_tick = 300; // 300 ticks, Clock stretch is about 210us, you can make changes according to the actual situation.
    ESP_ERROR_CHECK(i2c_driver_install(i2c_master_port, conf.mode));
    ESP_ERROR_CHECK(i2c_param_config(i2c_master_port, &conf));

    sht3x_sensor_t *sht30 = sht3x_init(SENSOR_SHT30_BUS, SENSOR_SHT30_ADDR);
    sht3x_control(sht30, SHT3X_CTRL_SET_MODE, (void*)sht3x_periodic_1mps);
    sht3x_control(sht30, SHT3X_CTRL_SET_REPEATABILITY, (void*)sht3x_high);
    
    xTaskCreate(task_sht30_refresh, "sht30 refresh", 2048 / sizeof(StackType_t), (void*)sht30, 6, NULL);
    return ESP_OK;
}