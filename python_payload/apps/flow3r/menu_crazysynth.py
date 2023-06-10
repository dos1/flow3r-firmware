from synth import tinysynth

from st3m import menu, event, control, ui
from st3m.system import hardware, audio

synths = [tinysynth(440, 0), tinysynth(440, 0), tinysynth(440, 0)]
# synth = tinysynth(440,0)


def set_play(value):
    print("set_controls_overlay")
    if value:
        for synth in synths:
            synth.start()
    else:
        for synth in synths:
            synth.stop()


the_freq = 440


def set_frequency(value):
    global the_freq
    the_freq = value
    for synth in synths:
        synth.freq(the_freq)


def set_pitch0(value):
    synths[0].freq(the_freq + the_freq * value)


def set_pitch1(value):
    synths[1].freq(the_freq + the_freq * value)


def set_pitch2(value):
    synths[2].freq(the_freq + the_freq * value)


def get_menu():
    m = menu.Menu("crazysynth")

    freq = control.ControlKnob(
        name="freq",
        on_set=set_frequency,
        default=the_freq,
        min=220,
        max=440 * 4,
        step=10,
    )
    # m.add_petal(menu.MenuItemControl("freq",freq), petal_index=7)
    m.add(menu.MenuItemControl("freq", freq))

    m.add(
        menu.MenuItemControl(
            "mute",
            control.ControlSwitch(
                name="mute", on_set=audio.set_mute, on_get=audio.get_mute
            ),
        )
    )

    pitch0 = control.ControlSlide(
        name="pitch", on_set=set_pitch0, default=0, min=-0.5, max=0.5
    )
    m.add_petal(menu.MenuItemControl("pitch0", pitch0), petal_index=3)

    pitch1 = control.ControlSlide(
        name="pitch", on_set=set_pitch1, default=0, min=-0.5, max=0.5
    )
    m.add_petal(menu.MenuItemControl("pitch1", pitch1), petal_index=5)

    pitch2 = control.ControlSlide(
        name="pitch", on_set=set_pitch2, default=0, min=-0.5, max=0.5
    )
    m.add_petal(menu.MenuItemControl("pitch2", pitch2), petal_index=7)

    vol = control.ControlKnob(
        name="vol", on_mod=audio.adjust_volume_dB, on_get=audio.get_volume_relative
    )

    mi_vol = menu.MenuItemControl("volume", vol)

    m.add(mi_vol)
    # m.add_petal(mi_vol,1)

    play = control.ControlSwitch(name="play", on_set=set_play, default=False)
    # m.add_petal(menu.MenuItemControl("play",play), petal_index=5)
    m.add(menu.MenuItemControl("play", play))

    m.ui.r = 60
    return m


m = get_menu()
