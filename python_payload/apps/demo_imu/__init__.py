# flow3r imports
from st3m import InputState
from st3m.application import Application, ApplicationContext
from ctx import Context
from st3m.utils import tau


class IMUDemo(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)
        self.v_x = 0.0
        self.v_y = 0.0
        self.p_x = 0.0
        self.p_y = 0.0

    def draw(self, ctx: Context) -> None:
        ctx.rectangle(-120, -120, 240, 240)
        ctx.rgb(0, 0, 0)
        ctx.fill()

        ctx.arc(self.p_x, self.p_y, 10, 0, tau, 0)
        ctx.rgb(117 / 255, 255 / 255, 226 / 255)
        ctx.fill()

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        self.v_y += ins.imu.acc[0] * delta_ms / 1000.0 * 10
        self.v_x += ins.imu.acc[1] * delta_ms / 1000.0 * 10

        x = self.p_x + self.v_x * delta_ms / 1000.0
        y = self.p_y + self.v_y * delta_ms / 1000.0

        if x**2 + y**2 < (120 - 10) ** 2:
            self.p_x = x
            self.p_y = y
        else:
            self.v_x = 0
            self.v_y = 0
