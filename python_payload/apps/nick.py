from st4m.application import Application
from st4m.property import PUSH_RED, GO_GREEN, BLACK
import leds


class NickApp(Application):
    def __init__(self, name):
        super().__init__(name)
        self._nick = self._read_nick_from_file()
        self._scale = 1
        self._dir = 1
        self._led = 0.0

    def draw(self, ctx):
        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE
        ctx.font_size = 75
        ctx.font = ctx.get_font_name(5)

        # TODO (q3k) bug: we have to do this, otherwise we have horrible blinking
        ctx.rgb(1, 1, 1)

        # TODO (q3k) bug: we have to do this two times, otherwise we have horrible blinking
        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
        ctx.rgb(*GO_GREEN)

        ctx.move_to(0, 0)
        ctx.save()
        ctx.scale(self._scale, 1)
        ctx.text(self._nick)
        ctx.restore()

        leds.set_hsv(int(self._led), abs(self._scale) * 360, 1, 0.2)

        leds.update()
        # ctx.fill()

    def on_enter(self):
        super().on_enter()

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        self._scale += delta_ms / 1000 * self._dir

        if abs(self._scale) > 1 and self._scale * self._dir > 0:
            self._dir *= -1

        self._led += delta_ms / 45
        if self._led >= 40:
            self._led = 0

    def _read_nick_from_file(self):
        return "iggy"


app = NickApp("nick")
