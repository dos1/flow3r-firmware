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
    full.fill((0, 0, 0, 255))
    full.blit(background, (0, 0))

    leds = pygame.Surface((screen_w, screen_h), flags=pygame.SRCALPHA)
    for pos, state in zip(leds_positions, leds_state):
        x = pos[0] + 3.0
        y = pos[1] + 3.0
        r, g, b = state
        for i in range(20):
            radius = 26 - i
            r2 = r / (20 - i)
            g2 = g / (20 - i)
            b2 = b / (20 - i)
            pygame.draw.circle(leds, (r2, g2, b2), (x, y), radius)
    full.blit(leds, (0, 0), special_flags=pygame.BLEND_ADD)

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

    screen.fill((0, 0, 0, 255))
    screen.blit(full, (0,0))
    pygame.display.flip()

leds_positions = [
    (660, 455), (608, 490), (631, 554), (648, 618), (659, 690), (655, 770), (571, 746), (502, 711),
    (452, 677), (401, 639), (352, 680), (299, 713), (241, 745), (151, 771), (147, 682), (160, 607),
    (176, 549), (197, 491), (147, 453), ( 98, 416), ( 43, 360), (  0, 292), ( 64, 267), (144, 252),
    (210, 248), (276, 249), (295, 190), (318, 129), (351,  65), (404,   0), (456,  64), (490, 131),
    (511, 186), (529, 250), (595, 247), (663, 250), (738, 264), (810, 292), (755, 371), (705, 419),
]
leds_state_buf = [(0, 0, 0) for _ in leds_positions]
leds_state = [(0, 0, 0) for _ in leds_positions]

def set_led_rgb(ix, r, g, b):
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
    leds_state_buf[ix] = (r, g, b)

def update_leds():
    for i, s in enumerate(leds_state_buf):
        leds_state[i] = s

def set_global_volume_dB(a):
    pass

def get_button(a):
    return 0

def get_captouch(a):
    return 0
