from st3m import logging

log = logging.Log(__name__, level=logging.INFO)
log.info("import")

import cmath
import math
import time

import hardware
from st4m import application


class Dot:
    def __init__(self, size, imag, real):
        self.size = size
        self.imag = imag
        self.real = real

    def draw(self, i, ctx):
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
    def __init__(self, name):
        super().__init__(name)
        self.dots = []
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
            size = (hardware.get_captouch(i) * 4) + 4
            size += int(
                max(
                    0,
                    sum(
                        [hardware.captouch_get_petal_pad(i, x) for x in range(0, 3 + 1)]
                    )
                    / 8000,
                )
            )
            x = 70 + (hardware.captouch_get_petal_rad(i) / 1000)
            x += (hardware.captouch_get_petal_phi(i) / 600) * 1j
            rot = cmath.exp(2j * math.pi * i / 10)
            x = x * rot

            self.dots.append(Dot(size, x.imag, x.real))

    def draw(self, ctx):
        # print(self.last_calib)

        # TODO (q3k) bug: we have to do this, otherwise we have horrible blinking
        ctx.rgb(1, 1, 1)

        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
        for i, dot in enumerate(self.dots):
            dot.draw(i, ctx)
        # if not self.last_calib is None and self.last_calib > 0:
        #    self.last_calib -= 1
        #    self.ui_autocalib.draw(ctx)

    def do_autocalib(self, data):
        log.info("Performing captouch autocalibration")
        hardware.captouch_autocalib()
        self.last_calib = 50


app = CapTouchDemo("cap touch")
