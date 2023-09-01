from st3m.application import Application
import sys_display


class App(Application):
    def __init__(self, app_ctx):
        super().__init__(app_ctx)

    def on_enter(self, vm):
        super().on_enter(vm)
        sys_display.set_gfx_mode(24)
        self.x = 0
        self.y = 0

    def draw(self, ctx: Context):
        fb = sys_display.fb(24)

        o = 0
        for y in range(240):
            for x in range(240):
                u = x + self.x
                v = y + self.y
                fb[(y * 240 + x) * 3 + 0] = u
                fb[(y * 240 + x) * 3 + 1] = v
                fb[(y * 240 + x) * 3 + 2] = u ^ v
        self.x += 1
        self.y += 2


if __name__ == "__main__":
    import st3m.run

    st3m.run.run_app(App)
