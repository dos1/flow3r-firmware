from synth import tinysynth
from hardware import *


octave = 0
synths = []
scale = [0,2,4,5,7,9,11]

def set_chord(i):
    global chord_index
    global chord
    if(i != chord_index):
        chord_index = i
        for j in range(40):
            hue = int(72*(i+0.5)) % 360
            set_led_hsv(j, hue, 1, 0.5)
        chord = chords[i]
        print("set chord " +str(i))
        update_leds()

def run():
    global scale
    global octave
    global synths
    for i in range(10):
        if(get_captouch(i)):
            if(i == 4):
                octave = -1
            elif(i == 5):
                octave = 0
            elif(i == 6):
                octave = 1
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

def highlight_bottom_petal(num, r, g, b):
    start = 4 + 8*num
    for i in range(7):
        set_led_rgb((i+start%40), r, g, b)
    update_leds()

def foreground():
    highlight_bottom_petal(0, 0, 0, 255)
    highlight_bottom_petal(1, 0, 0, 255)
    highlight_bottom_petal(4, 0, 0, 255)
    highlight_bottom_petal(3, 0, 0, 255)
    set_led_rgb(18, 255, 0, 255)
    set_led_rgb(19, 255, 0, 255)
    set_led_rgb(27, 255, 0, 255)
    set_led_rgb(28, 255, 0, 255)
    highlight_bottom_petal(2, 255, 0, 255)
