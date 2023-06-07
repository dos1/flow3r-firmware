from synth import tinysynth

import menu
import event
import control
import ui
import hardware
import audio

synth = tinysynth(440,0)

def set_play(value):
    print ("set_controls_overlay")
    if value:
        synth.start()
    else:
        synth.stop()

the_freq = 440

def set_frequency(value):
    global the_freq
    the_freq = value
    synth.freq(the_freq)
    
def set_pitch(value):
    synth.freq(the_freq+the_freq*value)
    print("value")
 

def get_menu():
    m = menu.Menu("tinysynth")

    freq=control.ControlKnob(
        name="freq",
        on_set=set_frequency,
        default=the_freq,
        min=220,max=440*4,step=10
    )
    m.add_petal(menu.MenuItemControl("freq",freq), petal_index=7)


    pitch = control.ControlSlide(
        name="pitch",
        on_set=set_pitch,
        default=0,
        min=-0.5,max=0.5
    )
    m.add_petal(menu.MenuItemControl("pitch",pitch), petal_index=9)


    vol = control.ControlKnob(name="vol",
        on_mod=audio.adjust_volume_dB,
        on_get=audio.get_volume_relative
    )

    mi_vol = menu.MenuItemControl("volume",vol)

    #m.add(mi_vol)
    m.add_petal(mi_vol,1)

    play = control.ControlSwitch(name="play",on_set=set_play,default=False)
    m.add_petal(menu.MenuItemControl("play",play), petal_index=5)
    
    return m



m = get_menu()
