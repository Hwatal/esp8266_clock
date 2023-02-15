#ifndef _SENSOR_INTERFACE_H_
#define _SENSOR_INTERFACE_H_

#include "esp_err.h"
#include "driver/i2c.h"

#define I2C_MASTER_SCL_IO           4                /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO           5                /*!< gpio number for I2C master data  */
#define I2C_MASTER_TX_BUF_DISABLE   0                /*!< I2C master do not need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                /*!< I2C master do not need buffer */

#define SENSOR_SHT30_BUS I2C_NUM_0
#define SENSOR_SHT30_ADDR 0X44

esp_err_t init_sensor_interface(void);

#endif