from machine import Pin
from hardware import *
import time
import cap_touch_demo
import melodic_demo

MODULES = [
    cap_touch_demo,
    melodic_demo,
]

BOOTSEL_PIN = Pin(0, Pin.IN)
VOL_UP_PIN = Pin(35, Pin.IN, Pin.PULL_UP)
VOL_DOWN_PIN = Pin(37, Pin.IN, Pin.PULL_UP)

foreground = 0
volume = 0

SELECT_TEXT = [
    " ##  #### #    ####  ##  ##### #",
    "#  # #    #    #    #  #   #   #",
    "#    #    #    #    #      #   #",
    " ##  #### #    #### #      #   #",
    "   # #    #    #    #      #   #",
    "#  # #    #    #    #  #   #    ",
    " ##  #### #### ####  ##    #   #",
]

background = 0
g = 0b0000011111100000
r = 0b1111100000000000
b = 0b0000000000011111

# pin numbers
# right side: left 37, down 0, right 35
# left side: left 7, down 6, right 5
# NOTE: All except for 0 should be initialized with Pin.PULL_UP
# 0 (bootsel) probably not but idk? never tried

def clear_all_leds():
    for i in range(40):
        set_led_rgb(i, 0, 0, 0)
    update_leds()

def draw_text_big(text, x, y):
    ypos = 120+int(len(text)/2) + int(y)
    xpos = 120+int(len(text[0])/2) + int(x)
    for l, line in enumerate(text):
        for p, pixel in enumerate(line):
            if(pixel == '#'):
                display_draw_pixel(xpos - 2*p, ypos - 2*l, r)
                display_draw_pixel(xpos - 2*p, ypos - 2*l-1, b)
                display_draw_pixel(xpos - 2*p-1, ypos - 2*l, b)
                display_draw_pixel(xpos - 2*p-1, ypos - 2*l-1, r)

def highlight_bottom_petal(num, r, g, b):
    start = 4 + 8*num
    for i in range(7):
        set_led_rgb(((i+start)%40), r, g, b)
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
    draw_text_big(SELECT_TEXT, 0, 0)
    draw_volume_slider()
    display_update()

    selected_petal = None
    selected_module = None
    for petal, module in enumerate(MODULES):
        if long_bottom_petal_captouch_blocking(petal, 20):
            selected_petal = petal
            selected_module = module
            break

    if selected_petal is not None:
        clear_all_leds()
        highlight_bottom_petal(selected_petal, 55, 0, 0)
        display_fill(background)
        display_update()
        foreground = selected_module.run
        time.sleep_ms(100)
        clear_all_leds()
        selected_module.foreground()

def foreground_menu():
    clear_all_leds()
    highlight_bottom_petal(0,0,55,55);
    highlight_bottom_petal(1,55,0,55);
    display_fill(background)
    draw_text_big(SELECT_TEXT, 0, 0)
    display_update()

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

def main():
    global foreground
    time.sleep_ms(5000)
    captouch_autocalib()

    for module in MODULES:
        module.init()

    foreground = run_menu
    foreground_menu()
    set_global_volume_dB(volume)

    while True:
        if(BOOTSEL_PIN.value() == 0):
            if foreground == run_menu:
                captouch_autocalib()
            else:
                foreground = run_menu
                foreground_menu()
        if(VOL_UP_PIN.value() == 0):
            set_rel_volume(+3)
        if(VOL_DOWN_PIN.value() == 0):
            set_rel_volume(-3)
        foreground()

main()
