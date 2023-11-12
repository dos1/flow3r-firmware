from _sim import _sim

import pygame


def set_rgb(ix, r, g, b):
    if r > 1:
        r /= 255
    if g > 1:
        g /= 255
    if b > 1:
        b /= 255
    r = min(1.0, max(0.0, r))
    g = min(1.0, max(0.0, g))
    b = min(1.0, max(0.0, b))
    _sim.set_led_rgb(ix, pow(r, 1 / 2.2), pow(g, 1 / 2.2), pow(b, 1 / 2.2))


def get_rgb(ix):
    return 0, 0, 0


def get_steady():
    return False


def set_all_rgb(r, g, b):
    for i in range(40):
        set_rgb(i, r, g, b)


def set_hsv(ix, h, s, v):
    color = pygame.Color(0)
    h = int(h)
    h = h % 360
    color.hsva = (h, s * 100, v * 100, 1.0)
    set_rgb(ix, color.r / 255, color.g / 255, color.b / 255)


def set_slew_rate(b: int):
    pass  # Better a no-op than not implemented at all.


def update():
    _sim.leds_update()
    pygame.event.post(pygame.event.Event(pygame.USEREVENT, {}))


def set_auto_update(b: int):
    pass


def set_brightness(b: float):
    pass


def set_gamma(r: float, g: float, b: float):
    pass
