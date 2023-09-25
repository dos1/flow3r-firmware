_volume = 0
_muted = False


def set_volume_dB(v: float) -> None:
    global _volume
    _volume = v


def get_volume_dB() -> float:
    global _volume
    return _volume


def get_volume_relative() -> float:
    return 0


def headphones_set_volume_dB(v: float) -> None:
    pass


def speaker_set_volume_dB(v: float) -> None:
    pass


def headphones_set_minimum_volume_dB(v: float) -> None:
    pass


def speaker_set_minimum_volume_dB(v: float) -> None:
    pass


def headphones_set_maximum_volume_dB(v: float) -> None:
    pass


def speaker_set_maximum_volume_dB(v: float) -> None:
    pass


def speaker_set_eq_on(enable: bool) -> None:
    pass


def adjust_volume_dB(v) -> float:
    global _volume
    _volume += v
    if _volume < -48:
        _volume = -47
    if _volume > 14:
        _volume = 14


def headphones_are_connected() -> bool:
    return False


def line_in_is_connected() -> bool:
    return False


def set_mute(v: bool) -> None:
    global _muted
    _muted = v


def get_mute() -> bool:
    global _muted
    return _muted


def headset_set_gain_dB(v: float) -> None:
    pass


def headset_get_gain_dB() -> float:
    return 10
