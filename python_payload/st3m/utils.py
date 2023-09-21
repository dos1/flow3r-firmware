import math
import os

try:
    import inspect
except ImportError:
    inspect = None

from st3m.goose import Any


class RingBuf:
    """Store max n values in a static buffer, overwriting oldest with latest."""

    size: int
    cursor: int

    def __init__(self, capacity: int) -> None:
        self.buf = [None] * capacity
        self.size = 0
        self.cursor = 0

    def append(self, val: any) -> None:
        self.buf[self.cursor] = val
        self.cursor += 1
        if self.cursor > self.size:
            self.size = self.cursor
        if self.cursor == len(self.buf):
            self.cursor = 0

    def _current(self) -> int:
        current = self.cursor - 1
        if current < 0:
            current = len(self.buf) - 1
        return current

    def peek(self) -> any:
        return self.buf[self._current()]

    def __repr__(self) -> str:
        return repr(list(self))

    def __len__(self) -> int:
        return self.size

    def __iter__(self):
        # could use yield from or slices, but ranges leave less memory behind
        if self.size == len(self.buf):
            for i in range(self.cursor, len(self.buf)):
                yield self.buf[i]
        for i in range(0, self.cursor):
            yield self.buf[i]

    def __getitem__(self, key: int) -> any:
        if self.size < len(self.buf):
            if key < 0:
                return self.buf[key - len(self.buf) + self.cursor]
            return self.buf[key]

        element = self.cursor + key
        if element >= len(self.buf):
            element -= len(self.buf)
        if element < 0:
            element += len(self.buf)
        return self.buf[element]


def lerp(a: float, b: float, v: float) -> float:
    """Interpolate between a and b, based on v in [0, 1]."""
    if v <= 0:
        return a
    if v >= 1.0:
        return b
    return a + (b - a) * v


def ease_cubic(x: float) -> float:
    """Cubic ease-in/ease-out function. Maps [0, 1] to [0, 1]."""
    if x < 0.5:
        return 4 * x * x * x
    else:
        return 1 - ((-2 * x + 2) ** 3) / 2


def ease_out_cubic(x: float) -> float:
    """Cubic ease-out function. Maps [0, 1] to [0, 1]."""
    return 1 - (1 - x) ** 3


def xy_from_polar(r: float, phi: float) -> tuple[float, float]:
    return (r * math.sin(phi), r * math.cos(phi))  # x  # y


def reduce(function: Any, iterable: Any, initializer: Any = None) -> Any:
    """functools.reduce but portable and poorly typed."""
    it = iter(iterable)
    if initializer is None:
        value = next(it)
    else:
        value = initializer
    for element in it:
        value = function(value, element)
    return value


def get_function_args(fun: Any) -> list[str]:
    """
    Returns a list of argument names for a function

    Excludes keyword-only arguments (those are in argspec[4] if anyone needs it)

    Uses either __argspec__ (an extension specific to our fork of micropython)
    or inspect.getfullargspec() which are very similar except the latter
    populates more fields and returns a named tuple
    """

    if hasattr(fun, "__argspec__"):
        return fun.__argspec__[0]
    elif inspect and hasattr(inspect, "getfullargspec"):
        return inspect.getfullargspec(fun)[0]
    else:
        raise NotImplementedError("No implementation of getfullargspec found")


def save_file_if_changed(filename: str, desired_contents: str) -> bool:
    save = False
    if not os.path.exists(filename):
        save = True
    else:
        with open(filename, "r") as f:
            save = f.read() != desired_contents

    if save:
        with open(filename, "w") as f:
            f.write(desired_contents)

    return save


tau = math.pi * 2
