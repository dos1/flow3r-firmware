#include "rec_gui.h"

#include "st3m_gfx.h"
#include "st3m_io.h"
#include "st3m_version.h"

// clang-format off
#include "ctx_config.h"
#include "ctx.h"
// clang-format on

static void _header_draw(st3m_ctx_desc_t *target) {
    // Draw background.
    ctx_rgb(target->ctx, 0.29, 0.10, 0.35);
    ctx_rectangle(target->ctx, -120, -120, 240, 240);
    ctx_fill(target->ctx);

    // Draw header.
    ctx_gray(target->ctx, 1.0);
    ctx_text_align(target->ctx, CTX_TEXT_ALIGN_CENTER);
    ctx_text_baseline(target->ctx, CTX_TEXT_BASELINE_MIDDLE);

    ctx_font_size(target->ctx, 15.0);
    ctx_move_to(target->ctx, 0, -100);
    ctx_text(target->ctx, "oh no it's");

    ctx_font_size(target->ctx, 30.0);
    ctx_move_to(target->ctx, 0, -80);
    ctx_text(target->ctx, "Recovery");
    ctx_move_to(target->ctx, 0, -55);
    ctx_text(target->ctx, "Mode");

    // Draw version.
    ctx_font_size(target->ctx, 15.0);
    ctx_gray(target->ctx, 0.6);
    ctx_move_to(target->ctx, 0, 100);
    ctx_text(target->ctx, st3m_version);
}

void rec_erasing_draw(void) {
    st3m_ctx_desc_t *target = st3m_gfx_drawctx_free_get(portMAX_DELAY);
    _header_draw(target);

    ctx_move_to(target->ctx, 0, 0);
    ctx_font_size(target->ctx, 20.0);
    ctx_gray(target->ctx, 0.8);
    ctx_text(target->ctx, "Erasing...");

    st3m_gfx_drawctx_pipe_put(target);
}

void rec_flashing_draw(int percent) {
    st3m_ctx_desc_t *target = st3m_gfx_drawctx_free_get(portMAX_DELAY);
    _header_draw(target);

    ctx_move_to(target->ctx, 0, 0);
    ctx_font_size(target->ctx, 20.0);
    ctx_gray(target->ctx, 0.8);

    char buf[128];
    snprintf(buf, sizeof(buf), "Flashing... %2d%%", percent);
    ctx_text(target->ctx, buf);

    ctx_rectangle(target->ctx, -120, 20, 240 * percent / 100, 20);
    ctx_fill(target->ctx);

    st3m_gfx_drawctx_pipe_put(target);
}

void rec_menu_draw(menu_t *menu) {
    st3m_ctx_desc_t *target = st3m_gfx_drawctx_free_get(portMAX_DELAY);
    _header_draw(target);

    int y = -20;
    ctx_font_size(target->ctx, 15.0);
    ctx_gray(target->ctx, 0.8);
    if (menu->help != NULL) {
        const char *help = menu->help;
        const char *next = NULL;
        while ((next = strstr(help, "\n")) != NULL) {
            size_t len = next - help;
            char *line = malloc(len + 1);
            memcpy(line, help, len);
            line[len] = 0;

            ctx_move_to(target->ctx, 0, y);
            ctx_text(target->ctx, line);
            free(line);
            y += 15;
            help = next + 1;
        }
        ctx_move_to(target->ctx, 0, y);
        ctx_text(target->ctx, help);
        y += 25;
    }

    ctx_font_size(target->ctx, 18.0);
    for (int i = 0; i < menu->entries_count; i++) {
        menu_entry_t *entry = &menu->entries[i];
        if (i == menu->selected) {
            ctx_gray(target->ctx, 1.0);
            ctx_rectangle(target->ctx, -120, y - 9, 240, 18);
            ctx_fill(target->ctx);
            ctx_rgb(target->ctx, 0.29, 0.10, 0.35);
        } else if (entry->disabled) {
            ctx_gray(target->ctx, 0.5);
        } else {
            ctx_gray(target->ctx, 1.0);
        }
        ctx_move_to(target->ctx, 0, y);
        ctx_text(target->ctx, entry->label);
        y += 18;
    }

    st3m_gfx_drawctx_pipe_put(target);
}

void rec_menu_process(menu_t *menu) {
    static int debounce = 0;
    menu_entry_t *entry = &menu->entries[menu->selected];

    st3m_tripos left = st3m_io_left_button_get();

    if (debounce > 0) {
        if (left == st3m_tripos_none) debounce--;
        return;
    }

    if (left != st3m_tripos_none) {
        bool going_left = false;
        bool going_right = false;
        switch (left) {
            case st3m_tripos_left:
                menu->selected--;
                going_left = true;
                break;
            case st3m_tripos_right:
                menu->selected++;
                going_right = true;
                break;
            case st3m_tripos_mid:
                if (entry->enter != NULL) {
                    entry->enter();
                }
                break;
            default:
                break;
        }
        for (;;) {
            if (menu->selected < 0) {
                menu->selected = 0;
            }
            if (menu->selected >= menu->entries_count) {
                menu->selected = menu->entries_count - 1;
            }
            menu_entry_t *entry = &menu->entries[menu->selected];
            if (entry->disabled && (going_left || going_right)) {
                // TODO(q3k): handle case when all menu items are disabled?
                if (going_left) {
                    menu->selected--;
                } else if (going_right) {
                    menu->selected++;
                }
            } else {
                break;
            }
        }
        debounce = 2;
    }
}
