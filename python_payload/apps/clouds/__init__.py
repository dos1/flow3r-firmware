import random
from st3m.application import Application
import sys_display


class Cloud:
    def __init__(self, path, x, y, z):
        self.path = path
        self.x = x
        self.y = y
        self.z = z

    def draw(self, ctx) -> None:
        x = self.x / self.z * 120
        y = self.y / self.z * 120
        width = 200.0 / self.z * 160
        height = 100.0 / self.z * 160
        ctx.image(
            self.path,
            x - width / 2,
            y - height / 2,
            width,
            height,
        )


class App(Application):
    def __init__(self, app_ctx):
        super().__init__(app_ctx)
        self.clouds = []
        bundle_path = app_ctx.bundle_path
        if bundle_path == "":
            bundle_path = "/flash/sys/apps/clouds"
        for i in range(10):
            self.clouds.append(
                Cloud(
                    bundle_path + "/cloud.png",
                    ((random.getrandbits(16) - 32767) / 32767.0) * 200,
                    ((random.getrandbits(16)) / 65535.0) * 60 - 10,
                    ((random.getrandbits(16)) / 65535.0) * 200 + 5,
                )
            )

    def think(self, ins, delta_ms):
        super().think(ins, delta_ms)
        for c in self.clouds:
            c.x -= (delta_ms / 1000.0) * ins.imu.acc[1] * 10
            c.z -= (delta_ms / 1000.0) * (ins.imu.acc[2] - 5) * 20

            # wrap x and z coordinates around
            if c.z < 10:
                c.z = 300
            elif c.z > 300:
                c.z = 10
            if c.x < -200:
                c.x = 200
            elif c.x > 200:
                c.x = -200
        self.clouds = sorted(self.clouds, key=lambda c: -c.z)

    def draw(self, ctx):
        # faster, and with smoothing is incorrect
        ctx.image_smoothing = False
        ctx.rectangle(-120, -120, 240, 120)
        ctx.rgb(0, 0.34, 0.72)
        ctx.fill()
        ctx.rectangle(-120, 0, 240, 120)
        ctx.rgb(0.8, 0.50, 0.1)
        ctx.fill()

        for c in self.clouds:
            c.draw(ctx)

    def on_enter(self, vm):
        super().on_enter(vm)
        sys_display.set_mode(24 + sys_display.osd)


if __name__ == "__main__":
    from st3m.run import run_app

    run_app(App)
