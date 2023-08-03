from st4m import application, logging
from st4m.goose import List
from st4m.ui.ctx import Ctx
from st4m.input import InputState

log = logging.Log(__name__, level=logging.INFO)
log.info("import")

import cmath
import math
import time

import hardware


class Dot:
    def __init__(self, size: float, imag: float, real: float) -> None:
        self.size = size
        self.imag = imag
        self.real = real

    def draw(self, i: int, ctx: Ctx) -> None:
        imag = self.imag
        real = self.real
        size = self.size

        col = (1.0, 0.0, 1.0)
        if i % 2:
            col = (0.0, 1.0, 1.0)
        ctx.rgb(*col).rectangle(
            -int(imag - (size / 2)), -int(real - (size / 2)), size, size
        ).fill()


class CapTouchDemo(application.Application):
    def __init__(self, name: str) -> None:
        super().__init__(name)
        self.dots: List[Dot] = []
        self.last_calib = None
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
            (rad, phi) = ins.petal_pos[i]
            size = 4
            if ins.petal_pressed[i]:
                size += 4
            x = 70 + (rad / 1000) + 0j
            x += (phi / 600) * 1j
            rot = cmath.exp(2j * math.pi * i / 10)
            x = x * rot

            self.dots.append(Dot(size, x.imag, x.real))

    def draw(self, ctx: Ctx) -> None:
        # print(self.last_calib)

        # TODO (q3k) bug: we have to do this, otherwise we have horrible blinking
        ctx.rgb(1, 1, 1)

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


app = CapTouchDemo("cap touch")
