from st3m import logging
from st3m.application import Application, ApplicationContext
from st3m.goose import List
from st3m.input import InputState
from ctx import Context

log = logging.Log(__name__, level=logging.INFO)
log.info("import")

import cmath
import math
import time


class Dot:
    def __init__(self, size: float, imag: float, real: float) -> None:
        self.size = size
        self.imag = imag
        self.real = real

    def draw(self, i: int, ctx: Context) -> None:
        imag = self.imag
        real = self.real
        size = self.size

        col = (1.0, 0.0, 1.0)
        if i % 2:
            col = (0.0, 1.0, 1.0)
        if i == 10:
            col = (1.0, 1.0, 1.0)
        ctx.rgb(*col).rectangle(
            -int(imag + (size / 2)), -int(real + (size / 2)), size, size
        ).fill()


class CapTouchDemo(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)
        self.dots: List[Dot] = []
        self.last_calib = None
        self._show_rotary_dial = True
        # self.ui_autocalib = ui.IconLabel("Autocalib done", size=30)

        # self.add_event(
        #    event.Event(
        #        name="captouch_autocalib",
        #        action=self.do_autocalib,
        #        condition=lambda data: data["type"] == "button"
        #        and data["index"] == 0
        #        and data["change"]
        #        and data["from"] == 2,
        #        enabled=True,
        #    )
        # )

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)
        self.dots = []
        for i in range(10):
            petal = ins.captouch.petals[i]
            (rad, phi) = petal.position
            size = 4
            if petal.pressed:
                size += 4
            x = 70 + (rad / 1000) + 0j
            x += ((-phi) / 600) * 1j
            rot = cmath.exp(-2j * math.pi * i / 10)
            x = x * rot

            self.dots.append(Dot(size, x.imag, x.real))

        if self._show_rotary_dial:
            angle = ins.captouch.rotary_dial.touch_angle_cw
            if angle is not None:
                x = 110 * cmath.exp(-1j * angle)
                self.dots.append(Dot(8, x.imag, x.real))
            else:
                self.dots.append(Dot(4, 0, 0))

        if ins.buttons.app.inward.press_event:
            self._show_rotary_dial = not self._show_rotary_dial

        if ins.buttons.app.middle.press_event:
            pass

    def draw(self, ctx: Context) -> None:
        # print(self.last_calib)

        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
        for i, dot in enumerate(self.dots):
            dot.draw(i, ctx)
        # if not self.last_calib is None and self.last_calib > 0:
        #    self.last_calib -= 1
        #    self.ui_autocalib.draw(ctx)

    # def do_autocalib(self, data) -> None:
    #    log.info("Performing captouch autocalibration")
    #    captouch.calibration_request()
    #    self.last_calib = 50


# For running with `mpremote run`:
if __name__ == "__main__":
    import st3m.run

    st3m.run.run_view(CapTouchDemo(ApplicationContext()))
