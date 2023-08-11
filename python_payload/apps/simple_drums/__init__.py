import bl00mbox
import hardware
import captouch
import leds

from st3m.application import Application, ApplicationContext
from st3m.input import InputState
from st3m.goose import Tuple
from ctx import Context


class Dot:
    def __init__(
        self,
        sizex: float,
        sizey: float,
        imag: float,
        real: float,
        col: Tuple[float, float, float],
    ) -> None:
        self.sizex = sizex
        self.sizey = sizey
        self.imag = imag
        self.real = real
        self.col = col

    def draw(self, i: int, ctx: Context) -> None:
        imag = self.imag
        real = self.real
        sizex = self.sizex
        sizey = self.sizey
        col = self.col

        ctx.rgb(*col).rectangle(
            -int(imag + (sizex / 2)), -int(real + (sizey / 2)), sizex, sizey
        ).fill()


class SimpleDrums(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)
        # ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
        self.blm = bl00mbox.Channel("simple drums")
        self.seq = self.blm.new(bl00mbox.patches.step_sequencer)
        self.hat = self.blm.new(bl00mbox.patches.sampler, "hihat.wav")
        # Dot(10, 10, -30, 0, self._track_col(0)).draw(0,ctx)
        self.kick = self.blm.new(bl00mbox.patches.sampler, "kick.wav")
        # Dot(20, 20, 0, 40, self._track_col(1)).draw(0,ctx)
        self.snare = self.blm.new(bl00mbox.patches.sampler, "snare.wav")
        # Dot(30, 30, 2, -20, self._track_col(2)).draw(0,ctx)
        self.kick.signals.output = self.blm.mixer
        self.snare.signals.output = self.blm.mixer
        self.hat.signals.output = self.blm.mixer
        self.kick.signals.trigger = self.seq.plugins.sequencer0.signals.output
        self.hat.signals.trigger = self.seq.plugins.sequencer1.signals.output
        self.snare.signals.trigger = self.seq.plugins.sequencer2.signals.output
        self.seq.signals.bpm.value = 80

        self.track_names = ["kick", "hihat", "snare"]
        self.track = 0
        self.blm.background_mute_override = True
        self.tap_tempo_press_counter = 0
        self.delta_acc = 0
        self.stopped = False
        self.bpm = self.seq.signals.bpm.value

        # True if a given group should be highlighted, when a corresponding
        # petal is pressed.
        self._group_highlight = [False for _ in range(4)]

        # Disable repeat functionality as we want to detect long holds.
        for i in range(10):
            self.input.captouch.petals[i].whole.repeat_disable()

    def _highlight_petal(self, num: int, r: int, g: int, b: int) -> None:
        for i in range(5):
            leds.set_rgb((4 * num - i + 2) % 40, r, g, b)

    def _track_col(self, track: int) -> Tuple[int, int, int]:
        rgb = (20, 20, 20)
        if track == 0:
            rgb = (0, 255, 0)
        elif track == 1:
            rgb = (0, 0, 255)
        elif track == 2:
            rgb = (255, 0, 0)
        return rgb

    def draw(self, ctx: Context) -> None:
        dots = []
        groupgap = 4
        for i in range(4):
            if self._group_highlight[i]:
                dots.append(
                    Dot(
                        48 + groupgap,
                        40,
                        int((12 * 4 + groupgap) * (1.5 - i)),
                        0,
                        (0.15, 0.15, 0.15),
                    )
                )
        st = self.seq.signals.step.value

        for track in range(3):
            rgb = self._track_col(track)
            rgbf = (rgb[0] / 256, rgb[1] / 256, rgb[2] / 256)
            y = 12 * (track - 1)
            for i in range(16):
                trigger_state = self.seq.trigger_state(track, i)
                size = 2
                if trigger_state:
                    size = 8
                x = 12 * (7.5 - i)
                x += groupgap * (1.5 - (i // 4))
                x = int(x)
                dots.append(Dot(size, size, x, y, rgbf))
                if (i == st) and (track == 0):
                    dots.append(Dot(size / 2, size / 2, x, 24, (1, 1, 1)))

        dots.append(Dot(1, 40, 0, 0, (0.5, 0.5, 0.5)))
        dots.append(Dot(1, 40, 4 * 12 + groupgap, 0, (0.5, 0.5, 0.5)))
        dots.append(Dot(1, 40, -4 * 12 - groupgap, 0, (0.5, 0.5, 0.5)))

        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
        for i, dot in enumerate(dots):
            dot.draw(i, ctx)

        ctx.font = ctx.get_font_name(4)
        ctx.font_size = 30

        ctx.move_to(0, 65)
        col = [x / 255 for x in self._track_col(self.track)]
        ctx.rgb(*col)
        ctx.text(self.track_names[self.track])

        ctx.font_size = 18

        ctx.move_to(0, 102)
        next_track = (self.track + 1) % 3
        col = [x / 255 for x in self._track_col(next_track)]
        ctx.rgb(*col)
        ctx.text(self.track_names[next_track])

        ctx.font = ctx.get_font_name(1)
        ctx.font_size = 20

        ctx.move_to(0, -65)
        ctx.rgb(255, 255, 255)
        ctx.text(str(self.seq.signals.bpm) + " bpm")

        ctx.font_size = 15

        ctx.move_to(0, -85)

        ctx.rgb(0.6, 0.6, 0.6)
        ctx.text("(hold) stop")

        ctx.move_to(0, -100)
        ctx.text("tap tempo")

        ctx.move_to(0, 85)
        ctx.text("next:")

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)
        st = self.seq.signals.step.value
        rgb = self._track_col(self.track)
        if st == 0:
            leds.set_all_rgb(*[int(x / 4) for x in rgb])
        else:
            leds.set_all_rgb(0, 0, 0)
        self._highlight_petal(10 - (4 - (st // 4)), *rgb)
        self._highlight_petal(10 - (6 + (st % 4)), *rgb)
        leds.update()

        petals = self.input.captouch.petals

        self._group_highlight = [False for _ in range(4)]
        for i in range(4):
            if petals[4 - i].whole.down:
                self._group_highlight[i] = True
                for j in range(4):
                    if petals[6 + j].whole.pressed:
                        self.seq.trigger_toggle(self.track, i * 4 + j)
        if petals[5].whole.pressed:
            self.track = (self.track - 1) % 3
        if petals[0].whole.pressed:
            if self.stopped:
                self.seq.signals.bpm = self.bpm
                self.blm.background_mute_override = True
                self.stopped = False
            elif self.delta_acc < 3000 and self.delta_acc > 10:
                bpm = int(60000 / self.delta_acc)
                if bpm > 40 and bpm < 500:
                    self.seq.signals.bpm = bpm
                    self.bpm = bpm
            self.delta_acc = 0

        if petals[0].whole.down:
            if self.tap_tempo_press_counter > 500:
                self.seq.signals.bpm = 0
                self.stopped = True
                self.blm.background_mute_override = False
            else:
                self.tap_tempo_press_counter += delta_ms
        else:
            self.tap_tempo_press_counter = 0

        if self.delta_acc < 3000:
            self.delta_acc += delta_ms
