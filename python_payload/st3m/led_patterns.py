import leds
import json


def set_menu_colors():
    """
    set all LEDs to the configured menu colors if provided in
    /flash/menu_leds.json. leds.update() must be called externally.
    """
    path = "/flash/menu_leds.json"
    try:
        with open(path, "r") as f:
            settings = json.load(f)
    except OSError:
        leds.set_all_rgb(0, 0, 0)
        return
    for i in range(40):
        col = settings["leds"][i]
        leds.set_rgb(i, col[0], col[1], col[2])


def highlight_petal(num, r, g, b, num_leds=5):
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
