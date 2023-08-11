# SPDX-License-Identifier: CC0-1.0
import math

import bl00mbox

try:
    import cpython.wave as wave
except ImportError:
    wave = None


class _Patch:
    def __init__(self, chan):
        self.plugins = _PatchPluginList()
        self.signals = _PatchSignalList()
        self._channel = chan

    def __repr__(self):
        ret = "[patch] " + type(self).__name__
        ret += "\n  [signals:]\n    " + "\n    ".join(repr(self.signals).split("\n"))
        ret += "\n  [plugins:]\n    " + "\n    ".join(repr(self.plugins).split("\n"))
        return ret


class _PatchItemList:
    def __init__(self):
        self._items = []

    def __iter__(self):
        return iter(self._items)

    def __repr__(self):
        return "\n".join([x + ": " + repr(getattr(self, x)) for x in self._items])

    def __setattr__(self, key, value):
        current_value = getattr(self, key, None)

        if current_value is None and not key.startswith("_"):
            self._items.append(key)

        super().__setattr__(key, value)


class _PatchSignalList(_PatchItemList):
    def __setattr__(self, key, value):
        current_value = getattr(self, key, None)
        if isinstance(current_value, bl00mbox.Signal):
            current_value.value = value
            return
        super().__setattr__(key, value)


class _PatchPluginList(_PatchItemList):
    pass


class tinysynth(_Patch):
    def __init__(self, chan):
        super().__init__(chan)
        self.plugins.osc = self._channel.new(bl00mbox.plugins.osc_fm)
        self.plugins.env = self._channel.new(bl00mbox.plugins.env_adsr)
        self.plugins.amp = self._channel.new(bl00mbox.plugins.ampliverter)

        self.plugins.amp.signals.gain = self.plugins.env.signals.output
        self.plugins.amp.signals.input = self.plugins.osc.signals.output
        self.plugins.env.signals.decay = 500

        self.signals.output = self.plugins.amp.signals.output
        self.signals.pitch = self.plugins.osc.signals.pitch
        self.signals.waveform = self.plugins.osc.signals.waveform

        self.signals.trigger = self.plugins.env.signals.trigger
        self.signals.attack = self.plugins.env.signals.attack
        self.signals.sustain = self.plugins.env.signals.sustain
        self.signals.decay = self.plugins.env.signals.decay
        self.signals.release = self.plugins.env.signals.release
        self.signals.volume = self.plugins.env.signals.input.value
        self.signals.release = 100


class tinysynth_fm(tinysynth):
    def __init__(self, chan):
        super().__init__(chan)
        self.plugins.mod_osc = self._channel.new(bl00mbox.plugins.osc_fm)
        self.plugins.mod_osc.signals.output = self.plugins.osc.signals.lin_fm
        self.signals.fm_waveform = self.plugins.mod_osc.signals.waveform
        self.signals.fm_pitch = self.plugins.mod_osc.signals.pitch
        self.signals.decay = 1000
        self.signals.attack = 20
        self.signals.waveform = -1
        self.signals.fm_waveform = 0

        self.sync_mod_osc(2.5)

    def sync_mod_osc(self, val):
        self.signals.fm_pitch.freq = int(val) * self.signals.pitch.freq


class sampler(_Patch):
    """
    needs a wave file with path relative to /flash/sys/samples/
    """

    def __init__(self, chan, filename):
        super().__init__(chan)
        if wave is None:
            pass
            # raise Bl00mboxError("wave library not found")
        f = wave.open("/flash/sys/samples/" + filename, "r")

        self.len_frames = f.getnframes()
        self.plugins.sampler = chan._new_plugin(696969, self.len_frames)

        assert f.getsampwidth() == 2
        assert f.getnchannels() in (1, 2)
        assert f.getcomptype() == "NONE"

        if f.getnchannels() == 1:
            # fast path for mono
            table = self.plugins.sampler.table_bytearray
            for i in range(0, self.len_frames * 2, 100):
                table[i : i + 100] = f.readframes(50)
        else:
            # somewhat fast path for stereo
            table = self.plugins.sampler.table_int16_array
            for i in range(self.len_frames):
                frame = f.readframes(1)
                value = int.from_bytes(frame[0:2], "little")
                table[i] = value

        f.close()
        self._filename = filename
        self.signals.trigger = self.plugins.sampler.signals.trigger
        self.signals.output = self.plugins.sampler.signals.output

    @property
    def filename(self):
        return self._filename


class step_sequencer(_Patch):
    def __init__(self, chan, num=4):
        super().__init__(chan)
        if num > 32:
            num = 32
        if num < 0:
            num = 0
        self.seqs = []
        prev_seq = None
        for i in range(num):
            seq = chan._new_plugin(56709)
            seq.table = [-32767] + ([0] * 16)
            if prev_seq is None:
                self.signals.bpm = seq.signals.bpm
                self.signals.beat_div = seq.signals.beat_div
                self.signals.step = seq.signals.step
                self.signals.step_len = seq.signals.step_len
            else:
                prev_seq.signals.sync_out = seq.signals.sync_in
            prev_seq = seq
            self.seqs += [seq]
            setattr(self.plugins, "sequencer" + str(i), seq)

    def __repr__(self):
        ret = "[patch] step sequencer"
        # ret += "\n  " + "\n  ".join(repr(self.seqs[0]).split("\n"))
        ret += (
            "\n  bpm: "
            + str(self.signals.bpm.value)
            + " @ 1/"
            + str(self.signals.beat_div.value)
        )
        ret += (
            " step: "
            + str(self.signals.step.value)
            + "/"
            + str(self.signals.step_len.value)
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
        ret += "\n" + "\n".join(super().__repr__().split("\n")[1:])
        return ret

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


class fuzz(_Patch):
    def __init__(self, chan):
        super().__init__(chan)
        self.plugins.dist = chan.new(bl00mbox.plugins.distortion)
        self.signals.input = self.plugins.dist.signals.input
        self.signals.output = self.plugins.dist.signals.output
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
        self.plugins.dist.table = table


class karplus_strong(_Patch):
    def __init__(self, chan):
        super().__init__(chan)
        self.plugins.noise = chan._new_plugin(bl00mbox.plugins.noise_burst)
        self.plugins.noise.signals.length = 25

        self.plugins.flanger = chan._new_plugin(bl00mbox.plugins.flanger)

        self.plugins.flanger.signals.resonance = 32500
        self.plugins.flanger.signals.manual.tone = "A2"

        self.plugins.flanger.signals.input = self.plugins.noise.signals.output

        self.signals.trigger = self.plugins.noise.signals.trigger
        self.signals.pitch = self.plugins.flanger.signals.manual
        self.signals.output = self.plugins.flanger.signals.output

        self.signals.level = self.plugins.flanger.signals.level
        self.decay = 1000

    @property
    def decay(self):
        return self._decay

    @decay.setter
    def decay(self, val):
        tone = self.plugins.flanger.signals.manual.tone
        loss = (50 * (2 ** (-tone / 12))) // (val / 1000)
        if loss < 2:
            loss = 2
        self.plugins.flanger.signals.resonance = 32767 - loss
        self._decay = val
