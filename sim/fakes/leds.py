from hardware import _sim

import pygame


def set_rgb(ix, r, g, b):
    ix = ((39-ix) + 1 + 32)%40;

    r = r << 3
    g = g << 2
    b = b << 3
    if r > 255:
        r = 255
    if g > 255:
        g = 255
    if b > 255:
        b = 255
    _sim.set_led_rgb(ix, r, g, b)

def set_all_rgb(r, g, b):
    for i in range(40):
        set_rgb(i, r, g, b)


def set_hsv(ix, h, s, v):
    color = pygame.Color(0)
    h /= 255.0
    color.hsva = (h, s, v, 1.0)
    r, g, b = color.r, color.g, color.b
    r *= 255
    g *= 255
    b *= 255
    _sim.set_led_rgb(ix, r, g, b)


def update():
    _sim.leds_update()
    _sim.render_gui_lazy()
