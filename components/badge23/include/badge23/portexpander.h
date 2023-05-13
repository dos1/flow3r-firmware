#pragma once

#include <stdbool.h>

void portexpander_set_badgelink(const bool enabled);
void portexpander_set_leds(const bool enabled);
void portexpander_set_lcd_reset(const bool enabled);
bool portexpander_rev6(void);
void portexpander_init(void);
