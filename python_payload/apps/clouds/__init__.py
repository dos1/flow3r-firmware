import st3m.run, random

from st3m.application import Application, ApplicationContext
from st3m.input import InputState
from ctx import Context


class Cloud:
    def __init__(self, x: float, y: float, z: float) -> None:
        self.x = x
        self.y = y
        self.z = z

    def draw(self, ctx: Context) -> None:
        x = self.x / self.z * 120
        y = self.y / self.z * 120
        width = 200.0 / self.z * 120
        height = 100.0 / self.z * 120
        ctx.image(
            "/flash/sys/apps/clouds/cloud.png",
            x - width / 2,
            y - height / 2,
            width,
            height,
        )


class Clouds(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)
        self.clouds = []
        for i in range(10):
            self.clouds.append(
                Cloud(
                    ((random.getrandbits(16) - 32767) / 32767.0) * 200,
                    ((random.getrandbits(16)) / 65535.0) * 50 - 5,
                    ((random.getrandbits(16)) / 65535.0) * 200 + 5,
                )
            )

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)
        for c in self.clouds:
            c.z -= 40 * delta_ms / 1000.0
            if c.z < 10:
                c.z = 300
        self.clouds = sorted(self.clouds, key=lambda c: -c.z)

    def draw(self, ctx: Context) -> None:
        ctx.image_smoothing = False
        ctx.rectangle(-120, -120, 240, 120)
        ctx.rgb(0, 0.34, 0.72)
        ctx.fill()
        ctx.rectangle(-120, 0, 240, 120)
        ctx.rgb(0.8, 0.50, 0.1)
        ctx.fill()

        for c in self.clouds:
            c.draw(ctx)


# uncomment to make runnable via mpremote
# st3m.run.run_view(Clouds(ApplicationContext()))
