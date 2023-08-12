"""
ctx.py implements a subset of uctx that is backed by a WebAssembly-compiled
ctx. The interface between our uctx fake and the underlying ctx is the
serialized ctx protocol as described in [1].

[1] - https://ctx.graphics/protocol/
"""
import os
import math

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

    def ctx_new_for_framebuffer(self, width, height):
        """
        Call ctx_new_for_framebuffer, but also first allocate the underlying
        framebuffer and return it alongside the Ctx*.
        """
        fb = self.malloc(width * height * 4)
        # Significant difference between on-device Ctx and simulation Ctx: we
        # render to a BRGA8 (24bpp color + 8bpp alpha) buffer instead of 16bpp
        # RGB565 like the device does. This allows us to directly blit the ctx
        # framebuffer into pygame's surfaces, which is a _huge_ speed benefit
        # (difference between ~10FPS and 500+FPS!).
        BRGA8 = 5
        return fb, self._i.exports.ctx_new_for_framebuffer(
            fb, width, height, width * 4, BRGA8
        )

    def ctx_new_drawlist(self, width, height):
        return self._i.exports.ctx_new_drawlist(width, height)

    def ctx_apply_transform(self, ctx, *args):
        args = [float(a) for a in args]
        return self._i.exports.ctx_apply_transform(ctx, *args)

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
    END = "end"
    MIDDLE = "middle"
    BEVEL = "bevel"

    def __init__(self, _ctx):
        self._ctx = _ctx

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
    def line_width(self):
        return None

    @line_width.setter
    def line_width(self, v):
        self._emit(f"lineWidth {v:.3f}")

    @property
    def font(self):
        return None

    @font.setter
    def font(self, v):
        self._emit(f'font "{v}"')

    @property
    def font_size(self):
        return None

    @font_size.setter
    def font_size(self, v):
        self._emit(f"fontSize {v:.3f}")

    @property
    def global_alpha(self):
        return None

    @global_alpha.setter
    def global_alpha(self, v):
        self._emit(f"globalAlpha {v:.3f}")

    def _emit(self, text):
        _wasm.ctx_parse(self._ctx, text)

    def move_to(self, x, y):
        self._emit(f"moveTo {int(x)} {int(y)}")
        return self

    def rel_move_to(self, x, y):
        self._emit(f"relMoveTo {int(x)} {int(y)}")
        return self

    def translate(self, x, y):
        self._emit(f"translate {int(x)} {int(y)}")
        return self

    def scale(self, x, y):
        self._emit(f"scale {x:.3f} {y:.3f}")
        return self

    def line_to(self, x, y):
        self._emit(f"lineTo {int(x)} {int(y)}")
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
            f"roundRectangle {int(x)} {int(y)} {int(width)} {int(height)} {radius}"
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

    def start_group(self):
        self._emit(f"start_group")
        return self

    def end_group(self):
        self._emit(f"end_group")
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

    def arc(self, x, y, radius, arc_from, arc_to, direction):
        self._emit(
            f"arc {int(x)} {int(y)} {int(radius)} {arc_from:.4f} {arc_to:.4f} {1 if direction else 0}"
        )
        return self

    def text_width(self, text):
        return _wasm.ctx_text_width(self._ctx, text)

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
        ][i]

    def scope(self):
        x = -120
        self.move_to(x, 0)
        for i in range(240):
            x2 = x + i
            y2 = math.sin(i / 10) * 80
            self.line_to(x2, y2)
        self.line_to(130, 0)
        self.line_to(130, 130)
        self.line_to(-130, 130)
        self.line_to(-130, 0)
        return self
