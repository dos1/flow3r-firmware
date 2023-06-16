from synth import tinysynth
from hardware import *
import leds


chords = [
    [-4, 0, 3, 8, 10],
    [-3, 0, 5, 7, 12],
    [-1, 2, 5, 7, 11],
    [0, 3, 7, 12, 14],
    [3, 7, 10, 14, 15],
]


from st3m.application import Application


class HarmonicApp(Application):
    def on_init(self):
        self.color_intensity = 0
        self.chord_index = None
        self.chord = None
        self.synths = [
            tinysynth(440, 1) for i in range(5)
        ]
        for synth in self.synths:
            synth.decay(100)
            synth.waveform(1)
        self._set_chord(3)

    def _set_chord(self, i):
        hue = int(72 * (i + 0.5)) % 360
        if i != self.chord_index:
            self.chord_index = i
            for j in range(40):
                leds.set_hsv(j, hue, 1, 0.2)
            self.chord = chords[i]
            leds.update()

    def on_foreground(self):
        pass

    def on_draw(self, ctx):
        i = self.color_intensity
        ctx.rgb(i, i, i).rectangle(-120, -120, 240, 240).fill()

        ctx.rgb(0, 0, 0)
        scope_draw(ctx)
        ctx.fill()

    def main_foreground(self):
        if self.color_intensity > 0:
            self.color_intensity -= self.color_intensity / 20
        for i in range(10):
            if get_captouch(i):
                if i % 2:
                    k = int((i - 1) / 2)
                    self._set_chord(k)
                else:
                    k = int(i / 2)
                    self.synths[k].tone(self.chord[k])
                    self.synths[k].start()
                    self.color_intensity = 1.0


app = HarmonicApp("harmonic")
