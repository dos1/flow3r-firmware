#pragma once

#include <stdbool.h>
#include <stdint.h>

// LEDs are triple-buffered:
//
// 1. The first buffer is written to directly by st3m_leds_set_*.
// 2. When st3m_leds_update is called, the first buffer is copied into a
//    secondary buffer.
// 3. Whenever the hardware is ready, the state of the second buffer is
//    transmitted to the LEDs.
//
// Enabling auto-updates makes the second step unnecessary.

// Initialized the LED subsystem.
void st3m_leds_init();

// Set a single/all LEDs to change color with the next (auto) update. Index
// starts above USB-C port and increments clockwise.
//
// There are 8 LEDs per top petal, or 4 LEDs per petal.
//
// After you're ready setting up your blink, call st3m_leds_update, or enable
// autoupdates.
void st3m_leds_set_single_rgb(uint8_t index, float red, float green,
                              float blue);
void st3m_leds_set_single_rgba(uint8_t index, float red, float green,
                               float blue, float alpha);
void st3m_leds_set_single_hsv(uint8_t index, float hue, float sat, float value);
void st3m_leds_set_all_rgb(float red, float green, float blue);
void st3m_leds_set_all_rgba(float red, float green, float blue, float alpha);
void st3m_leds_set_all_hsv(float hue, float sat, float value);
void st3m_leds_get_single_rgb(uint8_t index, float* red, float* green,
                              float* blue);

// Set/get global LED brightness, 0-255. Default 69.
//
// Only affects the LEDs after st3m_leds_update, or if autoupdate is enabled.
void st3m_leds_set_brightness(uint8_t brightness);
uint8_t st3m_leds_get_brightness();

// Set/get maximum change rate of brightness. Set to 1-3 for fade effects, set
// to 255 to disable. Currently clocks at 50Hz.
void st3m_leds_set_slew_rate(uint8_t slew_rate);
uint8_t st3m_leds_get_slew_rate();

void st3m_leds_set_max_slew_rate(uint8_t slew_rate);
uint8_t st3m_leds_get_max_slew_rate();

// Update LEDs. Ie., copy the LED state from the first buffer into the second
// buffer, effectively scheduling the LED state to be presented to the user.
void st3m_leds_update();

// Bend the rgb curves with an exponent each. (1,1,1) is default, (2,2,2) works
// well too If someone wants to do color calibration, this is ur friend
void st3m_leds_set_gamma(float red, float green, float blue);

// Make st3m_leds_update be periodically called by the background task for slew
// rate animation when set to 1. This adds user changes immediately. Useful with
// low slew rates.
void st3m_leds_set_auto_update(bool on);
bool st3m_leds_get_auto_update();
