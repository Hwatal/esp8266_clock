#include "lcd12864.h"

#include <string.h>


#define LCD_SIZE_WIDTH      (128)
#define LCD_SIZE_HEIGHT     (64)

static struct {
	struct {
		uint16_t data;
	}block[8];
}lcd_buffer[64];

#define SET_PIXEL(x,y) (lcd_buffer[y].block[x/16].data |= 1 << (15 - (x % 16)))
#define CLR_PIXEL(x,y) (lcd_buffer[y].block[x/16].data &= ~(1 << (15 - (x % 16))))
#define GET_PIXEL(x,y) ((lcd_buffer[y].block[x/16].data & (1 << (15 - (x % 16)))) >> (15 - (x % 16)))

int screen_init(void) {
    init_lcd_spi();
    lcd_udelay(10 *1000);
    lcd_write_cmd(0x0c);
    return 0;
}

void screen_setpixel(int posX, int posY, uint32_t color) {
	if ((posX < LCD_SIZE_WIDTH) && (posY < LCD_SIZE_HEIGHT) && (GET_PIXEL(posX, posY) != color)) {
        if(1 == color) {
			SET_PIXEL(posX, posY);
        } else if(0 == color){
			CLR_PIXEL(posX, posY);
        }
    }
}

int screen_getpixel(int posX, int posY) {
	int color = 0;
	if((posX < LCD_SIZE_WIDTH) && (posY < LCD_SIZE_HEIGHT)) {
		color = GET_PIXEL(posX, posY);
    }
	return color;
}

void screen_clear(void) {
	memset(lcd_buffer, 0, sizeof(lcd_buffer));
	lcd_write_cmd(0x01);
	lcd_udelay(5 * 1000);
}

void screen_sync(void) {
	lcd_write_cmd(0x34);
	lcd_write_cmd(0x34);
	for(int i = 0; i < 32; i ++) {
		lcd_write_cmd(0x80 + i);
		lcd_write_cmd(0x80);
		for(int j = 0; j < 8; j ++) {
			lcd_write_data(lcd_buffer[i].block[j].data >> 8);
			lcd_write_data(lcd_buffer[i].block[j].data & 0xff);
		}
	}
	for(int i = 0; i < 32; i ++) {
		lcd_write_cmd(0x80 + i);
		lcd_write_cmd(0x88);
		for(int j = 0; j < 8; j ++) {
			lcd_write_data(lcd_buffer[i + 32].block[j].data >> 8);
			lcd_write_data(lcd_buffer[i + 32].block[j].data & 0xff);
		}
	}
	lcd_write_cmd(0x36);
	lcd_write_cmd(0x30);
}