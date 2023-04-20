//--------------------------------------------------------------------------------------------------------
// Nadyrshin Ruslan - [YouTube-channel: https://www.youtube.com/channel/UChButpZaL5kUUl_zTyIDFkQ]
// Liyanboy74
//--------------------------------------------------------------------------------------------------------
#ifndef _GC9A01_H
#define _GC9A01_H

#include "sdkconfig.h"
#include <stdint.h>

#define CONFIG_GC9A01_Width 240
#define CONFIG_GC9A01_Height 240
//#define # CONFIG_USE_SPI1_HOST is not set
//#define # CONFIG_USE_SPI2_HOST is not set
#define CONFIG_USE_SPI3_HOST 1
//#define # CONFIG_USE_SPI4_HOST is not set
#define CONFIG_GC9A01_SPI_HOST 2
#define CONFIG_GC9A01_PIN_NUM_SCK 39
#define CONFIG_GC9A01_PIN_NUM_MOSI 41
#define CONFIG_GC9A01_PIN_NUM_CS 40
#define CONFIG_GC9A01_PIN_NUM_DC 42
#define CONFIG_GC9A01_SPI_SCK_FREQ_M 80
//#define # CONFIG_GC9A01_CONTROL_BACK_LIGHT_USED is not set
#define CONFIG_GC9A01_RESET_USED 1
#define CONFIG_GC9A01_PIN_NUM_RST 38
#define CONFIG_GC9A01_BUFFER_MODE 1
//#define # CONFIG_GC9A01_BUFFER_SCREEN_FAST_MODE is not set
//#define # end of GC9A01 LCD Config


#define GC9A01_Width	CONFIG_GC9A01_Width
#define GC9A01_Height 	CONFIG_GC9A01_Height

extern uint16_t ScreenBuff[GC9A01_Height * GC9A01_Width];

static inline void GC9A01_DrawPixel(int16_t x, int16_t y, uint16_t color) {
		if ((x < 0) || (x >= GC9A01_Width) || (y < 0) || (y >= GC9A01_Height))
			return;

		//SwapBytes(&color);

		ScreenBuff[y * GC9A01_Width + x] = color;
	}


uint16_t GC9A01_GetWidth();
uint16_t GC9A01_GetHeight();

void GC9A01_Init();
void GC9A01_SleepMode(uint8_t Mode);
void GC9A01_DisplayPower(uint8_t On);
void GC9A01_DrawPixel(int16_t x, int16_t y, uint16_t color);
void GC9A01_FillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void GC9A01_Update();
void GC9A01_SetBL(uint8_t Value);
uint16_t GC9A01_GetPixel(int16_t x, int16_t y);

void GC9A01_Screen_Shot(uint16_t x,uint16_t y,uint16_t width ,uint16_t height,uint16_t * Buffer);
void GC9A01_Screen_Load(uint16_t x,uint16_t y,uint16_t width ,uint16_t height,uint16_t * Buffer);

#endif
