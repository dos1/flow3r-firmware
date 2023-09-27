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


class TinySampler(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)
        self.blm = None

        self.is_recording = [False] * 5
        self.is_playing = [False] * 5
        self.has_data = [False] * 5
        self.has_data_stored = [False] * 5

        self.ct_prev = captouch.read()
        self.mode = 0
        self._num_modes = 3
        self.press_event = [False] * 10
        self.release_event = [False] * 10
        self.pitch_shift = [0] * 5

    def _build_synth(self):
        if self.blm is None:
            self.blm = bl00mbox.Channel("tiny sampler")
        self.samplers: List[bl00mbox.patches._Patch | Any] = [None] * 5
        self.line_in = self.blm.new(bl00mbox.plugins.bl00mbox_line_in)
        for i in range(5):
            self.samplers[i] = self.blm.new(bl00mbox.patches.sampler, 1000)
            self.samplers[i].signals.output = self.blm.mixer
            self.samplers[i].signals.rec_in = self.line_in.signals.right
            self.has_data[i] = self.samplers[i].load("tiny_sample_" + str(i) + ".wav")
            if self.has_data[i]:
                self.samplers[
                    i
                ].plugins.sampler.signals.pitch_shift.tone = self.pitch_shift[i]
            self.has_data_stored[i] = self.has_data[i]

    def _highlight_petal(self, num: int, r: int, g: int, b: int) -> None:
        for i in range(7):
            leds.set_rgb((4 * num - i + 3) % 40, r, g, b)

    def draw_two_petal_group(
        self, ctx, start=0, stop=5, outer_rad=110, inner_rad=50, offset=10
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

    def draw_mode_0(self, ctx):
        ctx.save()
        dist = 90
        ctx.line_width = 5
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
            if not self.is_recording[i]:
                ctx.rgb(0.7, 0, 0)
                ctx.arc(0, -dist, 6, 0, math.tau, 1)
                ctx.stroke()
            else:
                ctx.rgb(1, 0, 0)
                ctx.arc(0, -dist, 9, 0, math.tau, 1)
                ctx.fill()
            ctx.move_to(0, 0)
            ctx.rotate(6.28 / 10)
        ctx.restore()

    def draw_mode_1(self, ctx):
        ctx.save()
        dist = 80
        text_shift = 5
        ctx.font = "Comic Mono"
        ctx.text_align = ctx.MIDDLE
        ctx.font_size = 18
        rot_over = 6.28 / 50
        ctx.rotate(-(6.28 / 4) + rot_over)
        for i in range(5):
            if not self.has_data_stored[i]:
                ctx.rgb(0.4, 0.4, 0.4)
            else:
                ctx.rgb(0.8, 0.8, 0.8)
            ctx.move_to(dist, text_shift)
            ctx.text("load")
            ctx.rotate(6.28 / 10 - 2 * rot_over)

            if not self.has_data[i]:
                ctx.rgb(0.4, 0.4, 0.4)
            else:
                ctx.rgb(0.8, 0.8, 0.8)
            ctx.move_to(dist, text_shift)
            ctx.text("save")
            ctx.move_to(0, 0)
            ctx.rotate(6.28 / 10 + 2 * rot_over)
        ctx.restore()

    def draw_mode_2(self, ctx):
        ctx.save()
        dist = 80
        text_shift = 5
        ctx.font = "Comic Mono"
        ctx.text_align = ctx.MIDDLE
        ctx.font_size = 25
        ctx.move_to(0, 5)
        ctx.rgb(0.8, 0.8, 0.8)
        ctx.text("pitch")
        ctx.rotate(-(6.28 / 4) + (6.28 / 20))
        if self.blm is not None:
            for i in range(5):
                ctx.move_to(dist, text_shift)
                ctx.text(
                    str(int(self.samplers[i].plugins.sampler.signals.pitch_shift.tone))
                )
                ctx.move_to(0, 0)
                ctx.rotate(6.28 / 5)
        ctx.restore()

    def draw(self, ctx: Context) -> None:
        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()

        ctx.rgb(0.1, 0.5, 0.6)
        ctx.line_width = 6
        self.draw_two_petal_group(ctx, stop=5)
        if self.mode == 0:
            self.draw_mode_0(ctx)
        elif self.mode == 1:
            self.draw_mode_1(ctx)
        elif self.mode == 2:
            self.draw_mode_2(ctx)

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        leds.set_all_rgb(0, 0, 0)
        for i in range(5):
            if self.is_recording[i]:
                self._highlight_petal(i * 2, 255, 0, 0)
            else:
                self._highlight_petal(i * 2, 0, 255, 0)
        leds.update()

        if self.blm is None:
            return
        if self.input.buttons.app.left.pressed:
            self.mode = (self.mode - 1) % self._num_modes
            release_all = True
        elif self.input.buttons.app.right.pressed:
            self.mode = (self.mode + 1) % self._num_modes
            release_all = True
        else:
            release_all = False

        ct = captouch.read()

        self.press_event = [False] * 10
        if release_all:
            self.release_event = [True] * 10
        else:
            self.release_event = [False] * 10
            for i in range(10):
                if ct.petals[i].pressed and not self.ct_prev.petals[i].pressed:
                    self.press_event[i] = True
                elif not ct.petals[i].pressed and self.ct_prev.petals[i].pressed:
                    self.release_event[i] = True

        if self.mode == 0:
            if self.orig_source == audio.INPUT_SOURCE_NONE:
                if audio.input_engine_get_source_avail(audio.INPUT_SOURCE_ONBOARD_MIC):
                    audio.input_engine_set_source(audio.INPUT_SOURCE_ONBOARD_MIC)
                else:
                    audio.input_engine_set_source(audio.INPUT_SOURCE_AUTO)
            else:
                if audio.input_engine_get_source_avail(self.orig_source):
                    audio.input_engine_set_source(audio.self.orig_source)
                else:
                    audio.input_engine_set_source(audio.INPUT_SOURCE_AUTO)
        else:
            audio.input_engine_set_source(audio.INPUT_SOURCE_NONE)

        if self.mode == 0 or release_all:
            for i in range(5):
                if not self.is_recording[i]:
                    if self.press_event[i * 2]:
                        self.samplers[i].signals.trigger.start()
                        self.is_playing[i] = True
                    if self.release_event[i * 2]:
                        self.samplers[i].signals.trigger.stop()
                        self.is_playing[i] = False
            for i in range(5):
                if self.press_event[i * 2 + 1]:
                    if not self.is_recording[i]:
                        self.samplers[i].signals.rec_trigger.start()
                        self.is_recording[i] = True
                if self.release_event[i * 2 + 1]:
                    if self.is_recording[i]:
                        self.samplers[i].signals.rec_trigger.stop()
                        self.is_recording[i] = False
                        self.has_data[i] = True
        elif self.mode == 1 or release_all:
            for i in range(5):
                if self.press_event[i * 2]:
                    self.has_data_stored[i] = self.samplers[i].load(
                        "tiny_sample_" + str(i) + ".wav"
                    )
                    self.has_data[i] = self.has_data_stored[i]
            for i in range(5):
                if self.press_event[i * 2 + 1]:
                    if self.has_data[i]:
                        if self.samplers[i].save(
                            "tiny_sample_" + str(i) + ".wav", overwrite=True
                        ):
                            self.has_data_stored[i] = True
        elif self.mode == 2 or release_all:
            for i in range(5):
                if self.press_event[i * 2]:
                    self.samplers[i].plugins.sampler.signals.pitch_shift.tone += 1
                    self.pitch_shift[i] = self.samplers[
                        i
                    ].plugins.sampler.signals.pitch_shift.tone
                    self.samplers[i].signals.trigger.start()
                    self.is_playing[i] = True
                if self.release_event[i * 2]:
                    self.samplers[i].signals.trigger.stop()
                    self.is_playing[i] = False
            for i in range(5):
                if self.press_event[i * 2 + 1]:
                    self.samplers[i].plugins.sampler.signals.pitch_shift.tone -= 1
                    self.pitch_shift[i] = self.samplers[
                        i
                    ].plugins.sampler.signals.pitch_shift.tone
                    self.samplers[i].signals.trigger.start()
                    self.is_playing[i] = True
                if self.release_event[i * 2 + 1]:
                    self.samplers[i].signals.trigger.stop()
                    self.is_playing[i] = False

        self.ct_prev = ct

    def on_enter(self, vm) -> None:
        super().on_enter(vm)
        self.mode = 0
        self.orig_source = audio.input_engine_get_source()
        if self.blm is None:
            self._build_synth()

    def on_exit(self) -> None:
        audio.input_engine_set_source(self.orig_source)
        for i in range(5):
            if self.is_recording[i]:
                self.samplers[i].signals.rec_trigger.stop()
                self.is_recording[i] = False
        if self.blm is not None:
            self.blm.clear()
            self.blm.free = True
            self.blm = None


# For running with `mpremote run`:
if __name__ == "__main__":
    import st3m.run

    st3m.run.run_app(TinySampler)
