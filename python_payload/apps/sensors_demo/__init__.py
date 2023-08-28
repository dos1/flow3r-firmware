from st3m.application import Application, ApplicationContext
from st3m.goose import Dict, Any, List, Optional
from st3m.ui.view import View, ViewManager
from st3m.input import InputState
from ctx import Context

import json
import math

import leds
from st3m.application import Application, ApplicationContext


class App(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)
        self.offset = -60
        self.interval = 20
        self.data_exists = False
        self.rotate = 0

    def draw_background(self, ctx):
        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()

        offset = self.offset
        interval = self.interval

        ctx.font_size = 25
        ctx.font = ctx.get_font_name(5)
        ctx.text_align = ctx.RIGHT
        ctx.rgb(0.8, 0.8, 0.6)
        ctx.font_size = 22
        counter = offset

        gap = -4

        ctx.move_to(gap, counter)
        counter += interval
        ctx.text("pressure")
        ctx.move_to(gap, counter)
        counter += interval
        ctx.text("temperature")
        ctx.move_to(gap, counter)
        counter += interval
        ctx.text("battery voltage")
        ctx.move_to(gap, counter)
        counter += interval
        ctx.text("meters above sea")

        ctx.move_to(gap, counter)
        counter += 2 * interval
        ctx.text("accelerometer")
        ctx.move_to(gap, counter)
        counter += 2 * interval
        ctx.text("gyroscope")

        gap = 4

        counter -= 4 * interval
        ctx.text_align = ctx.LEFT
        ctx.rgb(0.6, 0.7, 0.6)
        ctx.font = ctx.get_font_name(3)
        ctx.font_size = 14

        ctx.move_to(gap, counter)
        counter += 2 * interval
        ctx.text("(x, y, z) [m/s]")
        ctx.move_to(gap, counter)
        counter += 2 * interval
        ctx.text("(x, y, z) [deg/s]")

    def draw(self, ctx: Context) -> None:
        offset = self.offset
        interval = self.interval

        damp = 0.7

        if self.data_exists:
            inc = self.imu.acc.inclination
            if (inc > 0.3) and (inc < 3.11):
                self.rotate *= damp
                self.rotate += (1 - damp) * self.imu.acc.azimuth
                ctx.rotate(-self.rotate)
            else:
                self.rotate = 0

        if self.draw_background_request > 0:
            self.draw_background(ctx)
            # rotation introduced, disabling partial refresh
            # self.draw_background_request -= 1
        else:
            ctx.rgb(0, 0, 0)
            ctx.rectangle(0, -120, 120, 125).fill()
            ctx.rectangle(-120, offset + 4 * interval + 5, 240, 20).fill()
            ctx.rectangle(-120, offset + 6 * interval + 5, 240, 20).fill()
        if not self.data_exists:
            return

        ctx.font = ctx.get_font_name(5)
        ctx.font_size = 25
        counter = offset

        gap = 4

        ctx.rgb(0.5, 0.8, 0.8)

        ctx.move_to(gap, counter)
        counter += interval
        ctx.text(str(self.pressure / 100)[:6] + "hPa")
        ctx.move_to(gap, counter)
        counter += interval
        ctx.text(str(self.temperature)[:5] + "degC")
        ctx.move_to(gap, counter)
        counter += interval
        if self.battery_voltage is None:
            ctx.text("n/a")
        else:
            ctx.text(str(self.battery_voltage)[:5] + "V")
        ctx.move_to(gap, counter)
        counter += interval
        ctx.text(str(self.meters_above_sea)[:6] + "m")

        counter += interval
        ctx.text_align = ctx.MIDDLE

        ctx.move_to(0, counter)
        counter += 2 * interval
        acc = ", ".join([str(x)[:4] for x in self.imu.acc])
        ctx.text("(" + acc + ")")
        ctx.move_to(0, counter)
        counter += 2 * interval
        gyro = ", ".join([str(x)[:4] for x in self.imu.gyro])
        ctx.text("(" + gyro + ")")

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        # these are simple numeric values, autocopy does the job for us
        self.pressure = ins.pressure
        self.meters_above_sea = ins.meters_above_sea
        self.battery_voltage = ins.battery_voltage
        self.temperature = ins.temperature

        # this is a regular class instance, manually use copy here
        self.imu = ins.imu.copy()

        hue = 360 * (self.meters_above_sea % 1.0)
        leds.set_all_hsv(hue, 1, 1)
        leds.update()

        self.data_exists = True

    def on_enter(self, vm: Optional[ViewManager]) -> None:
        super().on_enter(vm)
        self.data_exists = False
        self.draw_background_request = 10


# For running with `mpremote run`:
if __name__ == "__main__":
    import st3m.run

    st3m.run.run_view(App(ApplicationContext()))
