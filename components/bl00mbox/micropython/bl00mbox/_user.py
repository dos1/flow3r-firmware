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
            sig = Signal(bl00mbox._plugins._make_new_plugin(self._channel, 0, b), s)
            ret += [sig]
        return ret


class ValueSwitch:
    def __init__(self, plugin, signal_num):
        self._plugin = plugin
        self._signal_num = signal_num

    def __setattr__(self, key, value):
        if getattr(self, "_locked", False):
            if not value:
                return
            val = getattr(self, key, None)
            if val is None:
                return
            sys_bl00mbox.channel_bud_set_signal_value(
                self._plugin.channel_num,
                self._plugin.bud_num,
                self._signal_num,
                int(val),
            )
        else:
            super().__setattr__(key, value)

    def get_value_name(self, val):
        d = dict((k, v) for k, v in self.__dict__.items() if not k.startswith("_"))
        for k, v in d.items():
            if v == val:
                return k


class Signal:
    def __init__(self, plugin, signal_num):
        self._plugin = plugin
        self._signal_num = signal_num
        self._name = sys_bl00mbox.channel_bud_get_signal_name(
            plugin.channel_num, plugin.bud_num, signal_num
        )
        self._mpx = sys_bl00mbox.channel_bud_get_signal_name_multiplex(
            plugin.channel_num, plugin.bud_num, signal_num
        )
        self._description = sys_bl00mbox.channel_bud_get_signal_description(
            plugin.channel_num, plugin.bud_num, signal_num
        )
        self._unit = sys_bl00mbox.channel_bud_get_signal_unit(
            plugin.channel_num, plugin.bud_num, signal_num
        )
        constants = {}
        another_round = True
        while another_round:
            another_round = False
            for k, char in enumerate(self._unit):
                if char == "{":
                    for j, stopchar in enumerate(self._unit[k:]):
                        if stopchar == "}":
                            const = self._unit[k + 1 : k + j].split(":")
                            constants[const[0]] = int(const[1])
                            self._unit = self._unit[:k] + self._unit[k + j + 1 :]
                            break
                    another_round = True
                    break
        if constants:
            self._unit = self._unit.strip()
            self.switch = ValueSwitch(plugin, signal_num)
            for key in constants:
                setattr(self.switch, key, constants[key])
            self.switch._locked = True
        self._hints = ""

    def __repr__(self):
        ret = self._no_desc()
        if len(self._description):
            ret += "\n  " + "\n  ".join(self._description.split("\n"))
        return ret

    def _no_desc(self):
        self._plugin._check_existence()

        ret = self.name
        if self._mpx != -1:
            ret += "[" + str(self._mpx) + "]"

        if len(self.unit):
            ret += " [" + self.unit + "]"

        ret += " [" + self.hints + "]: "

        conret = []
        direction = " <?> "
        val = self.value
        if isinstance(self, SignalInput):
            direction = " <= "
            if len(self.connections):
                val = self.connections[0].value
        elif isinstance(self, SignalOutput):
            direction = " => "

        ret += str(val)

        if getattr(self, "switch", None) is not None:
            value_name = self.switch.get_value_name(val)
            if value_name is not None:
                ret += " (" + value_name.lower() + ")"

        if isinstance(self, SignalPitchMixin):
            ret += " / " + str(self.tone) + " semitones / " + str(self.freq) + "Hz"

        if isinstance(self, SignalGainMixin):
            if self.mult == 0:
                ret += " / (mute)"
            else:
                ret += " / " + str(self.dB) + "dB / x" + str(self.mult)

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

    @property
    def _tone(self):
        return (self.value - (32767 - 2400 * 6)) / 200

    @_tone.setter
    def _tone(self, val):
        if isinstance(self, SignalInput):
            if (type(val) == int) or (type(val) == float):
                self.value = (32767 - 2400 * 6) + 200 * val
            if type(val) == str:
                self.value = helpers.note_name_to_sct(val)
        else:
            raise AttributeError("can't set output signal")

    @property
    def _freq(self):
        tone = (self.value - (32767 - 2400 * 6)) / 200
        return 440 * (2 ** (tone / 12))

    @_freq.setter
    def _freq(self, val):
        if isinstance(self, SignalInput):
            tone = 12 * math.log(val / 440, 2)
            self.value = (32767 - 2400 * 6) + 200 * tone
        else:
            raise AttributeError("can't set output signal")

    @property
    def _dB(self):
        if self.value == 0:
            return -9999
        return 20 * math.log((abs(self.value) / 4096), 10)

    @_dB.setter
    def _dB(self, val):
        if isinstance(self, SignalInput):
            self.value = int(4096 * (10 ** (val / 20)))
        else:
            raise AttributeError("can't set output signal")

    @property
    def _mult(self):
        return self.value / 4096

    @_mult.setter
    def _mult(self, val):
        if isinstance(self, SignalInput):
            self.value = int(4096 * val)
        else:
            raise AttributeError("can't set output signal")


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
                cons += [
                    _makeSignal(
                        bl00mbox._plugins._make_new_plugin(Channel(chan), 0, b), s
                    )
                ]
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
            cons += [
                _makeSignal(bl00mbox._plugins._make_new_plugin(Channel(chan), 0, b), s)
            ]
        return cons

    def _start(self, velocity=32767):
        if self.value > 0:
            self.value = -abs(velocity)
        else:
            self.value = abs(velocity)

    def _stop(self):
        self.value = 0


class SignalInputTriggerMixin:
    def start(self, velocity=32767):
        self._start(velocity)

    def stop(self):
        self._stop()


class SignalPitchMixin:
    @property
    def tone(self):
        return self._tone

    @tone.setter
    def tone(self, val):
        self._tone = val

    @property
    def freq(self):
        return self._freq

    @freq.setter
    def freq(self, val):
        self._freq = val


class SignalGainMixin:
    @property
    def dB(self):
        return self._dB

    @dB.setter
    def dB(self, val):
        self._dB = val

    @property
    def mult(self):
        return self._mult

    @mult.setter
    def mult(self, val):
        self._mult = val


class SignalOutputTrigger(SignalOutput):
    pass


class SignalOutputPitch(SignalOutput, SignalPitchMixin):
    pass


class SignalOutputGain(SignalOutput, SignalGainMixin):
    pass


class SignalInputTrigger(SignalInput, SignalInputTriggerMixin):
    pass


class SignalInputPitch(SignalInput, SignalPitchMixin):
    pass


class SignalInputGain(SignalInput, SignalGainMixin):
    pass


class SignalMpxList:
    def __init__(self):
        self._list = []
        self._setattr_allowed = False

    def __len__(self):
        return len(self._list)

    def __getitem__(self, index):
        return self._list[index]

    def __setitem__(self, index, value):
        try:
            current_value = self._list[index]
        except:
            raise AttributeError("index does not exist")
        if isinstance(current_value, Signal):
            current_value.value = value
        else:
            raise AttributeError("signal does not exist")

    def __iter__(self):
        self._iter_index = 0
        return self

    def __next__(self):
        self._iter_index += 1
        if self._iter_index >= len(self._list):
            raise StopIteration
        else:
            return self._list[self._iter_index]

    def add_new_signal(self, signal, index):
        self._setattr_allowed = True
        index = int(index)
        if len(self._list) <= index:
            self._list += [None] * (1 + index - len(self._list))
        self._list[index] = signal
        self._setattr_allowed = False

    def __setattr__(self, key, value):
        """
        current_value = getattr(self, key, None)
        if isinstance(current_value, Signal):
            current_value.value = value
            return
        """
        if key == "_setattr_allowed" or getattr(self, "_setattr_allowed", True):
            super().__setattr__(key, value)
        else:
            raise AttributeError("signal does not exist")


class SignalList:
    def __init__(self, plugin):
        self._list = []
        for signal_num in range(
            sys_bl00mbox.channel_bud_get_num_signals(plugin.channel_num, plugin.bud_num)
        ):
            hints = sys_bl00mbox.channel_bud_get_signal_hints(
                plugin.channel_num, plugin.bud_num, signal_num
            )
            if hints & 4:
                if hints & 1:
                    signal = SignalInputTrigger(plugin, signal_num)
                    signal._hints = "input/trigger"
                elif hints & 2:
                    signal = SignalOutputTrigger(plugin, signal_num)
                    signal._hints = "output/trigger"
            elif hints & 32:
                if hints & 1:
                    signal = SignalInputPitch(plugin, signal_num)
                    signal._hints = "input/pitch"
                elif hints & 2:
                    signal = SignalOutputPitch(plugin, signal_num)
                    signal._hints = "output/pitch"
            elif hints & 8:
                if hints & 1:
                    signal = SignalInputGain(plugin, signal_num)
                    signal._hints = "input/gain"
                elif hints & 2:
                    signal = SignalOutputGain(plugin, signal_num)
                    signal._hints = "input/gain"
            else:
                if hints & 1:
                    signal = SignalInput(plugin, signal_num)
                    signal._hints = "input"
                elif hints & 2:
                    signal = SignalOutput(plugin, signal_num)
                    signal._hints = "output"
            self._list += [signal]
            name = signal.name.split(" ")[0]
            if signal._mpx == -1:
                setattr(self, name, signal)
            else:
                # <LEGACY SUPPORT>
                setattr(self, name + str(signal._mpx), signal)
                # </LEGACY SUPPORT>
                current_list = getattr(self, name, None)
                if not isinstance(current_list, SignalMpxList):
                    setattr(self, name, SignalMpxList())
                getattr(self, name, None).add_new_signal(signal, signal._mpx)
        self._setattr_allowed = False

    def __setattr__(self, key, value):
        current_value = getattr(self, key, None)
        if isinstance(current_value, Signal):
            current_value.value = value
            return
        elif getattr(self, "_setattr_allowed", True):
            super().__setattr__(key, value)
        else:
            raise AttributeError("signal does not exist")


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

    def _new_plugin(self, thing, *args, **kwargs):
        if isinstance(thing, bl00mbox._plugins._PluginDescriptor):
            return bl00mbox._plugins._make_new_plugin(
                self, thing.plugin_id, None, *args, **kwargs
            )
        else:
            raise TypeError("not a plugin")

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
        if isinstance(thing, bl00mbox._plugins._PluginDescriptor) or (
            type(thing) == int
        ):
            return self._new_plugin(thing, *args, **kwargs)

    @property
    def plugins(self):
        ret = []
        for i in range(sys_bl00mbox.channel_buds_num(self.channel_num)):
            b = sys_bl00mbox.channel_get_bud_by_list_pos(self.channel_num, i)
            ret += [bl00mbox._plugins._make_new_plugin(self, 0, b)]
        return ret

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
