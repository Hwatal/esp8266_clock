#include "lcd12864.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_log.h>

#include <driver/spi.h>

static const char* TAG = "lcd12864";

inline int lcd_write(uint8_t rs, uint8_t data)
{
    uint16_t cmd = 0xF8 | (rs << 1);
    uint32_t buf = 0;
    uint8_t *pbuf = (uint8_t*)&buf;
    pbuf[0] = data & 0xF0;
    pbuf[1] = data << 4;

    spi_trans_t trans = {0};
    trans.cmd = &cmd;
    trans.mosi = &buf;
    trans.bits.cmd = 8;
    trans.bits.mosi = 16;

    return spi_trans(HSPI_HOST, &trans);
}

esp_err_t lcd_write_cmd(uint8_t cmd, uint32_t delayus)
{
	esp_err_t err = lcd_write(0, cmd);
	lcd_udelay(delayus);
	return err;
}

esp_err_t lcd_write_data(uint8_t data, uint32_t delayus)
{
	esp_err_t err = lcd_write(1, data);
	lcd_udelay(delayus);
	return err;
}

void init_lcd_spi(void) {
    ESP_LOGI(TAG, "init hspi");
    spi_config_t spi_config;
    // Load default interface parameters
    // CS_EN:1, MISO_EN:1, MOSI_EN:1, BYTE_TX_ORDER:1, BYTE_TX_ORDER:1, BIT_RX_ORDER:0, BIT_TX_ORDER:0, CPHA:0, CPOL:0
    spi_config.interface.val = SPI_DEFAULT_INTERFACE;
    // Load default interrupt enable
    // Cancel hardware cs
    spi_config.interface.cs_en = 0;
    // MISO pin is used for DC
    spi_config.interface.miso_en = 0;
    spi_config.interface.cpol = 1;
    spi_config.interface.cpha = 1;
    // TRANS_DONE: false, WRITE_STATUS: false, READ_STATUS: false, WRITE_BUFFER: false, READ_BUFFER: false
    spi_config.intr_enable.val = 0;
    // Set SPI to master mode
    // 8266 Only support half-duplex
    spi_config.mode = SPI_MASTER_MODE;
    // Set the SPI clock frequency division factor
    spi_config.clk_div = SPI_10MHz_DIV;
    // Register SPI event callback function
    spi_config.event_cb = NULL;
    spi_init(HSPI_HOST, &spi_config);
}
