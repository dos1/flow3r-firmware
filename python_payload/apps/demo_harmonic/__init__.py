import captouch
import bl00mbox

import leds
import errno

from st3m.goose import List
from st3m.input import InputState
from st3m.goose import Optional
from st3m.ui.view import ViewManager
from ctx import Context
import cmath
import math
import json

tai = math.tau * 1j

from st3m.application import Application, ApplicationContext


def tone_to_note_name(tone):
    # TODO: add this to radspa helpers
    sct = tone * 200 + 18367
    return bl00mbox.helpers.sct_to_note_name(sct)


def note_name_to_tone(note_name):
    # TODO: add this to radspa helpers
    sct = bl00mbox.helpers.note_name_to_sct(note_name)
    return (sct - 18367) / 200


def bottom_petal_to_rgb(petal, soft=0):
    petal = int(petal) % 5
    ret = None
    if petal == 0:
        ret = (1.0, 0.5, 0)
    elif petal == 1:
        ret = (0, 1.0, 1.0)
    elif petal == 2:
        ret = (0, 0, 1.0)
    elif petal == 3:
        ret = (1.0, 0, 1.0)
    elif petal == 4:
        ret = (0, 1.0, 0)
    if soft > 0 and soft < 1:
        return tuple([x * (1 - soft) + soft for x in ret])
    else:
        return ret


def interval_to_rgb(interval):
    interval = int(interval) % 12
    if interval == 0:  # octave: neutral, light green
        return (0.5, 1, 0.5)
    if interval == 1:  # flat 9th: spicy, purple
        return (1, 0, 1)
    if interval == 2:  # 9th: mellow, blue
        return (0, 0, 1)
    if interval == 3:  # minor 3rd: gritty warm, red
        return (1.0, 0, 0)
    if interval == 4:  # major 3rd: warm, orange
        return (1.0, 0.5, 0)
    if interval == 5:  # 4th: natural, green
        return (0, 0.9, 0.3)
    if interval == 6:  # tritone, neon yellow
        return (0.8, 1.0, 0)
    if interval == 7:  # 5th: reliable, teal
        return (0, 0.7, 0.9)
    if interval == 8:  # augmented 5th: loud, cyan
        return (0, 1.0, 1.0)
    if interval == 9:  # 13th: pink
        return (1.0, 0.6, 0.5)
    if interval == 10:  # 7th: generic, lime green
        return (0.0, 0.9, 0.3)
    if interval == 11:  # major 7th: peaceful, sky blue
        return (0.7, 0.7, 1.0)


class Chord:
    _triads = ["diminished", "minor", "major", "augmented", "sus2", "sus4"]

    def __init__(self):
        self.root = 0
        self.triad = "major"
        self.seven = False
        self.nine = False
        self.j = False
        self.voicing = 0
        self._num_voicings = 2

    def __repr__(self):
        ret = tone_to_note_name(self.root)
        while ret[-1].isdigit() or ret[-1] == "-":
            ret = ret[:-1]
        if self.triad == "augmented":
            ret += " aug. "
        elif self.triad == "diminished":
            ret += " dim. "
        elif self.triad == "minor":
            ret += "-"
        elif self.triad == "sus2":
            ret += "sus2 "
        elif self.triad == "sus4":
            ret += "sus4 "

        if self.seven:
            if self.j:
                ret += "j7 "
            else:
                ret += "7 "

        if self.nine:
            if self.nine == "#":
                ret += "#9"
            elif self.nine == "b":
                ret += "b9"
            else:
                ret += "9"

        if ret[-1] == " ":
            ret = ret[:-1]
        return ret

    def triad_incr(self):
        i = Chord._triads.index(self.triad)
        i = (i + 1) % len(Chord._triads)
        self.triad = Chord._triads[i]

    def voicing_incr(self):
        self.voicing = (self.voicing + 1) % self._num_voicings

    def nine_incr(self):
        if self.nine == "#":
            self.nine = False
        elif self.nine == "b":
            self.nine = True
        elif self.nine:
            self.nine = "#"
        else:
            self.nine = "b"

    def seven_incr(self):
        if self.seven:
            if self.j:
                self.seven = False
                self.j = False
            else:
                self.seven = True
                self.j = True
        else:
            self.seven = True
            self.j = False

    @property
    def notes(self):
        chord = [0] * 5
        if self.voicing == 0:
            chord[0] = self.root
            # TRIADE
            if self.triad == "augmented":
                chord[1] = self.root + 4
                chord[2] = self.root + 8
                chord[4] = self.root + 16
            elif self.triad == "minor":
                chord[1] = self.root + 3
                chord[2] = self.root + 7
                chord[4] = self.root + 15
            elif self.triad == "diminished":
                chord[1] = self.root + 3
                chord[2] = self.root + 6
                chord[4] = self.root + 15
            elif self.triad == "sus2":
                chord[1] = self.root + 2
                chord[2] = self.root + 7
                chord[4] = self.root + 14
            elif self.triad == "sus4":
                chord[1] = self.root + 5
                chord[2] = self.root + 7
                chord[4] = self.root + 17
            else:  # self.triad == "major":
                chord[1] = self.root + 4
                chord[2] = self.root + 7
                chord[4] = self.root + 16
            # SEVENTH
            if self.seven:
                if self.j:
                    chord[3] = self.root + 11
                else:
                    chord[3] = self.root + 10
            else:
                chord[3] = self.root + 12
            # NINETH
            if self.nine:
                if self.nine == "#":
                    chord[4] = self.root + 15
                elif self.nine == "b":
                    chord[4] = self.root + 13
                else:
                    chord[4] = self.root + 14
            else:
                if self.seven:
                    chord[4] = self.root + 12
        elif self.voicing == 1:
            # TRIADE
            if self.triad == "augmented":
                chord[0] = self.root + 4 - 12
                chord[1] = self.root + 8 - 12
                chord[4] = self.root + 16
            elif self.triad == "minor":
                chord[0] = self.root + 3 - 12
                chord[1] = self.root + 7 - 12
                chord[4] = self.root + 15
            elif self.triad == "diminished":
                chord[0] = self.root + 3 - 12
                chord[1] = self.root + 6 - 12
                chord[4] = self.root + 15
            else:  # self.triad == "major":
                chord[0] = self.root + 4 - 12
                chord[1] = self.root + 7 - 12
                chord[4] = self.root + 16
            # SEVENTH
            if self.seven:
                if self.j:
                    chord[2] = self.root + 11 - 12
                    chord[3] = self.root
                    chord[4] = chord[0] + 12
                else:
                    chord[2] = self.root + 10 - 12
                    chord[3] = self.root
                    chord[4] = chord[0] + 12
            else:
                chord[2] = self.root
                chord[3] = chord[0] + 12
                chord[4] = chord[1] + 12
            # NINETH
            if self.nine:
                if self.nine == "#":
                    chord[3] = self.root + 15 - 12
                elif self.nine == "b":
                    chord[3] = self.root + 13 - 12
                else:
                    chord[3] = self.root + 14 - 12
        return chord


class HarmonicApp(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)

        self.chord_index = 0
        self.chord = None
        self.chords = [Chord() for x in range(5)]

        self.cp_prev = captouch.read()
        self.blm = None
        self.prev_captouch = [0] * 10
        self.fade = [0] * 5
        self.mode = 0
        self._set_chord(3, force_update=True)
        self._num_modes = 3

        self._file_settings = None

    def _try_load_settings(self, path):
        try:
            with open(path, "r") as f:
                return json.load(f)
        except OSError as e:
            if e.errno != errno.ENOENT:
                raise  # ignore file not found

    def _try_write_default_settings(self, path: str, default_path: str) -> None:
        with open(path, "w") as outfile, open(default_path, "r") as infile:
            outfile.write(infile.read())

    def _try_save_settings(self, path, settings):
        try:
            with open(path, "w+") as f:
                f.write(json.dumps(settings))
                f.close()
        except OSError as e:
            if e.errno != errno.ENOENT:
                raise  # ignore file not found

    def _save_settings(self):
        default_path = self._app_ctx.bundle_path + "/harmonic_demo-default.json"
        settings_path = "/flash/harmonic_demo.json"
        settings = self._try_load_settings(default_path)
        assert settings is not None, "failed to load default settings"

        file_is_different = False
        user_settings = self._try_load_settings(settings_path)
        if user_settings is None:
            file_is_different = True

        for i, chord in enumerate(settings["chords"]):
            chord["triad"] = self.chords[i].triad
            chord["j"] = self.chords[i].j
            chord["seven"] = self.chords[i].seven
            chord["nine"] = self.chords[i].nine
            chord["root"] = self.chords[i].root
            chord["voicing"] = self.chords[i].voicing
            chord["tones_readonly"] = self.chords[i].notes
            if not file_is_different:
                user_chord = user_settings["chords"][i]
                if self._file_settings is None:
                    file_is_different = True
                else:
                    file_chord = self._file_settings["chords"][i]
                    for i in chord:
                        if chord.get(i) != file_chord.get(i):
                            file_is_different = True
                            break

        if file_is_different:
            self._try_save_settings(settings_path, settings)
            self._file_settings = settings

    def _load_settings(self):
        default_path = self._app_ctx.bundle_path + "/harmonic_demo-default.json"
        settings_path = "/flash/harmonic_demo.json"

        settings = self._try_load_settings(default_path)
        assert settings is not None, "failed to load default settings"

        user_settings = self._try_load_settings(settings_path)
        if user_settings is None:
            self._try_write_default_settings(settings_path, default_path)
        else:
            settings.update(user_settings)
            self._file_settings = settings

        for i, chord in enumerate(settings["chords"]):
            self.chords[i].triad = chord["triad"]
            self.chords[i].j = chord["j"]
            self.chords[i].seven = chord["seven"]
            self.chords[i].nine = chord["nine"]
            self.chords[i].root = chord["root"]
            self.chords[i].voicing = chord["voicing"]

    def _build_synth(self):
        if self.blm is None:
            self.blm = bl00mbox.Channel("harmonic demo")
        self.synths = [self.blm.new(bl00mbox.patches.tinysynth) for i in range(5)]
        self.mixer = self.blm.new(bl00mbox.plugins.mixer, 5)
        self.lp = self.blm.new(bl00mbox.plugins.lowpass)
        for i, synth in enumerate(self.synths):
            synth.signals.decay = 500
            synth.signals.waveform = 0
            synth.signals.attack = 50
            synth.signals.volume = 0.3 * 32767
            synth.signals.sustain = 0.9 * 32767
            synth.signals.release = 800
            # synth.signals.output = self.blm.mixer
        # sorry gonna fix that soon iou xoxo
        self.synths[0].signals.output = self.mixer.signals.input0
        self.synths[1].signals.output = self.mixer.signals.input1
        self.synths[2].signals.output = self.mixer.signals.input2
        self.synths[3].signals.output = self.mixer.signals.input3
        self.synths[4].signals.output = self.mixer.signals.input4

        self.mixer.signals.output = self.lp.signals.input
        self.lp.signals.output = self.blm.mixer
        self.lp.signals.freq = 4000
        self.lp.signals.reso = 2000
        self.lp.signals.gain.dB = +3

    def _set_chord(self, i, force_update=False):
        hue = int(72 * (i + 0.5)) % 360
        if i != self.chord_index or force_update:
            self.chord_index = i
            leds.set_all_rgb(*bottom_petal_to_rgb(self.chord_index, soft=0))
            leds.update()
            self.chord = self.chords[i]

    def draw(self, ctx: Context) -> None:
        if self.blm is None:
            return
        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
        ctx.line_width = 4
        ctx.get_font_name(4)
        ctx.text_align = ctx.CENTER

        ctx.font_size = 20

        for top_petal in range(5):
            note_name = tone_to_note_name(self.chord.notes[top_petal])
            interval = self.chord.notes[top_petal] - self.chord.root
            color = interval_to_rgb(interval)
            ctx.rgb(*color)
            note_name = "".join([x for x in note_name if not x.isdigit()])
            pos = 95 * cmath.exp(tai * (top_petal - 2.5) / 5)
            end_pos = pos * (1 - 0.3j) * 1.27
            start_pos = pos * (1 + 0.3j) * 1.27
            mid_pos = (start_pos + end_pos) / 8
            fade = self.fade[top_petal]
            if fade > 0:
                ctx.rgb(fade / 4, 0, fade / 4)
                ctx.move_to(start_pos.imag, start_pos.real)
                ctx.quad_to(mid_pos.imag, mid_pos.real, end_pos.imag, end_pos.real)
                ctx.fill()
            ctx.rgb(*color)
            ctx.move_to(start_pos.imag, start_pos.real)
            ctx.quad_to(mid_pos.imag, mid_pos.real, end_pos.imag, end_pos.real)
            ctx.stroke()
            ctx.move_to(pos.imag * 1.07, pos.real * 1.07)
            ctx.text(note_name)

        if self.mode == 0:
            ctx.save()
            ctx.font_size = 25
            ctx.rotate(math.tau * (3 / 5 - 1 / 4))
            pos = -105
            ctx.text_align = ctx.LEFT
            for bottom_petal in range(5):
                chord = self.chords[bottom_petal]
                ctx.move_to(0, 0)
                if bottom_petal == 3:
                    pos = -pos
                    ctx.text_align = ctx.RIGHT
                    ctx.rotate(-math.tau / 2)
                ctx.rotate(-math.tau / 5)
                if bottom_petal != self.chord_index:
                    ctx.rgb(*bottom_petal_to_rgb(bottom_petal, soft=0.3))
                    text = self.chords[bottom_petal].__repr__()
                    shift = 1
                    if len(text) > 8:
                        shift += 0.05 * (len(text) - 8)
                    if abs(pos * shift) > 120:
                        ctx.font_size = 20
                        shift = abs(120 / pos)
                    ctx.move_to(pos * shift, 0)
                    ctx.text(text)
                    ctx.font_size = 25

            ctx.restore()

            ctx.save()
            ctx.font_size = 35
            ctx.rotate(math.tau * (3 / 5 - 1 / 4))
            pos = -90
            ctx.text_align = ctx.LEFT
            for bottom_petal in range(5):
                # lazy
                chord = self.chords[bottom_petal]
                ctx.move_to(0, 0)
                if bottom_petal == 3:
                    pos = -pos
                    ctx.text_align = ctx.RIGHT
                    ctx.rotate(-math.tau / 2)
                ctx.rotate(-math.tau / 5)
                if bottom_petal == self.chord_index:
                    ctx.rgb(*bottom_petal_to_rgb(bottom_petal))
                    text = self.chords[bottom_petal].__repr__()
                    shift = 1
                    if len(text) > 8:
                        shift += 0.05 * (len(text) - 8)
                    if abs(pos * shift) > 120:
                        shift = abs(120 / pos)
                        ctx.font_size = 30
                    ctx.move_to(pos * shift, 0)
                    ctx.text(text)
                    ctx.font_size = 35

            ctx.restore()

        if self.mode == 1:
            ctx.font_size = 25
            ctx.rgb(*bottom_petal_to_rgb(self.chord_index, soft=0.15))
            ctx.save()
            ctx.rotate(math.tau * (3 / 5 - 1 / 4))
            pos = -105
            ctx.text_align = ctx.LEFT
            for bottom_petal in range(5):
                ctx.move_to(0, 0)
                if bottom_petal == 3:
                    ctx.text_align = ctx.RIGHT
                    pos = -pos
                    ctx.rotate(-math.tau / 2)
                ctx.rotate(-math.tau / 5)
                ctx.move_to(pos, 0)
                if bottom_petal == 0:
                    ctx.text("oct+")
                if bottom_petal == 4:
                    ctx.text("tone+")
                if bottom_petal == 3:
                    ctx.text("tone-")
                if bottom_petal == 1:
                    ctx.text("oct-")
            ctx.restore()
            ctx.move_to(0, 0)
            ctx.font_size = 35
            ctx.text(tone_to_note_name(self.chords[self.chord_index].root))

        if self.mode == 2:
            ctx.font_size = 20
            ctx.rgb(*bottom_petal_to_rgb(self.chord_index, soft=0.15))
            chord = self.chords[self.chord_index]
            ctx.save()
            ctx.rotate(math.tau * (3 / 5 - 1 / 4))
            pos = -105
            ctx.text_align = ctx.LEFT
            for bottom_petal in range(5):
                ctx.move_to(0, 0)
                if bottom_petal == 3:
                    ctx.text_align = ctx.RIGHT
                    ctx.rotate(-math.tau / 2)
                    pos = -pos
                ctx.rotate(-math.tau / 5)
                ctx.move_to(pos, 0)
                if bottom_petal == 4:
                    ctx.move_to(pos * 1.05, 0)
                    ctx.text("voice " + str(chord.voicing))
                if bottom_petal == 0:
                    if chord.nine == "#":
                        ctx.text("#9")
                    elif chord.nine == "b":
                        ctx.text("b9")
                    elif chord.nine:
                        ctx.text("9")
                    else:
                        ctx.text("no9")
                if bottom_petal == 1:
                    if chord.seven:
                        if chord.j:
                            ctx.text("j7")
                        else:
                            ctx.text("7")
                    else:
                        ctx.text("no7")
                if bottom_petal == 3:
                    if chord.triad == "diminished":
                        ctx.text("dim.")
                    elif chord.triad == "augmented":
                        ctx.text("aug.")
                    else:
                        ctx.text(chord.triad)

            ctx.restore()
            ctx.move_to(0, 0)
            text = self.chords[self.chord_index].__repr__()
            if len(text) > 9:
                ctx.font_size = 30
            else:
                ctx.font_size = 35
            ctx.text(text)

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        if self.blm is None:
            return

        if self.input.buttons.app.left.pressed:
            self.mode = (self.mode - 1) % self._num_modes
        elif self.input.buttons.app.right.pressed:
            self.mode = (self.mode + 1) % self._num_modes

        cp = captouch.read()

        for i in range(0, 10, 2):
            j = (10 - i) % 10
            if cp.petals[j].pressed:
                if not self.cp_prev.petals[j].pressed:
                    self.synths[i // 2].signals.pitch.tone = self.chord.notes[i // 2]
                    self.synths[i // 2].signals.trigger.start()
                    self.fade[i // 2] = 1
            else:
                if self.fade[i // 2] > 0:
                    self.fade[i // 2] -= self.fade[i // 2] * float(delta_ms) / 1000
                    if self.fade[i // 2] < 0.05:
                        self.fade[i // 2] = 0
                if self.cp_prev.petals[j].pressed:
                    self.synths[i // 2].signals.trigger.stop()

        for j in range(5):
            if cp.petals[1 + 2 * j].pressed:
                if not self.cp_prev.petals[1 + 2 * j].pressed:
                    if self.mode == 0:
                        self._set_chord((4 - j) % 5)
                    elif self.mode == 1:
                        if j == 0:
                            self.chord.root += 1
                        elif j == 1:
                            self.chord.root -= 1
                        elif j == 3:
                            self.chord.root -= 12
                        elif j == 4:
                            self.chord.root += 12
                    elif self.mode == 2:
                        if j == 3:
                            self.chord.seven_incr()
                        elif j == 4:
                            self.chord.nine_incr()
                        elif j == 1:
                            self.chord.triad_incr()
                        elif j == 0:
                            self.chord.voicing_incr()

        self.cp_prev = cp

    def on_enter(self, vm: Optional[ViewManager]) -> None:
        super().on_enter(vm)
        self.mode = 0
        if self.blm is None:
            self._build_synth()
        self.blm.foreground = True
        self._load_settings()
        leds.set_slew_rate(130)
        self._set_chord(self.chord_index, force_update=True)

    def on_exit(self):
        if self.blm is not None:
            self.blm.free = True
        self.blm = None
        self._save_settings()


if __name__ == "__main__":
    from st3m.run import run_app

    run_app(HarmonicApp, "/flash/sys/apps/demo_harmonic")
