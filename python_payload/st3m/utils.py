import math

from st3m.goose import Any


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


tau = math.pi * 2
