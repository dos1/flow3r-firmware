_volume = 0
_muted = False


def set_volume_dB(v: float) -> None:
    global _volume
    _volume = v


def get_volume_dB() -> float:
    global _volume
    return _volume


def adjust_volume_dB(v) -> float:
    global _volume
    _volume += v
    if _volume < -48:
        _volume = -47
    if _volume > 14:
        _volume = 14


def headphones_are_connected() -> bool:
    return False


def set_mute(v: bool) -> None:
    global _muted
    _muted = v


def get_mute() -> bool:
    global _muted
    return _muted
