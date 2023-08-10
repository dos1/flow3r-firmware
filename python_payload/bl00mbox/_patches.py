# SPDX-License-Identifier: CC0-1.0
import math

import bl00mbox

try:
    import cpython.wave as wave
except ImportError:
    wave = None


class _Patch:
    def bl00mbox_patch_marker(self):
        return True


class _PatchSignalList:
    pass


class _PatchBudList:
    pass


class tinysynth(_Patch):
    def __init__(self, chan):
        self.channel = chan
        self.SINE = -32767
        self.TRI = -1
        self.SQUARE = 1
        self.SAW = 32767

        self.osc = chan.new_bud(420)
        self.env = chan.new_bud(42)
        self.amp = chan.new_bud(69)
        self.amp.signals.output.value = chan.mixer
        self.amp.signals.gain.value = self.env.signals.output
        self.amp.signals.input.value = self.osc.signals.output
        self.env.signals.sustain.value = 0
        self.env.signals.decay.value = 500
        self.release(100)

    def __repr__(self):
        ret = "[patch] tinysynth"
        ret += "\n  " + "\n  ".join(repr(self.osc).split("\n"))
        ret += "\n  " + "\n  ".join(repr(self.env).split("\n"))
        ret += "\n  " + "\n  ".join(repr(self.amp).split("\n"))
        return ret

    def tone(self, val):
        self.osc.signals.pitch.tone = val

    def freq(self, val):
        self.osc.signals.pitch.freq = val

    def volume(self, val):
        self.env.signals.input.value = 32767 * val

    def start(self):
        self.env.signals.trigger.start()

    def stop(self):
        self.env.signals.trigger.stop()

    def waveform(self, val):
        self.osc.signals.waveform.value = val

    def attack(self, val):
        self.env.signals.attack.value = val

    def decay(self, val):
        self.env.signals.decay.value = val

    def sustain(self, val):
        self.env.signals.sustain.value = val * 32767

    def release(self, val):
        self.env.signals.release.value = val


class tinysynth_fm(tinysynth):
    def __init__(self, chan):
        tinysynth.__init__(self, chan)
        self.mod_osc = chan.new_bud(420)
        self.fm_mult = 2.5
        self.mod_osc.signals.output.value = self.osc.signals.lin_fm
        self.decay(1000)
        self.attack(20)
        self._update_mod_osc()
        self.fm_waveform(self.SQUARE)
        self.waveform(self.TRI)

    def fm_waveform(self, val):
        self.mod_osc.signals.waveform.value = val

    def __repr__(self):
        ret = tinysynth.__repr__(self)
        ret = ret.split("\n")
        ret[0] += "_fm"
        ret = "\n".join(ret)
        ret += "\n  " + "\n  ".join(repr(self.mod_osc).split("\n"))
        return ret

    def fm(self, val):
        self.fm_mult = val
        self._update_mod_osc()

    def tone(self, val):
        self.osc.signals.pitch.tone = val
        self._update_mod_osc()

    def freq(self, val):
        self.osc.signals.pitch.freq = val
        self._update_mod_osc()

    def _update_mod_osc(self):
        self.mod_osc.signals.pitch.freq = self.fm_mult * self.osc.signals.pitch.freq


class sampler(_Patch):
    """needs a wave file with path relative to samples/"""

    def __init__(self, chan, filename):
        if wave is None:
            pass
            # raise Bl00mboxError("wave library not found")
        f = wave.open("/flash/sys/samples/" + filename, "r")
        len_frames = f.getnframes()
        self.sampler = chan.new_bud(696969, len_frames)
        table = [0] * len_frames
        for i in range(len_frames):
            frame = f.readframes(1)
            value = int.from_bytes(frame[0:2], "little")
            table[i] = value
        f.close()
        self._filename = filename
        self.sampler.table = table
        self.sampler.signals.output = chan.mixer

    def start(self):
        self.sampler.signals.trigger.start()

    def stop(self):
        self.sampler.signals.trigger.stop()

    @property
    def filename(self):
        return self._filename


class step_sequencer(_Patch):
    def __init__(self, chan):
        self.seqs = []
        for i in range(4):
            seq = chan.new_bud(56709)
            seq.table = [-32767] + ([0] * 16)
            if len(self.seqs):
                self.seqs[-1].signals.sync_out = seq.signals.sync_in
            self.seqs += [seq]
        self._bpm = 120

    def __repr__(self):
        ret = "[patch] step sequencer"
        # ret += "\n  " + "\n  ".join(repr(self.seqs[0]).split("\n"))
        ret += (
            "\n  bpm: "
            + str(self.seqs[0].signals.bpm.value)
            + " @ 1/"
            + str(self.seqs[0].signals.beat_div.value)
        )
        ret += (
            "\n  step: "
            + str(self.seqs[0].signals.step.value)
            + "/"
            + str(self.seqs[0].signals.step_len.value)
        )
        ret += "\n  [tracks]"
        for x, seq in enumerate(self.seqs):
            ret += (
                "\n    "
                + str(x)
                + " [  "
                + "".join(["X  " if x > 0 else ".  " for x in seq.table[1:]])
                + "]"
            )
        return ret

    @property
    def bpm(self):
        return self._bpm

    @bpm.setter
    def bpm(self, bpm):
        for seq in self.seqs:
            seq.signals.bpm.value = bpm
        self._bpm = bpm

    def trigger_start(self, track, step):
        a = self.seqs[track].table
        a[step + 1] = 32767
        self.seqs[track].table = a

    def trigger_stop(self, track, step):
        a = self.seqs[track].table
        a[step + 1] = 0
        self.seqs[track].table = a

    def trigger_state(self, track, step):
        a = self.seqs[track].table
        return a[step + 1]

    def trigger_toggle(self, track, step):
        if self.trigger_state(track, step) == 0:
            self.trigger_start(track, step)
        else:
            self.trigger_stop(track, step)

    @property
    def step(self):
        return self.seqs[0].signals.step


class fuzz(_Patch):
    def __init__(self, chan):
        self.buds = _PatchBudList()
        self.signals = _PatchSignalList()
        self.buds.dist = chan.new(bl00mbox.plugins.distortion)
        self.signals.input = self.buds.dist.signals.input
        self.signals.output = self.buds.dist.signals.output
        self._intensity = 2
        self._volume = 32767
        self._gate = 0

    @property
    def intensity(self):
        return self._intensity

    @intensity.setter
    def intensity(self, val):
        self._intensity = val
        self._update_table()

    @property
    def volume(self):
        return self._volume

    @volume.setter
    def volume(self, val):
        self._volume = val
        self._update_table()

    @property
    def gate(self):
        return self._gate

    @gate.setter
    def gate(self, val):
        self._gate = val
        self._update_table()

    def _update_table(self):
        table = list(range(129))
        for num in table:
            if num < 64:
                ret = num / 64  # scale to [0..1[ range
                ret = ret**self._intensity
                if ret > 1:
                    ret = 1
                table[num] = int(self._volume * (ret - 1))
            else:
                ret = (128 - num) / 64  # scale to [0..1] range
                ret = ret**self._intensity
                table[num] = int(self._volume * (1 - ret))
        gate = self.gate >> 9
        for i in range(64 - gate, 64 + gate):
            table[i] = 0
        self.buds.dist.table = table


class karplus_strong(_Patch):
    def __init__(self, chan):
        self.buds = _PatchBudList()
        self.signals = _PatchSignalList()
        self.buds.noise = chan.new_bud(bl00mbox.plugins.noise)
        self.buds.flanger = chan.new_bud(bl00mbox.plugins.flanger)
        self.buds.amp = chan.new_bud(bl00mbox.plugins.ampliverter)
        self.buds.env = chan.new_bud(bl00mbox.plugins.env_adsr)
        # self.buds.slew = chan.new_bud(bl00mbox.plugins.slew_rate_limiter)

        self.buds.flanger.signals.resonance = 32500
        self.buds.flanger.signals.manual.tone = "A2"

        self.buds.env.signals.sustain = 0
        self.buds.env.signals.decay = 20
        self.buds.env.signals.attack = 5

        self.buds.amp.signals.output = self.buds.flanger.signals.input
        self.buds.amp.signals.input = self.buds.noise.signals.output
        # self.buds.amp.signals.input = self.buds.slew.signals.output
        # self.buds.slew.signals.input = self.buds.noise.signals.output
        # self.buds.slew.signals.slew_rate = 10000
        self.buds.amp.signals.gain = self.buds.env.signals.output

        self.signals.trigger = self.buds.env.signals.trigger
        self.signals.pitch = self.buds.flanger.signals.manual
        self.signals.output = self.buds.flanger.signals.output
        # self.signals.slew_rate = self.buds.slew.signals.slew_rate
        self.signals.level = self.buds.flanger.signals.level
        self.decay = 1000

    @property
    def decay(self):
        return self._decay

    @decay.setter
    def decay(self, val):
        tone = self.buds.flanger.signals.manual.tone
        loss = (50 * (2 ** (-tone / 12))) // (val / 1000)
        if loss < 2:
            loss = 2
        self.buds.flanger.signals.resonance = 32767 - loss
        self._decay = val
