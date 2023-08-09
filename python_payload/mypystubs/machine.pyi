from typing import Any

def reset() -> None:
    pass

class Pin:
    """
    See the official micropython docs for more details:

    https://docs.micropython.org/en/latest/library/machine.Pin.html
    """

    IN: int = 1
    OUT: int = 2
    OPEN_DRAIN: int = 3
    PULL_UP: int = 4
    PULL_DOWN: int = 5
    IRQ_RISING: int = 6
    IRQ_FALLING: int = 7
    WAKE_LOW: int = 8
    WAKE_HIGH: int = 9
    DRIVE_0: int = 10
    DRIVE_1: int = 11
    DRIVE_2: int = 12
    DRIVE_3: int = 13

    def __init__(
        self,
        id: int,
        mode: int = -1,
        pull: int = -1,
        *,
        value: Any = None,
        drive: int = 0,
        alt: int = -1
    ) -> None:
        """
        Available pins for the flow3r badge are:

        * 4 (badge link, right tip)
        * 5 (badge link, right ring)
        * 6 (badge link, left ring)
        * 7 (badge link, left tip)
        * 17 (QWIIC SDA)
        * 45 (QWIIC SCL)
        """
    def init(
        self,
        mode: int = -1,
        pull: int = -1,
        *,
        value: Any = None,
        drive: int = 0,
        alt: int = -1
    ) -> None: ...
    def value(self, x: Any = None) -> int: ...
    def __call__(self, x: Any = None) -> None: ...
    def on(self) -> None: ...
    def off(self) -> None: ...
    def irq(
        self,
        handler: Any = None,
        trigger: Any = IRQ_FALLING | IRQ_RISING,
        *,
        priority: int = 1,
        wake: Any = None,
        hard: bool = False
    ) -> None: ...

class ADC:
    ATTN_11DB: int = 1
    def __init__(self, p: Pin, atten: int = 0): ...
    def read_uv(self) -> float: ...
