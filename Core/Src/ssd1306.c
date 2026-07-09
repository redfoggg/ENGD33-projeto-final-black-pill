#include "ssd1306.h"
#include <string.h>

/* Framebuffer em RAM: 1 byte = 8 pixels verticais (page addressing). */
static uint8_t ssd1306_buffer[SSD1306_BUFFER_SIZE];



/* ============================ TRANSPORTE I2C ============================ */
#define SSD1306_I2C_TIMEOUT_MS 20

__attribute__((used)) volatile uint32_t ssd1306_i2c_ok_count = 0;
__attribute__((used)) volatile uint32_t ssd1306_i2c_err_count = 0;
__attribute__((used)) volatile HAL_StatusTypeDef ssd1306_last_status = HAL_OK;

static void SSD1306_WriteCommand(uint8_t cmd)
{
	uint8_t payload[2] = {0x00, cmd};

	HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(
		&SSD1306_I2C_PORT,
		SSD1306_I2C_ADDR,
		payload,
		2,
		SSD1306_I2C_TIMEOUT_MS
	);

	ssd1306_last_status = status;

	if (status == HAL_OK) {
		ssd1306_i2c_ok_count++;
	} else {
		ssd1306_i2c_err_count++;
	}
}

static void SSD1306_WriteData(uint8_t *data, uint16_t len)
{
	HAL_StatusTypeDef status = HAL_I2C_Mem_Write(
		&SSD1306_I2C_PORT,
		SSD1306_I2C_ADDR,
		0x40,
		I2C_MEMADD_SIZE_8BIT,
		data,
		len,
		SSD1306_I2C_TIMEOUT_MS
	);

	ssd1306_last_status = status;

	if (status == HAL_OK) {
		ssd1306_i2c_ok_count++;
	} else {
		ssd1306_i2c_err_count++;
	}
}



/* ============================ INICIALIZACAO ============================ */

void SSD1306_Init(void)
{
	HAL_Delay(100);						/* aguarda estabilizacao do VCC do modulo */

	SSD1306_WriteCommand(0xAE);			/* display off */
	SSD1306_WriteCommand(0x20);			/* memory addressing mode */
	SSD1306_WriteCommand(0x00);			/* horizontal addressing */
	SSD1306_WriteCommand(0xB0);			/* page start address */
	SSD1306_WriteCommand(0xC8);			/* COM scan direction remap */
	SSD1306_WriteCommand(0x00);			/* low column address */
	SSD1306_WriteCommand(0x10);			/* high column address */
	SSD1306_WriteCommand(0x40);			/* start line address */
	SSD1306_WriteCommand(0x81);			/* contrast control */
	SSD1306_WriteCommand(0xFF);
	SSD1306_WriteCommand(0xA1);			/* segment remap */
	SSD1306_WriteCommand(0xA6);			/* display normal (nao invertido) */
	SSD1306_WriteCommand(0xA8);			/* multiplex ratio */
	SSD1306_WriteCommand(0x3F);			/* 1/64 duty */
	SSD1306_WriteCommand(0xA4);			/* saida segue o conteudo da RAM */
	SSD1306_WriteCommand(0xD3);			/* display offset */
	SSD1306_WriteCommand(0x00);
	SSD1306_WriteCommand(0xD5);			/* clock divide / freq */
	SSD1306_WriteCommand(0x80);
	SSD1306_WriteCommand(0xD9);			/* pre-charge period */
	SSD1306_WriteCommand(0xF1);
	SSD1306_WriteCommand(0xDA);			/* COM pins hardware config */
	SSD1306_WriteCommand(0x12);
	SSD1306_WriteCommand(0xDB);			/* VCOMH deselect level */
	SSD1306_WriteCommand(0x40);
	SSD1306_WriteCommand(0x8D);			/* charge pump */
	SSD1306_WriteCommand(0x14);			/* charge pump on */
	SSD1306_WriteCommand(0xAF);			/* display on */

	SSD1306_Fill(SSD1306_BLACK);
	SSD1306_UpdateScreen();
}

void SSD1306_UpdateScreen(void)
{
	SSD1306_WriteCommand(0x21);			/* column address */
	SSD1306_WriteCommand(0x00);
	SSD1306_WriteCommand(SSD1306_WIDTH - 1);
	SSD1306_WriteCommand(0x22);			/* page address */
	SSD1306_WriteCommand(0x00);
	SSD1306_WriteCommand((SSD1306_HEIGHT / 8) - 1);

	SSD1306_WriteData(ssd1306_buffer, SSD1306_BUFFER_SIZE);
}

/* ============================ PRIMITIVAS ============================ */

void SSD1306_Fill(uint16_t color)
{
	memset(ssd1306_buffer, (color != SSD1306_BLACK) ? 0xFF : 0x00, SSD1306_BUFFER_SIZE);
}

void SSD1306_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
	if ((x >= SSD1306_WIDTH) || (y >= SSD1306_HEIGHT)) return;

	if (color != SSD1306_BLACK)
	{
		ssd1306_buffer[x + (y / 8) * SSD1306_WIDTH] |= (1 << (y & 0x07));
	}
	else
	{
		ssd1306_buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y & 0x07));
	}
}

void SSD1306_FillRectangle(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
	for (uint16_t i = 0; i < width; i++)
	{
		for (uint16_t j = 0; j < height; j++)
		{
			SSD1306_DrawPixel(x + i, y + j, color);
		}
	}
}

void SSD1306_DrawHLine(uint16_t x, uint16_t y, uint16_t width, uint16_t color)
{
	for (uint16_t i = 0; i < width; i++)
	{
		SSD1306_DrawPixel(x + i, y, color);
	}
}

void SSD1306_DrawVLine(uint16_t x, uint16_t y, uint16_t height, uint16_t color)
{
	for (uint16_t j = 0; j < height; j++)
	{
		SSD1306_DrawPixel(x, y + j, color);
	}
}

/* ====================== TEXTO (motor reaproveitado) ====================== */
/* Logica de unpack identica a ILI9341_GFX.c (DrawChar/DrawText), trocando as
 * primitivas por SSD1306_* — assim as tabelas em fonts.c sao reaproveitadas. */

void SSD1306_DrawChar(char ch, const uint8_t font[], uint16_t X, uint16_t Y, uint16_t color, uint16_t bgcolor)
{
	if ((ch < 31) || (ch > 127)) return;

	uint8_t fOffset = font[0];
	uint8_t fWidth  = font[1];
	uint8_t fHeight = font[2];
	uint8_t fBPL    = font[3];

	uint8_t *tempChar = (uint8_t*)&font[((ch - 0x20) * fOffset) + 4];

	/* Limpa o fundo da celula do caractere */
	SSD1306_FillRectangle(X, Y, fWidth, fHeight, bgcolor);

	for (int j = 0; j < fHeight; j++)
	{
		for (int i = 0; i < fWidth; i++)
		{
			uint8_t z = tempChar[fBPL * i + ((j & 0xF8) >> 3) + 1];
			uint8_t b = 1 << (j & 0x07);
			if ((z & b) != 0x00)
			{
				SSD1306_DrawPixel(X + i, Y + j, color);
			}
		}
	}
}

void SSD1306_DrawText(const char* str, const uint8_t font[], uint16_t X, uint16_t Y, uint16_t color, uint16_t bgcolor)
{
	uint8_t fOffset = font[0];
	uint8_t fWidth  = font[1];

	while (*str)
	{
		SSD1306_DrawChar(*str, font, X, Y, color, bgcolor);

		uint8_t *tempChar = (uint8_t*)&font[((*str - 0x20) * fOffset) + 4];
		uint8_t charWidth = tempChar[0];

		if (charWidth + 2 < fWidth)
		{
			X += (charWidth + 2);
		}
		else
		{
			X += fWidth;
		}

		str++;
	}
}
