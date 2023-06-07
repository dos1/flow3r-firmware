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
    
def set_pitch(value):
    p = 440
    print("p:",p)
    synth.freq(value)
    

def get_menu():
    m = menu.Menu("tinysynth")

    freq=control.ControlKnob(name="freq",on_set=set_frequency,default=0.0)
    m.add(menu.MenuItemControl("freq",freq))

    pitch = control.ControlSlide(name="pitch",on_set=set_frequency,default=0)
    m.add(menu.MenuItemControl("pitch",pitch))

    vol = control.ControlKnob(name="vol",
        #on_set=set_volume
        on_mod=audio.adjust_volume_dB,
        on_get=audio.get_volume_relative
    )
    m.add(menu.MenuItemControl("volume",vol))

    play = control.ControlSwitch(name="play",on_set=set_play,default=False)
    m.add(menu.MenuItemControl("play",play))
    
    return m

synth = tinysynth(440,0)

m = get_menu()
