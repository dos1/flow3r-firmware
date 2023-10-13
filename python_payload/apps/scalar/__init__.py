from st3m.ui.view import ViewManager
from st3m.goose import Optional, List, Enum
from st3m.input import InputState
from st3m.application import Application, ApplicationContext
from ctx import Context
import captouch
import bl00mbox
import errno
import leds
import math
import time
import json
import os


class Scale:
    __slots__ = ("name", "notes")
    name: str
    notes: List[int]

    def __init__(self, name: str, notes: List[int]):
        self.name = name
        self.notes = notes

    def note(self, i: int, mode: int) -> int:
        octave, note = divmod(i + mode, len(self.notes))
        return self.notes[note] - self.notes[mode] + octave * 12


note_names = [
    "A",
    "A# / Bb",
    "B",
    "C",
    "C# / Db",
    "D",
    "D# / Eb",
    "E",
    "F",
    "F# / Gb",
    "G",
    "G# / Ab",
]

UI_PLAY = 0
UI_KEY = 1
UI_SCALE = 2
UI_MODE = 3
UI_OFFSET = 4
UI_SELECT = 5
DOUBLE_TAP_THRESH_MS = 500


class ScalarApp(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)

        self._load_settings()
        self._ui_state = UI_PLAY
        self._ui_mid_prev_time = 0
        self._ui_cap_prev = captouch.read()
        self._scale_key = 0
        self._scale_offset = 0
        self._scale_mode = 0
        self._scale_index = 0
        self._scale: Scale = self._scales[0]
        self.blm = None
        self._synths_active = [False for _ in range(10)]

    def _build_synth(self):
        if self.blm is not None:
            return

        self.blm = bl00mbox.Channel("Scalar")
        self.blm.volume = 32767
        self.noise_src = self.blm.new(bl00mbox.plugins.noise)
        self.noise_env = self.blm.new(bl00mbox.plugins.env_adsr)
        self.noise_lp = self.blm.new(bl00mbox.plugins.lowpass)
        self.noise_fm_lp = self.blm.new(bl00mbox.plugins.lowpass)
        self.noise_mixer = self.blm.new(bl00mbox.plugins.mixer, 2)
        self.noise_vol_osc = self.blm.new(bl00mbox.plugins.osc_fm)
        self.noise_vol_osc2 = self.blm.new(bl00mbox.plugins.osc_fm)
        self.noise_vol_amp = self.blm.new(bl00mbox.plugins.ampliverter)

        self.oscs = [self.blm.new(bl00mbox.plugins.osc_fm) for i in range(10)]
        self.envs = [self.blm.new(bl00mbox.plugins.env_adsr) for i in range(10)]
        self.synth_mixer = self.blm.new(bl00mbox.plugins.mixer, 10)
        self.synth_delay = self.blm.new(bl00mbox.plugins.delay)
        self.synth_lp = self.blm.new(bl00mbox.plugins.lowpass)
        self.synth_comb = self.blm.new(bl00mbox.plugins.flanger)
        self.synth_delay.signals.input = self.synth_mixer.signals.output
        self.synth_delay.signals.time = 69
        self.synth_delay.signals.level = 6969
        self.synth_delay.signals.feedback = 420
        self.synth_comb.signals.manual = self.noise_vol_amp.signals.output
        # self.synth_comb.signals.manual.tone = -24
        self.synth_comb.signals.mix = -8000
        self.synth_comb.signals.input = self.synth_delay.signals.output
        self.synth_lp.signals.input = self.synth_comb.signals.output
        self.synth_lp.signals.output = self.blm.mixer
        self.synth_lp.signals.freq = 2100
        self.synth_lp.signals.reso = 500
        self.synth_lp.signals.gain = 16000

        for i in range(len(self.oscs)):
            self.oscs[i].signals.waveform = 0
            self.envs[i].signals.decay = 500

            # soft flute
            # self.envs[i].signals.attack = 150
            # hard flute
            self.envs[i].signals.attack = 69

            self.envs[i].signals.sustain = 32767
            self.envs[i].signals.release = 200
            self.oscs[i].signals.lin_fm = self.noise_fm_lp.signals.output
            self.oscs[i].signals.output = self.envs[i].signals.input
            self.synth_mixer.signals.input[i] = self.envs[i].signals.output

        self.noise_lp.signals.input = self.noise_src.signals.output
        self.noise_lp.signals.gain.mult = -0.8
        self.noise_lp.signals.freq = 5000
        self.noise_mixer.signals.input0 = self.noise_lp.signals.output
        self.noise_mixer.signals.input1 = self.noise_src.signals.output
        self.noise_mixer.signals.gain = 4096

        self.noise_fm_lp.signals.input = self.noise_src.signals.output
        self.noise_fm_lp.signals.gain = 300
        self.noise_fm_lp.signals.freq = 200

        self.noise_vol_osc.signals.pitch.freq = 0.13
        self.noise_vol_osc.signals.waveform = -32767
        self.noise_vol_osc2.signals.pitch.freq = 0.69
        self.noise_vol_osc2.signals.lin_fm = self.noise_vol_osc.signals.output
        self.noise_vol_osc2.signals.waveform = -32767
        self.noise_vol_amp.signals.input = self.noise_vol_osc2.signals.output
        self.noise_vol_amp.signals.bias = 33
        self.noise_vol_amp.signals.gain = 15

        # soft flute
        # self.noise_env.signals.attack = 100
        # self.noise_env.signals.sustain = 20000
        # hard flute
        self.noise_env.signals.attack = 30
        self.noise_env.signals.sustain = 15000

        self.noise_env.signals.decay = 150
        self.noise_env.signals.release = 50
        self.noise_env.signals.gain = self.noise_vol_amp.signals.output
        self.noise_env.signals.input = self.noise_lp.signals.output
        self.noise_env.signals.output = self.blm.mixer

    def _load_settings(self) -> None:
        default_path = self.app_ctx.bundle_path + "/scalar-default.json"
        settings_path = "/flash/scalar.json"

        settings = self._try_load_settings(default_path)
        assert settings is not None, "failed to load default settings"

        user_settings = self._try_load_settings(settings_path)
        if user_settings is None:
            self._try_write_default_settings(settings_path, default_path)
        else:
            settings.update(user_settings)

        self._scales = [
            Scale(scale["name"], scale["notes"]) for scale in settings["scales"]
        ]
        self._ui_labels = settings["ui_labels"]

    def _try_load_settings(self, path: str) -> Optional[dict]:
        try:
            with open(path, "r") as f:
                return json.load(f)
        except OSError as e:
            if e.errno != errno.ENOENT:
                raise  # ignore file not found

    def _try_write_default_settings(self, path: str, default_path: str) -> None:
        with open(path, "w") as outfile, open(default_path, "r") as infile:
            outfile.write(infile.read())

    def _update_leds(self) -> None:
        hue = 30 * (self._scale_key % 12) + (30 / len(self._scales)) * self._scale_index
        leds.set_all_hsv(hue, 1, 0.7)
        leds.update()

    def _set_key(self, i: int) -> None:
        if i != self._scale_key:
            self._scale_key = i
            self._update_leds()

    def _set_scale(self, i: int) -> None:
        i = i % len(self._scales)
        if i != self._scale_index:
            self._scale_index = i
            self._scale = self._scales[i]
            self._scale_mode %= len(self._scale.notes)
            self._scale_offset %= len(self._scale.notes)
            self._update_leds()

    def _set_mode(self, i: int) -> None:
        i = i % len(self._scale.notes)
        self._scale_mode = i

    def _set_offset(self, i: int) -> None:
        i = i % len(self._scale.notes)
        self._scale_offset = i

    def _key_name(self) -> str:
        return note_names[self._scale_key % 12]

    def on_enter(self, vm: Optional[ViewManager]) -> None:
        super().on_enter(vm)
        self._load_settings()
        self._update_leds()
        self._build_synth()

    def on_exit(self) -> None:
        super().on_exit()
        if self.blm is not None:
            self.blm.clear()
            self.blm.free = True
            self.blm = None

    def draw(self, ctx: Context) -> None:
        ctx.rgb(0, 0, 0)
        ctx.rectangle(-120, -120, 240, 240)
        ctx.fill()

        # center UI text
        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE
        ctx.font_size = 32

        ctx.rgb(255, 255, 255)
        ctx.move_to(0, -12)
        ctx.text(self._key_name())

        while ctx.text_width(self._scale.name) > 200:
            ctx.font_size -= 1
        ctx.rgb(255, 255, 255)
        ctx.move_to(0, 12)
        ctx.text(self._scale.name)

        def draw_text(petal, r, text, inv=False):
            ctx.save()
            if inv:
                petal = (petal + 5) % 10
                r = -r
            ctx.rotate(petal * math.tau / 10 + math.pi)
            ctx.move_to(0, r)
            ctx.text(text)
            ctx.restore()

        def draw_dot(petal, r):
            ctx.save()
            ctx.rotate(petal * math.tau / 10 + math.pi)
            ctx.rectangle(-5, -5 + r, 10, 10).fill()
            ctx.restore()

        def draw_tri(petal, r):
            ctx.save()
            ctx.rotate(petal * math.tau / 10 + math.pi)
            ctx.move_to(-5, -5 + r)
            ctx.line_to(5, -5 + r)
            ctx.line_to(0, 5 + r)
            ctx.close_path()
            ctx.fill()
            ctx.restore()

        def draw_line(petal, r):
            ctx.save()
            ctx.rotate(petal * math.tau / 10 + math.pi)
            ctx.move_to(-1, -5 + r)
            ctx.line_to(1, -5 + r)
            ctx.line_to(1, 5 + r)
            ctx.line_to(-1, 5 + r)
            ctx.close_path()
            ctx.fill()
            ctx.restore()

        ctx.font_size = 14
        if self._ui_state == UI_KEY or self._ui_state == UI_SELECT:
            draw_dot(8, 90)
            if self._ui_labels:
                draw_text(8, 75, "KEY", inv=True)
        if self._ui_state == UI_SCALE or self._ui_state == UI_SELECT:
            draw_dot(2, 90)
            if self._ui_labels:
                draw_text(2, 75, "SCALE", inv=True)
        if self._ui_state == UI_MODE or self._ui_state == UI_SELECT:
            draw_dot(6, 90)
            if self._ui_labels:
                draw_text(6, 75, "MODE")
        if self._ui_state == UI_OFFSET or self._ui_state == UI_SELECT:
            draw_dot(4, 90)
            if self._ui_labels:
                draw_text(4, 75, "OFFSET")
        if self._ui_state == UI_SELECT:
            draw_dot(0, 90)
            if self._ui_labels:
                draw_text(0, 75, "PLAY", inv=True)

        draw_tri(self._scale_offset, 110)
        if self._scale_mode != 0:
            orig_root_scale_degree = len(self._scale.notes) - self._scale_mode
            orig_root_petal = (orig_root_scale_degree + self._scale_offset) % len(
                self._scale.notes
            )
            draw_line(orig_root_petal, 110)

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        if self.blm is None:
            return

        if self.input.buttons.app.middle.pressed:
            mid_time = time.ticks_ms()
            if self._ui_state == UI_SELECT:
                self._ui_state = UI_PLAY
            if (
                self._ui_state != UI_PLAY
                or mid_time - self._ui_mid_prev_time < DOUBLE_TAP_THRESH_MS
            ):
                self._ui_state = UI_SELECT
            self._ui_mid_prev_time = mid_time

        if self._ui_state == UI_PLAY:
            if self.input.buttons.app.left.pressed:
                self._set_key(self._scale_key - 12)
            if self.input.buttons.app.right.pressed:
                self._set_key(self._scale_key + 12)
        elif self._ui_state == UI_KEY:
            if self.input.buttons.app.left.pressed:
                self._set_key(self._scale_key - 1)
            if self.input.buttons.app.right.pressed:
                self._set_key(self._scale_key + 1)
        elif self._ui_state == UI_SCALE:
            if self.input.buttons.app.left.pressed:
                self._set_scale(self._scale_index - 1)
            if self.input.buttons.app.right.pressed:
                self._set_scale(self._scale_index + 1)
        elif self._ui_state == UI_MODE:
            if self.input.buttons.app.left.pressed:
                self._set_mode(self._scale_mode - 1)
            if self.input.buttons.app.right.pressed:
                self._set_mode(self._scale_mode + 1)
        elif self._ui_state == UI_OFFSET:
            if self.input.buttons.app.left.pressed:
                self._set_offset(self._scale_offset - 1)
            if self.input.buttons.app.right.pressed:
                self._set_offset(self._scale_offset + 1)

        cts = captouch.read()
        for i in range(10):
            press_event = (
                cts.petals[i].pressed and not self._ui_cap_prev.petals[i].pressed
            )
            release_event = (
                not cts.petals[i].pressed and self._ui_cap_prev.petals[i].pressed
            )
            if self._ui_state == UI_SELECT:
                if press_event:
                    if i == 0:
                        self._ui_state = UI_PLAY
                    elif i == 8:
                        self._ui_state = UI_KEY
                    elif i == 2:
                        self._ui_state = UI_SCALE
                    elif i == 6:
                        self._ui_state = UI_MODE
                    elif i == 4:
                        self._ui_state = UI_OFFSET
            else:
                half_step_up = int(self.input.buttons.app.middle.down)
                if press_event:
                    self.oscs[i].signals.pitch.tone = (
                        self._scale_key
                        + self._scale.note(i - self._scale_offset, self._scale_mode)
                        + half_step_up
                    )
                    if not any(self._synths_active):
                        self.noise_env.signals.trigger.start()
                    self.envs[i].signals.trigger.start()
                    self._synths_active[i] = True
                elif release_event:
                    self.envs[i].signals.trigger.stop()
                    self._synths_active[i] = False
                    if not any(self._synths_active):
                        self.noise_env.signals.trigger.stop()
        self._ui_cap_prev = cts


if __name__ == "__main__":
    from st3m.run import run_app

    run_app(ScalarApp, "/flash/sys/apps/scalar")
