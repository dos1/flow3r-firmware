from synth import tinysynth
from hardware import *
from utils import *

octave = 0
synths = []
scale = [0,2,4,5,7,9,11]


def change_playing_field_color(r,g,b):
    highlight_bottom_petal(0, r, g, b)
    highlight_bottom_petal(1, r, g, b)
    highlight_bottom_petal(3, r, g, b)
    highlight_bottom_petal(4, r, g, b)
    highlight_bottom_petal(2, 55, 0, 55)
    led_set_rgb(15, 55, 0, 55)
    led_set_rgb(16, 55, 0, 55)
    led_set_rgb(24, 55, 0, 55)
    led_set_rgb(25, 55, 0, 55)
    leds_update()

def adjust_playing_field_to_octave():
    global octave
    if(octave == -1):
        change_playing_field_color(0,0,55)
    elif(octave == 0):
        change_playing_field_color(0,27,27)
    elif(octave == 1):
        change_playing_field_color(0,55,0)

def run():
    global scale
    global octave
    global synths
    for i in range(10):
        if(captouch_get(i)):
            if(i == 4):
                octave = -1
                adjust_playing_field_to_octave()
            elif(i == 5):
                octave = 0
                adjust_playing_field_to_octave()
            elif(i == 6):
                octave = 1
                adjust_playing_field_to_octave()
            else:
                k = i
                if(k>3):
                    k -= 10
                k = 3 - k
                note = scale[k] + 12 * octave
                synths[0].tone(note)
                synths[0].start()

def init():
    global synths
    for i in range(1):
        synths += [tinysynth(440,1)]
    for synth in synths:
        synth.decay(100)
        synth.waveform(1)

def foreground():
    adjust_playing_field_to_octave()
