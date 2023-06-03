#pragma once

// Keep in sync with defines in usermodule/uctx/uctx.c.

#define CTX_TINYVG    1
#define CTX_TVG_STDIO 0
#define CTX_DITHER    1

#define CTX_PDF                       0
#define CTX_PROTOCOL_U8_COLOR         1
#define CTX_AVOID_CLIPPED_SUBDIVISION 0
#define CTX_LIMIT_FORMATS             1
#define CTX_ENABLE_FLOAT              0
#define CTX_32BIT_SEGMENTS            0
#define CTX_ENABLE_RGBA8              1
#define CTX_ENABLE_RGB332             1
#define CTX_ENABLE_GRAY1              1
#define CTX_ENABLE_GRAY2              1
#define CTX_ENABLE_GRAY4              1
#define CTX_ENABLE_RGB565             1
#define CTX_ENABLE_RGB565_BYTESWAPPED 1
#define CTX_ENABLE_CBRLE              0
#define CTX_BITPACK_PACKER            0
#define CTX_COMPOSITING_GROUPS        0
#define CTX_RENDERSTREAM_STATIC       0
#define CTX_GRADIENT_CACHE            1
#define CTX_ENABLE_CLIP               1
#define CTX_MIN_JOURNAL_SIZE        512  // grows dynamically
#define CTX_MIN_EDGE_LIST_SIZE      512  // is also max and limits complexity
                                         // of paths that can be filled
#define CTX_STATIC_OPAQUE       1
#define CTX_MAX_SCANLINE_LENGTH 512
#define CTX_1BIT_CLIP           1

#define CTX_MAX_DASHES          10
#define CTX_MAX_GRADIENT_STOPS  10
#define CTX_CM                  0
#define CTX_SHAPE_CACHE         0
#define CTX_SHAPE_CACHE_DEFAULT 0
#define CTX_RASTERIZER_MAX_CIRCLE_SEGMENTS 128
#define CTX_NATIVE_GRAYA8       0
#define CTX_ENABLE_SHADOW_BLUR  0
#define CTX_FONTS_FROM_FILE     0
#define CTX_MAX_KEYDB          16
#define CTX_FRAGMENT_SPECIALIZE 1
#define CTX_FAST_FILL_RECT      1
#define CTX_MAX_TEXTURES        1
#define CTX_PARSER_MAXLEN       512
#define CTX_PARSER_FIXED_TEMP   1
#define CTX_CURRENT_PATH        1
#define CTX_BLENDING_AND_COMPOSITING 1
#define CTX_STRINGPOOL_SIZE        256
#define CTX_AUDIO                    0
#define CTX_CLIENTS                  0

#define CTX_RAW_KB_EVENTS          0
#define CTX_MATH                   0
#define CTX_TERMINAL_EVENTS        0 // gets rid of posix bits and bobs
#define CTX_THREADS                0
#define CTX_TILED                  0
#define CTX_FORMATTER              0  // we want these eventually
#define CTX_PARSER                 0  // enabled
#define CTX_BRAILLE_TEXT           0

#define CTX_BAREMETAL              1

#define CTX_EVENTS                 1
#define CTX_MAX_DEVICES            1
#define CTX_MAX_KEYBINDINGS       16
#define CTX_RASTERIZER             1
#define CTX_MAX_STATES             5
#define CTX_MAX_EDGES            127
#define CTX_MAX_PENDING           64
#define CTX_MAX_CBS                8
#define CTX_MAX_LISTEN_FDS         1

#define CTX_ONE_FONT_ENGINE        1

#define CTX_STATIC_FONT(font) \
  ctx_load_font_ctx(ctx_font_##font##_name, \
                    ctx_font_##font,       \
                    sizeof (ctx_font_##font))



#define CTX_MAX_FONTS 10

#include "Arimo-Regular.h"
#include "Arimo-Bold.h"
#include "Arimo-Italic.h"
#include "Arimo-BoldItalic.h"
#define CTX_FONT_0 CTX_STATIC_FONT(Arimo_Regular)
#define CTX_FONT_1 CTX_STATIC_FONT(Arimo_Bold)
#define CTX_FONT_2 CTX_STATIC_FONT(Arimo_Italic)
#define CTX_FONT_3 CTX_STATIC_FONT(Arimo_BoldItalic)

#include "Tinos-Regular.h"
#define CTX_FONT_13 CTX_STATIC_FONT(Tinos_Regular)

#include "Cousine-Regular.h"
#include "Cousine-Bold.h"
#define CTX_FONT_21 CTX_STATIC_FONT(Cousine_Regular)
#define CTX_FONT_22 CTX_STATIC_FONT(Cousine_Bold)

