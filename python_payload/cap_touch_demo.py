from synth import tinysynth
from hardware import *
import time

set_global_volume_dB(0)
synths = []
for i in range(5):
    synths += [tinysynth(440,1)]

for synth in synths:
    synth.decay(100)
    synth.waveform(1)

chords = [[0,3,7,10,12],[-2,2,5,8,10],[-2,3,7,10,14],[-4,0,3,8,12],[-1,2,5,7,11]]

chord = chords[3]
chord_index = -1

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

set_chord(3)

def cap_touch_demo_start(delay):
    global chord_index
    global chord
    while True:
        update_leds()
        for i in range(10):
            if(get_captouch(i)):
                if(i%2):
                    k = int((i-1)/2)
                    set_chord(k)
                else:
                    k = int(i/2)
                    synths[k].tone(chord[k])
                    synths[k].start()
                    print("synth " +str(k))
            time.sleep_ms(delay)

