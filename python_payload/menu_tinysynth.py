from synth import tinysynth

import menu
import event
import control
import ui
import hardware
import audio


ui_input = ui.Icon("")


def set_play(value):
    print ("set_controls_overlay")
    if value:
        synth.start()
    else:
        synth.stop()

def set_volume(value):
    db = int(value*60-40)
    print("DB",db)
    audio.set_volume_dB(db)

def set_frequency(value):
    f = 440+value*440
    synth.freq(f)
    

def get_menu():
    m = menu.Menu("tinysynth")

    freq=control.ControlKnob(name="freq",action=set_frequency,default=0.0)
    m.add(menu.MenuItemControl("freq",freq))

    vol = control.ControlKnob(name="vol",action=set_volume,default=0.0)
    m.add(menu.MenuItemControl("volume",vol))

    play = control.ControlSwitch(name="play",action=set_play,default=False)
    m.add(menu.MenuItemControl("play",play))
    return m

synth = tinysynth(440,0)

m = get_menu()
