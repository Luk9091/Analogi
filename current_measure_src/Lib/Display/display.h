#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include "i2c.h"
#include "stdbool.h"

#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 64

void Display_Init();
void Display_setBrightness(uint8_t value);
void Display_draw();


void Display_putPixel(int x, int y, bool value);
void Display_clear();
void Display_putChar(int x, int y, char c);
void Display_putBigChar(int x, int y, char c);
void Display_print(int x, int y, char *str);

#endif