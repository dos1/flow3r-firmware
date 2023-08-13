# python imports
import random
import time
import math

# flow3r imports
from st3m.application import Application, ApplicationContext
from st3m.ui.colours import BLUE, WHITE
from st3m.goose import Optional, Tuple
from st3m.utils import xy_from_polar
from st3m.ui.view import ViewManager
from ctx import Context
from st3m.input import InputController, InputState
from st3m.utils import tau
from st3m import logging

# from st3m.ui.interactions import CapScrollController
import leds

# import st3m.run

log = logging.Log(__name__, level=logging.INFO)
log.info("hello led painter")


class LEDPainter(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)
        self.input = InputController()
        # self.scroll_R = CapScrollController()
        # self.scroll_G = CapScrollController()
        # self.scroll_B = CapScrollController()
        self._cursor = 0
        self._draw = True
        self.STEPS = 30
        self.r = 255
        self.g = 40
        self.b = 155
        self.PETAL_R_PLUS = 0
        self.PETAL_R_MINUS = 1
        self.PETAL_G_PLUS = 2
        self.PETAL_G_MINUS = 3
        self.PETAL_B_PLUS = 4
        self.PETAL_B_MINUS = 5
        self.PETAL_BLACK = 6
        self.PETAL_WHITE = 7
        self.LEDS = [[0, 0, 0] for i in range(40)]
        #  https://www.sr-research.com/circular-coordinate-calculator/
        self.PETAL_POS = [
            (120, 35),
            (170, 51),
            (201, 94),
            (201, 146),
            (170, 189),
            (120, 205),
            (70, 189),
            (39, 146),
            (39, 94),
            (70, 51),
        ]
        # self.PETAL_POS.reverse()

    def draw(self, ctx: Context) -> None:
        ctx.font = ctx.get_font_name(1)

        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()

        ctx.rgb(self.r / 255, self.g / 255, self.b / 255).rectangle(
            -30, -30, 60, 60
        ).fill()

        if (self.r == 0) and (self.g == 0) and (self.b == 0):
            ctx.font_size = 20
            ctx.move_to(-30, 0).rgb(255, 255, 255).text("BLACK")

        if self._draw:
            self.LEDS[self._cursor] = [self.r, self.g, self.b]
            ctx.font_size = 14
            ctx.move_to(-80, -40).rgb(255, 255, 255).text("(Center L) Brush Down")
            for i in range(len(self.LEDS)):
                leds.set_rgb(i, self.LEDS[i][0], self.LEDS[i][1], self.LEDS[i][2])

        else:
            ctx.font_size = 14
            ctx.move_to(-80, -40).rgb(255, 255, 255).text("(Center L) Brush Up")
            for i in range(len(self.LEDS)):
                leds.set_rgb(i, self.LEDS[i][0], self.LEDS[i][1], self.LEDS[i][2])
            leds.set_rgb(self._cursor, 255, 255, 255)
            if (self.r == 255) and (self.g == 255) and (self.b == 255):
                leds.set_rgb(self._cursor, 0, 0, 255)

        leds.update()
        off_x = 130
        off_y = 110
        ctx.font_size = 20
        ctx.move_to(self.PETAL_POS[0][0] - off_x, self.PETAL_POS[0][1] - off_y).rgb(
            255, 255, 255
        ).text("R+")
        ctx.move_to(self.PETAL_POS[1][0] - off_x, self.PETAL_POS[1][1] - off_y).rgb(
            255, 255, 255
        ).text("R-")
        ctx.move_to(self.PETAL_POS[2][0] - off_x, self.PETAL_POS[2][1] - off_y).rgb(
            255, 255, 255
        ).text("G+")
        ctx.move_to(self.PETAL_POS[3][0] - off_x, self.PETAL_POS[3][1] - off_y).rgb(
            255, 255, 255
        ).text("G-")
        ctx.move_to(self.PETAL_POS[4][0] - off_x, self.PETAL_POS[4][1] - off_y).rgb(
            255, 255, 255
        ).text("B+")
        ctx.move_to(self.PETAL_POS[5][0] - off_x, self.PETAL_POS[5][1] - off_y).rgb(
            255, 255, 255
        ).text("B-")
        ctx.move_to(self.PETAL_POS[6][0] - off_x, self.PETAL_POS[6][1] - off_y).rgb(
            255, 255, 255
        ).text("B")
        ctx.move_to(self.PETAL_POS[7][0] - off_x, self.PETAL_POS[7][1] - off_y).rgb(
            255, 255, 255
        ).text("W")

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)
        # self.scroll_R.update(self.input.captouch.petals[self.PETAL_R].gesture, delta_ms)
        # self.scroll_G.update(self.input.captouch.petals[self.PETAL_G].gesture, delta_ms)
        # self.scroll_B.update(self.input.captouch.petals[self.PETAL_B].gesture, delta_ms)

        if self.input.buttons.app.middle.pressed:
            self._draw = not self._draw

        if self.input.buttons.app.left.pressed:
            if self._cursor != 0:
                self._cursor -= 1
            else:
                self._cursor = 39
            # log.info(self._cursor)

        elif self.input.buttons.app.right.pressed:
            if self._cursor != 39:
                self._cursor += 1
            else:
                self._cursor = 0
            # log.info(self._cursor)

        if self.input.captouch.petals[self.PETAL_R_PLUS].whole.pressed:
            if self.r + self.STEPS <= 255:
                self.r += self.STEPS
            else:
                self.r = 255
            # log.info(self.r)

        if self.input.captouch.petals[self.PETAL_R_MINUS].whole.pressed:
            if self.r - self.STEPS >= 0:
                self.r -= self.STEPS
            else:
                self.r = 0
            # log.info(self.r)

        if self.input.captouch.petals[self.PETAL_G_PLUS].whole.pressed:
            if self.g + self.STEPS <= 255:
                self.g += self.STEPS
            else:
                self.g = 255
            # log.info(self.g)

        if self.input.captouch.petals[self.PETAL_G_MINUS].whole.pressed:
            if self.g - self.STEPS >= 0:
                self.g -= self.STEPS
            else:
                self.g = 0
            # log.info(self.g)

        if self.input.captouch.petals[self.PETAL_B_PLUS].whole.pressed:
            if self.b + self.STEPS <= 255:
                self.b += self.STEPS
            else:
                self.b = 255
            # log.info(self.b)

        if self.input.captouch.petals[self.PETAL_B_MINUS].whole.pressed:
            if self.b - self.STEPS >= 0:
                self.b -= self.STEPS
            else:
                self.b = 0
            # log.info(self.b)

        if self.input.captouch.petals[self.PETAL_BLACK].whole.pressed:
            self.r = 0
            self.g = 0
            self.b = 0

        if self.input.captouch.petals[self.PETAL_WHITE].whole.pressed:
            self.r = 255
            self.g = 255
            self.b = 255


# if __name__ == '__main__':
#     # Continue to make runnable via mpremote run.
#     st3m.run.run_view(LEDPainter(ApplicationContext()))
