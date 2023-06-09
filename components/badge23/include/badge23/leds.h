#pragma once

#include <stdint.h>
#include <stdbool.h>
void leds_init();
void leds_set_single_rgb(uint8_t index, uint8_t red, uint8_t green, uint8_t blue);
void leds_set_single_hsv(uint8_t index, float hue, float sat, float value);
void leds_set_all_rgb(uint8_t red, uint8_t green, uint8_t blue);
void leds_set_all_hsv(float hue, float sat, float value);
void leds_set_brightness(uint8_t brightness);
uint8_t leds_get_brightness();
void leds_set_slew_rate(uint8_t slew_rate);
uint8_t leds_get_slew_rate();
void leds_update();
void leds_update_hardware(); 
void leds_set_gamma(float red, float green, float blue); 
void leds_set_auto_update(bool on);
bool leds_get_auto_update();
