import captouch
import bl00mbox

import leds

from st3m.goose import List
from st3m.input import InputState
from st3m.goose import Optional
from st3m.ui.view import ViewManager
from ctx import Context
import cmath
import math

tai = math.tau * 1j

chords = [
    [[-4, 0, 3, 8, 10], "Fj9"],
    [[-3, 0, 5, 7, 12], "D9"],
    [[-1, 2, 5, 7, 11], "E7"],
    [[0, 3, 7, 12, 14], "A-9"],
    [[3, 7, 10, 14, 15], "Cj7"],
]


from st3m.application import Application, ApplicationContext


class HarmonicApp(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)

        self.chord_index = 0
        self.chord = None
        self.cp_prev = captouch.read()
        self.blm = None
        self._set_chord(3)
        self.prev_captouch = [0] * 10
        self.fade = [0] * 5

    def _build_synth(self):
        if self.blm is None:
            self.blm = bl00mbox.Channel("harmonic demo")
        self.synths = [self.blm.new(bl00mbox.patches.tinysynth) for i in range(5)]
        for i, synth in enumerate(self.synths):
            synth.signals.decay = 500
            synth.signals.waveform = 0
            synth.signals.attack = 50
            synth.signals.volume = 0.3 * 32767
            synth.signals.sustain = 0.9 * 32767
            synth.signals.release = 800
            synth.signals.output = self.blm.mixer

    def _set_chord(self, i: int) -> None:
        hue = int(72 * (i + 0.5)) % 360
        if i != self.chord_index:
            self.chord_index = i
            leds.set_all_hsv(hue, 1, 0.2)
            leds.update()
            self.chord = chords[i]

    def draw(self, ctx: Context) -> None:
        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
        ctx.line_width = 4
        ctx.get_font_name(4)
        ctx.text_align = ctx.CENTER

        ctx.font_size = 20
        for top_petal in range(5):
            note_name = self.tone_to_note_name(self.chord[0][top_petal])
            note_name = "".join([x for x in note_name if not x.isdigit()])
            pos = 95 * cmath.exp(tai * (top_petal - 2.5) / 5)
            end_pos = pos * (1 - 0.3j) * 1.27
            start_pos = pos * (1 + 0.3j) * 1.27
            mid_pos = (start_pos + end_pos) / 8
            fade = self.fade[top_petal]
            if fade > 0:
                ctx.rgb(0, 0, fade)
                ctx.move_to(start_pos.imag, start_pos.real)
                ctx.quad_to(mid_pos.imag, mid_pos.real, end_pos.imag, end_pos.real)
                ctx.fill()
            ctx.rgb(1, 0.5, 0)
            ctx.move_to(start_pos.imag, start_pos.real)
            ctx.quad_to(mid_pos.imag, mid_pos.real, end_pos.imag, end_pos.real)
            ctx.stroke()
            ctx.move_to(pos.imag * 1.07, pos.real * 1.07)
            ctx.rgb(1, 0.5, 0)
            ctx.text(note_name)

        ctx.font_size = 25
        ctx.rgb(0, 0.5, 1)
        for bottom_petal in range(5):
            pos = 90 * cmath.exp(tai * (bottom_petal - 2) / 5)
            if bottom_petal == self.chord_index:
                pos *= 0.5
                ctx.rgb(0, 0.8, 1)
                ctx.move_to(pos.imag, pos.real)
                ctx.font_size = 35
                ctx.text(chords[bottom_petal][1])
                ctx.font_size = 25
                ctx.rgb(0, 0.5, 1)
            else:
                ctx.move_to(pos.imag, pos.real)
                ctx.text(chords[bottom_petal][1])

    def tone_to_note_name(self, tone):
        # TODO: add this to radspa helpers
        sct = tone * 200 + 18367
        return bl00mbox.helpers.sct_to_note_name(sct)

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)
        cp = captouch.read()

        for i in range(10):
            j = (10 - i) % 10
            if cp.petals[j].pressed:
                if not self.cp_prev.petals[j].pressed:
                    if i % 2:
                        self._set_chord(i // 2)
                    else:
                        self.synths[i // 2].signals.pitch.tone = self.chord[0][i // 2]
                        self.synths[i // 2].signals.trigger.start()
                        self.fade[i // 2] = 1
            elif not i % 2:
                if self.fade[i // 2] > 0:
                    self.fade[i // 2] -= self.fade[i // 2] * float(delta_ms) / 1000
                    if self.fade[i // 2] < 0.05:
                        self.fade[i // 2] = 0
                if self.cp_prev.petals[j].pressed:
                    self.synths[i // 2].signals.trigger.stop()
        self.cp_prev = cp

    def on_enter(self, vm: Optional[ViewManager]) -> None:
        super().on_enter(vm)
        if self.blm is None:
            self._build_synth()
        self.blm.foreground = True

    def on_exit(self):
        if self.blm is not None:
            self.blm.free = True
        self.blm = None


# For running with `mpremote run`:
if __name__ == "__main__":
    import st3m.run

    st3m.run.run_view(HarmonicApp(ApplicationContext()))
