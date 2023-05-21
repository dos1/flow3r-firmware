#pragma GCC optimize ("O3")

#include <stdlib.h>
#include "py/binary.h"
#include "py/obj.h"
#include "py/objarray.h"
#include "py/runtime.h"
#include "py/mphal.h"

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

#ifdef MICROPY_UNIX_MACHINE_IDLE

  // we enable more for the unix port

#include <threads.h>
#define CTX_FB                     1
#define CTX_THREADS                1
#define CTX_TERMINAL_EVENTS        1 
#define CTX_TILED                  1
#define CTX_FORMATTER              1
#define CTX_PARSER                 1
#define CTX_MATH                   1
#define CTX_RAW_KB_EVENTS          1
#define CTX_BRAILLE_TEXT           0
#else
#define CTX_RAW_KB_EVENTS          0
#define CTX_MATH                   0
#define CTX_TERMINAL_EVENTS        0 // gets rid of posix bits and bobs
#define CTX_THREADS                0
#define CTX_TILED                  0
#define CTX_FORMATTER              0  // we want these eventually
#define CTX_PARSER                 0  // enabled
#define CTX_BRAILLE_TEXT           0

#define CTX_BAREMETAL              1
#endif


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
/* we keep the ctx implementation here, this compilation taget changes less
 * than the micropython target
 */
#define CTX_EXTERNAL_MALLOC

static inline void *ctx_malloc (size_t size)
{
  return m_malloc (size);
}

static inline void *ctx_calloc (size_t nmemb, size_t size)
{
  size_t byte_size = nmemb * size;
  char *ret        = (char *)m_malloc(byte_size);
  for (size_t i = 0; i < byte_size; i++)
    ret[i] = 0;
  return ret;
}

static inline void *ctx_realloc (void *ptr, size_t old_size, size_t new_size)
{
#if MICROPY_MALLOC_USES_ALLOCATED_SIZE
  return m_realloc(ptr, old_size, new_size);
#else
  return m_realloc(ptr, new_size);
#endif
}

static inline void ctx_free (void *ptr)
{
#if MICROPY_MALLOC_USES_ALLOCATED_SIZE
  return m_free(ptr, 0);  //  XXX  !
#else
  return m_free(ptr);
#endif
}

#define CTX_STATIC_FONT(font) \
  ctx_load_font_ctx(ctx_font_##font##_name, \
                    ctx_font_##font,       \
                    sizeof (ctx_font_##font))



#define CTX_MAX_FONTS 10

#if 0
#include "Roboto-Regular.h"
#include "Roboto-Bold.h"
#include "Roboto-Italic.h"
#include "Roboto-BoldItalic.h"
#define CTX_FONT_0 CTX_STATIC_FONT(Roboto_Regular)
#define CTX_FONT_1 CTX_STATIC_FONT(Roboto_Bold)
#define CTX_FONT_2 CTX_STATIC_FONT(Roboto_Italic)
//#define CTX_FONT_3 CTX_STATIC_FONT(Roboto_BoldItalic)
#endif


#if 1
#include "Arimo-Regular.h"
#include "Arimo-Bold.h"
#include "Arimo-Italic.h"
#include "Arimo-BoldItalic.h"
#define CTX_FONT_0 CTX_STATIC_FONT(Arimo_Regular)
#define CTX_FONT_1 CTX_STATIC_FONT(Arimo_Bold)
#define CTX_FONT_2 CTX_STATIC_FONT(Arimo_Italic)
#define CTX_FONT_3 CTX_STATIC_FONT(Arimo_BoldItalic)
#endif

#if 0
#define CTX_FONT_4 CTX_STATIC_FONT(Roboto_Thin)
#include "Roboto-Thin.h"
#endif

#if 0
#include "Carlito-Regular.h"
#include "Carlito-Bold.h"
#include "Carlito-Italic.h"
#include "Carlito-BoldItalic.h"
#define CTX_FONT_5  CTX_STATIC_FONT(Carlito_Regular)
#define CTX_FONT_6  CTX_STATIC_FONT(Carlito_Bold)
#define CTX_FONT_7  CTX_STATIC_FONT(Carlito_Italic)
#define CTX_FONT_8  CTX_STATIC_FONT(Carlito_BoldItalic)
#endif


#include "Tinos-Regular.h"
#define CTX_FONT_13 CTX_STATIC_FONT(Tinos_Regular)
//#include "Tinos-Bold.h"
//#define CTX_FONT_14 CTX_STATIC_FONT(Tinos_Bold)

#if 0
#include "Tinos-Italic.h"
#include "Tinos-BoldItalic.h"
#define CTX_FONT_15 CTX_STATIC_FONT(Tinos_Italic)
#define CTX_FONT_16 CTX_STATIC_FONT(Tinos_BoldItalic)
#endif

#if 0
#include "Caladea-Regular.h"
#include "Caladea-Bold.h"
#include "Caladea-Italic.h"
#define CTX_FONT_17 CTX_STATIC_FONT(Caladea_Regular)
#define CTX_FONT_18 CTX_STATIC_FONT(Caladea_Bold)
#define CTX_FONT_19 CTX_STATIC_FONT(Caladea_Italic)
#if 0
#include "Caladea-BoldItalic.h"
#define CTX_FONT_20 CTX_STATIC_FONT(Caladea_BoldItalic)
#endif
#endif

#if 1
#include "Cousine-Regular.h"
#include "Cousine-Bold.h"
#define CTX_FONT_21 CTX_STATIC_FONT(Cousine_Regular)
#define CTX_FONT_22 CTX_STATIC_FONT(Cousine_Bold)
//#include "Cousine-Italic.h"
//#include "Cousine-BoldItalic.h"
//#define CTX_FONT_23 CTX_STATIC_FONT(Cousine_Italic)
//#define CTX_FONT_24 CTX_STATIC_FONT(Cousine_BoldItalic)
#endif

#define CTX_IMPLEMENTATION
#include "ctx.h"

typedef struct _mp_ctx_event_obj_t mp_ctx_event_obj_t;
typedef struct _mp_ctx_obj_t {
	mp_obj_base_t base;
	Ctx *ctx;
	//mp_ctx_event_obj_t *ctx_event;
	mp_obj_t            user_data;
} mp_ctx_obj_t;

void gc_collect(void);
#ifdef EMSCRIPTEN
extern int _mp_quit;
void mp_idle (int ms);
#else
void mp_idle (int ms)
{
  if (ms == 0) gc_collect();
}
#endif

void gc_collect(void);
/* since a lot of the ctx API has similar function signatures, we use macros to
 * avoid repeating the marshalling of arguments
 */
#define MP_CTX_COMMON_FUN_0(name)                                              \
	static mp_obj_t mp_ctx_##name(mp_obj_t self_in)                        \
	{                                                                      \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);                   \
		ctx_##name(self->ctx);                                         \
		return self_in;                                                \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_1(mp_ctx_##name##_obj, mp_ctx_##name);

#define MP_CTX_COMMON_FUN_0_idle(name)                                         \
	static mp_obj_t mp_ctx_##name(mp_obj_t self_in)                        \
	{                                                                      \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);                   \
                gc_collect();\
		ctx_##name(self->ctx);                                         \
                mp_idle(0);\
                while(ctx_get_event(self->ctx)){};\
		return self_in;                                                \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_1(mp_ctx_##name##_obj, mp_ctx_##name);

#define MP_CTX_COMMON_FUN_1F(name)                                             \
	static mp_obj_t mp_ctx_##name(mp_obj_t self_in, mp_obj_t arg1)         \
	{                                                                      \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);                   \
		ctx_##name(self->ctx, (float)mp_obj_get_float(arg1));          \
		return self_in;                                                \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_2(mp_ctx_##name##_obj, mp_ctx_##name);

#define MP_CTX_COMMON_FUN_1I(name)                                             \
	static mp_obj_t mp_ctx_##name(mp_obj_t self_in, mp_obj_t arg1)         \
	{                                                                      \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);                   \
		ctx_##name(self->ctx, mp_obj_get_int(arg1));                   \
		return self_in;                                                \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_2(mp_ctx_##name##_obj, mp_ctx_##name);

#define MP_CTX_COMMON_FUN_2F(name)                                             \
	static mp_obj_t mp_ctx_##name(                                         \
		mp_obj_t self_in, mp_obj_t arg1, mp_obj_t arg2)                \
	{                                                                      \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);                   \
		ctx_##name(                                                    \
			self->ctx,                                             \
			(float)mp_obj_get_float(arg1),                         \
			(float)mp_obj_get_float(arg2));                        \
		return self_in;                                                \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_3(mp_ctx_##name##_obj, mp_ctx_##name);

#define MP_CTX_COMMON_FUN_3F(name)                                             \
	static mp_obj_t mp_ctx_##name(size_t n_args, const mp_obj_t *args)     \
	{                                                                      \
		assert(n_args == 4);                                           \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);                   \
		ctx_##name(                                                    \
			self->ctx,                                             \
			(float)mp_obj_get_float(args[1]),                      \
			(float)mp_obj_get_float(args[2]),                      \
			(float)mp_obj_get_float(args[3]));                     \
		return args[0];                                                \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(                                   \
		mp_ctx_##name##_obj, 4, 4, mp_ctx_##name);

#define MP_CTX_COMMON_FUN_4F(name)                                             \
	static mp_obj_t mp_ctx_##name(size_t n_args, const mp_obj_t *args)     \
	{                                                                      \
		assert(n_args == 5);                                           \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);                   \
		ctx_##name(                                                    \
			self->ctx,                                             \
			(float)mp_obj_get_float(args[1]),                      \
			(float)mp_obj_get_float(args[2]),                      \
			(float)mp_obj_get_float(args[3]),                      \
			(float)mp_obj_get_float(args[4]));                     \
		return args[0];                                                \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(                                   \
		mp_ctx_##name##_obj, 5, 5, mp_ctx_##name);

#define MP_CTX_COMMON_FUN_5F(name)                                             \
	static mp_obj_t mp_ctx_##name(size_t n_args, const mp_obj_t *args)     \
	{                                                                      \
		assert(n_args == 6);                                           \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);                   \
		ctx_##name(                                                    \
			self->ctx,                                             \
			(float)mp_obj_get_float(args[1]),                      \
			(float)mp_obj_get_float(args[2]),                      \
			(float)mp_obj_get_float(args[3]),                      \
			(float)mp_obj_get_float(args[4]),                      \
			(float)mp_obj_get_float(args[5]));                     \
		return args[0];                                                \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(                                   \
		mp_ctx_##name##_obj, 6, 6, mp_ctx_##name);

#define MP_CTX_COMMON_FUN_6F(name)                                             \
	static mp_obj_t mp_ctx_##name(size_t n_args, const mp_obj_t *args)     \
	{                                                                      \
		assert(n_args == 7);                                           \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);                   \
		ctx_##name(                                                    \
			self->ctx,                                             \
			(float)mp_obj_get_float(args[1]),                      \
			(float)mp_obj_get_float(args[2]),                      \
			(float)mp_obj_get_float(args[3]),                      \
			(float)mp_obj_get_float(args[4]),                      \
			(float)mp_obj_get_float(args[5]),                      \
			(int)mp_obj_get_float(args[6]));                       \
		return self;                                                   \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(                                   \
		mp_ctx_##name##_obj, 7, 7, mp_ctx_##name);


#define MP_CTX_COMMON_FUN_7F(name)                                             \
	static mp_obj_t mp_ctx_##name(size_t n_args, const mp_obj_t *args)     \
	{                                                                      \
		assert(n_args == 8);                                           \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);                   \
		ctx_##name(                                                    \
			self->ctx,                                             \
			(float)mp_obj_get_float(args[1]),                      \
			(float)mp_obj_get_float(args[2]),                      \
			(float)mp_obj_get_float(args[3]),                      \
			(float)mp_obj_get_float(args[4]),                      \
			(float)mp_obj_get_float(args[5]),                      \
			(float)mp_obj_get_float(args[6]),                      \
			(float)mp_obj_get_float(args[7]));                     \
		return self;                                                   \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(                                   \
		mp_ctx_##name##_obj, 8, 8, mp_ctx_##name);


#define MP_CTX_COMMON_FUN_9F(name)                                             \
	static mp_obj_t mp_ctx_##name(size_t n_args, const mp_obj_t *args)     \
	{                                                                      \
		assert(n_args == 10);                                          \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);                   \
		ctx_##name(                                                    \
			self->ctx,                                             \
			(float)mp_obj_get_float(args[1]),                      \
			(float)mp_obj_get_float(args[2]),                      \
			(float)mp_obj_get_float(args[3]),                      \
			(float)mp_obj_get_float(args[4]),                      \
			(float)mp_obj_get_float(args[5]),                      \
			(float)mp_obj_get_float(args[6]),                      \
			(float)mp_obj_get_float(args[7]),                      \
			(float)mp_obj_get_float(args[8]),                      \
			(float)mp_obj_get_float(args[9]));                     \
		return self;                                                   \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(                                   \
		mp_ctx_##name##_obj, 10, 10, mp_ctx_##name);

#define MP_CTX_TEXT_FUNB(name)                                                 \
	static mp_obj_t mp_ctx_##name(size_t n_args, const mp_obj_t *args)     \
	{                                                                      \
		assert(n_args == 4);                                           \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);                   \
		ctx_##name(                                                    \
			self->ctx,                                             \
			mp_obj_str_get_str(args[1]),                           \
			(float)mp_obj_get_float(args[2]),                      \
			(float)mp_obj_get_float(args[3]));                     \
                return args[0];                                                \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(                                   \
		mp_ctx_##name##_obj, 4, 4, mp_ctx_##name);

#define MP_CTX_TEXT_FUN(name)                                                  \
	static mp_obj_t mp_ctx_##name(size_t n_args, const mp_obj_t *args)     \
	{                                                                      \
		assert(n_args == 2);                                           \
		mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);                   \
		ctx_##name(                                                    \
			self->ctx,                                             \
			mp_obj_str_get_str(args[1]));                          \
                return args[0];                                                \
	}                                                                      \
	MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(                                   \
		mp_ctx_##name##_obj, 2, 2, mp_ctx_##name);

#define MP_CTX_ATTR(name) \
       { MP_ROM_QSTR(MP_QSTR_##name), MP_ROM_INT(0) }
#define MP_CTX_INT_CONSTANT_UNPREFIXED(ident)                                  \
        {                                                                      \
		MP_ROM_QSTR(MP_QSTR_##ident), MP_ROM_INT((int)CTX_##ident)     \
	}
#define MP_CTX_INT_CONSTANT(prefix, ident)                                     \
	{                                                                      \
		MP_ROM_QSTR(MP_QSTR_##ident), MP_ROM_INT((int)CTX_##prefix##_##ident)\
	}
#define MP_CTX_METHOD(name)                                                    \
	{                                                                      \
		MP_ROM_QSTR(MP_QSTR_##name), MP_ROM_PTR(&mp_ctx_##name##_obj)  \
	}

/* CTX API functions {{{ */

MP_CTX_TEXT_FUN(text);
#if CTX_PARSER
MP_CTX_TEXT_FUN(parse);
#endif

MP_CTX_COMMON_FUN_0(begin_path);
MP_CTX_COMMON_FUN_0(save);
MP_CTX_COMMON_FUN_0(restore);

MP_CTX_COMMON_FUN_0(consume_events);
MP_CTX_COMMON_FUN_0(start_frame);
MP_CTX_COMMON_FUN_0_idle(end_frame);

MP_CTX_COMMON_FUN_0(start_group);
MP_CTX_COMMON_FUN_0(end_group);
MP_CTX_COMMON_FUN_0(clip);
MP_CTX_COMMON_FUN_1F(rotate);
MP_CTX_COMMON_FUN_2F(scale);
MP_CTX_COMMON_FUN_2F(translate);
MP_CTX_COMMON_FUN_9F(apply_transform);
MP_CTX_COMMON_FUN_2F(line_to);
MP_CTX_COMMON_FUN_2F(move_to);
MP_CTX_COMMON_FUN_6F(curve_to);
MP_CTX_COMMON_FUN_4F(quad_to);
MP_CTX_COMMON_FUN_1F(gray);
MP_CTX_COMMON_FUN_3F(rgb);
MP_CTX_COMMON_FUN_4F(rgba);
MP_CTX_COMMON_FUN_5F(arc_to);
MP_CTX_COMMON_FUN_2F(rel_line_to);
MP_CTX_COMMON_FUN_2F(rel_move_to);
MP_CTX_COMMON_FUN_6F(rel_curve_to);
MP_CTX_COMMON_FUN_4F(rel_quad_to);
MP_CTX_COMMON_FUN_5F(rel_arc_to);
MP_CTX_COMMON_FUN_4F(rectangle);
MP_CTX_COMMON_FUN_5F(round_rectangle);
MP_CTX_COMMON_FUN_6F(arc);
MP_CTX_COMMON_FUN_0(close_path);

MP_CTX_COMMON_FUN_0(preserve);
MP_CTX_COMMON_FUN_0(fill);
MP_CTX_COMMON_FUN_0(stroke);
MP_CTX_COMMON_FUN_0(paint);

MP_CTX_COMMON_FUN_4F(linear_gradient);
MP_CTX_COMMON_FUN_6F(radial_gradient);

MP_CTX_COMMON_FUN_3F(logo);

static mp_obj_t mp_ctx_key_down(size_t n_args, const mp_obj_t *args)
{
  mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);
  ctx_key_down (self->ctx,
                mp_obj_get_int (args[1]), // keyval
                mp_obj_str_get_str(args[2]), // string
                mp_obj_get_int(args[3]));// time
		return args[0];
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_ctx_key_down_obj, 4, 4, mp_ctx_key_down);

static mp_obj_t mp_ctx_incoming_message (size_t n_args, const mp_obj_t *args)
{
  mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);
  ctx_incoming_message (self->ctx,
                mp_obj_str_get_str(args[1]), // string
                mp_obj_get_int(args[2]));// time
		return args[0];
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_ctx_incoming_message_obj, 3, 3, mp_ctx_incoming_message);

static mp_obj_t mp_ctx_key_up(size_t n_args, const mp_obj_t *args)
{
  mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);
  ctx_key_up (self->ctx,
                     mp_obj_get_int (args[1]), // keyval
                     mp_obj_str_get_str(args[2]), // string
                     mp_obj_get_int(args[3])); // time
		return args[0];
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_ctx_key_up_obj, 4, 4, mp_ctx_key_up);

static mp_obj_t mp_ctx_key_press(size_t n_args, const mp_obj_t *args)
{
  mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);
  ctx_key_press (self->ctx,
                     mp_obj_get_int (args[1]), // keyval
                     mp_obj_str_get_str(args[2]), // string
                     mp_obj_get_int(args[3])); // time
		return args[0];
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_ctx_key_press_obj, 4, 4, mp_ctx_key_press);

static mp_obj_t mp_ctx_scrolled(size_t n_args, const mp_obj_t *args)
{
  mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);
  ctx_scrolled (self->ctx,
                     (float)mp_obj_get_float(args[1]), // x
                     (float)mp_obj_get_float(args[2]), // y
                     mp_obj_get_int(args[3]), // scroll-direction
                     mp_obj_get_int(args[4])); // time
		return args[0];
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_ctx_scrolled_obj, 5, 5, mp_ctx_scrolled);

static mp_obj_t mp_ctx_pointer_press(size_t n_args, const mp_obj_t *args)
{
  mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);
  ctx_pointer_press (self->ctx,
                     (float)mp_obj_get_float(args[1]), // x
                     (float)mp_obj_get_float(args[2]),  // y
                     mp_obj_get_int(args[3]), // device-no
                     mp_obj_get_int(args[4])); // time
		return args[0];
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_ctx_pointer_press_obj, 5, 5, mp_ctx_pointer_press);

static mp_obj_t mp_ctx_pointer_motion(size_t n_args, const mp_obj_t *args)
{
  mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);
  ctx_pointer_motion(self->ctx,
                     (float)mp_obj_get_float(args[1]),
                     (float)mp_obj_get_float(args[2]),
                     mp_obj_get_int(args[3]),
                     mp_obj_get_int(args[4]));
		return args[0];
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_ctx_pointer_motion_obj, 5, 5, mp_ctx_pointer_motion);

static mp_obj_t mp_ctx_pointer_release(size_t n_args, const mp_obj_t *args)
{
  mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);
  ctx_pointer_release(self->ctx,
                     (float)mp_obj_get_float(args[1]),
                     (float)mp_obj_get_float(args[2]),
                     mp_obj_get_int(args[3]),
                     mp_obj_get_int(args[4]));
		return args[0];
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_ctx_pointer_release_obj, 5, 5, mp_ctx_pointer_release);

#if 0
static mp_obj_t mp_ctx_pointer_drop(size_t n_args, const mp_obj_t *args)
{
  mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);
  ctx_pointer_drop (self->ctx,
                     mp_obj_get_float(args[1]),  // x
                     mp_obj_get_float(args[2]),  // y
                     mp_obj_get_int(args[3]),    // device_no
                     mp_obj_get_int(args[4]),    // time
                     (char*)mp_obj_str_get_str(args[5])); // string
		return args[0];
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_ctx_pointer_drop_obj, 6, 6, mp_ctx_pointer_drop);
#endif

static mp_obj_t mp_ctx_line_dash(mp_obj_t self_in, mp_obj_t dashes_in)
{
	mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);

	size_t count  = mp_obj_get_int(mp_obj_len(dashes_in));
	float *dashes = m_malloc(sizeof(float) * count);
	for (size_t i = 0; i < count; i++) {
		dashes[i] = (float)mp_obj_get_float(mp_obj_subscr(
			dashes_in, mp_obj_new_int(i), MP_OBJ_SENTINEL)
		);
	}

	ctx_line_dash(self->ctx, dashes, count);

#if MICROPY_MALLOC_USES_ALLOCATED_SIZE
	m_free(dashes, sizeof(float)*count);
#else
	m_free(dashes);
#endif
        return self_in;
}
MP_DEFINE_CONST_FUN_OBJ_2(mp_ctx_line_dash_obj, mp_ctx_line_dash);


static mp_obj_t mp_ctx_in_fill(mp_obj_t self_in, mp_obj_t arg1, mp_obj_t arg2)
{
  mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);
  return mp_obj_new_bool (ctx_in_fill (self->ctx,
                                       (float)mp_obj_get_float(arg1),
                                       (float)mp_obj_get_float(arg2)));
}
MP_DEFINE_CONST_FUN_OBJ_3(mp_ctx_in_fill_obj, mp_ctx_in_fill);

static mp_obj_t mp_ctx_texture (size_t n_args, const mp_obj_t *args)
{
        mp_buffer_info_t buffer_info;
	assert(n_args == 7);
	mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);

        if (!mp_get_buffer(args[1], &buffer_info, MP_BUFFER_READ))
        {
           mp_raise_TypeError("not a buffer");
        }
        int format = mp_obj_get_int (args[2]);
        int width  = mp_obj_get_int (args[3]);
        int height = mp_obj_get_int (args[4]);
        int stride = mp_obj_get_int (args[5]);
        //static int eid_no = 0;
        //char ieid[10]; // this is a workaround for the rasterizer
                       // not keeping the cache validity expected
                       // as we drive it
                       // it also means we cannot properly use
                       // eid based APIs
        //sprintf (ieid, "%i", eid_no++);
	ctx_define_texture (self->ctx,
                        NULL, width, height, stride, format,
                        buffer_info.buf, NULL);
	return args[0];
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_ctx_texture_obj, 6, 6, mp_ctx_texture);


#if 0
static mp_obj_t mp_ctx_font(mp_obj_t self_in, mp_obj_t font_in)
{
	mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);
	const char *font   = mp_obj_str_get_str(font_in);
	ctx_font(self->ctx, font);
        return self_in;
}
MP_DEFINE_CONST_FUN_OBJ_2(mp_ctx_font_obj, mp_ctx_font);
#endif

static mp_obj_t mp_ctx_get_font_name (mp_obj_t self_in, mp_obj_t no_in)
{
	mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);
	int no = mp_obj_get_int (no_in);
        const char *name = ctx_get_font_name (self->ctx, no);
        if (name)
          return mp_obj_new_str(name, strlen(name));
	return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_2(mp_ctx_get_font_name_obj, mp_ctx_get_font_name);


static mp_obj_t mp_ctx_text_width(mp_obj_t self_in, mp_obj_t string_in)
{
	mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);
	const char *string = mp_obj_str_get_str(string_in);
	return mp_obj_new_float(ctx_text_width(self->ctx, string));
}
MP_DEFINE_CONST_FUN_OBJ_2(mp_ctx_text_width_obj, mp_ctx_text_width);

static mp_obj_t mp_ctx_add_stop(size_t n_args, const mp_obj_t *args)
{
	mp_ctx_obj_t *self = MP_OBJ_TO_PTR(args[0]);
	mp_obj_t color_in  = args[2];

	float pos = (float)mp_obj_get_float(args[1]);

	float alpha_f = 1.0f;
	if (n_args == 4) {
		alpha_f = (float)mp_obj_get_float(args[3]);
	}
	if (alpha_f < 0.0f || alpha_f > 1.0f) {
		mp_raise_ValueError("alpha must be between 0.0 or 1.0");
	}

	mp_obj_t red_in, green_in, blue_in;
	if (mp_obj_get_int(mp_obj_len(color_in)) < 3) {
		mp_raise_ValueError("color must have 3 elements");
	}
	red_in   = mp_obj_subscr(color_in, mp_obj_new_int(0), MP_OBJ_SENTINEL);
	green_in = mp_obj_subscr(color_in, mp_obj_new_int(1), MP_OBJ_SENTINEL);
	blue_in  = mp_obj_subscr(color_in, mp_obj_new_int(2), MP_OBJ_SENTINEL);

	/*
	 * The color can be either floats between 0 and 1 or integers between 0
	 * and 255.  Make this decision based on the first element we find.
	 */
	if (mp_obj_is_type(red_in, &mp_type_float)) {
		float red, green, blue;
		red   = (float)mp_obj_get_float(red_in);
		green = (float)mp_obj_get_float(green_in);
		blue  = (float)mp_obj_get_float(blue_in);

		ctx_gradient_add_stop(
			self->ctx, pos, red, green, blue, alpha_f
		);
	} else {
		uint8_t red, green, blue, alpha;
		red   = mp_obj_get_int(red_in);
		green = mp_obj_get_int(green_in);
		blue  = mp_obj_get_int(blue_in);

		alpha = (int)(alpha_f * 255.0f);
		ctx_gradient_add_stop_u8(
			self->ctx, pos, red, green, blue, alpha
		);
	}

        return args[0];
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_ctx_add_stop_obj, 3, 4, mp_ctx_add_stop);

#ifdef EMSCRIPTEN
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif


STATIC void generic_method_lookup(mp_obj_t obj, qstr attr, mp_obj_t *dest) {
    const mp_obj_type_t *type = mp_obj_get_type(obj);
    if (MP_OBJ_TYPE_HAS_SLOT(type, locals_dict)) {
         // generic method lookup
         // this is a lookup in the object (ie not class or type)
         //assert(type->locals_dict->base.type == &mp_type_dict); // MicroPython restriction, for now
         //mp_map_t *locals_map = &MP_OBJ_TYPE_GET_SLOT(type, locals_dict)->map;
         mp_map_elem_t *elem = mp_map_lookup(&MP_OBJ_TYPE_GET_SLOT(type, locals_dict)->map, MP_OBJ_NEW_QSTR(attr), MP_MAP_LOOKUP);
         if (elem != NULL) {
             mp_convert_member_lookup(obj, type, elem->value, dest);
         }
    }
}

extern const mp_obj_type_t mp_ctx_event_type;
struct _mp_ctx_event_obj_t {
	mp_obj_base_t base;
	CtxEvent  event;
        mp_obj_t  user_data;
	mp_obj_t  mp_ev;
};


static mp_obj_t mp_ctx_event_new (void)
{
	mp_ctx_event_obj_t *o = m_new_obj(mp_ctx_event_obj_t);
	o->base.type    = &mp_ctx_event_type;
	return MP_OBJ_FROM_PTR(o);
}
extern const mp_obj_type_t mp_ctx_type;

/** CtxEvent **/

STATIC mp_obj_t
mp_ctx_event_attr_op (mp_obj_t self_in, qstr attr, mp_obj_t set_val)
{
  mp_ctx_event_obj_t *self = MP_OBJ_TO_PTR(self_in);       
  if (set_val == MP_OBJ_NULL) {
    switch (attr)
    {
      case MP_QSTR_ctx:     
        {
	mp_ctx_obj_t *o = m_new_obj(mp_ctx_obj_t);
	o->base.type    = &mp_ctx_type;
        o->ctx = self->event.ctx;
	return MP_OBJ_FROM_PTR(o);
        }
      case MP_QSTR_x:         return mp_obj_new_float(self->event.x);
      case MP_QSTR_y:         return mp_obj_new_float(self->event.y);
      case MP_QSTR_device_x:  return mp_obj_new_float(self->event.device_x);
      case MP_QSTR_device_y:  return mp_obj_new_float(self->event.device_y);
      case MP_QSTR_start_x:   return mp_obj_new_float(self->event.start_x);
      case MP_QSTR_start_y:   return mp_obj_new_float(self->event.start_y);
      case MP_QSTR_prev_x:    return mp_obj_new_float(self->event.prev_x);
      case MP_QSTR_prev_y:    return mp_obj_new_float(self->event.prev_y);
      case MP_QSTR_delta_x:   return mp_obj_new_float(self->event.delta_x);
      case MP_QSTR_delta_y:   return mp_obj_new_float(self->event.delta_y);
      case MP_QSTR_device_no: return mp_obj_new_int(self->event.device_no);
      case MP_QSTR_unicode:   return mp_obj_new_int(self->event.unicode);
      case MP_QSTR_user_data: return self->user_data;
      case MP_QSTR_scroll_direction:  return mp_obj_new_int(self->event.scroll_direction);
      case MP_QSTR_time:      return mp_obj_new_int(self->event.time);
      case MP_QSTR_modifier_state:   return mp_obj_new_int(self->event.state);
      case MP_QSTR_string:    if (self->event.string)
                                 // gambling on validity
                                 return mp_obj_new_str(self->event.string, strlen(self->event.string));
                              else
                                 return mp_obj_new_str("", 0);
    }
  }
  else
  {
     return set_val;
  }
  return self_in;
}

STATIC void mp_ctx_event_attr(mp_obj_t obj, qstr attr, mp_obj_t *dest) {

    if(attr == MP_QSTR_x
     ||attr == MP_QSTR_y
     ||attr == MP_QSTR_ctx
     ||attr == MP_QSTR_start_x
     ||attr == MP_QSTR_start_y
     ||attr == MP_QSTR_prev_x
     ||attr == MP_QSTR_prev_y
     ||attr == MP_QSTR_delta_x
     ||attr == MP_QSTR_delta_y
     ||attr == MP_QSTR_device_no
     ||attr == MP_QSTR_unicode
     ||attr == MP_QSTR_user_data
     ||attr == MP_QSTR_scroll_direction
     ||attr == MP_QSTR_time
     ||attr == MP_QSTR_modifier_state
     ||attr == MP_QSTR_string
     ||attr == MP_QSTR_device_x
     ||attr == MP_QSTR_device_y)
    {
        if (dest[0] == MP_OBJ_NULL) {
            // load attribute
            mp_obj_t val = mp_ctx_event_attr_op(obj, attr, MP_OBJ_NULL);
            dest[0] = val;
        } else {
            // delete/store attribute
            if (mp_ctx_event_attr_op(obj, attr, dest[1]) != MP_OBJ_NULL)
                dest[0] = MP_OBJ_NULL; // indicate success
        }
    }
    else {
        // A method call
        generic_method_lookup(obj, attr, dest);
    }
}

static const mp_rom_map_elem_t mp_ctx_event_locals_dict_table[] = {
       MP_CTX_ATTR(x),
       MP_CTX_ATTR(y),
       MP_CTX_ATTR(ctx),
       MP_CTX_ATTR(device_x),
       MP_CTX_ATTR(device_y),
       MP_CTX_ATTR(start_x),
       MP_CTX_ATTR(start_y),
       MP_CTX_ATTR(prev_x),
       MP_CTX_ATTR(prev_y),
       MP_CTX_ATTR(delta_x),
       MP_CTX_ATTR(delta_y),
       MP_CTX_ATTR(device_no),
       MP_CTX_ATTR(unicode),
       MP_CTX_ATTR(user_data),
       MP_CTX_ATTR(scroll_direction),
       MP_CTX_ATTR(time),
       MP_CTX_ATTR(modifier_state),
       MP_CTX_ATTR(string)
};
static MP_DEFINE_CONST_DICT(mp_ctx_event_locals_dict, mp_ctx_event_locals_dict_table);

static mp_obj_t mp_ctx_event_make_new(
	const mp_obj_type_t *type,
	size_t n_args,
	size_t n_kw,
	const mp_obj_t *args
) {
	mp_ctx_event_obj_t *o = m_new_obj(mp_ctx_event_obj_t);
	o->base.type    = type;
	return MP_OBJ_FROM_PTR(o);
}

MP_DEFINE_CONST_OBJ_TYPE(
    mp_ctx_event_type,
    MP_QSTR_ctx_event_type,
    MP_TYPE_FLAG_NONE,
    //print, array_print,
    make_new, mp_ctx_event_make_new,
    attr, mp_ctx_event_attr,
    locals_dict, &mp_ctx_event_locals_dict
);

/*
const mp_obj_type_t mp_ctx_event_type = {
	.base        = { &mp_type_type },
	.name        = MP_QSTR_CtxEvent,
	.make_new    = mp_ctx_event_make_new,
	.locals_dict = (mp_obj_t)&mp_ctx_event_locals_dict,
        .attr = mp_ctx_event_attr
};
*/

static void mp_ctx_listen_cb_handler (CtxEvent *event, void *data1, void*data2)
{
  mp_obj_t event_in = data2;
  mp_ctx_event_obj_t *mp_ctx_event = MP_OBJ_TO_PTR(event_in);
  mp_ctx_event->event = *event; // XXX todo stop copying, make stop_propagate work
  mp_call_function_1(data1, event_in);
}

static void mp_ctx_listen_cb_handler_stop_propagate (CtxEvent *event, void *data1, void*data2)
{
  mp_obj_t event_in = data2;
  mp_ctx_event_obj_t *mp_ctx_event = MP_OBJ_TO_PTR(event_in);
  mp_ctx_event->event = *event;
  mp_call_function_1(data1, event_in);
  event->stop_propagate = 1;
}

static mp_obj_t mp_ctx_listen (size_t n_args, const mp_obj_t *args)
{
  mp_obj_t self_in = args[0];
  mp_obj_t event_mask = args[1];
  mp_obj_t cb_in = args[2];
  mp_obj_t user_data_in = n_args==4?args[3]:mp_const_none;
  mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);
  if (cb_in != mp_const_none &&
      !mp_obj_is_callable(cb_in))
          mp_raise_ValueError(MP_ERROR_TEXT("invalid handler"));
  mp_ctx_event_obj_t *ctx_event = mp_ctx_event_new ();
  ctx_event->user_data = user_data_in;
  ctx_listen (self->ctx,
              mp_obj_get_int(event_mask),
              mp_ctx_listen_cb_handler,
              (cb_in), ctx_event);
  return MP_OBJ_FROM_PTR(self);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_ctx_listen_obj, 3, 4, mp_ctx_listen);

static void mp_ctx_key_binding_cb_handler (CtxEvent *event, void *data1, void*data2)
{
  mp_ctx_event_obj_t *ctx_event = mp_ctx_event_new ();
  //mp_ctx_event_obj_t *mp_ctx_event = MP_OBJ_TO_PTR(event_in);
  ctx_event->event = *event;
#if 0
  mp_sched_schedule (data1, event_in);
#else
  mp_call_function_1(data1, ctx_event);
#endif
}

static mp_obj_t mp_ctx_add_key_binding (size_t n_args, const mp_obj_t *args)
{
  mp_obj_t self_in    = args[0];
  mp_obj_t key_in     = args[1];
  mp_obj_t action_in  = args[2];
  mp_obj_t label_in   = args[3];
  mp_obj_t cb_in      = args[4];
  //mp_obj_t user_data_in = args[5];
  mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);
  if (cb_in != mp_const_none &&
      !mp_obj_is_callable(cb_in))
          mp_raise_ValueError(MP_ERROR_TEXT("invalid handler"));
  ctx_add_key_binding (self->ctx,
              mp_obj_str_get_str(key_in),
              mp_obj_str_get_str(action_in),
              mp_obj_str_get_str(label_in),
              mp_ctx_key_binding_cb_handler,
              (cb_in));//, ctx_event);
  return MP_OBJ_FROM_PTR(self);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_ctx_add_key_binding_obj, 5, 5, mp_ctx_add_key_binding);


static mp_obj_t mp_ctx_listen_stop_propagate (size_t n_args, const mp_obj_t *args)
{
  mp_obj_t self_in = args[0];
  mp_obj_t event_mask = args[1];
  mp_obj_t cb_in = args[2];
  mp_obj_t user_data_in = n_args==4?args[3]:mp_const_none;
  mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);
  if (cb_in != mp_const_none &&
      !mp_obj_is_callable(cb_in))
          mp_raise_ValueError(MP_ERROR_TEXT("invalid handler"));
  mp_ctx_event_obj_t *ctx_event = mp_ctx_event_new ();
  ctx_event->user_data = user_data_in;
  ctx_listen (self->ctx,
              mp_obj_get_int(event_mask),
              mp_ctx_listen_cb_handler_stop_propagate,
              (cb_in), ctx_event);
  return MP_OBJ_FROM_PTR(self);
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mp_ctx_listen_stop_propagate_obj, 3, 4, mp_ctx_listen_stop_propagate);



#if CTX_TINYVG
static mp_obj_t mp_ctx_tinyvg_get_size (mp_obj_t self_in, mp_obj_t buffer_in)
{
  mp_buffer_info_t buffer_info;
  if (!mp_get_buffer(buffer_in, &buffer_info, MP_BUFFER_READ))
  {
     mp_raise_TypeError("not a buffer");
  }
  int width = 0, height = 0;
  ctx_tinyvg_get_size (buffer_info.buf,
                       buffer_info.len, &width, &height);
  mp_obj_t mp_w  = MP_OBJ_NEW_SMALL_INT(width);
  mp_obj_t mp_h  = MP_OBJ_NEW_SMALL_INT(height);
  mp_obj_t tup[] = { mp_w, mp_h };
  return mp_obj_new_tuple(2, tup);
}
MP_DEFINE_CONST_FUN_OBJ_2(mp_ctx_tinyvg_get_size_obj, mp_ctx_tinyvg_get_size);

static mp_obj_t mp_ctx_tinyvg_draw (mp_obj_t self_in, mp_obj_t buffer_in)
{
  mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);
  mp_buffer_info_t buffer_info;

  if (!mp_get_buffer(buffer_in, &buffer_info, MP_BUFFER_READ))
  {
     mp_raise_TypeError("not a buffer");
  }
  ctx_tinyvg_draw (self->ctx,
                   buffer_info.buf,
                   buffer_info.len,
                   0);
  return self_in;
}
MP_DEFINE_CONST_FUN_OBJ_2(mp_ctx_tinyvg_draw_obj, mp_ctx_tinyvg_draw);
#endif
/* CTX API functions }}} */

static void mp_ctx_set_pixels (Ctx *ctx, void *user_data,
                               int x_in, int y_in, int width_in, int height_in, void *buf_in,
                               int buf_size)
{
  mp_obj_t args[5] = {mp_obj_new_int (x_in),
                      mp_obj_new_int (y_in),
                      mp_obj_new_int (width_in),
                      mp_obj_new_int (height_in),
                  mp_obj_new_memoryview(BYTEARRAY_TYPECODE,
                    buf_size, buf_in)};
  mp_call_function_n_kw (user_data, 5, 0, args);
}

static int mp_ctx_update_fb (Ctx *ctx, void *user_data)
{
  mp_obj_t ret = mp_call_function_0(user_data);
  if (mp_obj_is_true (ret)) return 1;
  return 0;
}

static mp_obj_t mp_ctx_make_new(
	const mp_obj_type_t *type,
	size_t n_args,
	size_t n_kw,
	const mp_obj_t *all_args
) {
	mp_hal_stdout_tx_str("here\n\r");
	printf("HERE");
	mp_ctx_obj_t *o = m_new_obj(mp_ctx_obj_t);
	o->base.type    = type;
    enum { ARG_width, ARG_height, ARG_stride, ARG_format, ARG_buffer, ARG_memory_budget, ARG_flags,
           ARG_set_pixels, ARG_update, ARG_userdata };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_width,  MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_height, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_stride, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_format, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_buffer, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_memory_budget, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 24 * 1024} },
        { MP_QSTR_flags, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_set_pixels, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_update, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_userdata, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
    };
    mp_hal_stdout_tx_str("vars\n\r");
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    int format = args[ARG_format].u_int;
    int width = args[ARG_width].u_int;
    int height = args[ARG_height].u_int;
    int stride = args[ARG_stride].u_int;
    int memory_budget = args[ARG_memory_budget].u_int;
    int flags = args[ARG_flags].u_int;
	mp_hal_stdout_tx_str("args\n\r");
    if (args[ARG_set_pixels].u_obj != MP_OBJ_NULL)
    {
        mp_obj_t set_pixels_in = args[ARG_set_pixels].u_obj;
        mp_obj_t update_fb_in = args[ARG_update].u_obj;

        if (update_fb_in != mp_const_none &&
          !mp_obj_is_callable(update_fb_in))
             mp_raise_ValueError(MP_ERROR_TEXT("invalid update_fb handler"));

        if (set_pixels_in != mp_const_none &&
          !mp_obj_is_callable(set_pixels_in))
             mp_raise_ValueError(MP_ERROR_TEXT("invalid set_pixels handler"));

	o->ctx = ctx_new_cb (width, height, format,
             mp_ctx_set_pixels, set_pixels_in,
             update_fb_in != mp_const_none ? mp_ctx_update_fb : NULL, update_fb_in,
             memory_budget,
             NULL,
             flags);
	return MP_OBJ_FROM_PTR(o);
    }
	mp_hal_stdout_tx_str("after set_pixels\n\r");
    if (args[ARG_buffer].u_obj != MP_OBJ_NULL)
    {
        mp_buffer_info_t buffer_info;

        if (!mp_get_buffer(args[ARG_buffer].u_obj, &buffer_info, MP_BUFFER_READ))
        {
           mp_raise_TypeError("not a buffer");
        }
	o->ctx = ctx_new_for_framebuffer (buffer_info.buf,
                                          width, height, stride, format);
	return MP_OBJ_FROM_PTR(o);
    }
	mp_hal_stdout_tx_str("after buffer done\n\r");
#ifdef EMSCRIPTEN
	mp_hal_stdout_tx_str("EMSCRIPTEN\n\r");
    o->ctx          = ctx_wasm_get_context(memory_budget);
    mp_hal_stdout_tx_str("got context\n\r");
#else
	mp_hal_stdout_tx_str("NO EMSCRIPTEN\n\r");
	mp_hal_stdout_tx_str("here\n\r");
    o->ctx          = ctx_new (width, height, NULL);
    mp_hal_stdout_tx_str("but not here :( \n\r");
#endif
	mp_hal_stdout_tx_str("RET\n\r");
    return MP_OBJ_FROM_PTR(o);
}


STATIC mp_obj_t
mp_ctx_attr_op (mp_obj_t self_in, qstr attr, mp_obj_t set_val)
{
  mp_ctx_obj_t *self = MP_OBJ_TO_PTR(self_in);       
  if (set_val == MP_OBJ_NULL) {
    switch (attr)
    {
       case MP_QSTR_font:
            {
              const char *font = ctx_get_font (self->ctx);
              return mp_obj_new_str(font, strlen(font));
            }
       case MP_QSTR_image_smoothing:
            return mp_obj_new_int(ctx_get_image_smoothing (self->ctx));
       case MP_QSTR_fill_rule:
            return mp_obj_new_int(ctx_get_fill_rule (self->ctx));
#if CTX_BLENDING_AND_COMPOSITING
       case MP_QSTR_blend_mode:
            return mp_obj_new_int(ctx_get_blend_mode (self->ctx));
       case MP_QSTR_compositing_mode:
            return mp_obj_new_int(ctx_get_compositing_mode (self->ctx));
#endif
       case MP_QSTR_flags:
            return mp_obj_new_int(ctx_cb_get_flags (self->ctx));

       case MP_QSTR_need_redraw:
            return mp_obj_new_bool(ctx_need_redraw (self->ctx));

       case MP_QSTR_line_cap:
            return mp_obj_new_int(ctx_get_line_cap (self->ctx));
       case MP_QSTR_line_join:
            return mp_obj_new_int(ctx_get_line_join (self->ctx));
       case MP_QSTR_text_align:
            return mp_obj_new_int(ctx_get_text_align (self->ctx));
       case MP_QSTR_text_baseline:
            return mp_obj_new_int(ctx_get_text_baseline (self->ctx));
       case MP_QSTR_font_size:
            return mp_obj_new_float(ctx_get_font_size (self->ctx));
       case MP_QSTR_line_width:
            return mp_obj_new_float(ctx_get_line_width (self->ctx));
       case MP_QSTR_line_dash_offset:
            return mp_obj_new_float(ctx_get_line_dash_offset (self->ctx));
       case MP_QSTR_line_height:
            return mp_obj_new_float(ctx_get_line_height (self->ctx));
       case MP_QSTR_wrap_left:
            return mp_obj_new_float(ctx_get_wrap_left (self->ctx));
       case MP_QSTR_wrap_right:
            return mp_obj_new_float(ctx_get_wrap_right (self->ctx));
       case MP_QSTR_miter_limit:
            return mp_obj_new_float(ctx_get_miter_limit (self->ctx));
       case MP_QSTR_global_alpha:
            return mp_obj_new_float(ctx_get_global_alpha (self->ctx));
       case MP_QSTR_width:
            return mp_obj_new_int(ctx_width (self->ctx));
       case MP_QSTR_height:
            return mp_obj_new_int(ctx_height (self->ctx));
       case MP_QSTR_x:
            return mp_obj_new_float(ctx_x (self->ctx));
       case MP_QSTR_y:
            return mp_obj_new_float(ctx_y (self->ctx));
    }
  }
  else
  {
    switch (attr)
    {
       case MP_QSTR_font:
         ctx_font (self->ctx, mp_obj_str_get_str (set_val)); break;
       case MP_QSTR_image_smoothing:
         ctx_image_smoothing (self->ctx, mp_obj_get_int (set_val)); break;
       case MP_QSTR_fill_rule:
         ctx_fill_rule (self->ctx, mp_obj_get_int (set_val)); break;
       case MP_QSTR_line_cap:
         ctx_line_cap (self->ctx, mp_obj_get_int (set_val)); break;
       case MP_QSTR_flags:
         ctx_cb_set_flags (self->ctx, mp_obj_get_int (set_val)); break;
       case MP_QSTR_need_redraw:
         ctx_queue_draw (self->ctx); break;
       case MP_QSTR_line_join:
         ctx_line_join (self->ctx, mp_obj_get_int (set_val)); break;
       case MP_QSTR_text_align:
         ctx_text_align (self->ctx, mp_obj_get_int (set_val)); break;
#if CTX_BLENDING_AND_COMPOSITING
       case MP_QSTR_blend_mode:
         ctx_blend_mode (self->ctx, mp_obj_get_int (set_val)); break;
       case MP_QSTR_compositing_mode:
         ctx_compositing_mode (self->ctx, mp_obj_get_int (set_val)); break;
#endif
       case MP_QSTR_text_baseline:
         ctx_text_baseline (self->ctx, mp_obj_get_int (set_val)); break;
       case MP_QSTR_line_width:
         ctx_line_width (self->ctx, (float)mp_obj_get_float (set_val)); break;
       case MP_QSTR_line_height:
         ctx_line_height (self->ctx, (float)mp_obj_get_float (set_val)); break;
       case MP_QSTR_wrap_left:
         ctx_wrap_left (self->ctx, (float)mp_obj_get_float (set_val)); break;
       case MP_QSTR_wrap_right:
         ctx_wrap_right (self->ctx, (float)mp_obj_get_float (set_val)); break;

       case MP_QSTR_line_dash_offset:
         ctx_line_dash_offset (self->ctx, (float)mp_obj_get_float (set_val)); break;
       case MP_QSTR_miter_limit:
         ctx_miter_limit (self->ctx, (float)mp_obj_get_float (set_val)); break;
       case MP_QSTR_global_alpha:
         ctx_global_alpha (self->ctx, (float)mp_obj_get_float (set_val)); break;
       case MP_QSTR_font_size:
         ctx_font_size (self->ctx, (float)mp_obj_get_float (set_val)); break;
    }
    return set_val;
  }
  return self_in;
}


STATIC void mp_ctx_attr(mp_obj_t obj, qstr attr, mp_obj_t *dest) {

    if(attr == MP_QSTR_width
     ||attr == MP_QSTR_height
     ||attr == MP_QSTR_font
#if CTX_BLENDING_AND_COMPOSITING
     ||attr == MP_QSTR_blend_mode
     ||attr == MP_QSTR_compositing_mode
#endif
     ||attr == MP_QSTR_flags
     ||attr == MP_QSTR_need_redraw
     ||attr == MP_QSTR_line_cap
     ||attr == MP_QSTR_line_join
     ||attr == MP_QSTR_text_align
     ||attr == MP_QSTR_fill_rule
     ||attr == MP_QSTR_image_smoothing
     ||attr == MP_QSTR_text_baseline
     ||attr == MP_QSTR_line_width
     ||attr == MP_QSTR_line_dash_offset
     ||attr == MP_QSTR_line_height
     ||attr == MP_QSTR_wrap_left
     ||attr == MP_QSTR_wrap_right
     ||attr == MP_QSTR_miter_limit
     ||attr == MP_QSTR_global_alpha
     ||attr == MP_QSTR_font_size
     ||attr == MP_QSTR_font
     ||attr == MP_QSTR_x
     ||attr == MP_QSTR_y)
    {
        if (dest[0] == MP_OBJ_NULL) {
            // load attribute
            mp_obj_t val = mp_ctx_attr_op(obj, attr, MP_OBJ_NULL);
            dest[0] = val;
        } else {
            // delete/store attribute
            if (mp_ctx_attr_op(obj, attr, dest[1]) != MP_OBJ_NULL)
                dest[0] = MP_OBJ_NULL; // indicate success
        }
    }
    else {
        // A method call
        generic_method_lookup(obj, attr, dest);
    }
}

/* CTX class/type */


static const mp_rom_map_elem_t mp_ctx_locals_dict_table[] = {
	MP_CTX_METHOD(line_to),
	MP_CTX_METHOD(move_to),
	MP_CTX_METHOD(curve_to),
	MP_CTX_METHOD(quad_to),
	MP_CTX_METHOD(rel_line_to),
	MP_CTX_METHOD(rel_move_to),
	MP_CTX_METHOD(rel_curve_to),
	MP_CTX_METHOD(rel_quad_to),
	MP_CTX_METHOD(rectangle),
	MP_CTX_METHOD(arc),
	MP_CTX_METHOD(arc_to),
	MP_CTX_METHOD(rel_arc_to),
	MP_CTX_METHOD(round_rectangle),
	MP_CTX_METHOD(begin_path),
	MP_CTX_METHOD(close_path),
        MP_CTX_METHOD(in_fill),
	MP_CTX_METHOD(fill),
	MP_CTX_METHOD(stroke),
	MP_CTX_METHOD(paint),
	MP_CTX_METHOD(clip),
	MP_CTX_METHOD(text),
	MP_CTX_METHOD(text_width),
	MP_CTX_METHOD(rotate),
	MP_CTX_METHOD(scale),
	MP_CTX_METHOD(translate),
	MP_CTX_METHOD(apply_transform),
	MP_CTX_METHOD(start_group),
	MP_CTX_METHOD(end_group),
	MP_CTX_METHOD(preserve),
	MP_CTX_METHOD(linear_gradient),
	MP_CTX_METHOD(radial_gradient),
	MP_CTX_METHOD(add_stop),
	MP_CTX_METHOD(line_dash),
	MP_CTX_METHOD(texture),
	MP_CTX_METHOD(save),
	MP_CTX_METHOD(restore),
	MP_CTX_METHOD(start_frame),
	//MP_CTX_METHOD(consume_events),
	MP_CTX_METHOD(end_frame),
	MP_CTX_METHOD(add_key_binding),
	MP_CTX_METHOD(listen),
	MP_CTX_METHOD(listen_stop_propagate),
	MP_CTX_METHOD(get_font_name),

        MP_CTX_METHOD(gray),
        MP_CTX_METHOD(rgb),
        MP_CTX_METHOD(rgba),
#if CTX_PARSER
	MP_CTX_METHOD(parse),
#endif
#if 1
#if CTX_TINYVG
	MP_CTX_METHOD(tinyvg_draw),
	MP_CTX_METHOD(tinyvg_get_size),
#endif
#endif
	MP_CTX_METHOD(logo),

        MP_CTX_METHOD(pointer_motion),
        MP_CTX_METHOD(pointer_release),
        MP_CTX_METHOD(pointer_press),
        MP_CTX_METHOD(scrolled),
        MP_CTX_METHOD(key_press),
        MP_CTX_METHOD(key_up),
        MP_CTX_METHOD(key_down),
        MP_CTX_METHOD(incoming_message),


        // Instance attributes
        MP_CTX_ATTR(x),
        MP_CTX_ATTR(y),
        MP_CTX_ATTR(width),
        MP_CTX_ATTR(height),
        MP_CTX_ATTR(font),
        MP_CTX_ATTR(image_smoothing),
#if CTX_BLENDING_AND_COMPOSITING
        MP_CTX_ATTR(compositing_mode),
        MP_CTX_ATTR(blend_mode),
#endif
        MP_CTX_ATTR(need_redraw),
        MP_CTX_ATTR(flags),
        MP_CTX_ATTR(line_cap),
        MP_CTX_ATTR(line_join),
        MP_CTX_ATTR(text_align),
        MP_CTX_ATTR(fill_rule),
        MP_CTX_ATTR(text_baseline),
        MP_CTX_ATTR(line_width),
        MP_CTX_ATTR(line_dash_offset),
        MP_CTX_ATTR(line_height),
        MP_CTX_ATTR(wrap_left),
        MP_CTX_ATTR(wrap_right),
        MP_CTX_ATTR(miter_limit),
        MP_CTX_ATTR(global_alpha),
        MP_CTX_ATTR(font_size),

        MP_CTX_INT_CONSTANT(FLAG,LOWFI),
        MP_CTX_INT_CONSTANT(FLAG,GRAY2),
        MP_CTX_INT_CONSTANT(FLAG,GRAY4),
        MP_CTX_INT_CONSTANT(FLAG,GRAY8),
        MP_CTX_INT_CONSTANT(FLAG,RGB332),
        MP_CTX_INT_CONSTANT(FLAG,HASH_CACHE),
        //MP_CTX_INT_CONSTANT(FLAG,DAMAGE_CONTROL),
        MP_CTX_INT_CONSTANT(FLAG,KEEP_DATA),
        MP_CTX_INT_CONSTANT(FLAG,INTRA_UPDATE),
        MP_CTX_INT_CONSTANT(FLAG,STAY_LOW),
#if CTX_ENABLE_CBRLE
        MP_CTX_INT_CONSTANT(FLAG,CBRLE),
#endif

	MP_CTX_INT_CONSTANT(FILL_RULE,WINDING),
	MP_CTX_INT_CONSTANT(FILL_RULE,EVEN_ODD),
	MP_CTX_INT_CONSTANT(JOIN,BEVEL),
	MP_CTX_INT_CONSTANT(JOIN,ROUND),
	MP_CTX_INT_CONSTANT(JOIN,MITER),
	MP_CTX_INT_CONSTANT(CAP,NONE),
	MP_CTX_INT_CONSTANT(CAP,ROUND),
	MP_CTX_INT_CONSTANT(CAP,SQUARE),
#if CTX_BLENDING_AND_COMPOSITING
	MP_CTX_INT_CONSTANT(COMPOSITE,SOURCE_OVER),
	MP_CTX_INT_CONSTANT(COMPOSITE,COPY),
	MP_CTX_INT_CONSTANT(COMPOSITE,SOURCE_IN),
	MP_CTX_INT_CONSTANT(COMPOSITE,SOURCE_OUT),
	MP_CTX_INT_CONSTANT(COMPOSITE,SOURCE_ATOP),
	MP_CTX_INT_CONSTANT(COMPOSITE,CLEAR),
	MP_CTX_INT_CONSTANT(COMPOSITE,DESTINATION_OVER),
	MP_CTX_INT_CONSTANT(COMPOSITE,DESTINATION),
	MP_CTX_INT_CONSTANT(COMPOSITE,DESTINATION_IN),
	MP_CTX_INT_CONSTANT(COMPOSITE,DESTINATION_OUT),
	MP_CTX_INT_CONSTANT(COMPOSITE,DESTINATION_ATOP),
	MP_CTX_INT_CONSTANT(COMPOSITE,XOR),
	MP_CTX_INT_CONSTANT(BLEND,NORMAL),
	MP_CTX_INT_CONSTANT(BLEND,MULTIPLY),
	MP_CTX_INT_CONSTANT(BLEND,SCREEN),
	MP_CTX_INT_CONSTANT(BLEND,OVERLAY),
	MP_CTX_INT_CONSTANT(BLEND,DARKEN),
	MP_CTX_INT_CONSTANT(BLEND,LIGHTEN),
	MP_CTX_INT_CONSTANT(BLEND,COLOR_DODGE),
	MP_CTX_INT_CONSTANT(BLEND,COLOR_BURN),
	MP_CTX_INT_CONSTANT(BLEND,HARD_LIGHT),
	MP_CTX_INT_CONSTANT(BLEND,SOFT_LIGHT),
	MP_CTX_INT_CONSTANT(BLEND,DIFFERENCE),
	MP_CTX_INT_CONSTANT(BLEND,EXCLUSION),
	MP_CTX_INT_CONSTANT(BLEND,HUE),
	MP_CTX_INT_CONSTANT(BLEND,SATURATION),
	MP_CTX_INT_CONSTANT(BLEND,COLOR),
	MP_CTX_INT_CONSTANT(BLEND,LUMINOSITY),
	MP_CTX_INT_CONSTANT(BLEND,DIVIDE),
	MP_CTX_INT_CONSTANT(BLEND,ADDITION),
	MP_CTX_INT_CONSTANT(BLEND,SUBTRACT),
#endif
	MP_CTX_INT_CONSTANT(TEXT_BASELINE,ALPHABETIC),
	MP_CTX_INT_CONSTANT(TEXT_BASELINE,TOP),
	MP_CTX_INT_CONSTANT(TEXT_BASELINE,HANGING),
	MP_CTX_INT_CONSTANT(TEXT_BASELINE,MIDDLE),
	MP_CTX_INT_CONSTANT(TEXT_BASELINE,IDEOGRAPHIC),
	MP_CTX_INT_CONSTANT(TEXT_BASELINE,BOTTOM),
	MP_CTX_INT_CONSTANT(TEXT_ALIGN,START),
	MP_CTX_INT_CONSTANT(TEXT_ALIGN,END),
	MP_CTX_INT_CONSTANT(TEXT_ALIGN,CENTER),
	MP_CTX_INT_CONSTANT(TEXT_ALIGN,LEFT),
	MP_CTX_INT_CONSTANT(TEXT_ALIGN,RIGHT),

	MP_CTX_INT_CONSTANT(FORMAT,GRAY8),
	MP_CTX_INT_CONSTANT(FORMAT,GRAYA8),
	MP_CTX_INT_CONSTANT(FORMAT,RGB8),
	MP_CTX_INT_CONSTANT(FORMAT,RGBA8),
	MP_CTX_INT_CONSTANT(FORMAT,BGRA8),
	MP_CTX_INT_CONSTANT(FORMAT,RGB565),
	MP_CTX_INT_CONSTANT(FORMAT,RGB565_BYTESWAPPED),
	MP_CTX_INT_CONSTANT(FORMAT,RGB332),
	//MP_CTX_INT_CONSTANT(FORMAT,RGBAF),
	//MP_CTX_INT_CONSTANT(FORMAT,GRAYF),
	//MP_CTX_INT_CONSTANT(FORMAT,GRAYAF),
	MP_CTX_INT_CONSTANT(FORMAT,GRAY1),
	MP_CTX_INT_CONSTANT(FORMAT,GRAY2),
	MP_CTX_INT_CONSTANT(FORMAT,GRAY4),
	MP_CTX_INT_CONSTANT(FORMAT,YUV420),

	MP_CTX_INT_CONSTANT_UNPREFIXED(PRESS),
	MP_CTX_INT_CONSTANT_UNPREFIXED(MOTION),
	MP_CTX_INT_CONSTANT_UNPREFIXED(RELEASE),
	MP_CTX_INT_CONSTANT_UNPREFIXED(ENTER),
	MP_CTX_INT_CONSTANT_UNPREFIXED(LEAVE),
	MP_CTX_INT_CONSTANT_UNPREFIXED(TAP),
	MP_CTX_INT_CONSTANT_UNPREFIXED(TAP_AND_HOLD),
	MP_CTX_INT_CONSTANT_UNPREFIXED(DRAG_PRESS),
	MP_CTX_INT_CONSTANT_UNPREFIXED(DRAG_MOTION),
	MP_CTX_INT_CONSTANT_UNPREFIXED(DRAG_RELEASE),
	MP_CTX_INT_CONSTANT_UNPREFIXED(KEY_PRESS),
	MP_CTX_INT_CONSTANT_UNPREFIXED(KEY_DOWN),
	MP_CTX_INT_CONSTANT_UNPREFIXED(KEY_UP),
	MP_CTX_INT_CONSTANT_UNPREFIXED(SCROLL),
	MP_CTX_INT_CONSTANT_UNPREFIXED(MESSAGE),
	MP_CTX_INT_CONSTANT_UNPREFIXED(DROP),
	MP_CTX_INT_CONSTANT_UNPREFIXED(SET_CURSOR),

};
static MP_DEFINE_CONST_DICT(mp_ctx_locals_dict, mp_ctx_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    mp_ctx_type,
    MP_QSTR_ctx_type,
    MP_TYPE_FLAG_NONE,
    //print, array_print,
    make_new, mp_ctx_make_new,
    attr, mp_ctx_attr,
    locals_dict, &mp_ctx_locals_dict
);

/*
const mp_obj_type_t mp_ctx_type = {
	.base        = { &mp_type_type },
	.name        = MP_QSTR_Context,
	.make_new    = mp_ctx_make_new,
	.locals_dict = (mp_obj_t)&mp_ctx_locals_dict,
        .attr        = mp_ctx_attr
};
*/

/* The globals table for this module */
static const mp_rom_map_elem_t mp_ctx_module_globals_table[] = {
	{ MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_ctx_module) },
	{ MP_ROM_QSTR(MP_QSTR_Context), MP_ROM_PTR(&mp_ctx_type) },
	{ MP_ROM_QSTR(MP_QSTR_CtxEvent), MP_ROM_PTR(&mp_ctx_event_type) },

	MP_CTX_INT_CONSTANT(FORMAT,GRAY8),
	MP_CTX_INT_CONSTANT(FORMAT,GRAYA8),
	MP_CTX_INT_CONSTANT(FORMAT,RGB8),
	MP_CTX_INT_CONSTANT(FORMAT,RGBA8),
	MP_CTX_INT_CONSTANT(FORMAT,BGRA8),
	MP_CTX_INT_CONSTANT(FORMAT,RGB565),
	MP_CTX_INT_CONSTANT(FORMAT,RGB565_BYTESWAPPED),
	MP_CTX_INT_CONSTANT(FORMAT,RGB332),
	//MP_CTX_INT_CONSTANT(FORMAT,RGBAF),
	//MP_CTX_INT_CONSTANT(FORMAT,GRAYF),
	//MP_CTX_INT_CONSTANT(FORMAT,GRAYAF),
	MP_CTX_INT_CONSTANT(FORMAT,GRAY1),
	MP_CTX_INT_CONSTANT(FORMAT,GRAY2),
	MP_CTX_INT_CONSTANT(FORMAT,GRAY4),
	MP_CTX_INT_CONSTANT(FORMAT,YUV420),
        
        MP_CTX_INT_CONSTANT(FLAG,LOWFI),
        MP_CTX_INT_CONSTANT(FLAG,GRAY2),
        MP_CTX_INT_CONSTANT(FLAG,GRAY4),
        MP_CTX_INT_CONSTANT(FLAG,GRAY8),
        MP_CTX_INT_CONSTANT(FLAG,RGB332),
        MP_CTX_INT_CONSTANT(FLAG,HASH_CACHE),
//      MP_CTX_INT_CONSTANT(FLAG,DAMAGE_CONTROL),
        MP_CTX_INT_CONSTANT(FLAG,KEEP_DATA),
        MP_CTX_INT_CONSTANT(FLAG,INTRA_UPDATE),
        MP_CTX_INT_CONSTANT(FLAG,STAY_LOW),
#if CTX_ENABLE_CBRLE
        MP_CTX_INT_CONSTANT(FLAG,CBRLE),
#endif

};
static MP_DEFINE_CONST_DICT(mp_ctx_module_globals, mp_ctx_module_globals_table);

const mp_obj_module_t mp_module_ctx = {
	.base    = { &mp_type_module },
	.globals = (mp_obj_dict_t *)&mp_ctx_module_globals,
};

/* This is a special macro that will make MicroPython aware of this module */
/* clang-format off */
MP_REGISTER_MODULE(MP_QSTR_ctx, mp_module_ctx);
