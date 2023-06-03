import pygame
import math
import os

pygame.init()
screen_w = 814
screen_h = 854
screen = pygame.display.set_mode(size=(screen_w, screen_h))
simpath = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
bgpath = os.path.join(simpath, 'background.png')
background = pygame.image.load(bgpath)

_deprecated_notified = set()
def _deprecated(f):
    def wrapper(*args, **kwargs):
        if f not in _deprecated_notified:
            print(f'{f.__name__} is deprecated!')
            _deprecated_notified.add(f)
        return f(*args, **kwargs)
    return wrapper

def init_done():
    return True

def captouch_autocalib():
    pass

def captouch_calibration_active():
    return False

_global_ctx = None
def get_ctx():
    global _global_ctx
    import ctx

    if _global_ctx is None:
        _global_ctx = ctx.Ctx()
    return _global_ctx

@_deprecated
def display_fill(color):
    """
    display_fill is deprecated as it doesn't work well with ctx's framebuffer
    ownership / state diffing. Instead, callers should use plain ctx functions
    to fill the screen.
    """
    r = (color >> 11) & 0b11111
    g = (color >> 5 ) & 0b111111
    b =  color        & 0b11111
    get_ctx().rgb(r << 2, g << 3, b <<2).rectangle(-120, -120, 240, 240).fill()


def display_update():
    fb = get_ctx()._get_fb()

    full = pygame.Surface((screen_w, screen_h), flags=pygame.SRCALPHA)
    full.blit(background, (0, 0))

    center_x = 408
    center_y = 426
    off_x = center_x - (240 // 2)
    off_y = center_y - (240 // 2)

    oled = pygame.Surface((240, 240), flags=pygame.SRCALPHA)
    oled_buf = oled.get_buffer()
    for y in range(240):
        rgba = bytearray()
        for x in range(240):
            dx = (x - 120)
            dy = (y - 120)
            dist = math.sqrt(dx**2 + dy**2)

            fbh = fb[y * 240 * 2 + x * 2]
            fbl = fb[y * 240 * 2 + x * 2 + 1]
            fbv = (fbh << 8) | fbl
            r = (fbv >> 11) & 0b11111
            g = (fbv >>  5) & 0b111111
            b = (fbv >>  0) & 0b11111
            if dist > 120:
                rgba += bytes([255, 255, 255, 0])
            else:
                rgba += bytes([b << 3, g << 2, r << 3, 0xff])
        oled_buf.write(bytes(rgba), y * 240 * 4)

    del oled_buf
    full.blit(oled, (off_x, off_y))

    screen.blit(full, (0,0))
    pygame.display.flip()

def set_led_rgb(a, b, c, d):
    pass

def update_leds():
    pass

def set_global_volume_dB(a):
    pass

def get_button(a):
    return 0

def get_captouch(a):
    return 0
