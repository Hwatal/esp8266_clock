#ifndef _LCD12864_H_
#define _LCD12864_H_

#include <driver/gpio.h>

#include <unistd.h>

#define lcd_udelay usleep
esp_err_t lcd_write_cmd(uint8_t cmd);
esp_err_t lcd_write_data(uint8_t data);
void init_lcd_spi(void);

#endif