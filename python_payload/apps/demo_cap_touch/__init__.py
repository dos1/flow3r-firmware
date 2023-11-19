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
import captouch


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
            col = (0.0, 0.8, 0.8)
        ctx.rgb(*col).rectangle(
            -int(imag - (size / 2)), -int(real - (size / 2)), size, size
        ).fill()


class CapTouchDemo(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)
        self.dots: List[Dot] = []
        self.last_calib = None
        self.state = 0
        self.timer = 0
        self.button = None

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)
        if self.button is not None:
            press_event = (self.button != ins.buttons.app) and ins.buttons.app
        else:
            press_event = False
        self.button = int(ins.buttons.app)
        if press_event:
            print(self.button)
        if self.state == 0:
            if press_event:
                self.state = 1
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
        elif self.state == 1:
            if press_event:
                if self.button == ins.buttons.PRESSED_DOWN:
                    self.state = 2
                    self.timer = 2999
                else:
                    self.state = 0
        elif self.state == 2:
            self.timer -= delta_ms
            if self.timer < 0:
                self.state = 3
                captouch.calibration_request()
        elif self.state == 3:
            if not captouch.calibration_active():
                self.state = 0

    def draw(self, ctx: Context) -> None:
        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
        if self.state == 0:
            for i, dot in enumerate(self.dots):
                dot.draw(i, ctx)
        else:
            ctx.rgb(0, 0.8, 0.8)
            ctx.text_align = ctx.CENTER
            ctx.move_to(0, 0)
            ctx.font = ctx.get_font_name(0)
            ctx.font_size = 20
            pos = -55
            ctx.move_to(0, pos)
            ctx.text("calibration mode")
            pos += 30
            ctx.move_to(0, pos)
            ctx.text("do not touch the petals")
            pos += 20
            ctx.move_to(0, pos)
            ctx.text("from the front during")
            pos += 20
            ctx.move_to(0, pos)
            ctx.text("calibration")
            pos += 40
            ctx.move_to(0, pos)
            if self.state == 1:
                pos -= 10
                ctx.move_to(0, pos)
                ctx.text("down: start 3s countdown")
                pos += 20
                ctx.move_to(0, pos)
                ctx.text("left/right: canel")
            elif self.state == 2:
                ctx.text("calibrating in " + str(1 + int(self.timer / 1000)))
            elif self.state == 3:
                ctx.rgb(1.0, 0.5, 0.2)
                ctx.text("calibrating...")

    def on_enter(self, vm: Optional[ViewManager]) -> None:
        super().on_enter(vm)
        self.button = None


# For running with `mpremote run`:
if __name__ == "__main__":
    import st3m.run

    st3m.run.run_app(CapTouchDemo)
