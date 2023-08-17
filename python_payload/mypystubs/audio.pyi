def set_volume_dB(v: float) -> None:
    pass

def get_volume_dB() -> float:
    pass

def adjust_volume_dB(v: float) -> float:
    pass

def headphones_are_connected() -> bool:
    pass

def line_in_is_connected() -> bool:
    pass

def set_mute(v: bool) -> None:
    pass

def get_mute() -> bool:
    pass

def input_set_source(source: int) -> None:
    pass

def input_get_source() -> int:
    pass

INPUT_SOURCE_NONE: int
INPUT_SOURCE_LINE_IN: int
INPUT_SOURCE_HEADSET_MIC: int
INPUT_SOURCE_ONBOARD_MIC: int
