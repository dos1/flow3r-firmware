#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct {
    const char *label;
    void (*enter)(void);
    bool disabled;
} menu_entry_t;

typedef struct {
    const char *help;
    menu_entry_t *entries;
    size_t entries_count;
    int selected;
} menu_t;

void rec_erasing_draw(void);
void rec_menu_draw(menu_t *menu);
void rec_menu_process(menu_t *menu);