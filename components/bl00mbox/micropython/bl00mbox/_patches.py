# SPDX-License-Identifier: CC0-1.0
import math
import os
import bl00mbox
import cpython.wave as wave


class _Patch:
    def __init__(self, chan):
        self.plugins = _PatchPluginList()
        self.signals = _PatchSignalList()
        self._channel = chan

    def __repr__(self):
        ret = "[patch] " + type(self).__name__
        ret += "\n  [signals:]    " + "\n    ".join(repr(self.signals).split("\n"))
        ret += "\n  [plugins:]    " + "\n    ".join(repr(self.plugins).split("\n"))
        return ret


class _PatchItemList:
    def __init__(self):
        self._items = []

    def __iter__(self):
        return iter(self._items)

    def __repr__(self):
        ret = ""
        for x in self._items:
            a = ("\n" + repr(getattr(self, x))).split("]")
            a[0] += ": " + x
            ret += "]".join(a)
        return ret

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
        self.signals.volume = self.plugins.env.signals.input
        self.signals.release = 100


class tinysynth_fm(tinysynth):
    def __init__(self, chan):
        super().__init__(chan)
        self.plugins.mod_osc = self._channel.new(bl00mbox.plugins.osc_fm)
        self.plugins.mult = self._channel.new(bl00mbox.plugins.multipitch, 1)
        self.plugins.mod_osc.signals.output = self.plugins.osc.signals.lin_fm
        self.plugins.mod_osc.signals.pitch = self.plugins.mult.signals.output0
        self.plugins.osc.signals.pitch = self.plugins.mult.signals.thru

        self.signals.fm_waveform = self.plugins.mod_osc.signals.waveform
        self.signals.fm = self.plugins.mult.signals.shift0
        self.signals.pitch = self.plugins.mult.signals.input
        self.signals.decay = 1000
        self.signals.attack = 20
        self.signals.waveform = -1
        self.signals.fm_waveform = 0

        self.signals.fm.tone = 3173 / 200


class sampler(_Patch):
    """
    requires a wave file (str) or max sample length in milliseconds (int). default path: /sys/samples/
    """

    # DEPRECATED
    _READ_HEAD_POS = 0 // 2
    _WRITE_HEAD_POS = 2 // 2
    _SAMPLE_START = 4 // 2
    _SAMPLE_LEN = 6 // 2
    _SAMPLE_RATE = 8 // 2
    _STATUS = 10
    _BUFFER = 11

    def __init__(self, chan, init_var):
        # init can be filename to load into ram
        super().__init__(chan)
        self.buffer_offset_i16 = 11
        self._filename = ""
        if type(init_var) == str:
            self.plugins.sampler = chan.new(
                bl00mbox.plugins.sampler, self._convert_filename(init_var)
            )
        else:
            self.plugins.sampler = chan.new(bl00mbox.plugins.sampler, init_var)

        self.signals.trigger = self.plugins.sampler.signals.playback_trigger
        self.signals.output = self.plugins.sampler.signals.playback_output
        self.signals.rec_in = self.plugins.sampler.signals.record_input
        self.signals.rec_trigger = self.plugins.sampler.signals.record_trigger

        # terrible backwards compatibility hack, never do that IRL
        del self.plugins.sampler.signals._setattr_allowed
        self.plugins.sampler.signals.pitch_shift = (
            self.plugins.sampler.signals.playback_speed
        )
        self.plugins.sampler.signals._setattr_allowed = True

    def _convert_filename(self, filename):
        # TODO: waht if filename doesn't exist?
        if filename.startswith("/flash/") or filename.startswith("/sd/"):
            return filename
        elif filename.startswith("/"):
            return "/flash/" + filename
        else:
            return "/flash/sys/samples/" + filename

    def load(self, filename):
        filename = self._convert_filename(filename)
        try:
            self.plugins.sampler.load(filename)
        except:
            # no proper exception catching bc backwards compat
            return False
        return True

    def save(self, filename, overwrite=False):
        filename = self._convert_filename(filename)
        try:
            self.plugins.sampler.save(filename)
        except:
            # no proper exception catching bc backwards compat
            return False
        return True

    def _offset_index(self, index):
        index += self.sample_start
        if index >= self.memory_len:
            index -= self.memory_len
        index += self.buffer_offset_i16
        return index

    @property
    def memory_len(self):
        return self.plugins.sampler.buffer_length

    @property
    def read_head_position(self):
        table = self.plugins.sampler.table_uint32_array
        return table[self._READ_HEAD_POS]

    @read_head_position.setter
    def read_head_position(self, val):
        if val >= self.memory_len:
            val = self.memory_len - 1
        table = self.plugins.sampler.table_uint32_array
        table[self._READ_HEAD_POS] = int(val)

    @property
    def sample_start(self):
        table = self.plugins.sampler.table_uint32_array
        return table[self._SAMPLE_START]

    @sample_start.setter
    def sample_start(self, val):
        if val >= self.memory_len:
            val = self.memory_len - 1
        table = self.plugins.sampler.table_uint32_array
        table[self._SAMPLE_START] = int(val)

    @property
    def sample_len(self):
        table = self.plugins.sampler.table_uint32_array
        return table[self._SAMPLE_LEN]

    @sample_len.setter
    def sample_len(self, val):
        if val > self.memory_len:
            val = self.memory_len
        table = self.plugins.sampler.table_uint32_array
        table[self._SAMPLE_LEN] = int(val)

    @property
    def sample_rate(self):
        table = self.plugins.sampler.table_uint32_array
        return table[self._SAMPLE_RATE]

    @sample_rate.setter
    def sample_rate(self, val):
        table = self.plugins.sampler.table_uint32_array
        if val < 0:
            table[self._SAMPLE_RATE] = int(val)

    @property
    def filename(self):
        return self._filename

    @property
    def rec_event_autoclear(self):
        """
        returns true once after a record cycle has been completed. useful for checking whether a save is necessary if nothing else has modified the table.
        """
        a = self.plugins.sampler_table[10]
        if a & (1 << 4) & 0xFF:
            a = a & ~(1 << 4) & 0xFF
            self.plugins.sampler_table[10] = a
            return True
        return False


class sequencer(_Patch):
    def __init__(self, chan, num_tracks, num_steps):
        super().__init__(chan)
        self.num_steps = num_steps
        self.num_tracks = num_tracks
        init_var = (self.num_steps * 256) + (self.num_tracks)  # magic
        self.plugins.seq = chan.new(bl00mbox.plugins.sequencer, init_var)

        self.signals.bpm = self.plugins.seq.signals.bpm
        self.signals.beat_div = self.plugins.seq.signals.beat_div
        self.signals.step = self.plugins.seq.signals.step
        self.signals.step_end = self.plugins.seq.signals.step_end
        self.signals.step_start = self.plugins.seq.signals.step_start
        self.signals.sync_in = self.plugins.seq.signals.sync_in

        tracktable = [-32767] + ([0] * self.num_steps)
        self.plugins.seq.table = tracktable * self.num_tracks

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
            + str(self.signals.step.value - self.signals.step_start.value)
            + "/"
            + str(self.signals.step_end.value - self.signals.step_start.value)
        )
        ret += "\n  [tracks]"
        """
        for x, seq in enumerate(self.seqs):
            ret += (
                "\n    "
                + str(x)
                + " [  "
                + "".join(["X  " if x > 0 else ".  " for x in seq.table[1:]])
                + "]"
            )
        """
        ret += "\n" + "\n".join(super().__repr__().split("\n")[1:])
        return ret

    def _get_table_index(self, track, step):
        return step + 1 + track * (self.num_steps + 1)

    def trigger_start(self, track, step, val=32767):
        a = self.plugins.seq.table
        a[self._get_table_index(track, step)] = val
        self.plugins.seq.table = a

    def trigger_stop(self, track, step, val=32767):
        a = self.plugins.seq.table
        a[self._get_table_index(track, step)] = -1
        self.plugins.seq.table = a

    def trigger_clear(self, track, step):
        a = self.plugins.seq.table
        a[self._get_table_index(track, step)] = 0
        self.plugins.seq.table = a

    def trigger_state(self, track, step):
        a = self.plugins.seq.table
        return a[self._get_table_index(track, step)]

    def trigger_toggle(self, track, step):
        if self.trigger_state(track, step) == 0:
            self.trigger_start(track, step)
        else:
            self.trigger_clear(track, step)


class fuzz(_Patch):
    # DEPRECATED
    def __init__(self, chan):
        super().__init__(chan)
        self.plugins.dist = chan.new(bl00mbox.plugins._distortion)
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
        self.signals.decay = self.plugins.flanger.signals.decay
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
