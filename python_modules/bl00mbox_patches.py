import math
import wave

class _Patch():
    def bl00mbox_patch_marker(self):
        return True

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
    def __init__(self, chan, file):
        f = wave.open('samples/' + file, 'r')
        frames = f.getnframes()
        self.sampler = chan.new_bud(696969, 1+(frames//48))
        self.sampler.signals.output = chan.mixer
        table = []
        for i in range(frames):
            frame = f.readframes(1)
            value = int.from_bytes(frame[0:2], 'little')
            table += [value]
        f.close()
        self.sampler.table = table
    def start(self):
        self.sampler.signals.trigger.start()
    def stop(self):
        self.sampler.signals.trigger.stop()

class step_sequencer(_Patch):
    def __init__(self, chan):
        self.seqs = []
        for i in range(4):
            seq = chan.new_bud(56709)
            seq.table = [-32767] + ([0]*16)
            if(len(self.seqs)):
                self.seqs[-1].signals.sync_out = seq.signals.sync_in
            self.seqs += [seq]
        self._bpm = 120

    def __repr__(self):
        ret = "[patch] step sequencer"
        #ret += "\n  " + "\n  ".join(repr(self.seqs[0]).split("\n"))
        ret += "\n  bpm: " + str(self.seqs[0].signals.bpm.value) + " @ 1/" + str(self.seqs[0].signals.beat_div.value)
        ret += "\n  step: " + str(self.seqs[0].signals.step.value) + "/" + str(self.seqs[0].signals.step_len.value)
        ret += "\n  [tracks]"
        for x,seq in enumerate(self.seqs):
            ret += "\n    " + str(x) + " [  " + "".join(["X  " if x > 0 else ".  " for x in seq.table[1:]]) + "]"
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
        a[step+1] = 32767
        self.seqs[track].table = a

    def trigger_stop(self, track, step):
        a = self.seqs[track].table
        a[step+1] = 0
        self.seqs[track].table = a
    
    def trigger_state(self, track, step):
        a = self.seqs[track].table
        return a[step+1]

    def trigger_toggle(self, track, step):
        if self.trigger_state(track, step) == 0:
            self.trigger_start(track, step)
        else:
            self.trigger_stop(track, step)


    @property
    def step(self):
        return self.seqs[0].signals.step
