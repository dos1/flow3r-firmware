# python
import math

# badge23
from st3m import event, application, ui
from st3m.system import hardware, audio
from synth import tinysynth

popcorn = [9, 7, 9, 5, 0, 5, -3, 999]


class AppSparabo(application.Application):
    def on_init(self):
        self.x = None
        self.y = None
        self.step = None

        audio.set_volume_dB(0)

        self.synth = tinysynth(440, 1)
        self.synth.decay(25)

        print("here")
        self.sequencer = event.Sequence(bpm=160, steps=8, action=self.on_step, loop=True)
        self.sequencer.start()
        if self.sequencer.repeat_event:
            self.add_event(self.sequencer.repeat_event)

    def on_step(self, data):
        synth = app.synth
        note = popcorn[data["step"]]
        if note != 999:
            synth.tone(note)
            synth.start()

        self.x, self.y = ui.xy_from_polar(90, -2 * math.pi / 8 * data["step"] + math.pi)
        self.step = data['step']

    def on_draw(self, ctx):
        x, y = self.x, self.y
        if x is None or y is None:
            return

        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE

        ctx.rgb(1, 1, 0).rectangle(-120, -120, 240, 240).fill()
        size = 180
        ctx.rgb(0.8, 0.8, 0)
        ctx.round_rectangle(x - size / 2, y - size / 2, size, size, size // 2).fill()
        ctx.move_to(x, y).rgb(0.5, 0.5, 0).text("{}".format(self.step))

    def on_foreground(self):
        self.sequencer.start()

    def on_exit(self):
        self.sequencer.stop()


app = AppSparabo("sequencer")
