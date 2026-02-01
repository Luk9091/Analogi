#include "display.h"
#include "display_regMap.h"
#include "font.h"
#include "font_24pt.h"

#include <stdlib.h>
#include <memory.h>

#define DISPLAY_I2C &hi2c2

#define DISPLAY_ADDRESS 0x78
#define DISPLAY_TIMEOUT 100


static uint8_t DISPLAY_buffer[DISPLAY_HEIGHT/8][DISPLAY_WIDTH] = {0};


void Display_Init(){
    uint8_t init_cmds[] = {
        SSD1306_DISPLAY_OFF,                                            // 0xAE, Set Display OFF
        SSD1306_SET_MUX_RATIO, 0x3F,                                    // 0xA8, 0x3F for 128 x 64 version (64MUX)
                                                                        //     , 0x1F for 128 x 32 version (32MUX)
        SSD1306_MEMORY_ADDR_MODE, 0x00,                                 // 0x20 = Set Memory Addressing Mode
                                                                        // 0x00, Horizontal Addressing Mode
                                                                        // 0x01, Vertical Addressing Mode
                                                                        // 0x02, Page Addressing Mode (RESET)
        SSD1306_SET_START_LINE,                                         // 0x40
        SSD1306_DISPLAY_OFFSET, 0x00,                                   // 0xD3
        SSD1306_SEG_REMAP_OP,                                           // 0xA0 / remap 0xA1
        SSD1306_COM_SCAN_DIR_OP,                                        // 0xC0 / remap 0xC8
        SSD1306_COM_PIN_CONF, 0x12,                                     // 0xDA, 0x12 - Disable COM Left/Right remap, Alternative COM pin configuration
                                                                        //       0x12 - for 128 x 64 version
                                                                        //       0x02 - for 128 x 32 version
        SSD1306_SET_CONTRAST, 0x7F,                                     // 0x81, 0x7F - reset value (max 0xFF)
        SSD1306_DIS_ENT_DISP_ON,                                        // 0xA4
        SSD1306_DIS_NORMAL,                                             // 0xA6
        SSD1306_SET_OSC_FREQ, 0x80,                                     // 0xD5, 0x80 => D=1; DCLK = Fosc / D <=> DCLK = Fosc
        SSD1306_SET_PRECHARGE, 0xc2,                                    // 0xD9, higher value less blinking
                                                                        // 0xC2, 1st phase = 2 DCLK,  2nd phase = 13 DCLK
        SSD1306_VCOM_DESELECT, 0x20,                                    // Set V COMH Deselect, reset value 0x22 = 0,77xUcc
        SSD1306_SET_CHAR_REG, 0x14,                                     // 0x8D, Enable charge pump during display on
        SSD1306_DEACT_SCROLL,                                           // 0x2E
        SSD1306_SET_COLUMN_ADDR, 0, DISPLAY_WIDTH-1,                    // 0x21, Specifies column start address and end address / accord. mARTi-null #16
        SSD1306_SET_PAGE_ADDR, 0, (DISPLAY_HEIGHT/8)-1,                 // 0x22, Specifies page start address and end address / accord. mARTi-null #16
        SSD1306_DISPLAY_ON,                                             // 0xAF, Set Display ON
    };

    HAL_I2C_Mem_Write(DISPLAY_I2C, DISPLAY_ADDRESS, 0x00, 1, init_cmds, sizeof(init_cmds), DISPLAY_TIMEOUT);
}

void Display_setBrightness(uint8_t value){
    uint8_t payload[2] = {
        SSD1306_SET_CONTRAST,
        value
    };

    HAL_I2C_Mem_Write(DISPLAY_I2C, DISPLAY_ADDRESS, 0x00, 1, payload, 2, DISPLAY_TIMEOUT);
}

void Display_draw(){
    HAL_I2C_Mem_Write(DISPLAY_I2C, DISPLAY_ADDRESS, 0x40, 1, (uint8_t*)DISPLAY_buffer, DISPLAY_WIDTH*DISPLAY_HEIGHT/8, 100);
}

void Display_putPixel(int x, int y, bool value){
    int y_pri = y / 8;
    int y_sub = y % 8;
    DISPLAY_buffer[y_pri][x] &= ~(1 << y_sub);
    DISPLAY_buffer[y_pri][x] |= value << y_sub;
}

void Display_clear(){
    memset(DISPLAY_buffer, 0, sizeof(DISPLAY_buffer));
}


void Display_putChar(int x, int y, char c){
    for (int i = 0 ; i < FONT_WIDTH; i++){
        DISPLAY_buffer[y/8][x + i] = font_8x5[c-32][i];
    }
}

void Display_putBigChar(int x, int y, char c){
    for (int i = 0; i < 10; i ++){
        DISPLAY_buffer[(y)/8+0][x+i] = font_10x16[c-32][2*i];
        DISPLAY_buffer[(y)/8+1][x+i] = font_10x16[c-32][2*i + 1];
    }

}

void Display_print(int x, int y, char *str){
    for (int i = 0; i < strlen(str); i++){
        // Display_putChar(x+FONT_WIDTH*i, y, str[i]);
        Display_putBigChar(x+10*i, y, str[i]);
    }
}