#include "lcd12864.h"
#include "screen.h"
#include <string.h>


#define LCD_SIZE_WIDTH      (128)
#define LCD_SIZE_HEIGHT     (64)

static uint16_t lcd_buffer[16][32];

// #define SET_PIXEL(x,y) (lcd_buffer[y].block[x/16].data |= 1 << (15 - (x % 16)))
// #define CLR_PIXEL(x,y) (lcd_buffer[y].block[x/16].data &= ~(1 << (15 - (x % 16))))
// #define GET_PIXEL(x,y) ((lcd_buffer[y].block[x/16].data & (1 << (15 - (x % 16)))) >> (15 - (x % 16)))

#define SET_PIXEL(x,y) (lcd_buffer[x / 16 + 8 * (y / 32)][y % 32] |= 1 << (15 - (x % 16)))
#define CLR_PIXEL(x,y) (lcd_buffer[x / 16 + 8 * (y / 32)][y % 32] &= ~(1 << (15 - (x % 16))))
#define GET_PIXEL(x,y) ((lcd_buffer[x / 16 + 8 * (y / 32)][y % 32] & (1 << (15 - (x % 16)))) >> (15 - (x % 16)))

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
    lcd_write_cmd(0x01, 5 * 1000);
}

int init_screen(void) {
    init_lcd_spi();
    lcd_udelay(5 *1000);
    lcd_write_cmd(0x0c, 200);
    screen_clear();
    return 0;
}

#define LCD_W_DATA_UDELAY 100
#define LCD_W_CMD_UDELAY 100

// void screen_sync(void) {
//     // lcd_write_cmd(0x34, LCD_W_CMD_UDELAY);
//     lcd_write_cmd(0x36, LCD_W_CMD_UDELAY);
//     for(int i = 0; i < 32; i ++) {
//         lcd_write_cmd(0x80 + i, LCD_W_CMD_UDELAY);
//         lcd_write_cmd(0x80, LCD_W_CMD_UDELAY);
//         for(int j = 0; j < 8; j ++) {
//             lcd_write_data(lcd_buffer[i].block[j].data >> 8, LCD_W_DATA_UDELAY);
//             lcd_write_data(lcd_buffer[i].block[j].data & 0xff, LCD_W_DATA_UDELAY);
//         }
//     }
//     for(int i = 0; i < 32; i ++) {
//         lcd_write_cmd(0x80 + i, LCD_W_CMD_UDELAY);
//         lcd_write_cmd(0x88, LCD_W_CMD_UDELAY);
//         for(int j = 0; j < 8; j ++) {
//             lcd_write_data(lcd_buffer[i + 32].block[j].data >> 8, LCD_W_DATA_UDELAY);
//             lcd_write_data(lcd_buffer[i + 32].block[j].data & 0xff, LCD_W_DATA_UDELAY);
//         }
//     }
//     // lcd_write_cmd(0x36, LCD_W_CMD_UDELAY);
//     lcd_write_cmd(0x32, LCD_W_CMD_UDELAY);
// }

void screen_sync(void) {
    // lcd_write_cmd(0x34, LCD_W_CMD_UDELAY);
    lcd_write_cmd(0x36, LCD_W_CMD_UDELAY);
    for(int y = 0; y < 32; y ++) {
        lcd_write_cmd(0x80 + y, LCD_W_CMD_UDELAY);
        lcd_write_cmd(0x80, LCD_W_CMD_UDELAY);
        for(int x = 0; x < 16; x ++) {
            lcd_write_data(lcd_buffer[x][y] >> 8, LCD_W_DATA_UDELAY);
            lcd_write_data(lcd_buffer[x][y] & 0xff, LCD_W_DATA_UDELAY);
        }
    }
    // lcd_write_cmd(0x36, LCD_W_CMD_UDELAY);
    lcd_write_cmd(0x32, LCD_W_CMD_UDELAY);
}