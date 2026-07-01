#ifndef SSD1306_H
#define SSD1306_H

#include "main.h"
#include "stm32f4xx_hal.h"
#include "fonts.h"

/* Handle do periferico I2C gerado pelo CubeMX (MX_I2C1_Init em main.c) */
extern I2C_HandleTypeDef hi2c1;

/* Configuracao do display */
#define SSD1306_I2C_PORT        hi2c1
#define SSD1306_I2C_ADDR        (0x3C << 1)   /* 0x3C 7-bit -> 0x78 no formato HAL */

#define SSD1306_WIDTH           128
#define SSD1306_HEIGHT          64
#define SSD1306_BUFFER_SIZE     (SSD1306_WIDTH * SSD1306_HEIGHT / 8)

/* Cores (display monocromatico): 0 = pixel apagado, !=0 = pixel aceso */
#define SSD1306_BLACK           0
#define SSD1306_WHITE           1

/* Inicializacao e atualizacao da tela */
void SSD1306_Init(void);
void SSD1306_UpdateScreen(void);
void SSD1306_Fill(uint16_t color);

/* Primitivas de desenho (mesma API que o motor de texto consome) */
void SSD1306_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void SSD1306_FillRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void SSD1306_DrawHLine(uint16_t x, uint16_t y, uint16_t width, uint16_t color);
void SSD1306_DrawVLine(uint16_t x, uint16_t y, uint16_t height, uint16_t color);

/* Texto (reaproveita as fontes em fonts.c) */
void SSD1306_DrawChar(char ch, const uint8_t font[], uint16_t X, uint16_t Y, uint16_t color, uint16_t bgcolor);
void SSD1306_DrawText(const char* str, const uint8_t font[], uint16_t X, uint16_t Y, uint16_t color, uint16_t bgcolor);

#endif /* SSD1306_H */
