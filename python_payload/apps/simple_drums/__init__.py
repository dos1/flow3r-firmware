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
        self.blm = bl00mbox.Channel()
        self.num_samplers = 6
        self.seq = self.blm.new(bl00mbox.patches.step_sequencer, self.num_samplers)

        self.kick = self.blm.new(bl00mbox.patches.sampler, "kick.wav")
        self.hat = self.blm.new(bl00mbox.patches.sampler, "hihat.wav")
        self.close = self.blm.new(bl00mbox.patches.sampler, "close.wav")
        self.ride = self.blm.new(bl00mbox.patches.sampler, "open.wav")
        self.crash = self.blm.new(bl00mbox.patches.sampler, "crash.wav")
        self.snare = self.blm.new(bl00mbox.patches.sampler, "snare.wav")

        self.kick.sampler.signals.trigger = self.seq.seqs[0].signals.output
        self.hat.sampler.signals.trigger = self.seq.seqs[1].signals.output
        self.close.sampler.signals.trigger = self.seq.seqs[2].signals.output
        self.ride.sampler.signals.trigger = self.seq.seqs[3].signals.output
        self.crash.sampler.signals.trigger = self.seq.seqs[4].signals.output
        self.snare.sampler.signals.trigger = self.seq.seqs[5].signals.output

        self.track_names = ["kick", "hihat", "close", "ride", "crash", "snare"]
        self.ct_prev = captouch.read()
        self.track = 0
        self.seq.bpm = 80
        self.blm.background_mute_override = True
        self.tap_tempo_press_counter = 0
        self.track_back_press_counter = 0
        self.delta_acc = 0
        self.stopped = False
        self.track_back = False
        self.bpm = self.seq.bpm

    def _highlight_petal(self, num: int, r: int, g: int, b: int) -> None:
        for i in range(5):
            leds.set_rgb((4 * num - i + 2) % 40, r, g, b)

    def _track_col(self, track: int) -> Tuple[int, int, int]:
        rgb = (20, 20, 20)
        if track == 0:
            rgb = (120, 0, 137)
        elif track == 1:
            rgb = (0, 75, 255)
        elif track == 2:
            rgb = (0, 130, 27)
        elif track == 3:
            rgb = (255, 239, 0)
        elif track == 4:
            rgb = (255, 142, 0)
        elif track == 5:
            rgb = (230, 0, 0)
        return rgb

    def draw(self, ctx: Context) -> None:
        dots = []
        groupgap = 4
        for i in range(4):
            if self.ct_prev.petals[4 - i].pressed:
                dots.append(
                    Dot(
                        48 + groupgap,
                        10+10*self.num_samplers,
                        int((12 * 4 + groupgap) * (1.5 - i)),
                        0,
                        (0.15, 0.15, 0.15),
                    )
                )
        st = self.seq.seqs[0].signals.step.value

        for track in range(self.num_samplers):
            rgb = self._track_col(track)
            rgbf = (rgb[0] / 256, rgb[1] / 256, rgb[2] / 256)
            y = int(12 * (track - (self.num_samplers - 1)/2))
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
                    dots.append(Dot(size / 2, size / 2, x, 10 + 5 * self.num_samplers, (1, 1, 1)))

        dots.append(Dot(1, 10+10*self.num_samplers, 0, 0, (0.5, 0.5, 0.5)))
        dots.append(Dot(1, 10+10*self.num_samplers, 4 * 12 + groupgap, 0, (0.5, 0.5, 0.5)))
        dots.append(Dot(1, 10+10*self.num_samplers, -4 * 12 - groupgap, 0, (0.5, 0.5, 0.5)))

        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
        for i, dot in enumerate(dots):
            dot.draw(i, ctx)

        ctx.font = ctx.get_font_name(4)
        ctx.font_size = 30

        ctx.move_to(0, 55)
        col = [x / 255 for x in self._track_col(self.track)]
        ctx.rgb(*col)
        ctx.text(self.track_names[self.track])

        ctx.font_size = 18

        ctx.move_to(0, 105)
        next_track = (self.track - 1) % self.num_samplers
        col = [x / 255 for x in self._track_col(next_track)]
        ctx.rgb(*col)
        ctx.text(self.track_names[next_track])

        ctx.font = ctx.get_font_name(1)
        ctx.font_size = 20

        ctx.move_to(0, -65)
        ctx.rgb(255, 255, 255)
        ctx.text(str(self.seq.bpm) + " bpm")

        ctx.font_size = 15
        ctx.rgb(0.6, 0.6, 0.6)

        ctx.move_to(0, -85)
        ctx.text("(hold) stop")

        ctx.move_to(0, -100)
        ctx.text("tap tempo")

        ctx.move_to(0, 75)
        ctx.text("(hold) back")

        ctx.move_to(0, 90)
        ctx.text("next:")

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)
        st = self.seq.seqs[0].signals.step.value
        rgb = self._track_col(self.track)
        if st == 0:
            leds.set_all_rgb(*[int(x / 4) for x in rgb])
        else:
            leds.set_all_rgb(0, 0, 0)
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

        if not ct.petals[5].pressed and (self.ct_prev.petals[5].pressed):
            if self.track_back:
                self.track_back = False
            else:
                self.track = (self.track - 1) % self.num_samplers

        if ct.petals[0].pressed and not (self.ct_prev.petals[0].pressed):
            if self.stopped:
                self.seq.bpm = self.bpm
                self.stopped = False
            elif self.delta_acc < 3000 and self.delta_acc > 10:
                bpm = int(60000 / self.delta_acc)
                if bpm > 40 and bpm < 500:
                    self.seq.bpm = bpm
                    self.bpm = bpm
            self.delta_acc = 0

        if ct.petals[0].pressed:
            if self.tap_tempo_press_counter > 500:
                self.stopped = True
            else:
                self.tap_tempo_press_counter += delta_ms
        else:
            self.tap_tempo_press_counter = 0

        if ct.petals[5].pressed:
            if (self.track_back_press_counter > 500) and not self.track_back:
                self.track = (self.track + 1) % self.num_samplers
                self.track_back = True
            else:
                self.track_back_press_counter += delta_ms
        else:
            self.track_back_press_counter = 0

        if self.delta_acc < 3000:
            self.delta_acc += delta_ms
        self.ct_prev = ct
