"""
Leds API.

Index starts above USB-C port and increments clockwise.

There are 8 LEDs per top petal, or 4 LEDs per petal.

After you're ready setting up your blink, call update(), or enable autoupdates.
"""

def set_rgb(ix: int, r: int, g: int, b: int) -> None:
    """Set LED `ix` to rgb value r, g, b

    :param ix: LED index, from 0 to 39
    :param r: Red value, from 0 to 255
    :param g: Green value, from 0 to 255
    :param b: Blue value, from 0 to 255
    """

def set_hsv(ix: int, hue: float, sat: float, val: float) -> None:
    """Set LED `ix` to hsv value hue, sat, val

    :param ix: LED index, from 0 to 39
    :param hue: Hue, from 0 to 360
    :param sat: Saturation, from 0.0 to 1.0
    :param val: Value, from 0.0 to 1.0
    """

def set_all_rgb(r: int, g: int, b: int) -> None:
    """Set all LEDs to rgb value r, g, b

    :param r: Red value, from 0 to 255
    :param g: Green value, from 0 to 255
    :param b: Blue value, from 0 to 255
    """

def set_all_hsv(h: float, s: float, v: float) -> None:
    """Set all LEDs to hsv value hue, sat, val

    :param hue: Hue, from 0 to 360
    :param sat: Saturation, from 0.0 to 1.0
    :param val: Value, from 0.0 to 1.0
    """

def update() -> None:
    """
    Update LEDs. Ie., copy the LED state from the first buffer into the second
    buffer, effectively scheduling the LED state to be presented to the user.
    """

def get_brightness() -> int:
    """
    Returns global LED brightness, 0-255. Default 69.
    """

def set_brightness(b: int) -> None:
    """
    Set global LED brightness, 0-255. Default 69.

    Only affects the LEDs after update(), or if autoupdate is enabled.
    """

def get_auto_update() -> bool:
    """
    Returns whether auto updates are on. See set_auto_update()
    """

def set_auto_update(on: bool) -> None:
    """
    Make update() be periodically called by the background task for slew rate
    animation when set to 1. This adds user changes immediately. Useful with
    low slew rates.
    """

def set_gamma(r: float, g: float, b: float) -> None:
    """
    Bend the rgb curves with an exponent each. (1,1,1) is default, (2,2,2) works
    well too If someone wants to do color calibration, this is ur friend
    """

def get_slew_rate() -> int:
    """
    Get maximum change rate of brightness. See set_slew_rate()
    """

def set_slew_rate(b: int) -> None:
    """
    Set maximum change rate of brightness. Set to 1-3 for fade effects, set
    to 255 to disable. Currently clocks at 10Hz.
    """
