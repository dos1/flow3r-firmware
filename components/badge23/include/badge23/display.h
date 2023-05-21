#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "../../usermodule/uctx/uctx/ctx.h"

void display_init();
void display_draw_scope();
void display_update();
void display_draw_pixel(uint8_t x, uint8_t y, uint16_t col);
uint16_t display_get_pixel(uint8_t x, uint8_t y);
void display_fill(uint16_t col);

extern Ctx *the_ctx;
