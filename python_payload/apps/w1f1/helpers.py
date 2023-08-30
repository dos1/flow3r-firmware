import os
import leds


def sd_card_plugged() -> bool:
    try:
        os.listdir("/sd")
        return True
    except OSError:
        # OSError: [Errno 19] ENODEV
        return False


def copy_across_devices(src: str, dst: str):
    """
    copy a file across devices (flash->SD etc).
    does the whole file at once, should only be used on small files.
    """
    with open(src, "rb") as srcf:
        with open(dst, "wb") as dstf:
            dstf.write(srcf.read())


def set_direction_leds(direction, r, g, b):
    if direction == 0:
        leds.set_rgb(39, r, g, b)
    else:
        leds.set_rgb((direction * 4) - 1, r, g, b)
    leds.set_rgb(direction * 4, r, g, b)
    leds.set_rgb((direction * 4) + 1, r, g, b)
