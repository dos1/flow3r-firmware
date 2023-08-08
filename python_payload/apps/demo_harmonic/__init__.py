import captouch
import bl00mbox

blm = bl00mbox.Channel()
import leds
import hardware

from st3m.goose import List
from st3m.input import InputState
from ctx import Context

chords = [
    [-4, 0, 3, 8, 10],
    [-3, 0, 5, 7, 12],
    [-1, 2, 5, 7, 11],
    [0, 3, 7, 12, 14],
    [3, 7, 10, 14, 15],
]

from st3m.application import Application, ApplicationContext


class HarmonicApp(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)

        self.color_intensity = 0.0
        self.chord_index = 0
        self.chord: List[int] = []
        self.synths = [blm.new(bl00mbox.patches.tinysynth_fm) for i in range(5)]
        self.cp_prev = captouch.read()

        for i, synth in enumerate(self.synths):
            synth.decay(500)
            synth.waveform(-32767)
            synth.attack(50)
            synth.volume(0.3)
            synth.sustain(0.9)
            synth.release(800)
            synth.fm_waveform(-32767)
            synth.fm(1.5)

        self._set_chord(3)
        self.prev_captouch = [0] * 10

    def _set_chord(self, i: int) -> None:
        hue = int(72 * (i + 0.5)) % 360
        if i != self.chord_index:
            self.chord_index = i
            for j in range(40):
                leds.set_hsv(j, hue, 1, 0.2)
            self.chord = chords[i]
            leds.update()

    def draw(self, ctx: Context) -> None:
        i = self.color_intensity
        ctx.rgb(i, i, i).rectangle(-120, -120, 240, 240).fill()

        ctx.rgb(0, 0, 0)
        hardware.scope_draw(ctx)
        ctx.fill()

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)
        if self.color_intensity > 0:
            self.color_intensity -= self.color_intensity / 20
        cts = captouch.read()
        for i in range(10):
            if cts.petals[i].pressed and (not self.cp_prev.petals[i].pressed):
                if i % 2:
                    k = int((i - 1) / 2)
                    self._set_chord(k)
                else:
                    k = int(i / 2)
                    self.synths[k].tone(self.chord[k])
                    self.synths[k].start()
                    self.color_intensity = 1.0
            elif (not cts.petals[i].pressed) and self.cp_prev.petals[i].pressed:
                if (1 + i) % 2:
                    k = int(i / 2)
                    self.synths[k].stop()
        self.cp_prev = cts
