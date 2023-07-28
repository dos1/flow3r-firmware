from st3m import logging

log = logging.Log(__name__, level=logging.INFO)
log.info("import")

import cmath
import math
import time

import captouch

from st3m import utils, application, ui, event


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
    def on_init(self):
        self.dots = []
        self.last_calib = None
        self.ui_autocalib = ui.IconLabel("Autocalib done", size=30)

        self.add_event(
            event.Event(
                name="captouch_autocalib",
                action=self.do_autocalib,
                condition=lambda data: data["type"] == "button"
                and data["index"] == 0
                and data["change"]
                and data["from"] == 2,
                enabled=True,
            )
        )

    def main_foreground(self):
        self.dots = []
        cps = captouch.read()
        for i in range(10):
            petal = cps.petals[i]
            (rad, phi) = petal.position
            size = 4
            if petal.pressed:
                size += 4
            x = 70 + (rad / 1000)
            x += (phi / 600) * 1j
            rot = cmath.exp(2j * math.pi * i / 10)
            x = x * rot

            self.dots.append(Dot(size, x.imag, x.real))

    def on_draw(self, ctx):
        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
        for i, dot in enumerate(self.dots):
            dot.draw(i, ctx)
        if not self.last_calib is None and self.last_calib > 0:
            self.last_calib -= 1
            self.ui_autocalib.draw(ctx)

    def do_autocalib(self, data):
        log.info("Performing captouch autocalibration")
        captouch.calibration_request()
        self.last_calib = 50


app = CapTouchDemo("cap touch")
