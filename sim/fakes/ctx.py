"""
ctx.py implements a subset of uctx that is backed by a WebAssembly-compiled
ctx. The interface between our uctx fake and the underlying ctx is the
serialized ctx protocol as described in [1].

[1] - https://ctx.graphics/protocol/
"""
import os
import math
import sys

import wasmer
import wasmer_compiler_cranelift


class Wasm:
    """
    Wasm wraps access to WebAssembly functions, converting to/from Python types
    as needed. It's intended to be used as a singleton.
    """

    def __init__(self):
        store = wasmer.Store(wasmer.engine.JIT(wasmer_compiler_cranelift.Compiler))
        simpath = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
        wasmpath = os.path.join(simpath, "wasm", "ctx.wasm")
        module = wasmer.Module(store, open(wasmpath, "rb").read())
        wasi_version = wasmer.wasi.get_version(module, strict=False)
        wasi_env = wasmer.wasi.StateBuilder("badge23sim").finalize()
        import_object = wasi_env.generate_import_object(store, wasi_version)
        instance = wasmer.Instance(module, import_object)
        self._i = instance

    def malloc(self, n):
        return self._i.exports.malloc(n)

    def free(self, p):
        self._i.exports.free(p)

    def ctx_parse(self, ctx, s):
        s = s.encode("utf-8")
        slen = len(s) + 1
        p = self.malloc(slen)
        mem = self._i.exports.memory.uint8_view(p)
        mem[0 : slen - 1] = s
        mem[slen - 1] = 0
        self._i.exports.ctx_parse(ctx, p)
        self.free(p)

    def ctx_new_for_framebuffer(self, width, height, stride, format):
        """
        Call ctx_new_for_framebuffer, but also first allocate the underlying
        framebuffer and return it alongside the Ctx*.
        """
        fb = self.malloc(stride * height)
        return fb, self._i.exports.ctx_new_for_framebuffer(
            fb, width, height, stride, format
        )

    def ctx_new_drawlist(self, width, height):
        return self._i.exports.ctx_new_drawlist(width, height)

    def ctx_apply_transform(self, ctx, *args):
        args = [float(a) for a in args]
        return self._i.exports.ctx_apply_transform(ctx, *args)

    def ctx_define_texture(self, ctx, eid, *args):
        s = eid.encode("utf-8")
        slen = len(s) + 1
        p = self.malloc(slen)
        mem = self._i.exports.memory.uint8_view(p)
        mem[0 : slen - 1] = s
        mem[slen - 1] = 0
        res = self._i.exports.ctx_define_texture(ctx, p, *args)
        self.free(p)
        return res

    def ctx_draw_texture(self, ctx, eid, *args):
        s = eid.encode("utf-8")
        slen = len(s) + 1
        p = self.malloc(slen)
        mem = self._i.exports.memory.uint8_view(p)
        mem[0 : slen - 1] = s
        mem[slen - 1] = 0
        args = [float(a) for a in args]
        res = self._i.exports.ctx_draw_texture(ctx, p, *args)
        self.free(p)
        return res

    def ctx_text_width(self, ctx, text):
        s = text.encode("utf-8")
        slen = len(s) + 1
        p = self.malloc(slen)
        mem = self._i.exports.memory.uint8_view(p)
        mem[0 : slen - 1] = s
        mem[slen - 1] = 0
        res = self._i.exports.ctx_text_width(ctx, p)
        self.free(p)
        return res

    def ctx_destroy(self, ctx):
        return self._i.exports.ctx_destroy(ctx)

    def ctx_render_ctx(self, ctx, dctx):
        return self._i.exports.ctx_render_ctx(ctx, dctx)


_wasm = Wasm()


class Context:
    """
    Ctx implements a subset of uctx [1]. It should be extended as needed as we
    make use of more and more uctx features in the badge code.

    [1] - https://ctx.graphics/uctx/
    """

    LEFT = "left"
    RIGHT = "right"
    CENTER = "center"
    HANGING = "hanging"
    CLEAR = "clear"
    END = "end"
    MIDDLE = "middle"
    BEVEL = "bevel"
    NONE = "none"
    COPY = "copy"

    def __init__(self, _ctx):
        self._ctx = _ctx
        self._font_size = 0
        self._line_width = 0

    @property
    def image_smoothing(self):
        return 0

    @image_smoothing.setter
    def image_smoothing(self, v):
        self._emit(f"imageSmoothing 0")

    @property
    def text_align(self):
        return None

    @text_align.setter
    def text_align(self, v):
        self._emit(f"textAlign {v}")

    @property
    def text_baseline(self):
        return None

    @text_baseline.setter
    def text_baseline(self, v):
        self._emit(f"textBaseline {v}")

    @property
    def compositing_mode(self):
        return Context.NONE

    @compositing_mode.setter
    def compositing_mode(self, v):
        self._emit(f"compositingMode {v}")

    @property
    def line_width(self):
        return self._line_width

    @line_width.setter
    def line_width(self, v):
        self._line_width = v
        self._emit(f"lineWidth {v:.3f}")

    @property
    def font(self):
        return None

    @font.setter
    def font(self, v):
        self._emit(f'font "{v}"')

    @property
    def font_size(self):
        return self._font_size

    @font_size.setter
    def font_size(self, v):
        self._font_size = v
        self._emit(f"fontSize {v:.3f}")

    @property
    def global_alpha(self):
        return None

    @global_alpha.setter
    def global_alpha(self, v):
        self._emit(f"globalAlpha {v:.3f}")

    @property
    def x(self):
        return 0

    @property
    def y(self):
        return 0

    def _emit(self, text):
        _wasm.ctx_parse(self._ctx, text)

    def logo(self, x, y, r):
        return self

    def move_to(self, x, y):
        self._emit(f"moveTo {x:.3f} {y:.3f}")
        return self

    def curve_to(self, a, b, c, d, e, f):
        self._emit(f"curveTo {a:.3f} {b:.3f} {c:.3f} {d:.3f} {e:.3f} {f:.3f}")
        return self

    def quad_to(self, a, b, c, d):
        self._emit(f"quadTo {a:.3f} {b:.3f} {c:.3f} {d:.3f}")
        return self

    def rel_move_to(self, x, y):
        self._emit(f"relMoveTo {x:.3f} {y:.3f}")
        return self

    def rel_curve_to(self, a, b, c, d, e, f):
        self._emit(f"relCurveTo {a:.3f} {b:.3f} {c:.3f} {d:.3f} {e:.3f} {f:.3f}")
        return self

    def rel_quad_to(self, a, b, c, d):
        self._emit(f"relQuadTo {a:.3f} {b:.3f} {c:.3f} {d:.3f}")
        return self

    def close_path(self):
        self._emit(f"closePath")
        return self

    def translate(self, x, y):
        self._emit(f"translate {x:.3f} {y:.3f}")
        return self

    def scale(self, x, y):
        self._emit(f"scale {x:.3f} {y:.3f}")
        return self

    def line_to(self, x, y):
        self._emit(f"lineTo {x:.3f} {y:.3f}")
        return self

    def rel_line_to(self, x, y):
        self._emit(f"relLineTo {x:.3f} {y:.3f}")
        return self

    def rotate(self, v):
        self._emit(f"rotate {v:.3f}")
        return self

    def gray(self, v):
        self._emit(f"gray {v:.3f}")
        return self

    def rgba(self, r, g, b, a):
        # TODO(q3k): dispatch by type instead of value, warn on
        # ambiguous/unexpected values for type.
        if r > 1.0 or g > 1.0 or b > 1.0 or a > 1.0:
            r /= 255.0
            g /= 255.0
            b /= 255.0
            a /= 255.0
        self._emit(f"rgba {r:.3f} {g:.3f} {b:.3f} {a:.3f}")
        return self

    def rgb(self, r, g, b):
        # TODO(q3k): dispatch by type instead of value, warn on
        # ambiguous/unexpected values for type.
        if r > 1.0 or g > 1.0 or b > 1.0:
            r /= 255.0
            g /= 255.0
            b /= 255.0
        self._emit(f"rgb {r:.3f} {g:.3f} {b:.3f}")
        return self

    def text(self, s):
        self._emit(f'text "{s}"')
        return self

    def round_rectangle(self, x, y, width, height, radius):
        self._emit(
            f"roundRectangle {x:.3f} {y:.3f} {width:.3f} {height:.3f} {radius:.3f}"
        )
        return self

    def image(self, path, x, y, width, height):
        # TODO: replace with base64 encoded, decoded version of image
        self._emit(f"save")
        self._emit(f"rectangle {x} {y} {width} {height}")
        self._emit(f"rgba 0.5 0.5 0.5 0.5")
        self._emit(f"fill")
        self._emit(f"rectangle {x} {y} {width} {height}")
        self._emit(f"gray 1.0")
        self._emit(f"lineWidth 1")
        self._emit(f"stroke")
        self._emit(f"restore")
        return self

    def rectangle(self, x, y, width, height):
        self._emit(f"rectangle {x} {y} {width} {height}")
        return self

    def stroke(self):
        self._emit(f"stroke")
        return self

    def save(self):
        self._emit(f"save")
        return self

    def restore(self):
        self._emit(f"restore")
        return self

    def fill(self):
        self._emit(f"fill")
        return self

    def radial_gradient(self, x0, y0, r0, x1, y1, r1):
        self._emit(
            f"radialGradient {x0:.3f} {y0:.3f} {r0:.3f} {x1:.3f} {y1:.3f} {r1:.3f}"
        )
        return self

    def linear_gradient(self, x0, y0, x1, y1):
        self._emit(f"linearGradient {x0:.3f} {y0:.3f} {x1:.3f} {y1:.3f}")
        return self

    def add_stop(self, pos, color, alpha):
        red, green, blue = color
        if red > 1.0 or green > 1.0 or blue > 1.0:
            red /= 255.0
            green /= 255.0
            blue /= 255.0
        if alpha > 1.0:
            # Should never happen, since alpha must be a float < 1.0, see line 711 in uctx.c
            alpha = 1.0
            print(
                "alpha > 1.0, this is an error in the real uctx library.",
                file=sys.stderr,
            )
        if alpha < 0.0:
            alpha = 0.0
            print(
                "alpha < 0.0, this is an error in the real uctx library.",
                file=sys.stderr,
            )
        self._emit(
            f"gradientAddStop {pos:.3f} {red:.3f} {green:.3f} {blue:.3f} {alpha:.3f} "
        )
        return self

    def begin_path(self):
        self._emit(f"beginPath")
        return self

    def arc(self, x, y, radius, arc_from, arc_to, direction):
        self._emit(
            f"arc {x:.3f} {y:.3f} {radius:.3f} {arc_from:.4f} {arc_to:.4f} {1 if direction else 0}"
        )
        return self

    def text_width(self, text):
        return _wasm.ctx_text_width(self._ctx, text)

    def clip(self):
        self._emit(f"clip")
        return self

    def get_font_name(self, i):
        return [
            "Arimo Regular",
            "Arimo Bold",
            "Arimo Italic",
            "Arimo Bold Italic",
            "Camp Font 1",
            "Camp Font 2",
            "Camp Font 3",
            "Material Icons",
            "Comic Mono",
        ][i]

    def scope(self):
        x = -120
        self.move_to(x, 0)
        for i in range(240):
            x2 = x + i
            y2 = math.sin(i / 10) * 60
            self.line_to(x2, y2)
        self.stroke()
        return self


RGBA8 = 4
BGRA8 = 5
RGB565_BYTESWAPPED = 7
