# SPDX-License-Identifier: CC0-1.0

import sys_bl00mbox

# note: consider the 'sys_bl00mbox' api super unstable for now pls :3

import math
import uctypes
import bl00mbox
from bl00mbox import _helpers as helpers


class Bl00mboxError(Exception):
    pass


def _makeSignal(plugin, signal_num):
    hints = sys_bl00mbox.channel_bud_get_signal_hints(
        plugin.channel_num, plugin.bud_num, signal_num
    )
    if hints & 2:
        signal = SignalOutput(plugin, signal_num)
        signal._hints = "output"
    elif hints & 1:
        if hints & 4:
            signal = SignalInputTrigger(plugin, signal_num)
            signal._hints = "input/trigger"
        elif hints & 32:
            signal = SignalInputPitch(plugin, signal_num)
            signal._hints = "input/pitch"
        else:
            signal = SignalInput(plugin, signal_num)
            signal._hints = "input"
    return signal


class ChannelMixer:
    def __init__(self, channel):
        self._channel = channel
        pass

    def __repr__(self):
        ret = "[channel mixer]"
        ret += " (" + str(len(self.connections)) + " connections)"
        for con in self.connections:
            ret += "\n  " + con.name
            ret += " in [plugin " + str(con._plugin.bud_num) + "] " + con._plugin.name
        return ret

    def _unplug_all(self):
        # TODO
        pass

    @property
    def connections(self):
        ret = []
        for i in range(sys_bl00mbox.channel_mixer_num(self._channel.channel_num)):
            b = sys_bl00mbox.channel_get_bud_by_mixer_list_pos(
                self._channel.channel_num, i
            )
            s = sys_bl00mbox.channel_get_signal_by_mixer_list_pos(
                self._channel.channel_num, i
            )
            sig = Signal(Plugin(self._channel, 0, bud_num=b), s)
            ret += [sig]
        return ret


class Signal:
    def __init__(self, plugin, signal_num):
        self._plugin = plugin
        self._signal_num = signal_num
        self._name = sys_bl00mbox.channel_bud_get_signal_name(
            plugin.channel_num, plugin.bud_num, signal_num
        )
        mpx = sys_bl00mbox.channel_bud_get_signal_name_multiplex(
            plugin.channel_num, plugin.bud_num, signal_num
        )
        if mpx != (-1):
            self._name += str(mpx)
        self._description = sys_bl00mbox.channel_bud_get_signal_description(
            plugin.channel_num, plugin.bud_num, signal_num
        )
        self._unit = sys_bl00mbox.channel_bud_get_signal_unit(
            plugin.channel_num, plugin.bud_num, signal_num
        )
        self._hints = ""

    def __repr__(self):
        ret = self._no_desc()
        if len(self._description):
            ret += "\n  " + "\n  ".join(self._description.split("\n"))
        return ret

    def _no_desc(self):
        self._plugin._check_existence()

        ret = self.name
        if len(self.unit):
            ret += " [" + self.unit + "]"
        ret += " [" + self.hints + "]: "
        conret = []
        direction = " <?> "
        if isinstance(self, SignalInput):
            direction = " <= "
            if len(self.connections):
                ret += str(self.connections[0].value)
            else:
                ret += str(self.value)
        elif isinstance(self, SignalOutput):
            direction = " => "
            ret += str(self.value)

        for con in self.connections:
            if isinstance(con, Signal):
                conret += [
                    direction
                    + con.name
                    + " in [plugin "
                    + str(con._plugin.bud_num)
                    + "] "
                    + con._plugin.name
                ]
            if isinstance(con, ChannelMixer):
                conret += [" ==> [channel mixer]"]
        nl = "\n"
        if len(conret) > 1:
            ret += "\n"
            nl += "  "
        ret += nl.join(conret)

        if isinstance(self, SignalPitchMixin):
            ret += " / " + str(self.tone) + " semitones / " + str(self.freq) + "Hz"

        if isinstance(self, SignalVolumeMixin):
            ret += " / " + str(self.dB) + "dB / x" + str(self.mult)

        return ret

    @property
    def name(self):
        return self._name

    @property
    def description(self):
        return self._description

    @property
    def unit(self):
        return self._unit

    @property
    def hints(self):
        return self._hints

    @property
    def connections(self):
        return []

    @property
    def value(self):
        self._plugin._check_existence()
        return sys_bl00mbox.channel_bud_get_signal_value(
            self._plugin.channel_num, self._plugin.bud_num, self._signal_num
        )


class SignalOutput(Signal):
    @Signal.value.setter
    def value(self, val):
        if val == None:
            sys_bl00mbox.channel_disconnect_signal_tx(
                self._plugin.channel_num, self._plugin.bud_num, self._signal_num
            )
        elif isinstance(val, SignalInput):
            val.value = self
        elif isinstance(val, ChannelMixer):
            if val._channel.channel_num == self._plugin.channel_num:
                sys_bl00mbox.channel_connect_signal_to_output_mixer(
                    self._plugin.channel_num, self._plugin.bud_num, self._signal_num
                )

    @property
    def connections(self):
        cons = []
        chan = self._plugin.channel_num
        bud_num = self._plugin.bud_num
        sig = self._signal_num
        for i in range(sys_bl00mbox.channel_subscriber_num(chan, bud_num, sig)):
            b = sys_bl00mbox.channel_get_bud_by_subscriber_list_pos(
                chan, bud_num, sig, i
            )
            s = sys_bl00mbox.channel_get_signal_by_subscriber_list_pos(
                chan, bud_num, sig, i
            )
            if (s >= 0) and (b > 0):
                cons += [_makeSignal(Plugin(Channel(chan), 0, bud_num=b), s)]
            elif s == -1:
                cons += [ChannelMixer(Channel(chan))]
        return cons


class SignalInput(Signal):
    @Signal.value.setter
    def value(self, val):
        self._plugin._check_existence()
        if isinstance(val, SignalOutput):
            if len(self.connections):
                if not sys_bl00mbox.channel_disconnect_signal_rx(
                    self._plugin.channel_num, self._plugin.bud_num, self._signal_num
                ):
                    return
            sys_bl00mbox.channel_connect_signal(
                self._plugin.channel_num,
                self._plugin.bud_num,
                self._signal_num,
                val._plugin.bud_num,
                val._signal_num,
            )
        elif isinstance(val, SignalInput):
            # TODO
            pass
        elif (type(val) == int) or (type(val) == float):
            if len(self.connections):
                if not sys_bl00mbox.channel_disconnect_signal_rx(
                    self._plugin.channel_num, self._plugin.bud_num, self._signal_num
                ):
                    return
            sys_bl00mbox.channel_bud_set_signal_value(
                self._plugin.channel_num,
                self._plugin.bud_num,
                self._signal_num,
                int(val),
            )

    @property
    def connections(self):
        cons = []
        chan = self._plugin.channel_num
        bud = self._plugin.bud_num
        sig = self._signal_num
        b = sys_bl00mbox.channel_get_source_bud(chan, bud, sig)
        s = sys_bl00mbox.channel_get_source_signal(chan, bud, sig)
        if (s >= 0) and (b > 0):
            cons += [_makeSignal(Plugin(Channel(chan), 0, bud_num=b), s)]
        return cons


class SignalTriggerMixin:
    def start(self, velocity=32767):
        if self.value > 0:
            self.value = -velocity
        else:
            self.value = velocity

    def stop(self):
        self.value = 0


class SignalPitchMixin:
    @property
    def tone(self):
        return (self.value - (32767 - 2400 * 6)) / 200

    @tone.setter
    def tone(self, val):
        if (type(val) == int) or (type(val) == float):
            self.value = (32767 - 2400 * 6) + 200 * val
        if type(val) == str:
            self.value = helpers.note_name_to_sct(val)

    @property
    def freq(self):
        tone = (self.value - (32767 - 2400 * 6)) / 200
        return 440 * (2 ** (tone / 12))

    @freq.setter
    def freq(self, val):
        tone = 12 * math.log(val / 440, 2)
        self.value = (32767 - 2400 * 6) + 200 * tone


class SignalVolumeMixin:
    @property
    def dB(self):
        return 20 * math.log((self.value / 4096), 10)

    @dB.setter
    def dB(self, val):
        self.value = int(4096 * (10 ** (val / 20)))

    @property
    def mult(self):
        return self.value / 4096

    @mult.setter
    def mult(self, val):
        self.value = int(4096 * val)


class SignalInputTrigger(SignalInput, SignalTriggerMixin):
    pass


class SignalInputPitch(SignalInput, SignalPitchMixin):
    pass


class SignalInputVolume(SignalInput, SignalVolumeMixin):
    pass


class SignalList:
    def __init__(self, plugin):
        self._list = []
        for signal_num in range(
            sys_bl00mbox.channel_bud_get_num_signals(plugin.channel_num, plugin.bud_num)
        ):
            hints = sys_bl00mbox.channel_bud_get_signal_hints(
                plugin.channel_num, plugin.bud_num, signal_num
            )
            if hints & 2:
                signal = SignalOutput(plugin, signal_num)
                signal._hints = "output"
            elif hints & 1:
                if hints & 4:
                    signal = SignalInputTrigger(plugin, signal_num)
                    signal._hints = "input/trigger"
                elif hints & 32:
                    signal = SignalInputPitch(plugin, signal_num)
                    signal._hints = "input/pitch"
                elif hints & 8:
                    signal = SignalInputVolume(plugin, signal_num)
                    signal._hints = "input/volume"
                else:
                    signal = SignalInput(plugin, signal_num)
                    signal._hints = "input"
            self._list += [signal]
            setattr(self, signal.name.split(" ")[0], signal)

    def __setattr__(self, key, value):
        current_value = getattr(self, key, None)
        if isinstance(current_value, Signal):
            current_value.value = value
            return
        super().__setattr__(key, value)


class Plugin:
    def __init__(self, channel, plugin_id, init_var=0, bud_num=None):
        self._channel_num = channel.channel_num
        if bud_num == None:
            self._plugin_id = plugin_id
            self._bud_num = sys_bl00mbox.channel_new_bud(
                self.channel_num, self.plugin_id, init_var
            )
            if self._bud_num == None:
                raise Bl00mboxError("plugin init failed")
        else:
            self._bud_num = bud_num
            self._check_existence()
            self._name = sys_bl00mbox.channel_bud_get_plugin_id(
                self.channel_num, self.bud_num
            )
        self._name = sys_bl00mbox.channel_bud_get_name(self.channel_num, self.bud_num)
        self._signals = SignalList(self)

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
            raise Bl00mboxError("plugin has been deleted")

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
    def table_int16_array(self):
        pointer, max_len = self.table_pointer
        descriptor = {"table": (0 | uctypes.ARRAY, max_len | uctypes.INT16)}
        struct = uctypes.struct(pointer, descriptor)
        return struct.table


class Channel:
    def __init__(self, name=None):
        if name == None:
            self._channel_num = sys_bl00mbox.channel_get_free_index()
            self.name = "repl"
        elif type(name) == str:
            self._channel_num = sys_bl00mbox.channel_get_free_index()
            self.name = name
        elif type(name) == int:
            if (int(name) < sys_bl00mbox.NUM_CHANNELS) and (int(name >= 0)):
                self._channel_num = int(name)
            else:
                self._channel_num = sys_bl00mbox.NUM_CHANNELS - 1  # garbage channel
        else:
            self._channel_num = sys_bl00mbox.NUM_CHANNELS - 1  # garbage channel

    def __repr__(self):
        ret = "[channel " + str(self.channel_num)
        if self.name != "":
            ret += ": " + self.name
        ret += "]"
        if self.foreground:
            ret += " (foreground)"
        if self.background_mute_override:
            ret += " (background mute override)"
        ret += "\n  volume: " + str(self.volume)
        b = sys_bl00mbox.channel_buds_num(self.channel_num)
        ret += "\n  plugins: " + str(b)
        if len(self.plugins) != b:
            ret += " (desync" + str(len(self.plugins)) + ")"
        ret += "\n  " + "\n  ".join(repr(self.mixer).split("\n"))
        return ret

    def clear(self):
        sys_bl00mbox.channel_clear(self.channel_num)

    @property
    def name(self):
        if self._channel_num == 0:
            return "system"
        if self._channel_num == 31:
            return "garbage"
        name = sys_bl00mbox.channel_get_name(self.channel_num)
        if name is None:
            return ""
        return name

    @name.setter
    def name(self, newname: str):
        sys_bl00mbox.channel_set_name(self._channel_num, newname)

    @property
    def free(self):
        return sys_bl00mbox.channel_get_free(self.channel_num)

    @free.setter
    def free(self, val):
        if val:
            self.clear()
        sys_bl00mbox.channel_set_free(self.channel_num, val)

    def _new_plugin(self, thing, init_var=None):
        plugin_init_var = 0
        if (type(init_var) == int) or (type(init_var) == float):
            plugin_init_var = int(init_var)
        plugin = None
        if isinstance(thing, bl00mbox._plugins._Plugin):
            plugin = Plugin(self, thing.plugin_id, plugin_init_var)
        if type(thing) == int:
            plugin = Plugin(self, thing, plugin_init_var)
        return plugin

    @staticmethod
    def print_overview():
        ret = []
        for i in range(sys_bl00mbox.NUM_CHANNELS):
            c = Channel(i)
            if c.free:
                continue
            ret += [repr(c)]
        if len(ret) != 0:
            print("\n".join(ret))
        else:
            print("no active channels")

    def print_plugins(self):
        for plugin in self.plugins:
            print(repr(plugin))

    def new(self, thing, *args, **kwargs):
        self.free = False
        if type(thing) == type:
            if issubclass(thing, bl00mbox.patches._Patch):
                return thing(self, *args, **kwargs)
        if isinstance(thing, bl00mbox._plugins._Plugin) or (type(thing) == int):
            return self._new_plugin(thing, *args, **kwargs)

    @property
    def plugins(self):
        plugins = []

        for i in range(sys_bl00mbox.channel_buds_num(self.channel_num)):
            b = sys_bl00mbox.channel_get_bud_by_list_pos(self.channel_num, i)
            plugin = Plugin(self, 0, bud_num=b)
            plugins += [plugin]
        return plugins

    @property
    def channel_num(self):
        return self._channel_num

    @property
    def connections(self):
        return sys_bl00mbox.channel_conns_num(self.channel_num)

    @property
    def volume(self):
        return sys_bl00mbox.channel_get_volume(self.channel_num)

    @volume.setter
    def volume(self, value):
        sys_bl00mbox.channel_set_volume(self.channel_num, value)

    @property
    def mixer(self):
        return ChannelMixer(self)

    @mixer.setter
    def mixer(self, val):
        if isinstance(val, SignalOutput):
            val.value = self.mixer
        elif val is None:
            ChannelMixer(self)._unplug_all
        else:
            raise Bl00mboxError("can't connect this")

    @property
    def background_mute_override(self):
        return sys_bl00mbox.channel_get_background_mute_override(self.channel_num)

    @background_mute_override.setter
    def background_mute_override(self, val):
        sys_bl00mbox.channel_set_background_mute_override(self.channel_num, val)

    @property
    def foreground(self):
        return sys_bl00mbox.channel_get_foreground() == self.channel_num

    @foreground.setter
    def foreground(self, val):
        if val:
            sys_bl00mbox.channel_set_foreground(self.channel_num)
        elif sys_bl00mbox.channel_get_foreground() == self.channel_num:
            sys_bl00mbox.channel_set_foreground(0)
