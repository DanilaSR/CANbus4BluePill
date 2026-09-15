#ifndef __SSD1306_H
#define __SSD1306_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

/* 0.91" OLED = 128x32 */
#define SSD1306_WIDTH   128
#define SSD1306_HEIGHT  32

/* I2C address: 0x3C << 1 (default SA0=0) */
#define SSD1306_ADDR    0x3CU << 1

/* SSD1306 command bytes */
#define SSD1306_CMD_SETCONTRAST    0x81
#define SSD1306_CMD_DISPLAYALLON_RESUME 0xA4
#define SSD1306_CMD_DISPLAYALLON   0xA5
#define SSD1306_CMD_NORMALDISPLAY  0xA6
#define SSD1306_CMD_INVERTDISPLAY  0xA7
#define SSD1306_CMD_DISPLAYOFF     0xAE
#define SSD1306_CMD_DISPLAYON      0xAF
#define SSD1306_CMD_SETDISPLAYOFFSET 0xD3
#define SSD1306_CMD_SETCOMPINS     0xDA
#define SSD1306_CMD_SETVCOMDETECT  0xDB
#define SSD1306_CMD_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_CMD_SETPRECHARGE   0xD9
#define SSD1306_CMD_SETMULTIPLEX   0xA8
#define SSD1306_CMD_SETLOWCOLUMN   0x00
#define SSD1306_CMD_SETHIGHCOLUMN  0x10
#define SSD1306_CMD_SETSTARTLINE   0x40
#define SSD1306_CMD_MEMORYMODE     0x20
#define SSD1306_CMD_COLUMNADDR     0x21
#define SSD1306_CMD_PAGEADDR       0x22
#define SSD1306_CMD_COMSCANINC     0xC0
#define SSD1306_CMD_COMSCANDEC     0xC8
#define SSD1306_CMD_SEGREMAP       0xA0
#define SSD1306_CMD_CHARGEPUMP     0x8D

typedef struct {
    I2C_HandleTypeDef *hi2c;
    uint8_t buf[(SSD1306_WIDTH * SSD1306_HEIGHT) / 8];
} SSD1306_t;

HAL_StatusTypeDef SSD1306_Init(SSD1306_t *d, I2C_HandleTypeDef *hi2c);
void SSD1306_Clear(SSD1306_t *d);
void SSD1306_DrawPixel(SSD1306_t *d, uint8_t x, uint8_t y, uint8_t on);
void SSD1306_DrawChar(SSD1306_t *d, uint8_t x, uint8_t y, char c, uint8_t on);
void SSD1306_DrawString(SSD1306_t *d, uint8_t x, uint8_t y, const char *str, uint8_t on);
HAL_StatusTypeDef SSD1306_Update(SSD1306_t *d);

#ifdef __cplusplus
}
#endif

#endif /* __SSD1306_H */
