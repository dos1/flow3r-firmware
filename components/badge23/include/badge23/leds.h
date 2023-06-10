#pragma once

#include <stdint.h>
#include <stdbool.h>

/* inits stuff ig
 */
void leds_init();

/* Set a single/all LEDs to change color with the next (auto) update. Index starts above
 * USB-C port and increments ccw. There are 8 LEDs per top petal.
 */
void leds_set_single_rgb(uint8_t index, uint8_t red, uint8_t green, uint8_t blue);
void leds_set_single_hsv(uint8_t index, float hue, float sat, float value);
void leds_set_all_rgb(uint8_t red, uint8_t green, uint8_t blue);
void leds_set_all_hsv(float hue, float sat, float value);

/* Set/get global LED brightness. Default 69.
 */
void leds_set_brightness(uint8_t brightness);
uint8_t leds_get_brightness();

/* Set/get maximum change rate of brightness. Set to 1-3 for fade effects, set to
 * 255 to disable. Currently clocks at 10Hz.
 */
void leds_set_slew_rate(uint8_t slew_rate);
uint8_t leds_get_slew_rate();

/* Update LEDs. 
 */
void leds_update();

/* Used internally for slew rate animation loop.
 */
void leds_update_hardware(); 

/* Bend the rgb curves with an exponent each. (1,1,1) is default, (2,2,2) works well too
 * If someone wants to do color calibration, this is ur friend
 */
void leds_set_gamma(float red, float green, float blue); 

/* leds_update is periodically called by the background task for slew rate animation when
 * set to 1. This adds user changes immediately. Useful with low slew rates.
 */
void leds_set_auto_update(bool on);
bool leds_get_auto_update();
