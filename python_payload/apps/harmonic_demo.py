import bl00mbox
blm = bl00mbox.Channel()
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
        self.synths = [blm.new(bl00mbox.patches.tinysynth) for i in range(5)]
        self.synths += [blm.new(bl00mbox.patches.tinysynth_fm) for i in range(5)]
        for i, synth in enumerate(self.synths):
            synth.decay(100)
            synth.sustain(0.5)
            if i < 5:
                synth.waveform(-1)
                synth.volume(0.5)
                synth.release(1200)
            elif i < 10:
                synth.waveform(-1)
                synth.attack(300)
                synth.volume(0.1)
                synth.sustain(0.9)
                synth.release(800)
                synth.fm(2)
            else:
                synth.waveform(-1)
                synth.attack(500)
                synth.volume(0.03)
                synth.sustain(0.9)
        self._set_chord(3)
        self.prev_captouch = [0]*10

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
            cap = get_captouch(i)
            if cap != self.prev_captouch[i]:
                self.prev_captouch[i] = cap
                if cap:
                    if i % 2:
                        k = int((i - 1) / 2)
                        self._set_chord(k)
                    else:
                        k = int(i / 2)
                        self.synths[k].tone(self.chord[k])
                        if len(self.synths) >= 10:
                            self.synths[k+5].tone(12+self.chord[k])
                        if len(self.synths) >= 15:
                            self.synths[k+10].tone(7+self.chord[k])
                        self.synths[k].start()
                        if len(self.synths) >= 10:
                            self.synths[k+5].start()
                        if len(self.synths) >= 15:
                            self.synths[k+10].start()
                        self.color_intensity = 1.0
                else:
                    if (1 + i) % 2:
                        k = int(i / 2)
                        self.synths[k].stop()
                        if len(self.synths) >= 10:
                            self.synths[k + 5].stop()
                        if len(self.synths) >= 15:
                            self.synths[k + 10].stop()


app = HarmonicApp("harmonic")
