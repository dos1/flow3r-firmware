import time
from hardware import *

RED = 0b1111100000000000
GREEN = 0b0000011111100000
BLUE = 0b0000000000011111

def clear_all_leds():
    for i in range(40):
        led_set_rgb(i, 0, 0, 0)
    leds_update()

def draw_text_big(text, x, y):
    ypos = 120+int(len(text)) + int(y)
    xpos = 120+int(len(text[0])) + int(x)
    for l, line in enumerate(text):
        for p, pixel in enumerate(line):
            if(pixel == '#'):
                display_set_pixel(xpos - 2*p, ypos - 2*l, RED)
                display_set_pixel(xpos - 2*p, ypos - 2*l-1, BLUE)
                display_set_pixel(xpos - 2*p-1, ypos - 2*l, BLUE)
                display_set_pixel(xpos - 2*p-1, ypos - 2*l-1, RED)

def highlight_bottom_petal(num, RED, GREEN, BLUE):
    start = 1 + 8*num
    for i in range(7):
        led_set_rgb(((i+start)%40), RED, GREEN, BLUE)

def long_bottom_petal_captouch_blocking(num, ms):
    if(captouch_get((num*2) + 1) == 1):
        time.sleep_ms(ms)
        if(captouch_get((num*2) + 1) == 1):
            return True
    return False

def draw_rect(x,y,w,h,col):
    for j in range(w):
        for k in range(h):
            display_set_pixel(x+j,y+k,col)

def draw_volume_slider(volume):
    length = 96 + ((volume - 20) * 1.6)
    if length > 96:
        length = 96
    if length < 0:
        length = 0
    length = int(length)
    draw_rect(70,20,100,10,GREEN)
    draw_rect(71,21,98,8, 0)
    draw_rect(72+96-length,22,length,6,GREEN)
