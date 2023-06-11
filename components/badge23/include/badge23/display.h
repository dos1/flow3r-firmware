#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "../../usermodule/uctx/uctx/ctx.h"

void display_init();
void display_set_backlight(uint8_t percent);

// Transitional functionality operating on global framebuffer and global ctx.
// Will get replaced with 'real' ctx rendering system.
void display_ctx_init();
void display_update();
Ctx *display_global_ctx(void);
