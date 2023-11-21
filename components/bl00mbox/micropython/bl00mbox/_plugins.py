# SPDX-License-Identifier: CC0-1.0

import sys_bl00mbox
import bl00mbox
import uctypes
import os
import cpython.wave as wave


class _PluginDescriptor:
    def __init__(self, index):
        self.index = index
        self.plugin_id = sys_bl00mbox.plugin_index_get_id(self.index)
        self.name = sys_bl00mbox.plugin_index_get_name(self.index)
        self.description = sys_bl00mbox.plugin_index_get_description(self.index)

    def __repr__(self):
        return (
            "[plugin "
            + str(self.plugin_id)
            + "] "
            + self.name
            + ": "
            + self.description
        )


class _PluginDescriptors:
    pass


plugins = _PluginDescriptors()


def _fill():
    plugins_list = {}
    for i in range(sys_bl00mbox.plugin_registry_num_plugins()):
        plugins_list[sys_bl00mbox.plugin_index_get_name(i).replace(" ", "_")] = i
    for name, value in plugins_list.items():
        setattr(plugins, name, _PluginDescriptor(value))
        # legacy
        if name == "sequencer" or name == "distortion":
            setattr(plugins, "_" + name, _PluginDescriptor(value))
        elif name == "sampler":
            setattr(plugins, "_sampler_ram", _PluginDescriptor(value))
        elif name == "delay_static":
            setattr(plugins, "delay", _PluginDescriptor(value))


_fill()


class _Plugin:
    def __init__(self, channel, plugin_id, bud_num=None, init_var=0):
        self._channel_num = channel.channel_num
        if bud_num == None:
            self._plugin_id = plugin_id
            self._bud_num = sys_bl00mbox.channel_new_bud(
                self.channel_num, self.plugin_id, init_var
            )
            if self._bud_num == None:
                raise bl00mbox.Bl00mboxError("plugin init failed")
        else:
            self._bud_num = bud_num
            self._check_existence()
            self._plugin_id = sys_bl00mbox.channel_bud_get_plugin_id(
                self.channel_num, self.bud_num
            )
        self._name = sys_bl00mbox.channel_bud_get_name(self.channel_num, self.bud_num)
        self._signals = bl00mbox.SignalList(self)

    def __repr__(self):
        self._check_existence()
        ret = "[plugin " + str(self.bud_num) + "] " + self.name
        for sig in self.signals._list:
            ret += "\n  " + "\n  ".join(sig._no_desc().split("\n"))
        return ret

    def __del__(self):
        self._check_existence()
        sys_bl00mbox.channel_delete_bud(self.channel_num, self.bud_num)

    def _check_existence(self):
        if not sys_bl00mbox.channel_bud_exists(self.channel_num, self.bud_num):
            raise bl00mbox.Bl00mboxError("plugin has been deleted")

    @property
    def init_var(self):
        return sys_bl00mbox.channel_bud_get_init_var(self.channel_num, self.bud_num)

    @property
    def table_len(self):
        return sys_bl00mbox.channel_bud_get_table_len(self.channel_num, self.bud_num)

    @property
    def channel_num(self):
        return self._channel_num

    @property
    def plugin_id(self):
        return self._plugin_id

    @property
    def name(self):
        return self._name

    @property
    def bud_num(self):
        return self._bud_num

    @property
    def signals(self):
        return self._signals

    @property
    def table(self):
        ret = []
        for x in range(
            sys_bl00mbox.channel_bud_get_table_len(self.channel_num, self.bud_num)
        ):
            ret += [
                sys_bl00mbox.channel_bud_get_table_value(
                    self.channel_num, self.bud_num, x
                )
            ]
        return ret

    @table.setter
    def table(self, stuff):
        max_len = sys_bl00mbox.channel_bud_get_table_len(self.channel_num, self.bud_num)
        if len(stuff) > max_len:
            stuff = stuff[:max_len]
        for x, y in enumerate(stuff):
            sys_bl00mbox.channel_bud_set_table_value(
                self.channel_num, self.bud_num, x, y
            )

    @property
    def table_pointer(self):
        pointer = sys_bl00mbox.channel_bud_get_table_pointer(
            self.channel_num, self.bud_num
        )
        max_len = sys_bl00mbox.channel_bud_get_table_len(self.channel_num, self.bud_num)
        return (pointer, max_len)

    @property
    def table_bytearray(self):
        pointer, max_len = self.table_pointer
        bytes_len = max_len * 2
        return uctypes.bytearray_at(pointer, bytes_len)

    @property
    def table_int8_array(self):
        pointer, max_len = self.table_pointer
        descriptor = {"table": (0 | uctypes.ARRAY, max_len * 2 | uctypes.INT8)}
        struct = uctypes.struct(pointer, descriptor)
        return struct.table

    @property
    def table_int16_array(self):
        pointer, max_len = self.table_pointer
        descriptor = {"table": (0 | uctypes.ARRAY, max_len | uctypes.INT16)}
        struct = uctypes.struct(pointer, descriptor)
        return struct.table

    @property
    def table_uint32_array(self):
        pointer, max_len = self.table_pointer
        descriptor = {"table": (0 | uctypes.ARRAY, (max_len // 2) | uctypes.UINT32)}
        struct = uctypes.struct(pointer, descriptor)
        return struct.table


_plugin_subclasses = {}


def _make_new_plugin(channel, plugin_id, bud_num, *args, **kwargs):
    if bud_num is not None:
        plugin_id = sys_bl00mbox.channel_bud_get_plugin_id(channel.channel_num, bud_num)
    if plugin_id in _plugin_subclasses:
        return _plugin_subclasses[plugin_id](
            channel, plugin_id, bud_num, *args, **kwargs
        )
    else:
        init_var = kwargs.get("init_var", None)
        if init_var is None and len(args) == 1:
            init_var = args[0]
        try:
            init_var = int(init_var)
        except:
            return _Plugin(channel, plugin_id, bud_num)
        return _Plugin(channel, plugin_id, bud_num, init_var)


def _plugin_set_subclass(plugin_id):
    def decorator(cls):
        _plugin_subclasses[plugin_id] = cls
        return cls

    return decorator


@_plugin_set_subclass(696969)
class _Sampler(_Plugin):
    # divide by two if uint32_t
    _READ_HEAD_POS = 0 // 2
    _WRITE_HEAD_POS = 2 // 2
    _SAMPLE_START = 4 // 2
    _SAMPLE_LEN = 6 // 2
    _SAMPLE_RATE = 8 // 2
    _STATUS = 10
    _BUFFER = 11

    def __init__(self, channel, plugin_id, bud_num, init_var=1000):
        self._filename = ""
        if bud_num is not None:
            super().__init__(channel, plugin_id, bud_num=bud_num)
            self._memory_len = self.init_var
        elif type(init_var) is str:
            with wave.open(init_var, "r") as f:
                self._memory_len = f.getnframes()
                super().__init__(channel, plugin_id, init_var=self._memory_len)
                self.load(init_var)
        else:
            self._memory_len = int(48 * init_var)
            super().__init__(channel, plugin_id, init_var=self._memory_len)

    def __repr__(self):
        ret = super().__repr__()
        ret += "\n  playback "
        if self.playback_loop:
            ret += "(looped): "
        else:
            ret += "(single): "
        if self.playback_progress is not None:
            dots = int(self.playback_progress * 20)
            ret += (
                "["
                + "#" * dots
                + "-" * (20 - dots)
                + "] "
                + str(int(self.playback_progress * 100))
                + "%"
            )
        else:
            ret += "idle"

        ret += "\n  record "
        if self.record_overflow:
            ret += "(overflow): "
        else:
            ret += "(autostop): "
        if self.record_progress is not None:
            dots = int(self.record_progress * 20)
            ret += (
                "["
                + "#" * dots
                + "-" * (20 - dots)
                + "] "
                + str(int(self.record_progress * 100))
                + "%"
            )
        else:
            ret += "idle"
        ret += "\n  buffer: " + " " * 11
        rel_fill = self.sample_length / self.buffer_length
        dots = int(rel_fill * 20)
        if dots > 19:
            dots = 19
        ret += (
            "[" + "#" * dots + "-" * (19 - dots) + "] " + str(int(rel_fill * 100)) + "%"
        )
        if self.filename is not "":
            ret += "\n  file: " + self.filename
        ret += "\n  sample rate: " + " " * 6 + str(self.sample_rate)
        return ret

    def load(self, filename):
        with wave.open(filename, "r") as f:
            try:
                assert f.getsampwidth() == 2
                assert f.getnchannels() in (1, 2)
                assert f.getcomptype() == "NONE"
            except AssertionError:
                raise Bl00mboxError("incompatible file format")

            frames = f.getnframes()
            if frames > self._memory_len:
                frames = self._memory_len
            self._sample_len_frames = frames

            self.sample_rate = f.getframerate()

            BUFFER_SIZE = int(48000 * 2.5)

            if f.getnchannels() == 1:
                # fast path for mono
                table = self.table_bytearray
                for i in range(
                    2 * self._BUFFER,
                    (self._sample_len_frames + self._BUFFER) * 2,
                    BUFFER_SIZE * 2,
                ):
                    table[i : i + BUFFER_SIZE * 2] = f.readframes(BUFFER_SIZE)
            else:
                # somewhat fast path for stereo
                table = self.table_int16_array
                for i in range(
                    self._BUFFER,
                    self._sample_len_frames + self._BUFFER,
                    BUFFER_SIZE,
                ):
                    frame = f.readframes(BUFFER_SIZE)
                    for j in range(0, len(frame) // 4):
                        value = int.from_bytes(frame[4 * j : 4 * j + 2], "little")
                        table[i + j] = value
            self._filename = filename
            self._set_status_bit(4, 0)

    def save(self, filename):
        with wave.open(filename, "w") as f:
            f.setnchannels(1)
            f.setsampwidth(2)
            f.setframerate(self.sample_rate)
            start = self._offset_index(0)
            end = self._offset_index(self._sample_len_frames)
            table = self.table_bytearray
            if end > start:
                f.writeframes(table[2 * start : 2 * end])
            else:
                f.writeframes(table[2 * start :])
                f.writeframes(table[2 * self._BUFFER : 2 * end])
            if self._filename:
                self._filename = filename
                self._set_status_bit(4, 0)

    def _offset_index(self, index):
        index += self._sample_start
        if index >= self._memory_len:
            index -= self._memory_len
        index += self._BUFFER
        return index

    def _set_status_bit(self, bit, val):
        table = self.table_int16_array
        if val:
            table[self._STATUS] = (table[self._STATUS] | (1 << bit)) & 0xFF
        else:
            table[self._STATUS] = (table[self._STATUS] & ~(1 << bit)) & 0xFF

    @property
    def filename(self):
        if self._get_status_bit(4):
            self._filename = ""
            self._set_status_bit(4, 0)
        return self._filename

    def _get_status_bit(self, bit):
        table = self.table_int16_array
        return bool(table[self._STATUS] & (1 << bit))

    @property
    def playback_loop(self):
        return self._get_status_bit(1)

    @playback_loop.setter
    def playback_loop(self, val):
        self._set_status_bit(1, val)

    @property
    def record_overflow(self):
        return self._get_status_bit(3)

    @record_overflow.setter
    def record_overflow(self, val):
        self._set_status_bit(3, val)

    @property
    def record_progress(self):
        if self._get_status_bit(2):
            table = self.table_uint32_array
            return table[self._WRITE_HEAD_POS] / self._memory_len
        return None

    @property
    def playback_progress(self):
        if self._get_status_bit(0):
            table = self.table_uint32_array
            return table[0] / table[3]
        return None

    @property
    def _sample_start(self):
        table = self.table_uint32_array
        return table[2]

    @_sample_start.setter
    def _sample_start(self, val):
        if val >= self._memory_len:
            val = self._memory_len - 1
        table = self.table_uint32_array
        table[2] = int(val)

    @property
    def _sample_len_frames(self):
        table = self.table_uint32_array
        return table[self._SAMPLE_LEN]

    @_sample_len_frames.setter
    def _sample_len_frames(self, val):
        val = int(val)
        if val > 0:
            table = self.table_uint32_array
            table[self._SAMPLE_LEN] = val

    @property
    def sample_length(self):
        return self._sample_len_frames

    @property
    def buffer_length(self):
        return self._memory_len

    @property
    def sample_rate(self):
        table = self.table_uint32_array
        return table[self._SAMPLE_RATE]

    @sample_rate.setter
    def sample_rate(self, val):
        table = self.table_uint32_array
        if int(val) > 0:
            table[self._SAMPLE_RATE] = int(val)


@_plugin_set_subclass(9000)
class _Distortion(_Plugin):
    def curve_set_power(self, power=2, volume=32767, gate=0):
        volume = min(max(volume, -32767), 32767)
        table = [0] * 129
        for num in range(len(table)):
            if num < 64:
                ret = num / 64  # scale to [0..1[ range
                ret = ret**power
                if ret > 1:
                    ret = 1
                table[num] = int(volume * (ret - 1))
            else:
                ret = (128 - num) / 64  # scale to [0..1] range
                ret = ret**power
                table[num] = int(volume * (1 - ret))
        gate = min(abs(int(gate)), 32767) >> 9
        for i in range(64 - gate, 64 + gate):
            table[i] = 0
        self.table = table

    @property
    def _secret_sauce(self):
        table = self.table_int16_array
        return table[129]

    @_secret_sauce.setter
    def _secret_sauce(self, val):
        val = min(max(int(val), 0), 7)
        table = self.table_int16_array
        table[129] = val

    @property
    def curve(self):
        return self.table[:129]

    @curve.setter
    def curve(self, points):
        # interpolation only implemented for len(points) <= 129,
        # for longer lists data may be dropped.
        points_size = len(points)
        if not points_size:
            return
        table = [0] * 129
        for x, num in enumerate(table):
            position = x * points_size / 129
            lower = int(position)
            lerp = position - lower
            if position < points_size - 1:
                table[x] = int(
                    (1 - lerp) * points[position] + lerp * points[position + 1]
                )
            else:
                table[x] = int(points[points_size - 1])
        self.table = table

    def __repr__(self):
        ret = super().__repr__()
        wave = self.table[:129]
        ret += "\n  curve:\n"
        ret += "   " + "_" * 67 + "\n"
        ret += "  |" + " " * 67 + "|\n"
        symbols = "UW"
        symbol_counter = 0
        for i in range(15, -1, -1):
            line = "  |  "
            for k in range(63):
                vals = wave[2 * k : 2 * k + 4]
                upper = ((max(vals) >> 8) + 128) >> 4
                lower = ((min(vals) >> 8) + 128) >> 4
                if (i >= lower) and (i <= upper):
                    line += symbols[symbol_counter]
                    symbol_counter = (symbol_counter + 1) % len(symbols)
                else:
                    line += " "
            line += "  |\n"
            ret += line
        ret += "  |" + "_" * 67 + "|"
        return ret


@_plugin_set_subclass(172)
class _PolySqueeze(_Plugin):
    def __init__(self, channel, plugin_id, bud_num, num_outputs=3, num_inputs=10):
        if bud_num is None:
            outs = min(max(num_outputs, 16), 1)
            ins = min(max(num_inputs, 32), num_outputs)
            init_var = outs + (ins * 256)
            super().__init__(channel, plugin_id, init_var=init_var)
        else:
            super().__init__(channel, plugin_id, bud_num=bud_num)


@_plugin_set_subclass(420)
class _Osc(_Plugin):
    @property
    def wave(self):
        return tuple([self.table_int8_array[i] for i in range(64)])

    def __repr__(self):
        ret = super().__repr__()
        wave = self.wave
        ret += "\n  wave debug (lazy updates, may get stuck on old data):\n"
        ret += "   " + "_" * 68 + "\n"
        ret += "  |" + " " * 68 + "|\n"
        symbols = "UW"
        symbol_counter = 0
        for i in range(15, -1, -1):
            line = "  |  "
            for j in range(64):
                if j == 0:
                    upper = wave[63]
                else:
                    upper = wave[j - 1]
                upper = (upper + 128) >> 4
                lower = (wave[j] + 128) >> 4
                if lower > upper:
                    upper, lower = lower, upper
                if (i >= lower) and (i <= upper):
                    line += symbols[symbol_counter]
                    symbol_counter = (symbol_counter + 1) % len(symbols)
                else:
                    line += " "
            line += "  |\n"
            ret += line
        ret += "  |" + "_" * 68 + "|"
        return ret


@_plugin_set_subclass(56709)
class _Sequencer(_Plugin):
    def __init__(self, channel, plugin_id, bud_num, num_tracks=4, num_steps=16):
        if bud_num is None:
            self.num_steps = num_steps % 256
            self.num_tracks = num_tracks % 256
            init_var = (self.num_steps * 256) + (self.num_tracks)

            super().__init__(channel, plugin_id, init_var=init_var)

            tracktable = [-32767] + ([0] * self.num_steps)
            self.table = tracktable * self.num_tracks
        else:
            super().__init__(channel, plugin_id, bud_num=bud_num)
            self.num_tracks = self.init_var % 256
            self.num_steps = (self.init_var // 256) % 256

    def __repr__(self):
        ret = super().__repr__()
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
        for track in range(self.num_tracks):
            ret += (
                "\n    "
                + str(track)
                + " [  "
                + "".join(
                    [
                        "X  " if self.trigger_state(track, x) > 0 else ".  "
                        for x in range(self.num_steps)
                    ]
                )
                + "]"
            )
        return ret

    def _get_table_index(self, track, step):
        return step + 1 + track * (self.num_steps + 1)

    def trigger_start(self, track, step, val=32767):
        if val > 32767:
            val = 32767
        elif val < 1:
            val = 1
        table = self.table_int16_array
        table[self._get_table_index(track, step)] = val

    def trigger_stop(self, track, step):
        table = self.table_int16_array
        table[self._get_table_index(track, step)] = -1

    def trigger_clear(self, track, step):
        table = self.table_int16_array
        table[self._get_table_index(track, step)] = 0

    def trigger_state(self, track, step):
        table = self.table_int16_array
        return table[self._get_table_index(track, step)]

    def trigger_toggle(self, track, step):
        if self.trigger_state(track, step) == 0:
            self.trigger_start(track, step)
        else:
            self.trigger_clear(track, step)

    def save_track_pattern(self, track_index):
        start = track_index * (self.num_steps + 1)
        stop = start + self.num_steps + 1
        track = {}
        table = self.table
        if self.table[start] == -32767:
            track["type"] = "trigger"
        else:
            track["type"] = "value"
        track["steps"] = list(table[start + 1 : stop])
        return track

    def load_track_pattern(self, track, track_index):
        start = track_index * (self.num_steps + 1)
        table = self.table_int16_array
        stop = start + 1 + min(self.num_steps, len(track["steps"]))
        for i in range(start + 1, stop):
            x = track["steps"][i - start - 1]
            assert (x < 32768) and (x > -32768)
            table[i] = x
        if track["type"] == "trigger":
            table[start] = -32767
        else:
            table[start] = 32767

    def save_pattern(self):
        beat = {}
        beat["tracks"] = [self.save_track_pattern(i) for i in range(self.num_tracks)]
        return beat

    def load_pattern(self, beat):
        num_tracks = min(len(beat["tracks"]), self.num_tracks)
        [self.load_track_pattern(beat["tracks"][i], i) for i in range(num_tracks)]
