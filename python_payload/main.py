from hardware import *
import utils
import time
import harmonic_demo
import melodic_demo
import cap_touch_demo

MODULES = [
    harmonic_demo,
    melodic_demo,
    cap_touch_demo,
]

CURRENT_APP_RUN = None
VOLUME = 0

SELECT_TEXT = [
    " ##  #### #    ####  ##  #####          ",
    "#  # #    #    #    #  #   #        ##" ,
    "#    #    #    #    #      #     #    #",
    " ##  #### #    #### #      #         #" ,
    "   # #    #    #    #      #     #    #",
    "#  # #    #    #    #  #   #        ## ",
    " ##  #### #### ####  ##    #            ",
]

BACKGROUND_COLOR = 0

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
        if((get_button(1) == 2) and (CURRENT_APP_RUN == run_menu)):
            display_fill(255)
            display_update()
            captouch_autocalib()
            time.sleep_ms(2000)
            foreground_menu()
        else:
            if(get_button(0) == 2):
                if CURRENT_APP_RUN != run_menu:
                    CURRENT_APP_RUN = run_menu
                    foreground_menu()
            else:
                if(get_button(0) == 1):
                    set_rel_volume(+1)
                if(get_button(0) == -1):
                    set_rel_volume(-1)
                CURRENT_APP_RUN()

main()
