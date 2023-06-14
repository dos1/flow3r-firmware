from st3m import menu, event, control, ui
from st3m.system import audio, hardware

from apps import captouch_calibrator, captouch_test


def get_menu(app):
    m = menu.Menu("settings")
    m_captouch = menu.Menu("captouch")

    control_debug_input = control.ControlSwitch(
        name="debug",
        # TODO (iggy) think about a better way to get our app
        on_set=app.set_input_overlay,
        on_get=app.get_input_overlay,
        default=False,
    )

    item_input_overlay = menu.MenuItemControl("input overlay", control_debug_input)
    m_captouch.add(item_input_overlay)

    m_captouch.add(menu.MenuItemApp(captouch_calibrator.app))
    m_captouch.add(menu.MenuItemApp(captouch_test.app))

    m_audio = menu.Menu("audio")
    m_speaker = menu.Menu("speaker")
    m_head = menu.Menu("headphones")

    vol = control.ControlKnob(
        name="vol", on_mod=audio.adjust_volume_dB, on_get=audio.get_volume_relative
    )
    m_audio.add(menu.MenuItemControl("volume", vol))
    m_head.add(
        menu.MenuItemControl(
            "vol head",
            control.ControlKnob(
                name="vol",
                on_mod=audio.headphones_adjust_volume_dB,
                on_get=audio.headphones_get_volume_relative,
            ),
        )
    )

    m_speaker.add(
        menu.MenuItemControl(
            "vol speaker",
            control.ControlKnob(
                name="vol",
                on_mod=audio.speaker_adjust_volume_dB,
                on_get=audio.speaker_get_volume_relative,
            ),
        )
    )

    m_audio.add(
        menu.MenuItemControl(
            "mute",
            control.ControlSwitch(
                name="mute", on_set=audio.set_mute, on_get=audio.get_mute
            ),
        )
    )

    m_head.add(
        menu.MenuItemControl(
            "mute head",
            control.ControlSwitch(
                name="mute",
                on_set=audio.headphones_set_mute,
                on_get=audio.headphones_get_mute,
            ),
        )
    )

    m_head.add(
        menu.MenuItemControl(
            "detected headphones",
            control.ControlSwitch(
                name="connected?", on_get=audio.headphones_are_connected
            ),
        )
    )

    m_speaker.add(
        menu.MenuItemControl(
            "mute speaker",
            control.ControlSwitch(
                name="mute",
                on_set=audio.speaker_set_mute,
                on_get=audio.speaker_get_mute,
            ),
        )
    )

    m_speaker.add(
        menu.MenuItemControl(
            "min vol speaker",
            control.ControlKnob(
                name="min",
                min=-40,
                max=14,
                step=1,
                on_set=audio.speaker_set_minimum_volume_dB,
                on_get=audio.speaker_get_minimum_volume_dB,
            ),
        )
    )

    m_speaker.add(
        menu.MenuItemControl(
            "max vol speaker",
            control.ControlKnob(
                name="max",
                min=-40,
                max=14,
                step=1,
                on_set=audio.speaker_set_maximum_volume_dB,
                on_get=audio.speaker_get_maximum_volume_dB,
            ),
        )
    )

    m_head.add(
        menu.MenuItemControl(
            "min vol headphones",
            control.ControlKnob(
                name="min",
                min=-40,
                max=14,
                step=1,
                on_set=audio.headphones_set_minimum_volume_dB,
                on_get=audio.headphones_get_minimum_volume_dB,
            ),
        )
    )

    m_head.add(
        menu.MenuItemControl(
            "max vol headphones",
            control.ControlKnob(
                name="max",
                min=-40,
                max=14,
                step=1,
                on_set=audio.headphones_set_maximum_volume_dB,
                on_get=audio.headphones_get_maximum_volume_dB,
            ),
        )
    )

    m.add(menu.MenuItemSubmenu(m_audio))
    m.add(menu.MenuItemSubmenu(m_captouch))
    m_audio.add(menu.MenuItemSubmenu(m_speaker))
    m_audio.add(menu.MenuItemSubmenu(m_head))

    return m
