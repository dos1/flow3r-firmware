from st3m.application import Application
import sys_display, math, random


class App(Application):
    def __init__(self, app_ctx):
        super().__init__(app_ctx)

    def on_enter(self, vm):
        super().on_enter(vm)
        sys_display.set_mode(1)
        pal = bytearray(256 * 3)
        modr = random.getrandbits(7) + 1
        modg = random.getrandbits(7) + 1
        modb = random.getrandbits(7) + 1

        pal[0 * 3] = 0
        pal[0 * 3 + 1] = 0
        pal[0 * 3 + 2] = 0
        pal[1 * 3] = 255
        pal[1 * 3 + 1] = 255
        pal[1 * 3 + 2] = 255
        sys_display.set_palette(pal)
        self.y = 0
        self.xa = -1.5
        self.xb = 1.5
        self.ya = -2.0
        self.yb = 1.0

    def draw(self, ctx: Context):
        fb_info = sys_display.fb(0)
        fb = fb_info[0]

        max_iterations = 30

        width = fb_info[1]
        height = fb_info[2]
        stride = fb_info[3]

        for chunk in range(3):  # do 3 scanlines at a time
            y = self.y
            if y < height:
                zy = y * (self.yb - self.ya) / (height - 1) + self.ya
                inners = 0
                for x in range(width):
                    zx = x * (self.xb - self.xa) / (width - 1) + self.xa
                    z = zy + zx * 1j
                    c = z
                    reached = 0
                    for i in range(max_iterations):
                        if abs(z) > 2.0:
                            break
                        z = z * z + c
                        reached = i
                    val = reached * 255 / max_iterations
                    val = int(math.sqrt(val / 255) * 16) & 1
                    fb[int((y * stride * 8 + x) / 8)] |= int(val) << (x & 7)

                self.y += 1


if __name__ == "__main__":
    import st3m.run

    st3m.run.run_app(App)
