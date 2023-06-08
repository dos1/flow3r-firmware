import time
from hardware import *
from st3m.system import audio

RED = 0b1111100000000000
GREEN = 0b0000011111100000
BLUE = 0b0000000000011111

def clear_all_leds():
    for i in range(40):
        set_led_rgb(i, 0, 0, 0)
    update_leds()

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

def draw_volume_slider(ctx, volume):
    length = int(96*volume)

    ctx.rgb(0,0,0)#dummy
    ctx.round_rectangle(-49,41,98,8,3).fill()#dummy idk

    ctx.rgb(0,255,0)
    ctx.round_rectangle(-51,49,102,12,3).fill()
    ctx.rgb(0,0,0)
    ctx.round_rectangle(-50,50,100,10,3).fill()
    ctx.rgb(0,255,0)
    ctx.round_rectangle(-48,52, length ,6,3).fill()
