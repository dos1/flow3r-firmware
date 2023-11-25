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
import os


class TinySampler(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)
        self.blm = None
        self.file_path = "/sd/tiny_sampler/"

        self.is_recording = [False] * 5
        self.is_playing = [False] * 5
        self.has_data = [False] * 5
        self.has_data_stored = [False] * 5

        self.ct_prev = captouch.read()
        self._num_modes = 6
        self._mode = 0
        self.press_event = [False] * 10
        self.release_event = [False] * 10
        self.playback_speed = [0] * 5

    def _check_mode_avail(self, mode):
        if mode == 0:
            return audio.input_engines_get_source_avail(audio.INPUT_SOURCE_ONBOARD_MIC)
        if mode == 1:
            return audio.input_engines_get_source_avail(audio.INPUT_SOURCE_LINE_IN)
        if mode == 2:
            return audio.input_engines_get_source_avail(audio.INPUT_SOURCE_HEADSET_MIC)
        return True

    @property
    def mode(self):
        return self._mode

    @mode.setter
    def mode(self, val):
        val = val % self._num_modes
        if self.mode == 0:
            if val < 3:
                direction = 1
            else:
                direction = -1
        elif val == 0:
            if self.mode > 3:
                direction = 1
            else:
                direction = -1
        elif val > self.mode:
            direction = 1
        else:
            direction = -1

        while not self._check_mode_avail(val):
            val = (val + direction) % self._num_modes
        self._mode = val

    def _build_synth(self):
        if self.blm is None:
            self.blm = bl00mbox.Channel("tiny sampler")
        self.blm.volume = 32768
        self.samplers = [None] * 5
        self.line_in = self.blm.new(bl00mbox.plugins.bl00mbox_line_in)
        for i in range(5):
            self.samplers[i] = self.blm.new(bl00mbox.plugins.sampler, 1000)
            self.samplers[i].signals.playback_output = self.blm.mixer
            self.samplers[i].signals.record_input = self.line_in.signals.right
            path = self.file_path + "tiny_sample_" + str(i) + ".wav"
            if not os.path.exists(path):
                path = "/flash/sys/samples/" + "tiny_sample_" + str(i) + ".wav"
            try:
                self.samplers[i].load(path)
                self.has_data[i] = True
            except (OSError, bl00mbox.Bl00mboxError) as e:
                self.has_data[i] = False
            if self.has_data[i]:
                self.samplers[i].signals.playback_speed.tone = self.playback_speed[i]
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

    def draw_play_mode(self, ctx):
        ctx.save()
        dist = 90
        ctx.line_width = 5
        for i in range(5):
            if not self.has_data[i]:
                ctx.rgb(0.4, 0.4, 0.4)
            elif self.is_playing[i] and audio.input_thru_get_mute():
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

            if not self.has_data[i]:
                ctx.rgb(0.4, 0.4, 0.4)
            elif self.is_playing[i] and not audio.input_thru_get_mute():
                ctx.rgb(0.2, 0.2, 0.9)
            else:
                ctx.rgb(0.8, 0.8, 0.8)
            ctx.move_to(-7, -dist)
            ctx.rel_line_to(0, -8)
            ctx.rel_line_to(11, 8)
            ctx.rel_line_to(-11, 8)
            ctx.rel_line_to(0, -8)
            ctx.fill()
            ctx.move_to(-7, 12 - dist)
            ctx.rel_line_to(11, 0)
            ctx.stroke()
            ctx.move_to(0, 0)

            ctx.move_to(0, 0)
            ctx.rotate(6.28 / 10)
        ctx.restore()

    def draw_rec_mode(self, ctx):
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

    def draw_save_mode(self, ctx):
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

    def draw_pitch_mode(self, ctx):
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
                ctx.text(str(int(self.samplers[i].signals.playback_speed.tone)))
                ctx.move_to(0, 0)
                ctx.rotate(6.28 / 5)
        ctx.restore()

    def draw(self, ctx: Context) -> None:
        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()

        ctx.rgb(0.1, 0.5, 0.6)
        ctx.line_width = 6
        self.draw_two_petal_group(ctx, stop=5)
        if self.mode < 3:
            self.draw_rec_mode(ctx)
        if self.mode == 3:
            self.draw_play_mode(ctx)
        elif self.mode == 4:
            self.draw_save_mode(ctx)
        elif self.mode == 5:
            self.draw_pitch_mode(ctx)

        ctx.text_align = ctx.CENTER
        ctx.gray(0.8)
        ctx.font_size = 18
        if self.mode == 0:
            ctx.move_to(0, 0)
            ctx.text("onboard")
            ctx.move_to(0, ctx.font_size)
            ctx.text("mic")
        elif self.mode == 1:
            ctx.move_to(0, ctx.font_size / 2)
            ctx.text("line in")
        elif self.mode == 2:
            ctx.move_to(0, 0)
            ctx.text("headset")
            ctx.move_to(0, ctx.font_size)
            ctx.text("mic")

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
            self.mode -= 1
            if self.mode == 3:
                self.mode -= 1
            release_all = True
        elif self.input.buttons.app.right.pressed:
            self.mode += 1
            if self.mode == 3:
                self.mode += 1
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
            audio.input_engines_set_source(audio.INPUT_SOURCE_ONBOARD_MIC)
        elif self.mode == 1:
            audio.input_engines_set_source(audio.INPUT_SOURCE_LINE_IN)
        elif self.mode == 2:
            audio.input_engines_set_source(audio.INPUT_SOURCE_HEADSET_MIC)
        else:
            audio.input_engines_set_source(audio.INPUT_SOURCE_NONE)

        if self.mode < 3 or release_all:
            for i in range(5):
                if not self.is_recording[i]:
                    if self.press_event[i * 2]:
                        self.samplers[i].signals.playback_trigger.start()
                        self.is_playing[i] = True
                        audio.input_thru_set_mute(True)
                    if self.release_event[i * 2]:
                        self.samplers[i].signals.playback_trigger.stop()
                        self.is_playing[i] = False
                        audio.input_thru_set_mute(False)
            for i in range(5):
                if self.press_event[i * 2 + 1]:
                    if not self.is_recording[i]:
                        self.samplers[i].sample_rate = 12000
                        self.samplers[i].signals.record_trigger.start()
                        self.is_recording[i] = True
                        if self.mode == 0:
                            audio.input_thru_set_mute(True)
                if self.release_event[i * 2 + 1]:
                    if self.is_recording[i]:
                        self.samplers[i].signals.record_trigger.stop()
                        self.is_recording[i] = False
                        self.has_data[i] = True
                        if self.mode == 0:
                            audio.input_thru_set_mute(False)
        if self.mode == 3 or release_all:
            for i in range(5):
                if self.press_event[i * 2]:
                    self.samplers[i].signals.playback_trigger.start()
                    self.is_playing[i] = True
                    audio.input_thru_set_mute(True)
                if self.release_event[i * 2]:
                    self.samplers[i].signals.playback_trigger.stop()
                    self.is_playing[i] = False
                    audio.input_thru_set_mute(False)
            for i in range(5):
                if self.press_event[i * 2 + 1]:
                    self.samplers[i].signals.playback_trigger.start()
                    self.is_playing[i] = True
                if self.release_event[i * 2 + 1]:
                    self.samplers[i].signals.playback_trigger.stop()
                    self.is_playing[i] = False
        elif self.mode == 4 or release_all:
            for i in range(5):
                if self.press_event[i * 2]:
                    path = self.file_path + "tiny_sample_" + str(i) + ".wav"
                    if not os.path.exists(path):
                        path = "/flash/sys/samples/" + "tiny_sample_" + str(i) + ".wav"
                    if os.path.exists(path):
                        try:
                            self.samplers[i].load(path)
                            self.has_data[i] = True
                        except (OSError, bl00mbox.Bl00mboxError) as e:
                            self.has_data_stored[i] = False
            for i in range(5):
                if self.press_event[i * 2 + 1]:
                    if self.has_data[i]:
                        try:
                            if not os.path.exists(self.file_path):
                                os.mkdir(self.file_path)
                            path = self.file_path + "tiny_sample_" + str(i) + ".wav"
                            print("saving at: " + path)
                            self.samplers[i].save(path)
                            self.has_data_stored[i] = True
                        except (OSError, bl00mbox.Bl00mboxError) as e:
                            print("failed")
                            self.has_data_stored[i] = False
        elif self.mode == 5 or release_all:
            for i in range(5):
                if self.press_event[i * 2]:
                    self.samplers[i].signals.playback_speed.tone += 1
                    self.playback_speed[i] = self.samplers[
                        i
                    ].signals.playback_speed.tone
                    self.samplers[i].signals.playback_trigger.start()
                    self.is_playing[i] = True
                if self.release_event[i * 2]:
                    self.samplers[i].signals.playback_trigger.stop()
                    self.is_playing[i] = False
            for i in range(5):
                if self.press_event[i * 2 + 1]:
                    self.samplers[i].signals.playback_speed.tone -= 1
                    self.playback_speed[i] = self.samplers[
                        i
                    ].signals.playback_speed.tone
                    self.samplers[i].signals.playback_trigger.start()
                    self.is_playing[i] = True
                if self.release_event[i * 2 + 1]:
                    self.samplers[i].signals.playback_trigger.stop()
                    self.is_playing[i] = False

        self.ct_prev = ct

    def on_enter(self, vm) -> None:
        super().on_enter(vm)
        self.orig_source = audio.input_engines_get_source()
        self.orig_thru_mute = audio.input_thru_get_mute()
        self._mode = self._num_modes - 1
        self.mode = 0
        if self.blm is None:
            self._build_synth()

    def on_exit(self) -> None:
        audio.input_engines_set_source(self.orig_source)
        audio.input_thru_set_mute(self.orig_thru_mute)
        for i in range(5):
            if self.is_recording[i]:
                self.samplers[i].signals.record_trigger.stop()
                self.is_recording[i] = False
        if self.blm is not None:
            self.blm.clear()
            self.blm.free = True
            self.blm = None


# For running with `mpremote run`:
if __name__ == "__main__":
    import st3m.run

    st3m.run.run_app(TinySampler)
