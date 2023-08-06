#include "st3m_term.h"

#include "st3m_gfx.h"

/* a tiny dumb terminal */

#define ST3M_TERM_COLS 27
#define ST3M_TERM_LINES 16

static char st3m_term[ST3M_TERM_LINES][ST3M_TERM_COLS];
static int st3m_term_cx = 0;
static int st3m_term_cy = 0;

void st3m_term_feed(const char *str, size_t len) {
    for (int i = 0; i < len; i++) {
        char c = str[i];
        switch (c) {
            case '\t': {
                char *space_buf = " ";
                do {
                    st3m_term_feed(space_buf, 1);
                } while ((st3m_term_cx & 7) != 0);
            } break;
            case '\b':
                st3m_term_cx--;
                if (st3m_term_cx < 0) st3m_term_cx = 0;
                break;
            case '\r':
                st3m_term_cx = 0;
                break;
            case '\n':
                st3m_term_cx = 0;
                st3m_term_cy++;
                if (st3m_term_cy >= ST3M_TERM_LINES) st3m_term_cy = 0;
                memset(st3m_term[st3m_term_cy], 0, ST3M_TERM_COLS);
                break;
            default:
                st3m_term[st3m_term_cy][st3m_term_cx] = c;
                st3m_term_cx++;
                if (st3m_term_cx >= ST3M_TERM_COLS) {
                    st3m_term_cx = 0;
                    st3m_term_cy++;
                    if (st3m_term_cy >= ST3M_TERM_LINES) st3m_term_cy = 0;
                    memset(st3m_term[st3m_term_cy], 0, ST3M_TERM_COLS);
                }
        }
    }
}

// returns a line from terminal scrollback
// line_no is a value 0..15
const char *st3m_term_get_line(int line_no) {
    return st3m_term[(st3m_term_cy - line_no - 1) & 15];
}

// draws the last bit of terminal output, avoiding the last skip_lines
// lines added.
void st3m_term_draw(int skip_lines) {
    st3m_ctx_desc_t *target = st3m_gfx_drawctx_free_get(portMAX_DELAY);
    Ctx *ctx = target->ctx;
    float font_size = 20.0;
    float y = 64;

    ctx_save(ctx);

    ctx_font_size(ctx, font_size);
    ctx_text_align(ctx, CTX_TEXT_ALIGN_LEFT);  // XXX this should not be needed

    ctx_gray(ctx, 0.0);
    ctx_rectangle(ctx, -120, -120, 240, 240);
    ctx_fill(ctx);

    ctx_gray(ctx, 1.0);
    for (int i = 0; i < 7 && y > -70; i++) {
        ctx_move_to(ctx, -100, y);
        ctx_text(ctx, st3m_term_get_line(i + skip_lines));
        y -= font_size;
    }

    ctx_restore(ctx);

    st3m_gfx_drawctx_pipe_put(target);
}
