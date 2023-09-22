import leds
import json
import math
import random

from st3m.ui import colours
from st3m.settings import onoff_leds_random_menu

def _clip(val):
    if val > 1.:
        return 1.
    if val < 0.:
        return 1.
    return val

def set_menu_colors():
    """
    set all LEDs to the configured menu colors if provided in
    /flash/menu_leds.json and settings.onoff_leds_random_menu
    is false, else calls pretty_pattern.
    leds.update() must be called externally.
    """
    if onoff_leds_random_menu.value:
        pretty_pattern()
        return
        
    path = "/flash/menu_leds.json"
    try:
        with open(path, "r") as f:
            settings = json.load(f)
    except OSError:
        pretty_pattern()
        return
    for i in range(40):
        col = settings["leds"][i]
        leds.set_rgb(i, col[0], col[1], col[2])

def pretty_pattern():
    """
    generates a pretty random pattern.
    leds.update() must be called externally.
    """
    hsv = [0.,0.,0.]
    hsv[0] = random.random() * math.tau
    hsv[1] = random.random() * 0.3 + 0.7
    hsv[2] = random.random() * 0.3 + 0.7
    start = int(random.random() * 40)
    for i in range(48):
        hsv[0] += (random.random() - 0.5) * 2
        for j in range(1,2):
            hsv[j] += (random.random() - 0.5) / 2 
            # asymmetric clipping: draw it to bright colors
            if hsv[j] < 0.7:
                hsv[j] = 0.7 + (0.7 - hsv[j])/2
            if hsv[j] > 1:
                hsv[j] = 1

        j = (i+start)%40
        if i < 40: 
            leds.set_rgb(j, *colours.hsv_to_rgb(*hsv))
        else:
            hsv_old = colours.hsv_to_rgb(*leds.get_rgb(j))
            hsv_mixed = [0.,0.,0.]
            k = (i-39)/8
            for i in range(3):
                hsv_mixed[i] = hsv_old[i] * k + hsv[i] * (1-k)
            leds.set_rgb(j, *colours.hsv_to_rgb(*hsv_mixed))

def shift_all_hsv(h = 0, s = 0, v = 0):
    for i in range(40):
        hue, sat, val = colours.rgb_to_hsv(*leds.get_rgb(i))
        hue += h
        sat = _clip(sat + s)
        val = _clip(val + v)
        leds.set_rgb(i, *colours.hsv_to_rgb(hue, sat, val))

def highlight_petal_rgb(num, r, g, b, num_leds=5):
    """
    Sets the LED closest to the petal and num_leds-1 around it to
    the color. If num_leds is uneven the appearance will be symmetric.
    leds.update() must be called externally.
    """
    num = num % 10
    if num_leds < 0:
        num_leds = 0
    for i in range(num_leds):
        leds.set_rgb((num * 4 + i - num_leds // 2) % 40, r, g, b)
