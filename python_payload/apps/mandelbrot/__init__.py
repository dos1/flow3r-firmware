from st3m.application import Application
import sys_display, math, random


class App(Application):
    def __init__(self, app_ctx):
        super().__init__(app_ctx)

    def on_enter(self, vm):
        super().on_enter(vm)
        self.y = 0
        self.xa = -1.5
        self.xb = 1.5
        self.ya = -2.0
        self.yb = 1.0

    def on_enter_done(self):
        sys_display.set_mode(sys_display.cool | sys_display.x2)

    def draw(self, ctx: Context):
        if self.vm.transitioning:
            ctx.gray(0).rectangle(-120, -120, 240, 240).fill()
            return

        fb_info = sys_display.fb(sys_display.cool)
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
                for x in range(width / 2):
                    zx = x * (self.xb - self.xa) / (width - 1) + self.xa
                    z = zy + zx * 1j
                    c = z
                    reached = 0
                    if inners > 10 and y < height * 0.7:
                        reached = max_iterations - 1
                    else:
                        for i in range(max_iterations):
                            if abs(z) > 2.0:
                                break
                            z = z * z + c
                            reached = i
                    if reached == max_iterations - 1:
                        inners += 1
                    val = reached * 255 / max_iterations
                    val = math.sqrt(val / 255) * 255
                    fb[y * stride + x] = int(val)
                    fb[y * stride + width - 1 - x] = int(val)
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
