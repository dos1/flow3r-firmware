#pragma once
#include <stdint.h>

#define BUTTON_PRESSED_DOWN 2
#define BUTTON_PRESSED_LEFT -1
#define BUTTON_PRESSED_RIGHT 1
#define BUTTON_NOT_PRESSED 0

#define BADGE_LINK_PIN_MASK_LINE_IN_TIP      0b0001
#define BADGE_LINK_PIN_MASK_LINE_IN_RING     0b0010
#define BADGE_LINK_PIN_MASK_LINE_OUT_TIP     0b0100
#define BADGE_LINK_PIN_MASK_LINE_OUT_RING    0b1000
#define BADGE_LINK_PIN_MASK_LINE_IN ((BADGE_LINK_PIN_MASK_LINE_IN_TIP) | (BADGE_LINK_PIN_MASK_LINE_IN_RING))
#define BADGE_LINK_PIN_MASK_LINE_OUT ((BADGE_LINK_PIN_MASK_LINE_OUT_TIP) | (BADGE_LINK_PIN_MASK_LINE_OUT_RING))
#define BADGE_LINK_PIN_MASK_ALL ((BADGE_LINK_PIN_MASK_LINE_IN) | (BADGE_LINK_PIN_MASK_LINE_OUT))

#define BADGE_LINK_PIN_INDEX_LINE_IN_TIP 4
#define BADGE_LINK_PIN_INDEX_LINE_IN_RING 5
#define BADGE_LINK_PIN_INDEX_LINE_OUT_TIP 6
#define BADGE_LINK_PIN_INDEX_LINE_OUT_RING 7

/* Initializes GPIO modes, prefills structs, etc. Call before using library.
 */
void init_buttons();

/* Gets data from I2C portexpanders and GPIOs. Requires I2C lock.
 */
void update_button_state();

/* UI sugar: People might prefer using one button for in-application stuff and the other
 * for entering main menu/volume control, depending on their handedness and how they hold
 * the badge. This function allows them configure this and is meant to be only be used by
 * the OS user config handler.
 *
 * Set to 1 to use the left shoulder button as the menu button, 0 for the right
 */
void spio_menu_button_set_left(bool left);

/* Gets user menu button user preference as set with spio_menu_button_set_left.
 */
int8_t spio_menu_button_get_left();

/* Read the state of the menu/application button at the last call of update_button_state.
 * Compare with BUTTON_(NOT)PRESSED* constants for -h.
 */
int8_t spio_menu_button_get();
int8_t spio_application_button_get();

/* Read the state of the left/right button at the last call of update_button_state.
 * Compare with BUTTON_(NOT)PRESSED* constants for -h.
 * This ignores user preference and should be used only with good reason.
 */
int8_t spio_left_button_get();
int8_t spio_right_button_get();

/* Gets active badge links ports. Mask with BADGE_LINK_PIN_MASK_LINE_{IN/OUT}_{TIP/RING}. The corresponding
 * GPIO indices are listed in BADGE_LINK_PIN_INDEX_LINE_{OUT/IN}_{TIP/RING}.
 */
uint8_t spio_badge_link_get_active(uint8_t pin_mask);

/* Disables badge link ports. Mask with BADGE_LINK_PIN_MASK_LINE_{IN/OUT}_{TIP/RING}. The corresponding
 * GPIO indices are listed in BADGE_LINK_PIN_INDEX_LINE_{OUT/IN}_{TIP/RING}.
 * Returns the output of spio_badge_link_get_active after execution.
 */
uint8_t spio_badge_link_disable(uint8_t pin_mask);

/* Enables badge link ports. Mask with BADGE_LINK_PIN_MASK_LINE_{IN/OUT}_{TIP/RING}. The corresponding
 * GPIO indices are listed in BADGE_LINK_PIN_INDEX_LINE_{OUT/IN}_{TIP/RING}_PIN.
 * Returns the output of spio_badge_link_get_active after execution.
 *
 * Do NOT connect headphones to a badge link port. You might hear a ringing for a while. Warn user.
 */
uint8_t spio_badge_link_enable(uint8_t pin_mask);
