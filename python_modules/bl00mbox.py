import sys_bl00mbox
#note1: consider the 'sys_bl00mbox' api super unstable for now pls :3
#note2: we're trying to keep the 'bl00mbox' api somewhat slow moving tho
import math

import bl00mbox_patches as patches
import bl00mbox_helpers as helpers

class ChannelMixer():
    def __init__(self, channel_num):
        self.channel_num = channel_num
        self.connections = []
        pass
    def __repr__(self):
        ret = "[channel mixer]"
        if len(self.connections) > 0:
            ret += "\nconnections:" 
        for con in self.connections:
            ret += "\n ^== " + con.name
            ret +=  " in [bud " + str(con._bud.bud_num) + "] " + con._bud.name
        return ret

    @property
    def value(self):
        return len(self.connections)
    @value.setter
    def value(self, val):
        if isinstance(val, SignalOutput):
            val.value = self
    

class Signal:
    def __init__(self, bud, signal_num):
        self._bud = bud
        self._signal_num = signal_num
        self.name = sys_bl00mbox.channel_bud_get_signal_name(bud.channel_num, bud.bud_num, signal_num)
        self.connections = []
        self._value = sys_bl00mbox.channel_bud_get_signal_value(bud.channel_num, bud.bud_num, signal_num)
        self.hints = ''

    def __repr__(self):
        ret = self.name + " [" + self.hints + "]: " 
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
                conret += [direction + con.name + " in [bud " + str(con._bud.bud_num) + "] " +con._bud.name ]
            if isinstance(con, ChannelMixer):
                conret += [" ==> [channel mixer]"]
        nl = "\n"
        if len(conret) > 1:
            ret += "\n"
            nl += "  "
        ret += nl.join(conret)
        return ret

    @property
    def value(self):
        return sys_bl00mbox.channel_bud_get_signal_value(self._bud.channel_num, self._bud.bud_num, self._signal_num)

class SignalOutput(Signal):
    @Signal.value.setter
    def value(self, val):
        if isinstance(val, SignalInput):
            val.value = self
        elif isinstance(val, ChannelMixer):
            if val.channel_num == self._bud.channel_num:
                if sys_bl00mbox.channel_connect_signal_to_output_mixer(self._bud.channel_num, self._bud.bud_num, self._signal_num):
                    self.connections += [val]
                    val.connections += [self]

class SignalInput(Signal):
    @Signal.value.setter
    def value(self, val):
        if isinstance(val, SignalOutput):
            if sys_bl00mbox.channel_connect_signal(self._bud.channel_num, self._bud.bud_num, self._signal_num, val._bud.bud_num, val._signal_num):
                self.connections += [val]
                val.connections += [self]
        elif isinstance(val, SignalInput):
            #TODO
            pass
        elif (type(val) == int) or (type(val) == float):
            sys_bl00mbox.channel_bud_set_signal_value(self._bud.channel_num, self._bud.bud_num, self._signal_num, int(val))

class SignalInputTrigger(SignalInput):
    def start(self, velocity = 32767):
        if self.value > 0:
            self.value = -velocity
        else:
            self.value = velocity
    def stop(self):
        self.value = 0

class SignalInputPitch(SignalInput):
    def __init__(self, bud, signal_num):
        SignalInput.__init__(self, bud, signal_num);
        self._pitch = (self._value - (32767 - 2400 * 6))/2400
        self._freq = 440 * (2**(self._pitch/12))

    @property
    def tone(self):
        return (self.value - (32767 - 2400 * 6))/200
    @tone.setter
    def tone(self, val):
        self.value = (32767-2400*6) + 200 * val

    @property
    def freq(self):
        tone = (self.value - (32767 - 2400 * 6))/200
        return 440 * (2**(tone/12))
    @freq.setter
    def freq(self, val):
        tone = 12 * math.log(val / 440 ,2)
        self.value = (32767-2400*6) + 200 * tone

    def __repr__(self):
        ret = SignalInput.__repr__(self)
        ret += " / " + str(self.tone) + " semitones / " + str(self.freq) + "Hz"
        return ret
    

class SignalList:
    def __init__(self, bud):
        self._list = []
        for signal_num in range(sys_bl00mbox.channel_bud_get_num_signals(bud.channel_num, bud.bud_num)):
            hints = sys_bl00mbox.channel_bud_get_signal_hints(bud.channel_num, bud.bud_num, signal_num)
            if hints & 2: 
                signal = SignalOutput(bud, signal_num)
                signal.hints = "output"
            elif hints & 1:
                if hints & 4:
                    signal = SignalInputTrigger(bud, signal_num)
                    signal.hints = "input/trigger"
                elif hints & 32:
                    signal = SignalInputPitch(bud, signal_num)
                    signal.hints = "input/pitch"
                else:
                    signal = SignalInput(bud, signal_num)
                    signal.hints = "input"
            self._list += [signal]
            setattr(self, signal.name.split(' ')[0], signal)

class Bud:
    def __init__(self, channel, plugin_id, nick = ""):
        self.channel_num = channel.channel_num
        self.plugin_id = plugin_id
        self.bud_num = sys_bl00mbox.channel_new_bud(self.channel_num, self.plugin_id)
        channel.buds += [self]
        self.name = sys_bl00mbox.channel_bud_get_name(self.channel_num, self.bud_num)
        self.nick = nick
        self.signals = SignalList(self)

    def __repr__(self):
        ret = "[bud " + str(self.bud_num) + "] " + self.name
        if self.nick != "":
            ret += " \"" + self.nick + "\" "
        for sig in self.signals._list:
            ret += "\n  " + "\n  ".join(repr(sig).split("\n"))
        return ret

class Channel:
    def __init__(self, nick = ""):
        self.channel_num = 0
        self.buds = []
        self.nick = nick
        #self.volume = sys_bl00mbox.channel_get_volume(self.channel_num)
        self.volume = 3000
        self.mixer = ChannelMixer(self.channel_num)
    
    def __repr__(self):
        ret = "[channel " + str(self.channel_num) + "]"
        if self.nick != "":
            ret += " \"" + self.nick + "\" "
        ret += "\nvolume: " + str(sys_bl00mbox.channel_get_volume(self.channel_num))
        ret += "\nbuds: " + str(len(self.buds))
        ret += "\noutput mixer connections: " + str(len(self.mixer.connections))
        return ret

    def new_bud(self, plugin_id):
        bud = Bud(self, plugin_id)
        return bud
    
    def new_patch(self, patch):
        #if isinstance(patch, patches.bl00mboxPatch):
        return patch(self)

    @property
    def volume(self):
        return sys_bl00mbox.channel_get_volume(self.channel_num)

    @volume.setter
    def volume(self, value):
        sys_bl00mbox.channel_set_volume(self.channel_num, value)

class PluginRegistry:
    @staticmethod
    def print_plugin_list():
        for index in range(sys_bl00mbox.plugin_registry_num_plugins()):
            print(sys_bl00mbox.plugin_registry_print_index(index))
    @staticmethod
    def print_plugin_info(plugin_id):
        print(sys_bl00mbox.plugin_registry_print_id(plugin_id))
