from st3m.application import Application, ApplicationContext
from st3m.goose import Dict, Any, List, Optional
from st3m.ui.view import View, ViewManager
from st3m.input import InputState
from ctx import Context
from st3m.ui import colours

import json
import errno
import math
import captouch
import bl00mbox

import leds
import random
import math
import sys_display

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

        # synth is initialized in on_enter_done!
        self.blm: Optional[bl00mbox.Channel] = None
        self.chord_index = 0
        self.chord: List[int] = []
        self._organ_chords = [None] * 5
        self._tilt_bias = 0.0
        self._detune_prev = 0.0
        self._git_string_tuning = [0] * 4
        self._spinny = -0.5
        self._gaze_counter = 0
        self._rand_counter = 0
        self._rand_limit = 16
        self._rand_rot = 0.0
        self.delay_on = True
        self.organ_on = False
        self.hue = 0
        self._set_chord(3, force_update=True)

    def _build_synth(self) -> None:
        if self.blm is None:
            self.blm = bl00mbox.Channel("shoegaze")
        self.main_fuzz = self.blm.new(bl00mbox.plugins.distortion)
        self.main_mixer = self.blm.new(bl00mbox.plugins.mixer, 2)
        self.git_strings = [
            self.blm.new(bl00mbox.patches.karplus_strong) for i in range(4)
        ]
        self.bass_string = self.blm.new(bl00mbox.patches.karplus_strong)
        self.git_mixer = self.blm.new(bl00mbox.plugins.mixer, 4)
        self.git_mixer.signals.block_dc.switch.ON = True
        self.main_mixer.signals.block_dc.switch.ON = True
        self.git_fuzz = self.blm.new(bl00mbox.plugins.distortion)
        self.git_delay = self.blm.new(bl00mbox.plugins.delay_static)

        self.git_lp = self.blm.new(bl00mbox.plugins.filter)
        self.bass_lp = self.blm.new(bl00mbox.plugins.filter)
        self.main_lp = self.blm.new(bl00mbox.plugins.filter)
        self.git_lp.signals.cutoff.freq = 700
        self.git_lp.signals.reso = 10000
        self.bass_lp.signals.cutoff.freq = 400
        self.bass_lp.signals.reso = 12000
        self.main_lp.signals.cutoff.freq = 2500
        self.main_lp.signals.reso = 8000

        for i in range(4):
            self.git_mixer.signals.input[i] = self.git_strings[i].signals.output
            self.git_strings[i].signals.reso = -2
            self.git_strings[i].signals.decay = 3000
        self.bass_string.signals.reso = -2
        self.bass_string.signals.decay = 1000

        self.git_mixer.signals.output = self.git_lp.signals.input
        self.git_fuzz._special_sauce = 2
        self.git_fuzz.signals.input = self.git_lp.signals.output
        self.main_mixer.signals.input[0] = self.git_delay.signals.output

        self.bass_lp.signals.input = self.bass_string.signals.output
        self.main_mixer.signals.input[1] = self.bass_lp.signals.output
        self.main_mixer.signals.input_gain[1].dB = -3

        self.main_fuzz._special_sauce = 2
        self.main_fuzz.signals.input = self.main_mixer.signals.output
        self.main_fuzz.signals.output = self.main_lp.signals.input
        self.main_lp.signals.output = self.blm.mixer
        self.git_delay.signals.input = self.git_fuzz.signals.output

        self.git_delay.signals.time = 200
        self.git_delay.signals.dry_vol = 32767
        self.git_delay.signals.feedback = 27000

        self.main_fuzz.curve_set_power(8, 32000, 1500)
        self.git_fuzz.curve_set_power(9, 12000, 750)

        self.bass_lp.signals.gain = 32767
        self.git_lp.signals.gain = 32767
        self.main_lp.signals.gain = 2000
        self.main_lp.signals.input = self.main_fuzz.signals.output
        self._update_connections()

    def _update_connections(self) -> None:
        if self.blm is None:
            return
        if self.delay_on:
            self.git_delay.signals.level = 16767
        else:
            self.git_delay.signals.level = 0

    def _try_load_settings(self, path):
        try:
            with open(path, "r") as f:
                return json.load(f)
        except OSError as e:
            if e.errno != errno.ENOENT:
                raise  # ignore file not found

    def _load_settings(self):
        settings_path = "/flash/harmonic_demo.json"

        settings = self._try_load_settings(settings_path)
        if settings is not None:
            for i, chord in enumerate(settings["chords"]):
                if i > 4:
                    break
                if "tones_readonly" in chord:
                    self._organ_chords[i] = chord["tones_readonly"]

    def organ_toggle(self) -> None:
        self.organ_on = not self.organ_on
        if self.organ_on and self._organ_chords[0] is None:
            self._load_settings()
            if self._organ_chords[0] is None:
                self.organ_on = False
        self._set_chord(self.chord_index, force_update=True)

    def delay_toggle(self) -> None:
        self.delay_on = not self.delay_on
        self._update_connections()

    def _set_chord(self, i: int, force_update=False) -> None:
        if i != self.chord_index or force_update:
            self.hue = (54 * (i + 0.5)) * math.tau / 360
            self.chord_index = i
            if self.organ_on and self._organ_chords[i] is not None:
                tmp = self._organ_chords[i]
                # tmp[0] -= 12
                self.chord = tmp
            else:
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

        ctx.move_to(0, -105)
        ctx.rgb(0.2, 0, 0.2)
        ctx.text("bass")

        rot = self._spinny  # + self._detune_prev
        ctx.rotate(rot + self._rand_rot)
        if self.organ_on:
            ctx.rgb(0.5, 0, 0.6)
            ctx.rotate(0.69)
            ctx.move_to(0, -10)
            ctx.text("chordorganchordorganchord")
            ctx.move_to(0, 10)
            ctx.text("organchordorganchordorgan")
            ctx.rotate(4.20 + 0.69)
        else:
            ctx.rgb(0, 0.5, 0.5)
            ctx.move_to(0, -10)
            ctx.text("shoegazeshoegazeshoe")
            ctx.move_to(0, 10)
            ctx.text("gazeshoegazeshoegaze")
        ctx.rotate(-2.2 * (rot + self._rand_rot) - 0.5)

        if self.organ_on:
            rgb = (0.7, 0.7, 0)
        else:
            rgb = (0, 0.8, 0)

        ctx.move_to(40, 40)

        if self.delay_on:
            ctx.rgb(*rgb)
            ctx.text("delay ON!")
        else:
            ctx.rgb(*tuple([x * 0.75 for x in rgb]))
            ctx.text("delay off")
            ctx.rgb(*rgb)

        ctx.rotate(0.2 + self._rand_rot)
        ctx.move_to(50, -50)
        detune = "detune: " + str(int(self._detune_prev * 100))
        ctx.rgb(*rgb)
        ctx.text(detune)
        ctx.rotate(-2.5 * (rot + 4 * self._rand_rot))
        ctx.move_to(-50, 50)
        ctx.text("fuzz ON!")

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
            self.organ_toggle()

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
                self.git_strings[k].signals.trigger.start()

            self.git_strings[k].signals.pitch.tone = self._git_string_tuning[k] + detune

        if petals[0].whole.pressed:
            self.bass_string.signals.pitch.tone = self.chord[0] - 24
            self.bass_string.signals.trigger.start()

        leds.set_all_rgb(*colours.hsv_to_rgb(self.hue + self._rand_rot * 1.2, 1, 0.7))
        leds.update()

    def on_enter(self, vm: Optional[ViewManager]) -> None:
        super().on_enter(vm)
        leds.set_slew_rate(min(leds.get_slew_rate(), 200))
        self._set_chord(self.chord_index, force_update=True)

    def on_enter_done(self) -> None:
        if self.blm is None:
            self._build_synth()
        self.blm.foreground = True

    def on_exit(self) -> None:
        if self.blm is not None:
            self.blm.clear()
            self.blm.free = True
        self.blm = None


# For running with `mpremote run`:
if __name__ == "__main__":
    import st3m.run

    st3m.run.run_app(ShoegazeApp)
