#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"

#include "sht30.h"

const static char *TAG = "SHT30";
typedef uint8_t sht3x_raw_data_t[6];

#define WRITE_BIT I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ   /*!< I2C master read */


#define SHT3x_MEAS_DURATION_REP_HIGH 15
#define SHT3x_MEAS_DURATION_REP_MEDIUM 6
#define SHT3x_MEAS_DURATION_REP_LOW 4

const uint16_t SHT3x_MEAS_DURATION_MS[3] = {SHT3x_MEAS_DURATION_REP_HIGH,
                                            SHT3x_MEAS_DURATION_REP_MEDIUM,
                                            SHT3x_MEAS_DURATION_REP_LOW};

const uint16_t SHT3x_MEASURE_CMD[6][3] = {
    {0x2400, 0x240b, 0x2416},  // [SINGLE_SHOT][H,M,L] without clock stretching
    {0x2032, 0x2024, 0x202f},  // [PERIODIC_05][H,M,L]
    {0x2130, 0x2126, 0x212d},  // [PERIODIC_1 ][H,M,L]
    {0x2236, 0x2220, 0x222b},  // [PERIODIC_2 ][H,M,L]
    {0x2234, 0x2322, 0x2329},  // [PERIODIC_4 ][H,M,L]
    {0x2737, 0x2721, 0x272a}}; // [PERIODIC_10][H,M,L]

#define SHT3x_FETCH_DATA_CMD 0xE000
#define SHT3x_ART_CMD 0x2B32
#define SHT3x_BREAK_CMD 0x3093
#define SHT3x_RESET_CMD 0x30A2
#define SHT3x_GET_STATUS_CMD 0xF32D
#define SHT3x_CLR_STATUS_CMD 0x3041
#define SHT3x_HEATER_ON_CMD 0x306D
#define SHT3x_HEATER_OFF_CMD 0x3066

const uint8_t g_polynom = 0x31;

static uint8_t crc8(uint8_t *data, int len)
{
    // initialization value
    uint8_t crc = 0xff;

    // iterate over all bytes
    for (int i = 0; i < len; i++)
    {
        crc ^= data[i];

        for (int i = 0; i < 8; i++)
        {
            bool xor = crc & 0x80;
            crc = crc << 1;
            crc = xor? crc ^ g_polynom : crc;
        }
    }

    return crc;
}

static esp_err_t write_cmd(i2c_port_t i2c_num, uint8_t addr, uint16_t sensor_cmd)
{
    esp_err_t ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, addr << 1 | WRITE_BIT, 1);
    i2c_master_write_byte(cmd, (uint8_t)(sensor_cmd >> 8), 1);
    i2c_master_write_byte(cmd, (uint8_t)(sensor_cmd & 0xff), 1);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "write cmd \"0x%.4x\" fail!\n", sensor_cmd);
    }
    return ret;
}

static esp_err_t read_data(i2c_port_t i2c_num, uint8_t addr, uint8_t *data, size_t data_len)
{
    esp_err_t ret;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, addr << 1 | READ_BIT, 1);
    i2c_master_read(cmd, data, data_len, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    return ret;
}

inline void sht3x_delay(sht3x_repeat_t repeat) {
    vTaskDelay(pdMS_TO_TICKS(SHT3x_MEAS_DURATION_MS[repeat]));
}

static esp_err_t sht3x_compute_values(sht3x_raw_data_t raw, float *temperature, float *humidity)
{
    if (!raw)
        return ESP_FAIL;

    if (temperature)
        *temperature = ((((raw[0] * 256.0) + raw[1]) * 175) / 65535.0) - 45;

    if (humidity)
        *humidity = ((((raw[3] * 256.0) + raw[4]) * 100) / 65535.0);

    return ESP_OK;
}

static esp_err_t sht3x_measure(sht3x_sensor_t *sensor, float *temperature, float *humidity)
{
    if (!sensor || (!temperature && !humidity))
        return ESP_FAIL;

    if (sensor->mode == sht3x_single_shot) {
        write_cmd(sensor->bus, sensor->addr, SHT3x_MEASURE_CMD[sht3x_single_shot][sensor->repeatability]);
        sensor->meas_started = true;
        sht3x_delay(sensor->repeatability);
        sensor->meas_started = false;
    } else {
        write_cmd(sensor->bus, sensor->addr, SHT3x_FETCH_DATA_CMD);
    }

    sht3x_raw_data_t raw;
    if (ESP_OK != read_data(sensor->bus, sensor->addr, raw, sizeof(sht3x_raw_data_t)))
    {
        ESP_LOGE(TAG, "read data \"fetch data\" fail!\n");
        return ESP_FAIL;
    }

    if (crc8(&raw[0], 2) != raw[2])
    {
        ESP_LOGE(TAG, "crc check \"temperature\" fail!\n");
    }
    if (crc8(&raw[3], 2) != raw[5])
    {
        ESP_LOGE(TAG, "crc check \"humidity\" fail!\n");
    }

    return sht3x_compute_values(raw, temperature, humidity);
}


sht3x_sensor_t *sht3x_init(i2c_port_t bus, uint8_t addr)
{
    sht3x_sensor_t *sensor;
    if ((sensor = malloc(sizeof(sht3x_sensor_t))) == NULL)
        return NULL;

    // inititalize sensor data structure
    sensor->bus = bus;
    sensor->addr = addr;

    sensor->mode = sht3x_single_shot;
    sensor->repeatability = sht3x_high;

    sensor->meas_started = false;

    return sensor;
}

int sht3x_write(void *dev, int pos, void *data, int len) {
    return 0;
}

int sht3x_read(void *dev, int pos, void *data, int len) {
    sht3x_sensor_t *sensor = (sht3x_sensor_t *)dev;

    if (dev == NULL) return 0;

    int rlen = 0;
    float measure[2];
    if (ESP_OK != sht3x_measure(sensor, &measure[0], &measure[1])) {
        return 0;
    }
    rlen = len > sizeof(measure) ? sizeof(measure) : len;
    memcpy(data, measure, rlen);
    
    return rlen;
}

int sht3x_control(void *dev, int cmd, void *arg)
{
    int ret = ESP_FAIL;
    sht3x_sensor_t *sensor = (sht3x_sensor_t *)dev;
    if (dev == NULL) return ESP_FAIL;

    switch (cmd)
    {
    case SHT3X_CTRL_SET_MODE:
        if (sensor->mode == (sht3x_mode_t)arg) return ESP_OK;
        if (sensor->meas_started && sensor->mode != sht3x_single_shot) {
            write_cmd(sensor->bus, sensor->addr, SHT3x_BREAK_CMD);
            sensor->meas_started = false;
        }
        sensor->mode = (sht3x_mode_t)arg;
        ret = write_cmd(sensor->bus, sensor->addr, SHT3x_MEASURE_CMD[sensor->mode][sensor->repeatability]);
        sensor->meas_started = true;
        sht3x_delay(sensor->repeatability);
        break;
    case SHT3X_CTRL_SET_REPEATABILITY:
        if (sensor->repeatability == (sht3x_repeat_t)arg) return ESP_OK;
        if (sensor->meas_started && sensor->mode != sht3x_single_shot) {
            write_cmd(sensor->bus, sensor->addr, SHT3x_BREAK_CMD);
            sensor->meas_started = false;
        }
        sensor->repeatability = (sht3x_repeat_t)arg;
        ret = write_cmd(sensor->bus, sensor->addr, SHT3x_MEASURE_CMD[sensor->mode][sensor->repeatability]);
        sensor->meas_started = true;
        sht3x_delay(sensor->repeatability);
        break;
    case SHT3X_CTRL_SOFT_RESET:
        write_cmd(sensor->bus, sensor->addr, SHT3x_RESET_CMD);
        sensor->meas_started = false;
        break;
    case SHT3X_CTRL_GET_STATUS: {
        uint8_t status[3];
        write_cmd(sensor->bus, sensor->addr, SHT3x_GET_STATUS_CMD);
        read_data(sensor->bus, sensor->addr, status, sizeof(status));
        if (crc8(status, 2) == status[2]) {
            memcpy(arg, status, 2);
            ret = ESP_OK;
        }
        break;
    }
    case SHT3X_CTRL_CLR_STATUS:
        ret = write_cmd(sensor->bus, sensor->addr, SHT3x_CLR_STATUS_CMD);
        break;
    default:
        break;
    }
    return ret;
}