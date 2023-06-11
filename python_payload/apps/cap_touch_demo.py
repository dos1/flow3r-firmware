import cmath
import math
import time

import hardware
from st3m import utils, application


class Dot:
    def __init__(self, size, imag, real):
        self.size = size
        self.imag = imag
        self.real = real

    def draw(self, i, ctx):
        imag = self.imag
        real = self.real
        size = self.szie

        col = (1.0, 0.0, 1.0)
        if i % 2:
            col = (0.0, 1.0, 1.0)
        ctx.rgb(*col).rectangle(
            -int(imag - (size / 2)), -int(real - (size / 2)), size, size
        ).fill()


class CapTouchDemo(application.Application):
    def on_init(self):
        self.dots = []

    def main_foreground(self):
        self.dots = []
        for i in range(10):
            size = (hardware.get_captouch(i) * 4) + 4
            size += int(
                max(
                    0,
                    sum([hardware.captouch_get_petal_pad(i, x) for x in range(0, 3 + 1)])
                    / 8000,
                )
            )
            x = 70 + (hardware.captouch_get_petal_rad(i) / 1000)
            x += (hardware.captouch_get_petal_phi(i) / 600) * 1j
            rot = cmath.exp(2j * math.pi * i / 10)
            x = x * rot

            self.dots.append(Dot(size, x.imag, x.real))

    def on_draw(self, ctx):
        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
        for i, dot in enumerate(self.dots):
            dot.draw(i, ctx)


app = CapTouchDemo("cap touch")