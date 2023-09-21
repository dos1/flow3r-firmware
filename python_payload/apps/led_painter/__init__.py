# python imports
import random
import time
import math
import json
import errno

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
        self.PETAL_POS = [
            tuple([(k - 120) * 1.1 + 120 for k in g]) for g in self.PETAL_POS
        ]

    def _try_load_settings(self, path):
        try:
            with open(path, "r") as f:
                return json.load(f)
        except OSError as e:
            if e.errno != errno.ENOENT:
                raise  # ignore file not found

    def _try_save_settings(self, path, settings):
        try:
            with open(path, "w+") as f:
                f.write(json.dumps(settings))
                f.close()
        except OSError as e:
            if e.errno != errno.ENOENT:
                raise  # ignore file not found

    def _load_settings(self):
        settings_path = "/flash/menu_leds.json"
        settings = self._try_load_settings(settings_path)
        if settings is None:
            return
        self.LEDS = settings["leds"]
        for i in range(40):
            col = settings["leds"][i]
            leds.set_rgb(i, col[0], col[1], col[2])

    def _save_settings(self):
        settings_path = "/flash/menu_leds.json"
        old_settings = self._try_load_settings(settings_path)
        file_is_different = False
        if old_settings is None:
            file_is_different = True
        else:
            try:
                ref_LEDS = old_settings["leds"]
                for l in range(40):
                    if file_is_different:
                        break
                    for c in range(3):
                        if self.LEDS[l][c] != ref_LEDS[l][c]:
                            file_is_different = True
            except:
                file_is_different = True

        if file_is_different:
            settings = {}
            settings["leds"] = self.LEDS
            self._try_save_settings(settings_path, settings)

    def on_enter(self, vm):
        super().on_enter(vm)
        self._load_settings()
        leds.set_slew_rate(max(leds.get_slew_rate(), 220))

    def on_exit(self):
        self._save_settings()

    def draw(self, ctx: Context) -> None:
        ctx.font = ctx.get_font_name(1)

        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()

        ctx.rgb(1, 1, 1).rectangle(-31, -31, 62, 62).fill()
        ctx.rgb(self.r / 255, self.g / 255, self.b / 255).rectangle(
            -30, -30, 60, 60
        ).fill()

        # if (self.r == 0) and (self.g == 0) and (self.b == 0):
        #    ctx.move_to(0, 0).rgb(255, 255, 255).text("BLACK")

        ctx.font_size = 16

        ctx.text_align = ctx.LEFT
        ctx.move_to(39, 5 - 17)
        ctx.rgb(1, 0, 0).text(str(self.r))
        ctx.move_to(39, 5)
        ctx.rgb(0, 1, 0).text(str(self.g))
        ctx.move_to(39, 5 + 17)
        ctx.rgb(0, 0, 1).text(str(self.b))

        ctx.rgb(0.7, 0.7, 0.7)
        text_shift = -20
        ctx.text_align = ctx.RIGHT
        ctx.move_to(text_shift - 5, -62).text("brush:")
        ctx.move_to(text_shift - 5, -42).text("<-/->:")

        ctx.text_align = ctx.LEFT
        if self._draw:
            self.LEDS[self._cursor] = [self.r, self.g, self.b]
            ctx.move_to(text_shift, -60).text("down")
            ctx.move_to(text_shift, -44).text("draw")
            for i in range(len(self.LEDS)):
                leds.set_rgb(i, self.LEDS[i][0], self.LEDS[i][1], self.LEDS[i][2])

        else:
            ctx.move_to(text_shift, -60).text("up")
            ctx.move_to(text_shift, -44).text("move")
            for i in range(len(self.LEDS)):
                leds.set_rgb(i, self.LEDS[i][0], self.LEDS[i][1], self.LEDS[i][2])
            leds.set_rgb(self._cursor, 255, 255, 255)
            if (self.r == 255) and (self.g == 255) and (self.b == 255):
                leds.set_rgb(self._cursor, 0, 0, 255)

        ctx.text_align = ctx.CENTER
        ctx.font_size = 20

        leds.update()
        off_x = 120
        off_y = 120
        ctx.move_to(self.PETAL_POS[0][0] - off_x, self.PETAL_POS[0][1] - off_y).rgb(
            1, 0, 0
        ).text("R+")
        ctx.move_to(self.PETAL_POS[1][0] - off_x, self.PETAL_POS[1][1] - off_y).rgb(
            1, 0, 0
        ).text("R-")
        ctx.move_to(self.PETAL_POS[2][0] - off_x, self.PETAL_POS[2][1] - off_y).rgb(
            0, 1, 0
        ).text("G+")
        ctx.move_to(self.PETAL_POS[3][0] - off_x, self.PETAL_POS[3][1] - off_y).rgb(
            0, 1, 0
        ).text("G-")
        ctx.move_to(self.PETAL_POS[4][0] - off_x, self.PETAL_POS[4][1] - off_y).rgb(
            0, 0, 1
        ).text("B+")
        ctx.move_to(self.PETAL_POS[5][0] - off_x, self.PETAL_POS[5][1] - off_y).rgb(
            0, 0, 1
        ).text("B-")

        ctx.font_size = 16
        pos_x = self.PETAL_POS[6][0] - off_x
        pos_y = self.PETAL_POS[6][1] - off_y
        ctx.move_to(pos_x, pos_y).rgb(255, 255, 255)
        ctx.rectangle(pos_x - 19, pos_y - 18, 38, 26).stroke()
        ctx.text("BLK")

        pos_x = self.PETAL_POS[7][0] - off_x
        pos_y = self.PETAL_POS[7][1] - off_y
        ctx.move_to(pos_x, pos_y).rgb(255, 255, 255)
        ctx.rectangle(pos_x - 19, pos_y - 18, 38, 26).fill()
        ctx.rgb(0, 0, 0)
        ctx.text("WHT")

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


# For running with `mpremote run`:
if __name__ == "__main__":
    import st3m.run

    st3m.run.run_app(LEDPainter)
