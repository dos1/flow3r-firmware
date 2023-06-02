from synth import tinysynth
from hardware import *

chords = [\
[-4,0,3,8,10],\
[-3,0,5,7,12],\
[-1,2,5,7,11],\
[0,3,7,12,14],\
[3,7,10,14,15]\
]

chord_names = [
"Fadd9",\
"Dadd9",\
"E7",\
"A-add9",\
"Cj7",\
]

chord_index = 3
chord = chords[3]
synths = []
ctx = None

def set_chord(i):
    global chord_index
    global chord_names
    global chord
    global ctx
    if(i != chord_index):
        chord_index = i
        for j in range(40):
            hue = int(72*(i+0.5)) % 360
            set_led_hsv(j, hue, 1, 0.2)
        chord = chords[i]
        update_leds()
        display_fill(0)
        ctx.move_to(0,0).rgb(0,255,0).text(chord_names[i])
        display_update()


def run():
    global chord_index
    global chord
    global synths
    for i in range(10):
        if(get_captouch(i)):
            if(i%2):
                k = int((i-1)/2)
                set_chord(k)
            else:
                k = int(i/2)
                synths[k].tone(chord[k])
                synths[k].start()

def init():
    global chord_index
    global chord
    global synths
    global ctx
    ctx = get_ctx()
    ctx.text_align = ctx.CENTER
    ctx.text_baseline = ctx.MIDDLE

    for i in range(5):
        synths += [tinysynth(440,1)]

    for synth in synths:
        synth.decay(100)
        synth.waveform(1)

def foreground():
    global chord_index
    global chord
    global synths
    tmp = chord_index
    chord_index = -1
    set_chord(tmp)

from application import Application
class HarmonicApp(Application):
    def on_init(self):
        init()
        #foreground()
    
    def on_foreground(self):
        #foreground()
        pass
    
    def main_foreground(self):
        print ("hererer")
        run()

app=HarmonicApp("harmonic")