'''
sequence_timer requirements:
1. provide root:
periodic event at bpm * beats / beat_div
'''

class Event():
    def __init__(self, timer):
        self._timer = timer
        self.output = timer.signals.output

class Sequencer():
    def __init__(self, chan, beat_div = 8, bpm = 120, beats = 1):
        self.beat_div = beat_div
        self.bpm = bpm
        self.beats = beats
        self.root_timer = chan.new(bl00mbox.plugins.sequence_timer)
        self.root_timer.signals.repeats = 0 # periodic
        self.root_timer.signals.bpm = self.bpm
        self.root_timer.signals.beats = self.beats
        self.root_timer.signals.beat_div = self.beat_div
        self.timers = []
    def new_event(self, beats):
        timer = chan.new(bl00mbox.plugins.sequence_timer)
        timer.signals.repeats = 1 # single shot
        timer.signals.bpm = self.bpm
        timer.signals.beats = beats
        timer.signals.beat_div = self.beat_div
        timer.signals.hard_sync_in = self.root_timer.signals.hard_sync_out
        self.timers += [timer]
        return Event(timer)
