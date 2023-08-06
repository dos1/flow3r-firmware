import bl00mbox
import hardware
import captouch
import leds

from st3m.application import Application
from st3m.ui.ctx import Ctx
from st3m.input import InputState


class Dot:
    def __init__(self, sizex, sizey, imag, real, col):
        self.sizex = sizex
        self.sizey = sizey
        self.imag = imag
        self.real = real
        self.col = col

    def draw(self, i, ctx):
        imag = self.imag
        real = self.real
        sizex = self.sizex
        sizey = self.sizey
        col = self.col

        ctx.rgb(*col).rectangle(
            -int(imag + (sizex / 2)), -int(real + (sizey / 2)), sizex, sizey
        ).fill()


class SimpleDrums(Application):
    def __init__(self, name: str) -> None:
        super().__init__(name)
        # ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
        self.blm = bl00mbox.Channel()
        self.seq = self.blm.new(bl00mbox.patches.step_sequencer)
        self.hat = self.blm.new(bl00mbox.patches.sampler, "hihat.wav")
        # Dot(10, 10, -30, 0, self._track_col(0)).draw(0,ctx)
        self.kick = self.blm.new(bl00mbox.patches.sampler, "kick.wav")
        # Dot(20, 20, 0, 40, self._track_col(1)).draw(0,ctx)
        self.snare = self.blm.new(bl00mbox.patches.sampler, "snare.wav")
        # Dot(30, 30, 2, -20, self._track_col(2)).draw(0,ctx)
        self.kick.sampler.signals.trigger = self.seq.seqs[0].signals.output
        self.hat.sampler.signals.trigger = self.seq.seqs[1].signals.output
        self.snare.sampler.signals.trigger = self.seq.seqs[2].signals.output
        self.ct_prev = captouch.read()
        self.track = 0
        self.seq.bpm = 80
        self.blm.background_mute_override = True

    def _highlight_petal(self, num, r, g, b):
        for i in range(5):
            leds.set_rgb((4 * num - i + 2) % 40, r, g, b)

    def on_foreground(self):
        pass

    def _track_col(self, track, smol=False):
        rgb = (20, 20, 20)
        if track == 0:
            rgb = (0, 255, 0)
        elif track == 1:
            rgb = (0, 0, 255)
        elif track == 2:
            rgb = (255, 0, 0)
        if smol:
            rgb = [x / 256 for x in rgb]
        return rgb

    def draw(self, ctx):
        dots = []
        groupgap = 4
        for i in range(4):
            if self.ct_prev.petals[4 - i].pressed:
                dots.append(
                    Dot(
                        48 + groupgap,
                        40,
                        int((12 * 4 + groupgap) * (1.5 - i)),
                        0,
                        (0.1, 0.1, 0.1),
                    )
                )

        for track in range(3):
            rgb = self._track_col(track, smol=True)
            y = 12 * (track - 1)
            for i in range(16):
                trigger_state = self.seq.trigger_state(track, i)
                size = 2
                if trigger_state:
                    size = 8
                x = 12 * (7.5 - i)
                x += groupgap * (1.5 - (i // 4))
                x = int(x)
                dots.append(Dot(size, size, x, y, rgb))

        dots.append(Dot(1, 40, 0, 0, (0.5, 0.5, 0.5)))
        dots.append(Dot(1, 40, 4 * 12 + groupgap, 0, (0.5, 0.5, 0.5)))
        dots.append(Dot(1, 40, -4 * 12 - groupgap, 0, (0.5, 0.5, 0.5)))

        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
        for i, dot in enumerate(dots):
            dot.draw(i, ctx)
        return

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)
        st = self.seq.seqs[0].signals.step.value
        leds.set_all_rgb(0, 0, 0)
        rgb = self._track_col(self.track)
        self._highlight_petal(4 - (st // 4), *rgb)
        self._highlight_petal(6 + (st % 4), *rgb)
        leds.update()
        ct = captouch.read()
        for i in range(4):
            if ct.petals[4 - i].pressed:
                for j in range(4):
                    if ct.petals[6 + j].pressed and not (
                        self.ct_prev.petals[6 + j].pressed
                    ):
                        self.seq.trigger_toggle(self.track, i * 4 + j)
        if ct.petals[5].pressed and not (self.ct_prev.petals[5].pressed):
            self.track = (self.track + 1) % 3
        if ct.petals[0].pressed and not (self.ct_prev.petals[0].pressed):
            self.track = (self.track + 1) % 3
        self.ct_prev = ct
