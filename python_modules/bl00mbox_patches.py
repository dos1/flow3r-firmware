import math

class bl00mboxPatch():
    pass

class tinysynth(bl00mboxPatch):
    def __init__(self, chan):
        self.channel = chan
        self.SINE = -32767
        self.TRI = -1
        self.SQUARE = 1
        self.SAW = 32767

        self.osc = chan.new_bud(420)
        self.env = chan.new_bud(42)
        self.amp = chan.new_bud(69)
        self.amp.signals.output.value = chan.output
        self.amp.signals.gain.value = self.env.signals.output
        self.amp.signals.input.value = self.osc.signals.output
        self.env.signals.sustain.value = 0
        self.env.signals.decay.value = 500

    def __repr__(self):
        ret = "[PATCH] tinysynth"
        ret += "\n" + repr(self.osc)
        ret += "\n" + repr(self.env)
        ret += "\n" + repr(self.amp)
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
        self.fm_mult = 2**.5
        self.detune = 7
        self.tone(self.osc.signals.pitch.tone)
        self.mod_osc.signals.output.value = self.osc.signals.lin_fm

    def fm_waveform(self,val):
        self.mod_osc.signals.waveform.value = val
    def __repr__(self):
        ret = tinysynth.__repr__(self)
        ret = ret.split("\n")
        ret[0] += "_fm"
        ret = "\n".join(ret)
        ret += "\n" + repr(self.mod_osc)
        return ret

    def fm(self,val):
        self.fm_mult = val
        self._update_mod_osc

    def tone(self, val):
        self.osc.signals.pitch.tone = val
        self._update_mod_osc

    def freq(self, val):
        self.osc.signals.pitch.freq = val
        self._update_mod_osc
    
    def _update_mod_osc(self):
        self.mod_osc.signals.pitch.freq = self.fm_mult * self.osc.signals.pitch.freq
