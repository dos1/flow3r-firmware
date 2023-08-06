try:
    from typing import Protocol
except ImportError:
    from typing_extensions import Protocol

class Pin(Protocol):
    """
    See the official micropython docs for more details:

    https://docs.micropython.org/en/latest/library/machine.Pin.html
    """

    IN: int
    OUT: int
    OPEN_DRAIN: int
    PULL_UP: int
    PULL_DOWN: int
    IRQ_RISING: int = 1
    IRQ_FALLING: int = 2
    WAKE_LOW: int
    WAKE_HIGH: int
    DRIVE_0: int
    DRIVE_1: int
    DRIVE_2: int
    DRIVE_3: int

    def __init__(self, id, mode=-1, pull=-1, *, value=None, drive=0, alt=-1):
        """
        Available pins for the flow3r badge are:

        * 4 (badge link, right tip)
        * 5 (badge link, right ring)
        * 6 (badge link, left ring)
        * 7 (badge link, left tip)
        * 17 (QWIIC SDA)
        * 45 (QWIIC SCL)
        """
    def init(self, mode=-1, pull=-1, *, value=None, drive=0, alt=-1): ...
    def value(self, x=None): ...
    def __call__(self, x=None): ...
    def on(self): ...
    def off(self): ...
    def irq(
        self,
        handler=None,
        trigger=IRQ_FALLING | IRQ_RISING,
        *,
        priority=1,
        wake=None,
        hard=False
    ): ...
