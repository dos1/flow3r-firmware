"""
ctx.py implements a subset of uctx that is backed by a WebAssembly-compiled
ctx. The interface between our uctx fake and the underlying ctx is the
serialized ctx protocol as described in [1].

[1] - https://ctx.graphics/protocol/
"""
import os

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
        wasmpath = os.path.join(simpath, 'wasm', 'ctx.wasm')
        module = wasmer.Module(store, open(wasmpath, 'rb').read())
        wasi_version = wasmer.wasi.get_version(module, strict=False)
        wasi_env = wasmer.wasi.StateBuilder('badge23sim').finalize()
        import_object = wasi_env.generate_import_object(store, wasi_version)
        instance = wasmer.Instance(module, import_object)
        self._i = instance

    def malloc(self, n):
        return self._i.exports.malloc(n)

    def free(self, p):
        self._i.exports.free(p)

    def ctx_parse(self, ctx, s):
        s = s.encode('utf-8')
        slen = len(s) + 1
        p = self.malloc(slen)
        mem = self._i.exports.memory.uint8_view(p)
        mem[0:slen] = s
        mem[slen-1] = 0
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
        return fb, self._i.exports.ctx_new_for_framebuffer(fb, width, height, width * 4, BRGA8)

    def ctx_apply_transform(self, ctx, *args):
        args = [float(a) for a in args]
        return self._i.exports.ctx_apply_transform(ctx, *args)

_wasm = Wasm()

class Ctx:
    """
    Ctx implements a subset of uctx [1]. It should be extended as needed as we
    make use of more and more uctx features in the badge code.

    [1] - https://ctx.graphics/uctx/
    """
    LEFT = 'left'
    RIGHT = 'right'
    CENTER = 'center'
    END = 'end'
    MIDDLE = 'middle'

    def __init__(self):
        self._fb, self._ctx = _wasm.ctx_new_for_framebuffer(240, 240)
        # Place (0, 0) in the center of the screen, mathing what the real badge
        # software does.
        _wasm.ctx_apply_transform(self._ctx, 1, 0, 120, 0, 1, 120, 0, 0, 1)

        self.text_align = 'start'
        self.text_baseline = 'alphabetic'

    def _get_fb(self):
        return _wasm._i.exports.memory.uint8_view(self._fb)

    def _emit(self, text):
        _wasm.ctx_parse(self._ctx, text)

    def move_to(self, x, y):
        self._emit(f"moveTo {x} {x}")
        return self

    def rgb(self, r, g, b):
        # TODO(q3k): dispatch by type instead of value, warn on
        # ambiguous/unexpected values for type.
        if r > 1.0 or g > 1.0 or b > 1.0:
            r /= 255.0
            g /= 255.0
            b /= 255.0
        self._emit(f"rgb {r} {g} {b}")
        return self

    def text(self, s):
        self._emit(f"textAlign {self.text_align}")
        self._emit(f"textBaseline {self.text_baseline}")
        self._emit(f"text \"{s}\"")
        return self

    def round_rectangle(self, x, y, width, height, radius):
        self._emit(f"roundRectangle {x} {y} {width} {height} {radius}")
        return self

    def rectangle(self, x, y, width, height):
        self._emit(f"rectangle {x} {y} {width} {height}")
        return self

    def fill(self):
        self._emit(f"fill")
        return self
