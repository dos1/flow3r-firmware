from st3m.application import Application, ApplicationContext
from st3m.goose import Dict, Any, List, Optional
from st3m.ui.view import View, ViewManager
from st3m.input import InputState
from ctx import Context

import json
import math
import captouch
import bl00mbox

import leds
import random

chords = [
    [-5, -5, 2, 7, 10],
    [-4, -4, 0, 5, 8],
    [-10, -2, 5, 8, 12],
    [-12, 0, 3, 10, 15],
    [-9, 3, 7, 14, 15],
]

from st3m.application import Application, ApplicationContext


class ShoegazeApp(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)

        # synth is initialized in on_enter!
        self.blm: Optional[bl00mbox.Channel] = None
        self.chord_index = 0
        self.chord: List[int] = []
        self._set_chord(3)
        self._tilt_bias = 0.0
        self._detune_prev = 0.0
        self._git_string_tuning = [0] * 4
        self._spinny = -0.5
        self._gaze_counter = 0
        self._rand_counter = 0
        self._rand_limit = 16
        self._rand_rot = 0.0
        self.delay_on = True
        self.fuzz_on = True

    def _build_synth(self) -> None:
        if self.blm is None:
            self.blm = bl00mbox.Channel("shoegaze")
        self.main_lp = self.blm.new(bl00mbox.plugins.lowpass)
        self.main_fuzz = self.blm.new(bl00mbox.patches.fuzz)
        self.main_mixer = self.blm.new(bl00mbox.plugins.mixer, 2)
        self.git_strings = [
            self.blm.new(bl00mbox.patches.karplus_strong) for i in range(4)
        ]
        self.bass_string = self.blm.new(bl00mbox.patches.karplus_strong)
        self.git_mixer = self.blm.new(bl00mbox.plugins.mixer, 4)
        self.git_fuzz = self.blm.new(bl00mbox.patches.fuzz)
        self.git_delay = self.blm.new(bl00mbox.plugins.delay)
        self.git_lp = self.blm.new(bl00mbox.plugins.lowpass)
        self.bass_lp = self.blm.new(bl00mbox.plugins.lowpass)

        self.git_mixer.signals.input0 = self.git_strings[0].signals.output
        self.git_mixer.signals.input1 = self.git_strings[1].signals.output
        self.git_mixer.signals.input2 = self.git_strings[2].signals.output
        self.git_mixer.signals.input3 = self.git_strings[3].signals.output
        self.git_mixer.signals.output = self.git_lp.signals.input
        self.git_fuzz.signals.input = self.git_lp.signals.output

        self.bass_lp.signals.input = self.bass_string.signals.output
        self.main_mixer.signals.input1 = self.bass_lp.signals.output

        self.main_fuzz.signals.input = self.main_mixer.signals.output
        self.main_fuzz.signals.output = self.main_lp.signals.input
        self.main_lp.signals.output = self.blm.mixer

        self.git_delay.signals.time = 200
        self.git_delay.signals.dry_vol = 32767
        self.git_delay.signals.level = 16767
        self.git_delay.signals.feedback = 27000

        self.git_fuzz.intensity = 8
        self.git_fuzz.gate = 1500
        self.git_fuzz.volume = 12000
        self.git_lp.signals.freq = 700
        self.git_lp.signals.reso = 2500

        self.bass_lp.signals.freq = 400
        self.bass_lp.signals.reso = 3000
        self.main_fuzz.intensity = 8
        self.main_fuzz.gate = 1500
        self.main_fuzz.volume = 32000

        self.main_mixer.signals.gain = 2000
        self.main_lp.signals.reso = 2000
        self._update_connections()

    def _update_connections(self) -> None:
        if self.blm is None:
            return
        if self.fuzz_on:
            self.bass_lp.signals.gain = 32767
            self.git_lp.signals.gain = 32767
            self.main_lp.signals.freq = 2500
            self.main_lp.signals.gain = 2000
            self.git_mixer.signals.gain = 4000
            self.main_lp.signals.input = self.main_fuzz.signals.output
            if self.delay_on:
                self.git_delay.signals.input = self.git_fuzz.signals.output
                self.main_mixer.signals.input0 = self.git_delay.signals.output
            else:
                self.main_mixer.signals.input0 = self.git_fuzz.signals.output
        else:
            self.bass_lp.signals.gain = 2000
            self.git_lp.signals.gain = 2000
            self.main_lp.signals.freq = 6000
            self.main_lp.signals.gain = 4000
            self.git_mixer.signals.gain = 500
            self.main_lp.signals.input = self.main_mixer.signals.output
            if self.delay_on:
                self.git_delay.signals.input = self.git_lp.signals.output
                self.main_mixer.signals.input0 = self.git_delay.signals.output
            else:
                self.main_mixer.signals.input0 = self.git_lp.signals.output

    def fuzz_toggle(self) -> None:
        self.fuzz_on = not self.fuzz_on
        self._update_connections()

    def delay_toggle(self) -> None:
        self.delay_on = not self.delay_on
        self._update_connections()

    def _set_chord(self, i: int) -> None:
        hue = int(54 * (i + 0.5)) % 360
        if i != self.chord_index:
            self.chord_index = i
            leds.set_all_hsv(hue, 1, 0.2)
            leds.update()
            self.chord = chords[i]

    def draw(self, ctx: Context) -> None:
        ctx.text_align = ctx.CENTER
        self._rand_counter += 1
        if self._rand_counter > self._rand_limit:
            self._rand_counter = 0
            self._rand_rot = 0.01 * float(random.getrandbits(3))
        if self._rand_counter == 1:
            self._rand_limit = 2 + random.getrandbits(3)

        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
        ctx.font = ctx.get_font_name(5)
        ctx.font_size = 35

        ctx.move_to(0, -112)
        ctx.rgb(0.2, 0, 0.2)
        ctx.text("bass")

        rot = self._spinny  # + self._detune_prev
        ctx.rgb(0, 0.5, 0.5)
        ctx.rotate(rot + self._rand_rot)
        ctx.move_to(0, -10)
        ctx.text("shoegazeshoegazeshoe")
        ctx.move_to(0, 10)
        ctx.text("gazeshoegazeshoegaze")
        ctx.rotate(-2.2 * (rot + self._rand_rot) - 0.5)

        ctx.move_to(40, 40)
        if self.delay_on:
            ctx.rgb(0, 0.8, 0)
            ctx.text("delay ON!")
        else:
            ctx.rgb(0, 0.6, 0)
            ctx.text("delay off")

        ctx.rotate(0.2 + self._rand_rot)

        ctx.move_to(50, -50)
        detune = "detune: " + str(int(self._detune_prev * 100))
        ctx.rgb(0, 0.8, 0)
        ctx.text(detune)

        ctx.rotate(-2.5 * (rot + 4 * self._rand_rot))

        ctx.move_to(-50, 50)
        if self.fuzz_on:
            ctx.rgb(0, 0.8, 0)
            ctx.text("fuzz ON!")
        else:
            ctx.rgb(0, 0.6, 0)
            ctx.text("fuzz off")

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        y = ins.imu.acc[0]
        x = ins.imu.acc[1]
        tilt = (y**2) + (x**2)
        tilt = tilt / 50
        self._tilt_bias = 0.99 * self._tilt_bias + 0.01 * tilt
        detune = (tilt - self._tilt_bias) * 0.5 + self._detune_prev * 0.5
        self._detune_prev = detune

        buttons = self.input.buttons
        petals = self.input.captouch.petals
        if buttons.app.right.pressed:
            self.delay_toggle()
        if buttons.app.left.pressed:
            pass
            # self.fuzz_toggle()

        for i in range(1, 10, 2):
            if petals[i].whole.pressed:
                k = int(((10 - i) - 1) / 2)
                self._set_chord(k)

        if self.blm is None:
            return
        for i in range(2, 10, 2):
            k = int((10 - i) / 2) - 1
            if petals[i].whole.pressed:
                self._git_string_tuning[k] = self.chord[k] - 12
                self.git_strings[k].signals.pitch.tone = self._git_string_tuning[k]
                self.git_strings[k].decay = 3000
                self.git_strings[k].signals.trigger.start()

            self.git_strings[k].signals.pitch.tone = self._git_string_tuning[k] + detune

        if petals[0].whole.pressed:
            self.bass_string.signals.pitch.tone = self.chord[0] - 24
            self.bass_string.decay = 1000
            self.bass_string.signals.trigger.start()

    def on_enter(self, vm: Optional[ViewManager]) -> None:
        if self.blm is None:
            self._build_synth()
        if self.blm is not None:  # silly mypy
            self.blm.foreground = True

    def on_exit(self) -> None:
        if self.blm is not None:
            self.blm.free = True  # yeeting the channel in the backend
        self.blm = None
