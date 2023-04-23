from synth import tinysynth
from hardware import *

set_global_volume_dB(0)
synths = []
for i in range(5):
    synths += [tinysynth(440,1)]

chords = [[0,3,7,10,12],[-2,2,5,8,10],[i-2,3,7,10,14],[-4,0,3,8,12],[-1,2,5,7,11]]

def tune_guitar_to_chord(chord):
    for j in range(5):
        frq = 440 * (2**(chord[j]/12))
        synths[j].freq(int(frq))

tune_guitar_to_chord(chords[2])

while True:
    for i in range(10):
        if(get_captouch(i)):
            if(i%2):
                tune_guitar_to_chord(chords[int((i-1)/2)])
            else:
                synths[int(i/2)].start()
