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
        self._wah = 0.0
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
        v *= 0.66 + 0.33 * self._wah
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
    WAH_PETAL_NO = 3

    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)
        self._ts = 0
        self._blob = Blob()
        self._blm = None
        self._intensity = 0.0
        self._formants = [
            [250, 595, 595],
            [360, 640, 640],
            [310, 870, 2250],
            [450, 1030, 2380],
            [550, 869, 2540],
            [710, 1100, 2540],
            [690, 1660, 2490],
            [550, 1770, 2490],
            [400, 1920, 2560],
            [280, 2250, 2890],
        ]

    def _build_synth(self):
        if self._blm is None:
            self._blm = bl00mbox.Channel("Otamatone")

        self._osc = self._blm.new(bl00mbox.plugins.osc)
        self._env = self._blm.new(bl00mbox.plugins.env_adsr)
        self._bp = self._blm.new(bl00mbox.plugins.filter)
        self._bp2 = self._blm.new(bl00mbox.plugins.filter)
        self._bp3 = self._blm.new(bl00mbox.plugins.filter)
        self._mixer = self._blm.new(bl00mbox.plugins.mixer, 4)

        self._osc.signals.output = self._env.signals.input
        self._bp.signals.input = self._env.signals.output
        self._bp2.signals.input = self._env.signals.output
        self._bp3.signals.input = self._env.signals.output

        self._bp.signals.output = self._mixer.signals.input[0]
        self._bp2.signals.output = self._mixer.signals.input[1]
        self._bp3.signals.output = self._mixer.signals.input[2]
        self._env.signals.output = self._mixer.signals.input[3]
        self._mixer.signals.input_gain[0].dB = 0
        self._mixer.signals.input_gain[1].dB = 0
        self._mixer.signals.input_gain[2].dB = -6
        self._mixer.signals.input_gain[3].dB = -15
        self._mixer.signals.output = self._blm.mixer

        self._osc.signals.waveform = (
            self._osc.signals.waveform.switch.SQUARE * 3
            + self._osc.signals.waveform.switch.SAW
        ) // 4
        self._osc.signals.morph = 27000
        self._env.signals.attack = 1
        self._env.signals.decay = 0
        self._env.signals.sustain = 32767
        self._bp.signals.mode.switch.BANDPASS = True
        self._bp2.signals.mode.switch.BANDPASS = True
        self._bp3.signals.mode.switch.BANDPASS = True
        self._bp.signals.reso = 24000
        self._bp2.signals.reso = 24000
        self._bp3.signals.reso = 24000
        self._set_wah(0.8)

    def _set_wah(self, wah_ctrl):
        lerp = (wah_ctrl + 1) / 2 * (len(self._formants) - 1)
        index = int(lerp)
        lerp = lerp - index
        if index == (len(self._formants) - 1):
            self._bp.signals.cutoff.freq = self._formants[index][0]
            self._bp2.signals.cutoff.freq = self._formants[index][1]
            self._bp3.signals.cutoff.freq = self._formants[index][2]
        else:
            self._bp.signals.cutoff.freq = (1 - lerp) * self._formants[index][
                0
            ] + lerp * self._formants[index + 1][0]
            self._bp2.signals.cutoff.freq = (1 - lerp) * self._formants[index][
                1
            ] + lerp * self._formants[index + 1][1]
            self._bp3.signals.cutoff.freq = (1 - lerp) * self._formants[index][
                2
            ] + lerp * self._formants[index + 1][2]
        self._wah = wah_ctrl

    def on_exit(self):
        if self._blm is not None:
            self._blm.clear()
            self._blm.free = True
        self._blm = None
        self._intensity = 0

    def on_enter(self, vm: Optional[ViewManager]) -> None:
        super().on_enter(vm)
        for i in range(40):
            leds.set_rgb(i, 0, 0, 0)
        for i in range(26, 30 + 1):
            leds.set_rgb(i, 62, 159, 229)
        for i in range(10, 14 + 1):
            leds.set_rgb(i, 62, 229, 159)
        leds.update()
        if self._blm is None:
            self._build_synth()
        self._blm.foreground = True

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

        wah_petal = self.input.captouch.petals[self.WAH_PETAL_NO]
        wah_pos = ins.captouch.petals[self.WAH_PETAL_NO].position
        wah_ctrl = wah_pos[0] / 40000 - 0.2
        if wah_ctrl < -1:
            wah_ctrl = -1
        if wah_ctrl > 1:
            wah_ctrl = 1
        wah_ctrl *= -1

        if petal.whole.pressed:
            self._env.signals.trigger.start()

        if petal.whole.down or petal.whole.pressed:
            if self._intensity < 1.0:
                self._intensity += 0.1 * (delta_ms / 20)
            self._osc.signals.pitch.tone = (ctrl * 15) - 3

        if petal.whole.released:
            self._intensity = 0
            self._env.signals.trigger.stop()

        if wah_petal.whole.down:
            self._set_wah(wah_ctrl)

        self._blob._yell = self._intensity * 0.8 + (ctrl + 1) * 0.1
        self._blob._wah = self._blob._wah * 0.8 + self._wah * 0.2


if __name__ == "__main__":
    from st3m.run import run_app

    run_app(Otamatone)
