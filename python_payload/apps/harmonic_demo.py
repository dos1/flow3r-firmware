from bl00mbox import tinysynth
import captouch
import leds
import hardware

from st4m.ui.ctx import Ctx
from st4m.goose import List
from st4m.input import InputState

chords = [
    [-4, 0, 3, 8, 10],
    [-3, 0, 5, 7, 12],
    [-1, 2, 5, 7, 11],
    [0, 3, 7, 12, 14],
    [3, 7, 10, 14, 15],
]


from st4m.application import Application


class HarmonicApp(Application):
    def __init__(self, name: str) -> None:
        super().__init__(name)

        self.color_intensity = 0.0
        self.chord_index = 0
        self.chord: List[int] = []
        self.synths = [tinysynth(440) for i in range(15)]
        for i, synth in enumerate(self.synths):
            synth.decay_ms(100)
            synth.sustain(0.5)
            if i < 5:
                synth.waveform(1)
                synth.volume(0.5)
                synth.release_ms(1200)
            elif i < 10:
                synth.waveform(1)
                synth.attack_ms(300)
                synth.volume(0.1)
                synth.sustain(0.9)
                synth.release_ms(2400)
            else:
                synth.waveform(1)
                synth.attack_ms(500)
                synth.volume(0.03)
                synth.sustain(0.9)
                synth.release_ms(800)
        self._set_chord(3)

    def _set_chord(self, i: int) -> None:
        hue = int(72 * (i + 0.5)) % 360
        if i != self.chord_index:
            self.chord_index = i
            for j in range(40):
                leds.set_hsv(j, hue, 1, 0.2)
            self.chord = chords[i]
            leds.update()

    def draw(self, ctx: Ctx) -> None:
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
            if cts.petals[i].pressed:
                if i % 2:
                    k = int((i - 1) / 2)
                    self._set_chord(k)
                else:
                    k = int(i / 2)
                    self.synths[k].tone(self.chord[k])
                    self.synths[k + 5].tone(12 + self.chord[k])
                    self.synths[k + 10].tone(7 + self.chord[k])
                    self.synths[k].start()
                    self.synths[k + 5].start()
                    self.synths[k + 10].start()
                    self.color_intensity = 1.0
            else:
                if (1 + i) % 2:
                    k = int(i / 2)
                    self.synths[k].stop()
                    self.synths[k + 5].stop()
                    self.synths[k + 10].stop()


app = HarmonicApp("harmonic")
