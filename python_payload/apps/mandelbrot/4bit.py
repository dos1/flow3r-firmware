from st3m.application import Application
import sys_display, math, random


class App(Application):
    def __init__(self, app_ctx):
        super().__init__(app_ctx)

    def on_enter(self, vm):
        super().on_enter(vm)
        sys_display.set_mode(4)
        self.y = 0
        self.xa = -1.5
        self.xb = 1.5
        self.ya = -2.0
        self.yb = 1.0

    def draw(self, ctx: Context):
        fb = sys_display.fb(4)

        max_iterations = 30

        imgx = 240
        imgy = 240

        for chunk in range(3):  # do 3 scanlines at a time
            y = self.y
            if y < 240:
                zy = y * (self.yb - self.ya) / (imgy - 1) + self.ya
                inners = 0
                for x in range(imgx):
                    zx = x * (self.xb - self.xa) / (imgx - 1) + self.xa
                    z = zy + zx * 1j
                    c = z
                    reached = 0
                    for i in range(max_iterations):
                        if abs(z) > 2.0:
                            break
                        z = z * z + c
                        reached = i
                    val = reached * 255 / max_iterations
                    val = math.sqrt(val / 255) * 16
                    if val > 15:
                        val = 15
                    oval = fb[int((y * 240 + x) / 2)]
                    fb[int((y * 240 + x) / 2)] = oval | (int(val) << ((x & 1) * 4))
                self.y += 1

    def think(self, ins, delta_ms):
        super().think(ins, delta_ms)
        if self.input.buttons.app.right.pressed:
            pal = bytearray(256 * 3)
            modr = random.getrandbits(7) + 1
            modg = random.getrandbits(7) + 1
            modb = random.getrandbits(7) + 1

            for i in range(256):
                pal[i * 3] = int((i % modr) * (255 / modr))
                pal[i * 3 + 1] = int((i % modg) * (255 / modg))
                pal[i * 3 + 2] = int((i % modb) * (255 / modb))
            sys_display.set_palette(pal)
        if self.input.buttons.app.left.pressed:
            pal = bytearray(256 * 3)
            for i in range(256):
                pal[i * 3] = i
                pal[i * 3 + 1] = i
                pal[i * 3 + 2] = i
            sys_display.set_palette(pal)


if __name__ == "__main__":
    import st3m.run

    st3m.run.run_app(App)
