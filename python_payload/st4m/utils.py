import math


def lerp(a: float, b: float, v: float) -> float:
    if v <= 0:
        return a
    if v >= 1.0:
        return b
    return a + (b - a) * v


def xy_from_polar(r: float, phi: float) -> tuple[float, float]:
    return (r * math.sin(phi), r * math.cos(phi))  # x  # y


tau = math.pi * 2
