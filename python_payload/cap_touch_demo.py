import hardware
import utils
import cmath
import math
import time

def init():
    pass

def run():
    hardware.display_fill(0)
    time.sleep_ms(30)
    for i in range(10):
        size = (hardware.get_captouch(i) * 4) + 4
        size += int(max(0, sum([hardware.captouch_get_petal_pad(i, x) for x in range(0, 3+1)]) / 8000))
        x = 70 + (hardware.captouch_get_petal_rad(i)/1000)
        x += (hardware.captouch_get_petal_phi(i)/600)*1j
        rot =  cmath.exp(2j *math.pi * i / 10)
        x = x * rot
        col = 0b1111100000011111
        if i%2:
            col = 0b0000011111111111
        utils.draw_rect(int(x.imag+120-(size/2)),int(x.real+120-(size/2)),size,size,col)
    hardware.display_update()

def foreground():
    pass


from application import Application

app = Application("cap touch")
app.main_foreground = run
