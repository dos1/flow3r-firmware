#pragma once

#include <stdbool.h>

void portexpander_set_badgelink(const bool enabled);
void portexpander_set_leds(const bool enabled);
void portexpander_set_lcd_reset(const bool enabled);
void portexpander_init(void);
