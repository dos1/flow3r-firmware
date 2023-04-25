from machine import Pin
from hardware import *
import time
import cap_touch_demo
import melodic_demo
boot = Pin(0, Pin.IN)
vol_up = Pin(35, Pin.IN, Pin.PULL_UP)
vol_down = Pin(37, Pin.IN, Pin.PULL_UP)

# pin numbers
# right side: left 37, down 0, right 35
# left side: left 7, down 6, right 5
# NOTE: All except for 0 should be initialized with Pin.PULL_UP
# 0 (bootsel) probably not but idk? never tried

def clear_all_leds():
    for i in range(40):
        set_led_rgb(i, 0, 0, 0)
    update_leds()

select = [\
[0,1,1,0,0,1,1,1,1,0,1,0,0,0,0,1,1,1,1,0,0,1,1,0,0,1,1,1,1,1,0,1],\
[1,0,0,1,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,1,0,0,0,1,0,0,0,1],\
[1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,1],\
[0,1,1,0,0,1,1,1,1,0,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,0,1,0,0,0,1],\
[0,0,0,1,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,1],\
[1,0,0,1,0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1,0,0,1,0,0,0,1,0,0,0,0],\
[0,1,1,0,0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0,0,1,1,0,0,0,0,1,0,0,0,1],\
]

def draw_text_big(text, x, y):
    ypos = 120+int(len(text)/2) + int(y)
    xpos = 120+int(len(text[0])/2) + int(x)
    for l, line in enumerate(text):
        for p, pixel in enumerate(line):
            if(pixel == 1):
                display_draw_pixel(xpos - 2*p, ypos - 2*l, r)
                display_draw_pixel(xpos - 2*p, ypos - 2*l-1, b)
                display_draw_pixel(xpos - 2*p-1, ypos - 2*l, b)
                display_draw_pixel(xpos - 2*p-1, ypos - 2*l-1, r)

def highlight_bottom_petal(num, r, g, b):
    start = 4 + 8*num
    for i in range(7):
        set_led_rgb((i+start%40), r, g, b)
    update_leds()
    
def long_bottom_petal_captouch_blocking(num, ms):
    if(get_captouch((num*2) + 1) == 1):
        time.sleep_ms(ms)
        if(get_captouch((num*2) + 1) == 1):
            return True
    return False
    
foreground = 0
volume = 0

def init_menu():
    pass

def draw_rect(x,y,w,h,col):
    for j in range(w):
        for k in range(h):
            display_draw_pixel(x+j,y+k,col)
            
    
def draw_volume_slider():
    global volume
    length = 96 + ((volume - 20) * 1.6)
    if length > 96:
        length = 96
    if length < 0:
        length = 0
    length = int(length)
    draw_rect(70,20,100,10,g)
    draw_rect(71,21,98,8, 0)
    draw_rect(72+96-length,22,length,6,g)
    

def run_menu():
    global foreground
    display_fill(background)
    draw_text_big(select, 0, 0)
    draw_volume_slider()
    display_update()

    if long_bottom_petal_captouch_blocking(0,20):
        clear_all_leds()
        highlight_bottom_petal(0,255,0,0)
        display_fill(background)
        display_update()
        foreground = cap_touch_demo.run
        time.sleep_ms(100)
        clear_all_leds()
        captouch_autocalib()
        cap_touch_demo.foreground()
    if long_bottom_petal_captouch_blocking(1,20):
        clear_all_leds()
        highlight_bottom_petal(0,255,0,0)
        display_fill(background)
        display_update()
        foreground = melodic_demo.run
        time.sleep_ms(100)
        clear_all_leds()
        captouch_autocalib()
        melodic_demo.foreground()

def foreground_menu():
    clear_all_leds()
    highlight_bottom_petal(0,0,255,255);
    highlight_bottom_petal(1,255,0,255);
    display_fill(background)
    draw_text_big(select, 0, 0)
    display_update()

background = 0;
g = 0b0000011111100000;
r = 0b1111100000000000;
b = 0b0000000000011111;

time.sleep_ms(5000)
captouch_autocalib()
cap_touch_demo.init()
melodic_demo.init()

init_menu()
foreground = run_menu
foreground_menu()
set_global_volume_dB(volume)

def set_rel_volume(vol):
    global volume
    vol += volume
    if vol > 20:
        vol = 20
    if vol < -40:
        vol = -40
    volume = vol
    if vol == -40: #mute
        set_global_volume_dB(-90)
    else:
        set_global_volume_dB(volume)
    time.sleep_ms(100)

while True:
    if(boot.value() == 0):
        if foreground == run_menu:
            captouch_autocalib()
        else:
            foreground = run_menu
            foreground_menu()
    if(vol_up.value() == 0):
        set_rel_volume(+3)
    if(vol_down.value() == 0):
        set_rel_volume(-3)
    foreground()
