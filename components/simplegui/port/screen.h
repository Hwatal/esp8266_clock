#ifndef _SCREEN_H_
#define _SCREEN_H_

int screen_getpixel(int posX, int posY);
void screen_setpixel(int posX, int posY, uint color);
void screen_sync(void);
void screen_clear(void);
int init_screen(void);

#endif