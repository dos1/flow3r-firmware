from typing import Tuple
import pygame
from math import tau


def hsv_to_rgb(h: float, s: float, v: float) -> Tuple[float, float, float]:
    color = pygame.Color(0)
    color.hsva = ((h % tau) / tau * 360, s * 100, v * 100, 100)
    return color.normalize()[:3]


def rgb_to_hsv(r: float, g: float, b: float) -> Tuple[float, float, float]:
    r = min(1, max(0, r))
    g = min(1, max(0, g))
    b = min(1, max(0, b))
    color = pygame.Color(r, g, b)
    h, s, v, a = color.hsva
    return (h / 360 * tau, s / 100, v / 100)
