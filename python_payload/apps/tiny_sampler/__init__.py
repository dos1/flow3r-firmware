import bl00mbox
import captouch
import leds
import audio

from st3m.application import Application, ApplicationContext
from st3m.input import InputState
from st3m.goose import Tuple, Iterator, Optional, Callable, List, Any, TYPE_CHECKING
from ctx import Context
from st3m.ui.view import View, ViewManager

import math


class Pictograms:
    @classmethod
    def two_petal_group(
        cls, ctx, start=0, stop=5, outer_rad=110, inner_rad=50, offset=10
    ):
        cos72 = 0.31
        sin72 = 0.95
        cos36 = 0.81
        sin36 = 0.59
        ctx.save()
        ctx.rotate(start * math.tau / 5 - math.tau / 20)
        curve = 1.15
        for x in range(stop - start):
            ctx.move_to(offset, -inner_rad)
            ctx.line_to(offset, -outer_rad)
            ctx.quad_to(
                sin36 * outer_rad * curve,
                -cos36 * outer_rad * curve,
                sin72 * outer_rad - cos72 * offset,
                -cos72 * outer_rad - sin72 * offset,
            )
            ctx.rotate(math.tau / 5)
            ctx.line_to(-offset, -inner_rad)
            ctx.quad_to(
                -sin36 * inner_rad * curve,
                -cos36 * inner_rad * curve,
                -sin72 * inner_rad + cos72 * offset,
                -cos72 * inner_rad - sin72 * offset,
            )
            ctx.stroke()
        ctx.restore()


class TinySampler(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)
        self.blm = bl00mbox.Channel("tiny sampler")

        self.samplers: List[bl00mbox.patches._Patch | Any] = [None] * 5
        self.line_in = self.blm.new(bl00mbox.plugins.bl00mbox_line_in)
        self.line_in.signals.gain = 32000
        for i in range(5):
            self.samplers[i] = self.blm.new(bl00mbox.patches.sampler, 1000)
            self.samplers[i].signals.output = self.blm.mixer
            self.samplers[i].signals.rec_in = self.line_in.signals.right
        self.is_recording = [False] * 5
        self.is_playing = [False] * 5
        self.has_data = [False] * 5
        if audio.input_get_source() == audio.INPUT_SOURCE_NONE:
            audio.input_set_source(audio.INPUT_SOURCE_ONBOARD_MIC)

        self.ct_prev = captouch.read()

    def _highlight_petal(self, num: int, r: int, g: int, b: int) -> None:
        for i in range(7):
            leds.set_rgb((4 * num - i + 3) % 40, r, g, b)

    def draw(self, ctx: Context) -> None:
        dist = 90
        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()

        ctx.rgb(0.1, 0.5, 0.6)
        ctx.line_width = 6
        Pictograms.two_petal_group(ctx, stop=5)

        ctx.save()
        ctx.line_width = 4
        ctx.font = ctx.get_font_name(0)
        ctx.text_align = ctx.MIDDLE
        ctx.font_size = 24
        for i in range(5):
            if not self.has_data[i]:
                ctx.rgb(0.4, 0.4, 0.4)
            elif self.is_playing[i]:
                ctx.rgb(0.2, 0.9, 0.2)
            else:
                ctx.rgb(0.8, 0.8, 0.8)
            ctx.move_to(0, -dist)
            ctx.rel_line_to(0, -8)
            ctx.rel_line_to(11, 8)
            ctx.rel_line_to(-11, 8)
            ctx.rel_line_to(0, -8)

            ctx.fill()
            ctx.rotate(6.28 / 10)
            ctx.rgb(1, 0, 0)
            if self.is_recording[i]:
                ctx.arc(0, -dist, 8, 0, math.tau, 1)
                ctx.stroke()
                ctx.arc(0, -dist, 5, 0, math.tau, 1)
                ctx.fill()
            else:
                ctx.arc(0, -dist, 7, 0, math.tau, 1)
                ctx.fill()
            ctx.move_to(0, 0)
            ctx.rotate(6.28 / 10)
        ctx.restore()

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        leds.set_all_rgb(0, 0, 0)
        for i in range(5):
            if self.is_recording[i]:
                self._highlight_petal(i * 2, 255, 0, 0)
            else:
                self._highlight_petal(i * 2, 0, 255, 0)
        leds.update()

        ct = captouch.read()
        for i in range(5):
            if not self.is_recording[i]:
                if ct.petals[i * 2].pressed and not self.ct_prev.petals[i * 2].pressed:
                    self.samplers[i].signals.trigger.start()
                    self.is_playing[i] = True
                if not ct.petals[i * 2].pressed and self.ct_prev.petals[i * 2].pressed:
                    self.samplers[i].signals.trigger.stop()
                    self.is_playing[i] = False

        for i in range(5):
            if (
                ct.petals[(i * 2) + 1].pressed
                and not self.ct_prev.petals[(i * 2) + 1].pressed
            ):
                if not self.is_recording[i]:
                    self.samplers[i].signals.rec_trigger.start()
                    self.is_recording[i] = True
            if (
                not ct.petals[(i * 2) + 1].pressed
                and self.ct_prev.petals[(i * 2) + 1].pressed
            ):
                if self.is_recording[i]:
                    self.samplers[i].signals.rec_trigger.stop()
                    self.is_recording[i] = False
                    self.has_data[i] = True

        self.ct_prev = ct

    def on_enter(self, vm) -> None:
        super().on_enter(vm)
        self.blm.foreground = True

    def on_exit(self) -> None:
        super().on_exit()
        for i in range(5):
            if self.is_recording[i]:
                self.samplers[i].signals.rec_trigger.stop()
                self.is_recording[i] = False
        audio.input_set_source(audio.INPUT_SOURCE_NONE)
        self.blm.foreground = False


# For running with `mpremote run`:
if __name__ == "__main__":
    import st3m.run

    st3m.run.run_view(TinySampler(ApplicationContext()))
