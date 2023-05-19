from machine import Pin
from hardware import *
import utils
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

CURRENT_APP_RUN = None
VOLUME = 0

SELECT_TEXT = [
    " ##  #### #    ####  ##  ##### #",
    "#  # #    #    #    #  #   #   #",
    "#    #    #    #    #      #   #",
    " ##  #### #    #### #      #   #",
    "   # #    #    #    #      #   #",
    "#  # #    #    #    #  #   #    ",
    " ##  #### #### ####  ##    #   #",
]

BACKGROUND_COLOR = 0

# pin numbers
# right side: left 37, down 0, right 35
# left side: left 7, down 6, right 5
# NOTE: All except for 0 should be initialized with Pin.PULL_UP
# 0 (bootsel) probably not but idk? never tried

def run_menu():
    global CURRENT_APP_RUN
    display_fill(BACKGROUND_COLOR)
    utils.draw_text_big(SELECT_TEXT, 0, 0)
    utils.draw_volume_slider(VOLUME)
    display_update()

    selected_petal = None
    selected_module = None
    for petal, module in enumerate(MODULES):
        if utils.long_bottom_petal_captouch_blocking(petal, 20):
            selected_petal = petal
            selected_module = module
            break

    if selected_petal is not None:
        utils.clear_all_leds()
        utils.highlight_bottom_petal(selected_petal, 55, 0, 0)
        display_fill(BACKGROUND_COLOR)
        display_update()
        CURRENT_APP_RUN = selected_module.run
        time.sleep_ms(100)
        utils.clear_all_leds()
        selected_module.foreground()

def foreground_menu():
    utils.clear_all_leds()
    utils.highlight_bottom_petal(0,0,55,55);
    utils.highlight_bottom_petal(1,55,0,55);
    display_fill(BACKGROUND_COLOR)
    utils.draw_text_big(SELECT_TEXT, 0, 0)
    display_update()

def set_rel_volume(vol):
    global VOLUME
    vol += VOLUME
    if vol > 20:
        vol = 20
    if vol < -40:
        vol = -40
    VOLUME = vol
    if vol == -40: #mute
        set_global_volume_dB(-90)
    else:
        set_global_volume_dB(VOLUME)
    time.sleep_ms(100)

def main():
    global CURRENT_APP_RUN
    time.sleep_ms(5000)
    captouch_autocalib()

    for module in MODULES:
        module.init()

    CURRENT_APP_RUN = run_menu
    foreground_menu()
    set_global_volume_dB(VOLUME)

    while True:
        if(BOOTSEL_PIN.value() == 0):
            if CURRENT_APP_RUN == run_menu:
                captouch_autocalib()
            else:
                CURRENT_APP_RUN = run_menu
                foreground_menu()
        if(VOL_UP_PIN.value() == 0):
            set_rel_volume(+3)
        if(VOL_DOWN_PIN.value() == 0):
            set_rel_volume(-3)
        CURRENT_APP_RUN()

main()
