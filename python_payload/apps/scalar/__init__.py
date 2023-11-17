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

from st3m.ui import colours


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
UI_SOUND = 6
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
        self._synth_sound_index = 0
        self._synth_sounds = ["flute", "square", "tri", "delay", "filter"]
        self._scale: Scale = self._scales[0]
        self.blm = None
        self._synths_active = [False for _ in range(10)]

    @property
    def _synth_sound_name(self):
        return self._synth_sounds[self._synth_sound_index]

    def _build_synth(self):
        if self.blm is not None:
            return

        self.blm = bl00mbox.Channel("Scalar")
        self.blm.volume = 32767
        self.noise_src = self.blm.new(bl00mbox.plugins.noise)
        self.noise_env = self.blm.new(bl00mbox.plugins.env_adsr)
        self.noise_lp = self.blm.new(bl00mbox.plugins.filter)
        self.noise_fm_lp = self.blm.new(bl00mbox.plugins.filter)
        self.noise_mixer = self.blm.new(bl00mbox.plugins.mixer, 2)
        self.noise_vol_osc = self.blm.new(bl00mbox.plugins.osc)
        self.noise_vol_osc2 = self.blm.new(bl00mbox.plugins.osc)
        self.noise_vol_amp = self.blm.new(bl00mbox.plugins.range_shifter)
        self.noise_fm_lp_amp = self.blm.new(bl00mbox.plugins.range_shifter)

        self.oscs = [self.blm.new(bl00mbox.plugins.osc) for i in range(10)]
        self.envs = [self.blm.new(bl00mbox.plugins.env_adsr) for i in range(10)]
        self.mods = [self.blm.new(bl00mbox.plugins.range_shifter) for i in range(10)]
        self.mods2 = [self.blm.new(bl00mbox.plugins.range_shifter) for i in range(10)]
        self.mod = self.blm.new(bl00mbox.plugins.range_shifter)
        self.synth_mixer = self.blm.new(bl00mbox.plugins.mixer, 10)
        self.synth_delay = self.blm.new(bl00mbox.plugins.delay)
        self.synth_lp = self.blm.new(bl00mbox.plugins.filter)
        self.synth_comb = self.blm.new(bl00mbox.plugins.flanger)
        self.synth_delay.signals.input = self.synth_mixer.signals.output
        self.synth_comb.signals.manual = self.noise_vol_amp.signals.output
        self.synth_comb.signals.input = self.synth_delay.signals.output
        self.synth_lp.signals.input = self.synth_comb.signals.output
        self.synth_lp.signals.output = self.blm.mixer

        self.mod.signals.speed.switch.SLOW = True
        for i in range(len(self.oscs)):
            self.mods[i].signals.speed.switch.SLOW = True
            self.mods2[i].signals.speed.switch.SLOW = True
            self.mods2[i].signals.input_range[0] = 22000
            self.mods2[i].signals.input_range[1] = 30000
            self.mods2[i].signals.output_range[0] = 200
            self.mods2[i].signals.output_range[1] = 50
            self.mods2[i].signals.input = self.mods[i].signals.output
            self.oscs[i].signals.waveform = 0
            self.envs[i].signals.attack = 69
            self.envs[i].signals.sustain = 25125
            self.envs[i].signals.release = 200
            self.envs[i].signals.decay = 121

            self.oscs[i].signals.fm = self.noise_fm_lp_amp.signals.output
            self.oscs[i].signals.output = self.envs[i].signals.input
            self.synth_mixer.signals.input[i] = self.envs[i].signals.output

        self.noise_lp.signals.input = self.noise_src.signals.output
        self.noise_lp.signals.gain.mult = -0.8
        self.noise_lp.signals.cutoff.freq = 5000
        self.noise_mixer.signals.input0 = self.noise_lp.signals.output
        self.noise_mixer.signals.input1 = self.noise_src.signals.output
        self.noise_mixer.signals.gain = 4096

        self.noise_fm_lp.signals.input = self.noise_src.signals.output
        self.noise_fm_lp.signals.gain = 350 // 4
        self.noise_fm_lp.signals.cutoff.freq = 200
        self.noise_fm_lp_amp.signals.speed.switch.SLOW = True
        self.noise_fm_lp.signals.output = self.noise_fm_lp_amp.signals.input

        self.noise_vol_osc.signals.speed.switch.LFO = True
        self.noise_vol_osc2.signals.speed.switch.LFO = True
        self.noise_vol_amp.signals.speed.switch.SLOW = True
        self.noise_vol_osc.signals.pitch.freq = 0.13
        self.noise_vol_osc.signals.waveform = -32767
        self.noise_vol_osc2.signals.pitch.freq = 0.69
        self.noise_vol_osc2.signals.fm = self.noise_vol_osc.signals.output
        self.noise_vol_osc2.signals.waveform = -32767
        self.noise_vol_amp.signals.input = self.noise_vol_osc2.signals.output
        self.noise_vol_amp.signals.output_range[0] = 18
        self.noise_vol_amp.signals.output_range[1] = 48
        self.noise_env.signals.attack = 30
        self.noise_env.signals.sustain = 15000

        self.noise_env.signals.decay = 150
        self.noise_env.signals.release = 50
        self.noise_env.signals.gain = self.noise_vol_amp.signals.output
        self.noise_env.signals.input = self.noise_lp.signals.output
        self.noise_env.signals.output = self.blm.mixer
        self._set_sound(self._synth_sound_index)

    def _load_user_settings(self):
        try:
            user_settings = self._try_load_settings("/sd/scalar/user_settings.json")
        except OSError:
            user_settings = None
        if user_settings is not None:
            valid_settings = True
            try:
                scale_key = user_settings["scale_key"]
                scale_offset = user_settings["scale_offset"]
                scale_mode = user_settings["scale_mode"]
                scale_index = user_settings["scale_index"]
                synth_sound_index = user_settings["sound_index"]
            except:
                valid_settings = False
            if valid_settings:
                self._scale_key = scale_key
                self._scale_offset = scale_offset
                self._scale_mode = scale_mode
                self._scale_index = scale_index
                self._synth_sound_index = synth_sound_index

    def _save_user_settings(self):
        user_settings = {}
        user_settings["scale_key"] = self._scale_key
        user_settings["scale_offset"] = self._scale_offset
        user_settings["scale_mode"] = self._scale_mode
        user_settings["scale_index"] = self._scale_index
        user_settings["sound_index"] = self._synth_sound_index
        nothing_changed = False
        try:
            old_user_settings = self._try_load_settings("/sd/scalar/user_settings.json")
        except OSError:
            old_user_settings = None

        if old_user_settings is not None:
            nothing_changed = True
            for key in user_settings:
                try:
                    assert user_settings[key] == old_user_settings[key]
                except:
                    nothing_changed = False

        if not nothing_changed:
            try:
                os.mkdir("/sd/scalar")
            except OSError:
                pass
            try:
                self._try_save_settings("/sd/scalar/user_settings.json", user_settings)
            except OSError:
                pass

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

    def _try_save_settings(self, path, settings):
        try:
            with open(path, "w+") as f:
                f.write(json.dumps(settings))
                f.close()
        except OSError as e:
            if e.errno != errno.ENOENT:
                raise  # ignore file not found

    def _try_write_default_settings(self, path: str, default_path: str) -> None:
        with open(path, "w") as outfile, open(default_path, "r") as infile:
            outfile.write(infile.read())

    def _update_leds(self) -> None:
        hue = (
            6.28
            / 12
            * ((self._scale_key % 12) + (self._scale_index / len(self._scales)))
        )
        leds.set_all_rgb(*colours.hsv_to_rgb(hue, 1, 0.7))
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

    def _set_sound(self, i: int) -> None:
        i = i % len(self._synth_sounds)
        self._synth_sound_index = i
        if self._synth_sound_name == "flute":
            for i in range(len(self.oscs)):
                self.oscs[i].signals.morph = 0
                self.mods[i].signals.output_range[0] = 22000
                self.mods[i].signals.output_range[1] = 30000
                self.envs[i].signals.sustain = self.mods[i].signals.output
                self.envs[i].signals.attack = self.mods2[i].signals.output
                self.oscs[i].signals.waveform.switch.SQUARE = True
                self.envs[i].signals.release = 200
                self.envs[i].signals.decay = 121
            self.noise_env.signals.attack = 100
            self.noise_env.signals.sustain = 12207
            self.noise_fm_lp.signals.gain = 350
            self.noise_vol_amp.signals.output_range[0] = 18
            self.noise_vol_amp.signals.output_range[1] = 48
            self.synth_comb.signals.mix = -8000
            self.synth_lp.signals.cutoff.freq = 2100
            self.synth_lp.signals.reso = 2000
            self.synth_lp.signals.gain = 16000
            self.synth_delay.signals.time = 69
            self.synth_delay.signals.level = 6969
            self.synth_delay.signals.feedback = 420

        elif self._synth_sound_name == "square":
            for i in range(len(self.oscs)):
                self.oscs[i].signals.morph = 0
                self.oscs[i].signals.waveform.switch.SQUARE = True
                self.envs[i].signals.attack = 20
                self.envs[i].signals.sustain = 32767
                self.envs[i].signals.release = 200
                self.envs[i].signals.decay = 500
            self.noise_fm_lp.signals.gain = 0
            self.noise_vol_amp.signals.output_range[0] = 0
            self.noise_vol_amp.signals.output_range[1] = 0
            self.synth_delay.signals.level = 0
            self.synth_comb.signals.mix = 0
            self.synth_lp.signals.cutoff.freq = 15000
            self.synth_lp.signals.reso = 2000
            self.synth_lp.signals.gain = 3000

        elif self._synth_sound_name == "tri":
            for i in range(len(self.oscs)):
                self.mods[i].signals.output = self.oscs[i].signals.morph
                self.mods[i].signals.output_range[0] = 0
                self.mods[i].signals.output_range[1] = 10000
                self.oscs[i].signals.waveform.switch.TRI = True
                self.envs[i].signals.attack = 20
                self.envs[i].signals.sustain = 32767
                self.envs[i].signals.release = 200
                self.envs[i].signals.decay = 500
            self.noise_fm_lp.signals.gain = 0
            self.noise_vol_amp.signals.output_range[0] = 0
            self.noise_vol_amp.signals.output_range[1] = 0
            self.synth_delay.signals.level = 0
            self.synth_comb.signals.mix = 0
            self.synth_lp.signals.cutoff.freq = 15000
            self.synth_lp.signals.reso = 2000
            self.synth_lp.signals.gain = 12800

        elif self._synth_sound_name == "delay":
            for i in range(len(self.oscs)):
                self.mods[i].signals.output = self.oscs[i].signals.morph
                self.mods[i].signals.output_range[0] = 0
                self.mods[i].signals.output_range[1] = 16000
                self.oscs[i].signals.waveform = 0
                self.envs[i].signals.attack = 20
                self.envs[i].signals.sustain = 32767
                self.envs[i].signals.release = 200
                self.envs[i].signals.decay = 500
            self.noise_fm_lp.signals.gain = 0
            self.noise_vol_amp.signals.output_range[0] = 0
            self.noise_vol_amp.signals.output_range[1] = 0
            self.synth_comb.signals.mix = 0
            self.synth_lp.signals.cutoff.freq = 8000
            self.synth_lp.signals.reso = 2000
            self.synth_lp.signals.gain = 7200
            self.synth_delay.signals.time = 420
            self.synth_delay.signals.level = 16000
            self.synth_delay.signals.feedback = 25000

        elif self._synth_sound_name == "filter":
            for i in range(len(self.oscs)):
                self.oscs[i].signals.morph = 0
                self.oscs[i].signals.waveform.switch.SAW = True
                self.envs[i].signals.attack = 20
                self.envs[i].signals.sustain = 26000
                self.envs[i].signals.release = 200
                self.envs[i].signals.decay = 100
            self.synth_mixer.signals.gain.mult = 0.07
            self.noise_fm_lp.signals.gain = 0
            self.noise_vol_amp.signals.output_range[0] = 0
            self.noise_vol_amp.signals.output_range[1] = 0
            self.synth_comb.signals.mix = 0
            self.mod.signals.output_range[0] = 19000
            self.mod.signals.output_range[1] = 29000
            self.synth_lp.signals.cutoff = self.mod.signals.output
            self.synth_lp.signals.reso = 16000
            self.synth_lp.signals.gain = 8000
            self.synth_delay.signals.level = 0

    def _key_name(self) -> str:
        return note_names[self._scale_key % 12]

    def on_enter(self, vm: Optional[ViewManager]) -> None:
        super().on_enter(vm)
        self._load_settings()
        self._load_user_settings()
        self._update_leds()
        self._build_synth()

    def on_exit(self) -> None:
        super().on_exit()
        self._save_user_settings()
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
        if self._ui_state == UI_SOUND or self._ui_state == UI_SELECT:
            draw_dot(0, 90)
            if self._ui_labels:
                draw_text(0, 75, "SOUND", inv=True)
            ctx.move_to(0, 40)
            ctx.text(str(self._synth_sound_name))

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
        elif self._ui_state == UI_SOUND:
            if self.input.buttons.app.left.pressed:
                self._set_sound(self._synth_sound_index - 1)
            if self.input.buttons.app.right.pressed:
                self._set_sound(self._synth_sound_index + 1)

        cts = captouch.read()
        cv_list = []
        for i in range(10):
            if cts.petals[i].pressed:
                slew = 12000
                p = ins.captouch.petals[i].position[0]
                p = p * abs(p) / 40000
                p -= self.mods[i].signals.input.value
                p = max(-slew, min(slew, p))
                p += self.mods[i].signals.input.value
                p = max(-32767, min(32767, p))
                self.mods[i].signals.input = p
                cv_list += [p]

            press_event = (
                cts.petals[i].pressed and not self._ui_cap_prev.petals[i].pressed
            )
            release_event = (
                not cts.petals[i].pressed and self._ui_cap_prev.petals[i].pressed
            )
            if self._ui_state == UI_SELECT:
                if press_event:
                    if i == 0:
                        self._ui_state = UI_SOUND
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
        if cv_list:
            self.mod.signals.input = sum(cv_list) / len(cv_list)


if __name__ == "__main__":
    from st3m.run import run_app

    run_app(ScalarApp, "/flash/sys/apps/scalar")
