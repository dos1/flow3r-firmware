import math
import os
import time

import pygame


pygame.init()
screen_w = 814
screen_h = 854
screen = pygame.display.set_mode(size=(screen_w, screen_h))
simpath = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
bgpath = os.path.join(simpath, 'background.png')
background = pygame.image.load(bgpath)


class Input:
    """
    Input implements an input overlay (for petals or buttons) that can be
    mouse-picked by the user, and in the future also keyboard-controlled.
    """
    # Pixels positions of each marker.
    POSITIONS = []
    # Pixel size (diameter) of each marker.
    MARKER_SIZE = 100

    # Colors for various states (RGBA).
    COLOR_HELD = (0x5b, 0x5b, 0x5b, 0xa0)
    COLOR_HOVER = (0x6b, 0x6b, 0x6b, 0xa0)
    COLOR_IDLE = (0x8b, 0x8b, 0x8b, 0x80)

    def __init__(self):
        self._state = [False for _ in self.POSITIONS]
        self._mouse_hover = None
        self._mouse_held = None

    def state(self):
        s = [ss for ss in self._state]
        if self._mouse_held is not None:
            s[self._mouse_held] = True
        return s

    def _mouse_coords_to_id(self, mouse_x, mouse_y):
        for i, (x, y) in enumerate(self.POSITIONS):
            x += self.MARKER_SIZE // 2
            y += self.MARKER_SIZE // 2
            dx = mouse_x - x
            dy = mouse_y - y
            if math.sqrt(dx**2 + dy**2) < self.MARKER_SIZE // 2:
                return i
        return None

    def process_event(self, ev):
        prev_hover = self._mouse_hover
        prev_state = self.state()

        if ev.type == pygame.MOUSEMOTION:
            x, y = ev.pos
            self._mouse_hover = self._mouse_coords_to_id(x, y)
        if ev.type == pygame.MOUSEBUTTONDOWN:
            self._mouse_held = self._mouse_hover
        if ev.type == pygame.MOUSEBUTTONUP:
            self._mouse_held = None

        if prev_hover != self._mouse_hover:
            return True
        if prev_state != self.state():
            return True
        return False

    def render(self, surface):
        s = self.state()
        for i, (x, y) in enumerate(self.POSITIONS):
            x += self.MARKER_SIZE // 2
            y += self.MARKER_SIZE // 2
            if s[i]:
                pygame.draw.circle(surface, self.COLOR_HELD, (x, y), self.MARKER_SIZE//2)
            elif i == self._mouse_hover:
                pygame.draw.circle(surface, self.COLOR_HOVER, (x, y), self.MARKER_SIZE//2)
            else:
                pygame.draw.circle(surface, self.COLOR_IDLE, (x, y), self.MARKER_SIZE//2)


class PetalsInput(Input):
    # TODO(q3k): document order
    POSITIONS = [
        (114, 302), (163, 112), (204, 587), ( 49, 477), (504, 587), (352, 696), (602, 298), (660, 477), (356, 122), (547, 117),
    ]


class ButtonsInput(Input):
    POSITIONS = [
        ( 14, 230), ( 46, 230), ( 78, 230),
        (714, 230), (746, 230), (778, 230),
    ]
    MARKER_SIZE = 20
    COLOR_HELD = (0x80, 0x80, 0x80, 0xff)
    COLOR_HOVER = (0x40, 0x40, 0x40, 0xff)
    COLOR_IDLE = (0x20, 0x20, 0x20, 0xff)


class Simulation:
    """
    Simulation implements the state and logic of the on-host pygame-based badge
    simulator.
    """

    # Pixel coordinates of each LED. The order is the same as the hardware
    # WS2812 chain, not the order as expected by the micropython API!
    LED_POSITIONS = [
        (660, 455), (608, 490), (631, 554), (648, 618), (659, 690), (655, 770), (571, 746), (502, 711),
        (452, 677), (401, 639), (352, 680), (299, 713), (241, 745), (151, 771), (147, 682), (160, 607),
        (176, 549), (197, 491), (147, 453), ( 98, 416), ( 43, 360), (  0, 292), ( 64, 267), (144, 252),
        (210, 248), (276, 249), (295, 190), (318, 129), (351,  65), (404,   0), (456,  64), (490, 131),
        (511, 186), (529, 250), (595, 247), (663, 250), (738, 264), (810, 292), (755, 371), (705, 419),
    ]

    def __init__(self):
        # Buffered LED state. Will be propagated to led_state when the
        # simulated update_leds function gets called.
        self.led_state_buf = [(0, 0, 0) for _ in self.LED_POSITIONS]
        # Actual LED state as rendered.
        self.led_state = [(0, 0, 0) for _ in self.LED_POSITIONS]
        self.petals = PetalsInput()
        self.buttons = ButtonsInput()
        # Timestamp of last GUI render. Used by the lazy render GUI
        # functionality.
        self.last_gui_render = None

        # Surfaces for different parts of the simulator render. Some of them
        # have a dirty bit which is an optimization to skip rendering the
        # corresponding surface when there was no change to its render data.
        self._led_surface = pygame.Surface((screen_w, screen_h), flags=pygame.SRCALPHA)
        self._led_surface_dirty = True
        self._petal_surface = pygame.Surface((screen_w, screen_h), flags=pygame.SRCALPHA)
        self._petal_surface_dirty = True
        self._full_surface = pygame.Surface((screen_w, screen_h), flags=pygame.SRCALPHA)
        self._oled_surface = pygame.Surface((240, 240), flags=pygame.SRCALPHA)

        # Calculate OLED per-row offset.
        #
        # The OLED disc (240px diameter) will be written into a 240px x 240px
        # axis-aligned bounding box. The rendering routine iterates over the
        # bounding box row-per-row, and we only want to write that row's disc
        # fragment for each row into the square bounding box. This fragment
        # will be offset by some pixels from the left edge, and will be also
        # shortened by the same count of pixels from the right edge.
        #
        # The way we calculate these offsets is quite naïve, but it's easy to
        # reason about. First, we start off by calculating a 240x240px bitmask
        # that is True if the pixel corresponding to this mask's bit is part of
        # the OLED disc, and false otherwise.
        mask = [
            [
                math.sqrt((x - 120)**2 + (y - 120)**2) <= 120
                for x in range(240)
            ]
            for y in range(240)
        ]
        # Now, we iterate the mask row-by-row and find the first True bit in
        # it. The offset within that row is our per-row offset for the
        # rendering routine.
        self._oled_offset = [
            m.index(True)
            for m in mask
        ]

    def process_events(self):
        """
        Process pygame events and update mouse_{x,y}, {petal,button}_held and
        {petal,button}_hover.
        """
        evs = pygame.event.get()
        for ev in evs:
            if self.petals.process_event(ev):
                self._petal_surface_dirty = True
            if self.buttons.process_event(ev):
                self._petal_surface_dirty = True

    def _render_petal_markers(self, surface):
        self.petals.render(surface)
        self.buttons.render(surface)

    def _render_leds(self, surface):
        for pos, state in zip(self.LED_POSITIONS, self.led_state):
            # TODO(q3k): pre-apply to LED_POSITIONS
            x = pos[0] + 3.0
            y = pos[1] + 3.0
            r, g, b = state
            for i in range(20):
                radius = 26 - i
                r2 = r / (20 - i)
                g2 = g / (20 - i)
                b2 = b / (20 - i)
                pygame.draw.circle(surface, (r2, g2, b2), (x, y), radius)

    def _render_oled(self, surface):
        surface.fill((0, 0, 0, 0))
        buf = surface.get_buffer()

        fb = get_ctx()._get_fb()
        fb = fb[:240*240*4]
        for y in range(240):
            # Use precalculated row offset to turn OLED disc into square
            # bounded plane.
            offset = self._oled_offset[y]
            start_offs_bytes = y * 240 * 4
            start_offs_bytes += offset * 4
            end_offs_bytes = (y+1) * 240 * 4
            end_offs_bytes -= offset * 4
            buf.write(bytes(fb[start_offs_bytes:end_offs_bytes]), start_offs_bytes)

    def render_gui_now(self):
        """
        Render the GUI elements, skipping overlay elements that aren't dirty.

        This does _not_ render the Ctx state into the OLED surface. For that,
        call render_display.
        """
        self.last_gui_render = time.time()

        full = self._full_surface
        need_overlays = False
        if self._led_surface_dirty or self._petal_surface_dirty:
            full.fill((0, 0, 0, 255))
            full.blit(background, (0, 0))
            need_overlays = True

        if self._led_surface_dirty:
            self._render_leds(self._led_surface)
            self._led_surface_dirty = False
        if need_overlays:
            full.blit(self._led_surface, (0, 0), special_flags=pygame.BLEND_ADD)

        if self._petal_surface_dirty:
            self._render_petal_markers(self._petal_surface)
            self._petal_surface_dirty = False
        if need_overlays:
            full.blit(self._petal_surface, (0, 0))

        # Always blit oled. Its' alpha blending is designed in a way that it
        # can be repeatedly applied to a dirty _full_surface without artifacts.
        center_x = 408
        center_y = 426
        off_x = center_x - (240 // 2)
        off_y = center_y - (240 // 2)
        full.blit(self._oled_surface, (off_x, off_y))

        screen.blit(full, (0,0))
        pygame.display.flip()

    def render_gui_lazy(self):
        """
        Render the GUI elements if needed to maintain a responsive 60fps of the
        GUI itself. As with render_gui_now, the OLED surface is not rendered by
        this call.
        """
        target_fps = 60.0
        d = 1/target_fps

        if self.last_gui_render is None:
            self.render_gui_now()
        elif time.time() - self.last_gui_render > d:
            self.render_gui_now()

    def render_display(self):
        """
        Render the OLED surface from Ctx state.

        Afterwards, render_gui_{lazy,now} should still be called to actually
        present the new OLED surface state to the user.
        """
        self._render_oled(self._oled_surface)

    def set_led_rgb(self, ix, r, g, b):
        self.led_state_buf[ix] = (r, g, b)

    def leds_update(self):
        for i, s in enumerate(_sim.led_state_buf):
            if _sim.led_state[i] != s:
                _sim.led_state[i] = s
                self._led_surface_dirty = True


_sim = Simulation()


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
    _sim.process_events()
    _sim.render_display()
    _sim.render_gui_now()


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
    _sim.set_led_rgb(ix, r, g, b)


def set_led_hsv(ix, h, s, v):
    color = pygame.Color(0)
    h /= 255.0
    color.hsva = (h, s, v, 1.0)
    r, g, b = color.r, color.g, color.b
    r *= 255
    g *= 255
    b *= 255
    _sim.set_led_rgb(ix, r, g, b)


def update_leds():
    _sim.leds_update()
    _sim.render_gui_lazy()


def set_global_volume_dB(a):
    pass


def get_button(a):
    _sim.process_events()
    _sim.render_gui_lazy()

    state = _sim.buttons.state()
    if a == 1:
        sub = state[:3]
    elif a == 0:
        sub = state[3:6]
    else:
        return 0

    if sub[0]:
        return -1
    elif sub[1]:
        return 2
    elif sub[2]:
        return +1
    return 0


def get_captouch(a):
    _sim.process_events()
    _sim.render_gui_lazy()
    return _sim.petals.state()[a]
