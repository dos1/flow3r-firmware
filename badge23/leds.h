#pragma once

#include <stdint.h>
void leds_init();
void leds_animate(int leaf);
void leds_set_single_rgb(uint8_t index, uint8_t red, uint8_t green, uint8_t blue);
void leds_set_single_hsv(uint8_t index, float hue, float sat, float value);
void leds_update();

