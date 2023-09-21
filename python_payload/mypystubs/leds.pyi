from typing import Tuple

"""
Leds API.

Index starts above USB-C port and increments clockwise.

There are 8 LEDs per top petal, or 4 LEDs per petal.

After you're ready setting up your blink, call update(), or enable autoupdates.
"""

def set_slew_rate(b: int) -> None:
    """
    Set maximum change rate of channel brightness. Set to 255 to disable.
    Takes the edge off at around 200, mellows things at 150, gets slow at 100,
    takes its time at 50, molasses at 0. Default 235.
    """

def get_slew_rate() -> int:
    """
    Get maximum change rate of brightness. See set_slew_rate()
    """

def set_rgb(i: int, r: float, g: float, b: float) -> None:
    """Set LED i to rgb value r, g, b

    :param i: LED index, from 0 to 39
    :param r: Red value, from 0.0 to 1.0
    :param g: Green value, from 0.0 to 1.0
    :param b: Blue value, from 0.0 to 1.0
    """

def get_rgb(i: int) -> Tuple[float, float, float]:
    """Get rgb tuple of LED i

    :param i: LED index, from 0 to 39
    """

def set_all_rgb(r: float, g: float, b: float) -> None:
    """Set all LEDs to rgb value r, g, b

    :param r: Red value, from 0.0 to 1.0
    :param g: Green value, from 0.0 to 1.0
    :param b: Blue value, from 0.0 to 1.0
    """

def set_rgba(ix: int, r: float, g: float, b: float, a: float) -> None:
    """Set LED i to rgb alpha value r, g, b, a

    :param ix: LED index, from 0 to 39
    :param r: Red value, from 0.0 to 1.0
    :param g: Green value, from 0.0 to 1.0
    :param b: Blue value, from 0.0 to 1.0
    :param a: Alpha value, from 0.0 to 1.0
    """

def set_all_rgba(r: float, g: float, b: float, a: float) -> None:
    """Set all LEDs to rgb alpha value r, g, b, a

    :param r: Red value, from 0.0 to 1.0
    :param g: Green value, from 0.0 to 1.0
    :param b: Blue value, from 0.0 to 1.0
    :param a: Alpha value, from 0.0 to 1.0
    """

def update() -> None:
    """
    Update LEDs. Ie., copy the LED state from the first buffer into the second
    buffer, effectively scheduling the LED state to be presented to the user.
    """

def set_brightness(b: int) -> None:
    """
    Set global LED brightness, 0-255. Default 70.

    Only affects the LEDs after update(), or if autoupdate is enabled.
    """

def get_brightness() -> int:
    """
    Returns global LED brightness, 0-255. Default 70.
    """

def set_auto_update(on: bool) -> None:
    """
    Make update() be periodically called by the background task for slew rate
    animation when set to 1. This adds user changes immediately. Useful with
    low slew rates.
    """

def get_auto_update() -> bool:
    """
    Returns whether auto updates are on. See set_auto_update()
    """
