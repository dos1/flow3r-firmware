import bl00mbox
import random
import time
import math
import leds

from st3m import InputState, Responder
from st3m.application import Application, ApplicationContext
from st3m.ui.colours import BLUE, WHITE
from st3m.goose import Optional
from st3m.utils import xy_from_polar, tau
from st3m.ui.view import ViewManager
from st3m.ui.interactions import CapScrollController
from ctx import Context


class Blob(Responder):
    def __init__(self) -> None:
        self._yell = 0.0
        self._blink = False
        self._blinking: Optional[int] = None

    def think(self, ins: InputState, delta_ms: int) -> None:
        if self._blinking is None:
            if random.random() > 0.99:
                self._blinking = 100
        else:
            self._blinking -= delta_ms
            if self._blinking < 0:
                self._blinking = None

    def draw(self, ctx: Context) -> None:
        blink = self._blinking is not None
        v = self._yell
        if v > 1.0:
            v = 1.0
        if v < 0:
            v = 0

        v /= 1.5
        if v < 0.1:
            v = 0.1

        ctx.rgb(62 / 255, 159 / 255, 229 / 255)

        ctx.save()
        ctx.rotate(-v)
        ctx.arc(0, 0, 80, tau, tau / 2, 1)
        ctx.fill()

        ctx.gray(60 / 255)
        if blink:
            ctx.line_width = 1
            ctx.move_to(50, -30)
            ctx.line_to(70, -30)
            ctx.stroke()
            ctx.move_to(30, -20)
            ctx.line_to(50, -20)
            ctx.stroke()
        else:
            ctx.arc(60, -30, 10, 0, tau, 0)
            ctx.arc(40, -20, 10, 0, tau, 0)
            ctx.fill()
        ctx.restore()

        ctx.arc(0, 0, 80, v / 2, tau / 2 + v / 2, 0)
        ctx.fill()

        ctx.rectangle(-80, 0, 20, -120)
        ctx.fill()


class Otamatone(Application):
    """
    A friendly lil' guy that is not annoying at all.
    """

    PETAL_NO = 7

    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)
        self._ts = 0
        self._blob = Blob()

        self._blm = bl00mbox.Channel("Otamatone")

        # Sawtooth oscillator
        self._osc = self._blm.new(bl00mbox.patches.tinysynth)
        # Distortion plugin used as a LUT to convert sawtooth into custom square
        # wave.
        self._dist = self._blm.new(bl00mbox.plugins._distortion)
        # Lowpass.
        self._lp = self._blm.new(bl00mbox.plugins.lowpass)

        # Wire sawtooth -> distortion -> lowpass
        self._osc.signals.output = self._dist.signals.input
        self._dist.signals.output = self._lp.signals.input
        self._lp.signals.output = self._blm.mixer

        # Closest thing to a waveform number for 'saw'.
        self._osc.signals.waveform = 20000
        self._osc.signals.attack = 1
        self._osc.signals.decay = 0

        # Built custom square wave (low duty cycle)
        table_len = 129
        self._dist.table = [
            32767 if i > (0.1 * table_len) else -32768 for i in range(table_len)
        ]

        self._lp.signals.freq = 4000

        self._intensity = 0.0

        self.input.captouch.petals[self.PETAL_NO].whole.repeat_disable()

    def on_enter(self, vm: Optional[ViewManager]) -> None:
        super().on_enter(vm)
        for i in range(26, 30 + 1):
            leds.set_rgb(i, 62, 159, 229)
        leds.update()

    def draw(self, ctx: Context) -> None:
        ctx.save()
        ctx.move_to(0, 0)
        ctx.gray(0)
        ctx.rectangle(-120, -120, 240, 240)
        ctx.fill()

        self._blob.draw(ctx)

        ctx.restore()

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)
        self._ts += delta_ms
        self._blob.think(ins, delta_ms)

        petal = self.input.captouch.petals[self.PETAL_NO]
        pos = ins.captouch.petals[self.PETAL_NO].position
        ctrl = pos[0] / 40000
        if ctrl < -1:
            ctrl = -1
        if ctrl > 1:
            ctrl = 1
        ctrl *= -1

        if petal.whole.down:
            if self._intensity < 1.0:
                self._intensity += 0.1 * (delta_ms / 20)
            self._osc.signals.pitch.tone = ctrl * 12
            self._osc.signals.trigger.start()

        if petal.whole.up:
            self._intensity = 0
            self._osc.signals.trigger.stop()

        self._blob._yell = self._intensity * 0.8 + (ctrl + 1) * 0.1


if __name__ == "__main__":
    from st3m.run import run_view

    run_view(Otamatone(ApplicationContext()))
