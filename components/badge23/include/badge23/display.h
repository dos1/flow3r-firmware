#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "../../usermodule/uctx/uctx/ctx.h"

void display_init();
void display_ctx_init();
void display_update();
void display_draw_pixel(uint8_t x, uint8_t y, uint16_t col);
uint16_t display_get_pixel(uint8_t x, uint8_t y);
void display_fill(uint16_t col);
void display_set_backlight(uint8_t percent);

extern volatile Ctx *the_ctx;
