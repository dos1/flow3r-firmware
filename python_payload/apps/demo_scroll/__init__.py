# python imports
import random
import time
import math

# flow3r imports
from st3m import InputState
from st3m.application import Application, ApplicationContext
from st3m.ui.colours import BLUE, WHITE
from st3m.goose import Optional
from st3m.utils import xy_from_polar, tau
from st3m.ui.interactions import CapScrollController
from ctx import Context


class ScrollDemo(Application):
    PETAL_NO = 2

    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)
        self.scroll = CapScrollController()

    def draw(self, ctx: Context) -> None:
        ctx.save()
        ctx.move_to(0, 0)
        ctx.gray(0)
        ctx.rectangle(-120, -120, 240, 240)
        ctx.fill()

        ctx.font = "Material Icons"
        ctx.font_size = 20
        ctx.text_baseline = ctx.MIDDLE
        ctx.text_align = ctx.CENTER

        ctx.rotate((self.PETAL_NO * tau / 10))
        ctx.translate(0, -90)
        ctx.gray(1)
        ctx.text("\ue5c4\ue5c8")
        ctx.restore()

        ctx.save()
        ctx.move_to(0, 0)
        _, y = self.scroll.position
        y = -y

        ctx.gray(0.1)
        ctx.rectangle(-30, -120, 60, 240)
        ctx.fill()

        nrects = 10
        for i in range(nrects):
            ypos = y + i * 240 / nrects

            ypos += 120
            ypos %= 240
            ypos -= 120

            if i == 0:
                ctx.gray(1)
            elif i == nrects / 2:
                ctx.gray(0.2)
            elif i < nrects / 2:
                ctx.gray(1 - (i / nrects * 2))
            else:
                ctx.gray(i / nrects * 2)
            ctx.rectangle(-20, -5 + ypos, 40, 10)
            ctx.fill()
        ctx.restore()

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)
        self.scroll.update(self.input.captouch.petals[self.PETAL_NO].gesture, delta_ms)
