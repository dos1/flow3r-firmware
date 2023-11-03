from st3m.application import Application, ApplicationContext
from st3m.goose import Dict, Any, List, Optional
from st3m.ui.view import View, ViewManager
from st3m.ui import colours
from st3m.input import InputState
from ctx import Context

import json
import math

import leds
from st3m.application import Application, ApplicationContext


def inclination(vec):
    x, y, z = vec[0], vec[1], vec[2]
    if z > 0:
        return math.atan((((x**2) + (y**2)) ** 0.5) / z)
    elif z < 0:
        return math.tau / 2 + math.atan((((x**2) + (y**2)) ** 0.5) / z)
    return math.tau / 4


def azimuth(vec):
    x, y, z = vec[0], vec[1], vec[2]
    if x > 0:
        return math.atan(y / x)
    elif x < 0:
        if y < 0:
            return math.atan(y / x) - math.tau / 2
        else:
            return math.atan(y / x) + math.tau / 2
    elif y < 0:
        return -math.tau / 4
    return math.tau / 4


def relative_altitude(pascal, celsius):
    # https://en.wikipedia.org/wiki/Hypsometric_equation
    return (celsius + 273.15) * (287 / 9.81) * math.log(101325 / pascal, math.e)


class App(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)
        self.offset = -60
        self.interval = 20
        self.data_exists = False
        self.rotate = 0
        self.alt_smooth = None

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
        ctx.text("relative altitude")

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

        damp = 0.69

        if self.data_exists:
            inc = inclination(self.acc)
            if (inc > 0.3) and (inc < 3.11):
                delta = azimuth(self.acc) - self.rotate
                if delta > 3.14:
                    delta -= 6.28
                elif delta < -3.14:
                    delta += 6.28
                self.rotate += (1 - damp) * delta
            else:
                self.rotate *= damp
            ctx.rotate(-self.rotate)

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
        ctx.text(str(self.altitude)[:6] + "m")

        counter += interval
        ctx.text_align = ctx.MIDDLE

        ctx.move_to(0, counter)
        counter += 2 * interval
        acc = ", ".join([str(x)[:4] for x in self.acc])
        ctx.text("(" + acc + ")")
        ctx.move_to(0, counter)
        counter += 2 * interval
        gyro = ", ".join([str(x)[:4] for x in self.gyro])
        ctx.text("(" + gyro + ")")

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        self.pressure = ins.imu.pressure
        self.battery_voltage = ins.battery_voltage
        self.temperature = ins.temperature

        # not using actual temperature here since keeping rel. altitude
        # identical when going on a balcony outweighs the extra outdoors
        # precision in our opinion
        self.altitude = relative_altitude(self.pressure, 15)

        self.acc = tuple(ins.imu.acc)
        self.gyro = tuple(ins.imu.gyro)

        n = 4
        damp = 0.69  # cool q bro
        if self.alt_smooth is None:
            self.alt_smooth = [self.altitude] * (n + 1)
        else:
            self.alt_smooth[0] = self.altitude
            for i in range(n):
                self.alt_smooth[i + 1] *= damp
                self.alt_smooth[i + 1] += self.alt_smooth[i] * (1 - damp)

        hue = 6.28 * (self.alt_smooth[n] % 1.0)

        leds.set_all_rgb(*colours.hsv_to_rgb(hue, 1, 1))
        leds.update()

        self.data_exists = True

    def on_enter(self, vm: Optional[ViewManager]) -> None:
        super().on_enter(vm)
        self.data_exists = False
        self.alt_smooth = None
        self.draw_background_request = 10


# For running with `mpremote run`:
if __name__ == "__main__":
    import st3m.run

    st3m.run.run_view(App(ApplicationContext()))
