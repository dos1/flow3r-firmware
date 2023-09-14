def mode_set(kind: int) -> None:
    """
    Sets the mode of the badge, which handles what is displayed on screen and what behavior
    is used for pressing the OS shoulder button.

    Very low level, should not be used if you don't know what you're doing.
    kind values can be viewed on components/st3m/st3m_mode.h

    Need to switch to flash or SD mode? Use machine.disk_mode_flash/disk_mode_sd instead.
    """
    ...
