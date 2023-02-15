#ifndef _SHT3X_H_
#define _SHT3X_H_

#include "driver/i2c.h"

#define SHT3X_CTRL_SET_MODE 0x010000
#define SHT3X_CTRL_SET_REPEATABILITY 0x020000
#define SHT3X_CTRL_SOFT_RESET 0x030000
#define SHT3X_CTRL_GET_STATUS 0x040000
#define SHT3X_CTRL_CLR_STATUS 0x050000
#define SHT3X_CTRL_STOP_MEAS 0x060000

typedef enum
{
    sht3x_single_shot = 0, // one single measurement
    sht3x_periodic_05mps,  // periodic with 0.5 measurements per second (mps)
    sht3x_periodic_1mps,   // periodic with   1 measurements per second (mps)
    sht3x_periodic_2mps,   // periodic with   2 measurements per second (mps)
    sht3x_periodic_4mps,   // periodic with   4 measurements per second (mps)
    sht3x_periodic_10mps   // periodic with  10 measurements per second (mps)
} sht3x_mode_t;

typedef enum
{
    sht3x_high = 0,
    sht3x_medium,
    sht3x_low
} sht3x_repeat_t;

typedef struct
{

    uint32_t error_code; // combined error codes

    i2c_port_t bus; // I2C bus at which sensor is connected
    uint8_t addr;   // I2C slave address of the sensor

    sht3x_mode_t mode;            // used measurement mode
    sht3x_repeat_t repeatability; // used repeatability

    bool meas_started;        // indicates whether measurement started

} sht3x_sensor_t;


sht3x_sensor_t *sht3x_init(i2c_port_t bus, uint8_t addr);
int sht3x_write(void *dev, int pos, void *data, int len);
int sht3x_read(void *dev, int pos, void *data, int len);
int sht3x_control(void *dev, int cmd, void *arg);


#endif