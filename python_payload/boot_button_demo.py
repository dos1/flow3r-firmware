from machine import Pin
from synth import tinysynth
p = Pin(0, Pin.IN)
s = tinysynth(440,1)
while True:
    if(p.value() == 0):
        s.start()
