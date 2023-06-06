#pragma once
#include <stdint.h>

#define BUTTON_PRESSED_DOWN 2
#define BUTTON_PRESSED_LEFT -1
#define BUTTON_PRESSED_RIGHT 1
#define BUTTON_NOT_PRESSED 0

int8_t get_button_state(bool leftbutton);
void update_button_state();
void init_buttons();

int8_t spio_menu_button_get();
int8_t spio_application_button_get();
int8_t spio_left_button_get();
int8_t spio_right_button_get();
int8_t spio_menu_button_get_left();
void spio_menu_button_set_left(bool left);
