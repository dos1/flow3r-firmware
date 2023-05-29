from hardware import *
import utils
import time
import harmonic_demo
import melodic_demo
import cap_touch_demo
import demo_worms

MODULES = [
    harmonic_demo,
    melodic_demo,
    cap_touch_demo,
    demo_worms,
]

CURRENT_APP_RUN = None
VOLUME = 0
ctx = None

BACKGROUND_COLOR = 0


def draw_volume_slider(volume):
    global ctx
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

def run_menu():
    global CURRENT_APP_RUN
    global ctx
    display_fill(BACKGROUND_COLOR)
    draw_volume_slider(VOLUME)
    ctx.move_to(0,0).rgb(255,0,255).text("select :3")
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
    utils.highlight_bottom_petal(2,55,55,0);
    utils.highlight_bottom_petal(3,0,110,0);
    display_fill(BACKGROUND_COLOR)
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

def captouch_cal():
    global ctx
    display_fill(0b0000000111100111)
    ctx.move_to(0,0).rgb(0,255,0).text("cal")
    display_update()
    time.sleep_ms(500)
    display_fill(0b0011100000000111)
    ctx.move_to(0,0).rgb(0,255,0).text("cal")
    display_update()
    captouch_autocalib()
    while(captouch_calibration_active()):
        pass
    display_fill(0)
    display_update()

def main():
    global CURRENT_APP_RUN
    global ctx
    while not init_done():
        pass

    captouch_autocalib() #dry run
    while(captouch_calibration_active()):
        pass

    ctx = get_ctx()
    ctx.text_align = ctx.CENTER
    ctx.text_baseline = ctx.MIDDLE

    captouch_cal()

    for module in MODULES:
        module.init()

    CURRENT_APP_RUN = run_menu
    foreground_menu()
    set_global_volume_dB(VOLUME)

    while True:
        if((get_button(1) == 2) and (CURRENT_APP_RUN == run_menu)):
            captouch_cal()
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
