from synth import tinysynth
from hardware import *

synths = []
notes = [2,5,9,12,16]
drums = [1800,4000,6000,12000,16000]


def run():
    global synths
    for i in range(10):
        if(get_captouch(i)):
            if(synths[i][0] == 0):
                synths[i][0] = 1
                synths[i][1].start()
        else:
            if(synths[i][0] == 1):
                synths[i][0] = 0
                synths[i][1].stop()

def init():
    global synths
    global notes
    for i in range(10):
        synth = tinysynth(440,0)
        synth.attack(0)
        if i % 2:
            synth.tone(notes[int(i/2)])
            synth.waveform(4)
        else:
            synth.waveform(8)
            synth.decay(3)
            synth.freq(drums[int(i/2)])
            if i == 8:
                synth.freq(16000)
                synth.decay(50)
        synths += [[0, synth]]

def foreground():
    display_scope_start()
    for i in range(10):
        for y in range(4):
            captouch_set_petal_pad_threshold(i,y,4000)
    for i in range(40):
        set_led_rgb(i, 0,60,60)
    update_leds()
