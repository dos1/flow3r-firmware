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
        self.amp.signals.output.value = chan.mixer
        self.amp.signals.gain.value = self.env.signals.output
        self.amp.signals.input.value = self.osc.signals.output
        self.env.signals.sustain.value = 0
        self.env.signals.decay.value = 500

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
        self.fm_mult = 2**.5
        self.tone(self.osc.signals.pitch.tone - 17)
        self.mod_osc.signals.output.value = self.osc.signals.lin_fm
        self.decay(5000)
        self.attack(20)
        self.fm_waveform(self.SAW)

    def fm_waveform(self,val):
        self.mod_osc.signals.waveform.value = val
    def __repr__(self):
        ret = tinysynth.__repr__(self)
        ret = ret.split("\n")
        ret[0] += "_fm"
        ret = "\n".join(ret)
        ret += "\n  " + "\n  ".join(repr(self.mod_osc).split("\n"))
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
