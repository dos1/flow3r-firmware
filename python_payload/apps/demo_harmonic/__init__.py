import captouch
import bl00mbox

blm = bl00mbox.Channel("Harmonic Demo")
import leds

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
        self.synths = [blm.new(bl00mbox.patches.tinysynth) for i in range(5)]
        self.cp_prev = captouch.read()

        for i, synth in enumerate(self.synths):
            synth.signals.decay = 500
            synth.signals.waveform = 0
            synth.signals.attack = 50
            synth.signals.volume = 0.3 * 32767
            synth.signals.sustain = 0.9 * 32767
            synth.signals.release = 800
            synth.signals.output = blm.mixer

        self._set_chord(3)
        self.prev_captouch = [0] * 10

    def _set_chord(self, i: int) -> None:
        hue = int(72 * (i + 0.5)) % 360
        if i != self.chord_index:
            self.chord_index = i
            leds.set_all_hsv(hue, 1, 0.2)
            leds.update()
            self.chord = chords[i]

    def draw(self, ctx: Context) -> None:
        i = self.color_intensity
        ctx.rgb(i, i, i).rectangle(-120, -120, 240, 240).fill()

        ctx.rgb(0, 0, 0)
        ctx.scope()
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
                    self.synths[k].signals.pitch.tone = self.chord[k]
                    self.synths[k].signals.trigger.start()
                    self.color_intensity = 1.0
            elif (not cts.petals[i].pressed) and self.cp_prev.petals[i].pressed:
                if (1 + i) % 2:
                    k = int(i / 2)
                    self.synths[k].signals.trigger.stop()
        self.cp_prev = cts
