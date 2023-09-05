#include "rec_gui.h"

#include "st3m_gfx.h"
#include "st3m_io.h"
#include "st3m_version.h"

// clang-format off
#include "ctx_config.h"
#include "ctx.h"
// clang-format on

static void _header_draw(Ctx *ctx) {
    // Draw background.
    ctx_rgb(ctx, 0.29, 0.10, 0.35);
    ctx_rectangle(ctx, -120, -120, 240, 240);
    ctx_fill(ctx);

    // Draw header.
    ctx_gray(ctx, 1.0);
    ctx_text_align(ctx, CTX_TEXT_ALIGN_CENTER);
    ctx_text_baseline(ctx, CTX_TEXT_BASELINE_MIDDLE);

    ctx_font_size(ctx, 15.0);
    ctx_move_to(ctx, 0, -100);
    ctx_text(ctx, "oh no it's");

    ctx_font_size(ctx, 30.0);
    ctx_move_to(ctx, 0, -80);
    ctx_text(ctx, "Recovery");
    ctx_move_to(ctx, 0, -55);
    ctx_text(ctx, "Mode");

    // Draw version.
    ctx_font_size(ctx, 15.0);
    ctx_gray(ctx, 0.6);
    ctx_move_to(ctx, 0, 100);
    ctx_text(ctx, st3m_version);
}

void rec_erasing_draw(void) {
    Ctx *ctx = st3m_gfx_ctx(st3m_gfx_default);
    _header_draw(ctx);

    ctx_move_to(ctx, 0, 0);
    ctx_font_size(ctx, 20.0);
    ctx_gray(ctx, 0.8);
    ctx_text(ctx, "Erasing...");

    st3m_gfx_end_frame(ctx);
}

void rec_flashing_draw(int percent) {
    Ctx *ctx = st3m_gfx_ctx(st3m_gfx_default);
    _header_draw(ctx);

    ctx_move_to(ctx, 0, 0);
    ctx_font_size(ctx, 20.0);
    ctx_gray(ctx, 0.8);

    char buf[128];
    snprintf(buf, sizeof(buf), "Flashing... %2d%%", percent);
    ctx_text(ctx, buf);

    ctx_rectangle(ctx, -120, 20, 240 * percent / 100, 20);
    ctx_fill(ctx);

    st3m_gfx_end_frame(ctx);
}

void rec_menu_draw(menu_t *menu) {
    Ctx *ctx = st3m_gfx_ctx(st3m_gfx_default);
    _header_draw(ctx);

    int y = -20;
    ctx_font_size(ctx, 15.0);
    ctx_gray(ctx, 0.8);
    if (menu->help != NULL) {
        const char *help = menu->help;
        const char *next = NULL;
        while ((next = strstr(help, "\n")) != NULL) {
            size_t len = next - help;
            char *line = malloc(len + 1);
            memcpy(line, help, len);
            line[len] = 0;

            ctx_move_to(ctx, 0, y);
            ctx_text(ctx, line);
            free(line);
            y += 15;
            help = next + 1;
        }
        ctx_move_to(ctx, 0, y);
        ctx_text(ctx, help);
        y += 25;
    }

    ctx_font_size(ctx, 18.0);
    for (int i = 0; i < menu->entries_count; i++) {
        menu_entry_t *entry = &menu->entries[i];
        if (i == menu->selected) {
            ctx_gray(ctx, 1.0);
            ctx_rectangle(ctx, -120, y - 9, 240, 18);
            ctx_fill(ctx);
            ctx_rgb(ctx, 0.29, 0.10, 0.35);
        } else if (entry->disabled) {
            ctx_gray(ctx, 0.5);
        } else {
            ctx_gray(ctx, 1.0);
        }
        ctx_move_to(ctx, 0, y);
        ctx_text(ctx, entry->label);
        y += 18;
    }

    st3m_gfx_end_frame(ctx);
}

void rec_menu_process(menu_t *menu) {
    static int debounce = 0;
    menu_entry_t *entry = &menu->entries[menu->selected];

    st3m_tripos left = st3m_io_app_button_get();

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
