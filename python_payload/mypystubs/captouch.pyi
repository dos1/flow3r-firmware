from typing import Protocol, List


class CaptouchPetalPadsState(Protocol):
    """
    Current state of pads on a captouch petal.

    Not all petals have all pads. Top petals have a a base, cw and ccw pad.
    Bottom petals have a base and tip pad.
    """

    @property
    def tip(self) -> bool:
        """
        True if the petals's tip is currently touched.
        """
        ...
    
    @property
    def base(self) -> bool:
        """
        True if the petal's base is currently touched.
        """
        ...

    @property
    def cw(self) -> bool:
        """
        True if the petal's clockwise pad is currently touched.
        """
        ...

    @property
    def ccw(self) -> bool:
        """
        True if the petal's counter clockwise pad is currently touched.
        """
        ...


class CaptouchPetalState(Protocol):
    @property
    def pressed(self) -> bool:
        """
        True if any of the petal's pads is currently touched.
        """
        ...

    @property
    def top(self) -> bool:
        """
        True if this is a top petal.
        """
        ...

    @property
    def bottom(self) -> bool:
        """
        True if this is a bottom petal.
        """
        ...

    @property
    def pads(self) -> CaptouchPetalPadsState:
        """
        State of individual pads of the petal.
        """
        ...


class CaptouchState(Protocol):
    """
    State of captouch sensors, captured at some time.
    """
    @property
    def petals(self) -> List[CaptouchPetalState]:
        """
        State of individual petals.

        Contains 10 elements, with the zeroth element being the pad closest to
        the USB port. Then, every other pad in a counter-clockwise direction.

        Pads 0, 2, 4, 6, 8 are Top pads.

        Pads 1, 3, 5, 7, 9 are Bottom pads.
        """
        ...


def read() -> CaptouchState:
    """
    Reads current captouch state from hardware and returns a snapshot in time.
    """
    ...

def calibration_active() -> bool:
    """
    Returns true if the captouch system is current recalibrating.
    """
    ...