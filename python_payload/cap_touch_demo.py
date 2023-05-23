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
    for i in range(0,10,2):
        size = (hardware.get_captouch(i) * 4) + 4
        x = 60 + (utils.captouch_get_rad(i)/600)
        x += (utils.captouch_get_phi(i)/600)*1j
        rot =  cmath.exp(2j *math.pi * i / 10)
        x = x * rot
        utils.draw_rect(int(x.imag+120-(size/2)),int(x.real+120-(size/2)),size,size,0b1111100000011111)
    for i in range(1,10,2):
        size = (hardware.get_captouch(i) * 4) + 4
        x = 60 + (utils.captouch_get_rad(i)/600)
        x += (utils.captouch_get_phi(i)/600)*1j
        rot =  cmath.exp(2j *math.pi * i / 10)
        x = x * rot
        utils.draw_rect(int(x.imag+120-(size/2)),int(x.real+120-(size/2)),size,size,0b0000011111111111)
    hardware.display_update()

def foreground():
    pass

