import time
from hardware import *

RED = 0b1111100000000000
GREEN = 0b0000011111100000
BLUE = 0b0000000000011111

def clear_all_leds():
    for i in range(40):
        set_led_rgb(i, 0, 0, 0)
    update_leds()

def draw_text_big(text, x, y):
    ypos = 120+int(len(text)) + int(y)
    xpos = 120+int(len(text[0])) + int(x)
    for l, line in enumerate(text):
        for p, pixel in enumerate(line):
            if(pixel == '#'):
                display_draw_pixel(xpos - 2*p, ypos - 2*l, RED)
                display_draw_pixel(xpos - 2*p, ypos - 2*l-1, BLUE)
                display_draw_pixel(xpos - 2*p-1, ypos - 2*l, BLUE)
                display_draw_pixel(xpos - 2*p-1, ypos - 2*l-1, RED)

def highlight_bottom_petal(num, RED, GREEN, BLUE):
    start = 4 + 8*num
    for i in range(7):
        set_led_rgb(((i+start)%40), RED, GREEN, BLUE)
    update_leds()

def long_bottom_petal_captouch_blocking(num, ms):
    if(get_captouch((num*2) + 1) == 1):
        time.sleep_ms(ms)
        if(get_captouch((num*2) + 1) == 1):
            return True
    return False

def draw_rect(x,y,w,h,col):
    for j in range(w):
        for k in range(h):
            display_draw_pixel(x+j,y+k,col)

def draw_volume_slider(ctx, volume):
    length = 96 + ((volume - 20) * 1.6)
    if length > 96:
        length = 96
    if length < 0:
        length = 0
    length = int(length)

    ctx.rgb(0,0,0)#dummy
    ctx.round_rectangle(-49,41,98,8,3).fill()#dummy idk

    ctx.rgb(0,255,0)
    ctx.round_rectangle(-51,49,102,12,3).fill()
    ctx.rgb(0,0,0)
    ctx.round_rectangle(-50,50,100,10,3).fill()
    ctx.rgb(0,255,0)
    ctx.round_rectangle(-48,52, length ,6,3).fill()
