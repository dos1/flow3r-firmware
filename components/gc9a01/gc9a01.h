//--------------------------------------------------------------------------------------------------------
// Nadyrshin Ruslan - [YouTube-channel: https://www.youtube.com/channel/UChButpZaL5kUUl_zTyIDFkQ]
// Liyanboy74
//--------------------------------------------------------------------------------------------------------
#ifndef _GC9A01_H
#define _GC9A01_H

#include "sdkconfig.h"
#include <stdint.h>
#include "badge23_hwconfig.h"


#ifdef CONFIG_GC9A01_PIN_NUM_MOSI
#warning "idf menuconfig is ignored for display config, it breaks hardware revision switching atm"
#endif

#if defined(CONFIG_BADGE23_HW_GEN_P1)
#define USE_SPI3_HOST 1
#define GC9A01_SPI_HOST 2
#define GC9A01_PIN_NUM_SCK 39
#define GC9A01_PIN_NUM_MOSI 41
#define GC9A01_PIN_NUM_CS 40
#define GC9A01_PIN_NUM_DC 42
#define GC9A01_SPI_SCK_FREQ_M 80
#define GC9A01_RESET_USED 1
#define GC9A01_PIN_NUM_RST 38
#define GC9A01_BUFFER_MODE 1

#elif defined(CONFIG_BADGE23_HW_GEN_P3) || defined(CONFIG_BADGE23_HW_GEN_P4) || defined(CONFIG_BADGE23_HW_GEN_P6)
#define USE_SPI3_HOST 1
#define GC9A01_SPI_HOST 2
#define GC9A01_PIN_NUM_SCK 41
#define GC9A01_PIN_NUM_MOSI 42
#define GC9A01_PIN_NUM_CS 40
#define GC9A01_PIN_NUM_DC 38
#define GC9A01_SPI_SCK_FREQ_M 80
#define GC9A01_CONTROL_BACK_LIGHT_USED 1
#define GC9A01_PIN_NUM_BCKL 46
#define GC9A01_BACK_LIGHT_MODE_PWM 1
#define GC9A01_CONTROL_BACK_LIGHT_MODE 1
#define GC9A01_BUFFER_MODE 1

#else
#error "gc9a01 unimplemented for this badge generation"
#endif

#define GC9A01_Width	240
#define GC9A01_Height 	240

extern uint16_t ScreenBuff[GC9A01_Height * GC9A01_Width];

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
