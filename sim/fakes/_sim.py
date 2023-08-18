import math
import os
import time
import itertools
import sys

import pygame


SCREENSHOT = False
SCREENSHOT_DELAY = 5
FULL_SCREEN = os.environ.get("SIM_FULL_SCREEN", "0") == "1"


pygame.init()
if FULL_SCREEN:
    screen = pygame.display.set_mode((0, 0), pygame.FULLSCREEN)
else:
    screen = pygame.display.set_mode(size=(814, 854))
screen_w = screen.get_width()
screen_h = screen.get_height()
simpath = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))
bgpath = os.path.join(simpath, "background.png")
background = pygame.image.load(bgpath)

OLED_SIZE = int(os.environ["SIM_OLED_SIZE"])
OLED_ASPECT = float(os.environ["SIM_OLED_ASPECT"])


class Input:
    """
    Input implements an input overlay (for petals or buttons) that can be
    mouse-picked by the user, and in the future also keyboard-controlled.
    """

    # Pixels positions of each marker.
    POSITIONS = []
    # Keyboard mapping
    KEYS = []
    # Pixel size (diameter) of each marker.
    MARKER_SIZE = 100

    # Colors for various states (RGBA).
    COLOR_HELD = (0x5B, 0x5B, 0x5B, 0xA0)
    COLOR_HOVER = (0x6B, 0x6B, 0x6B, 0xA0)
    COLOR_IDLE = (0x8B, 0x8B, 0x8B, 0x80)

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
        if ev.type in [pygame.KEYDOWN, pygame.KEYUP]:
            if ev.key in self.KEYS:
                self._mouse_hover = self.KEYS.index(ev.key)
            if ev.type == pygame.KEYDOWN:
                self._mouse_held = self._mouse_hover
            if ev.type == pygame.KEYUP:
                self._mouse_held = None
        if ev.type == pygame.QUIT:
            pygame.quit()
            sys.exit()

        if prev_hover != self._mouse_hover:
            return True
        if prev_state != self.state():
            return True
        return False

    def render(self, surface):
        s = self.state()
        for i, (x, y) in enumerate(self.POSITIONS):
            if s[i]:
                pygame.draw.circle(
                    surface, self.COLOR_HELD, (x, y), self.MARKER_SIZE // 2
                )
            elif i == self._mouse_hover:
                pygame.draw.circle(
                    surface, self.COLOR_HOVER, (x, y), self.MARKER_SIZE // 2
                )
            else:
                pygame.draw.circle(
                    surface, self.COLOR_IDLE, (x, y), self.MARKER_SIZE // 2
                )


class PetalsInput(Input):
    _petal_positions_top = [
        (406, 172),
        (164, 352),
        (254, 637),
        (554, 637),
        (652, 348),
    ]
    _petal_positions_bottom = [
        (213, 162),
        (99, 527),
        (402, 746),
        (710, 527),
        (597, 167),
    ]
    POSITIONS = list(
        itertools.chain(
            *[
                [
                    (
                        x + math.cos(i * -1.256 + 1.57) * 40,
                        y + math.sin(i * -1.256 + 1.57) * 40,
                    ),  # base
                    (
                        x + math.cos(i * -1.256 + 5.75) * 40,
                        y + math.sin(i * -1.256 + 5.75) * 40,
                    ),  # cw
                    (
                        x + math.cos(i * -1.256 + 3.66) * 40,
                        y + math.sin(i * -1.256 + 3.66) * 40,
                    ),  # ccw
                ]
                for i, (x, y) in enumerate(_petal_positions_top)
            ]
            + [
                [
                    (
                        x + math.cos(i * -1.256 - 2.20) * 40,
                        y + math.sin(i * -1.256 - 2.20) * 40,
                    ),  # tip
                    (
                        x + math.cos(i * -1.256 - 5.34) * 40,
                        y + math.sin(i * -1.256 - 5.34) * 40,
                    ),  # base
                ]
                for i, (x, y) in enumerate(_petal_positions_bottom)
            ]
        )
    )
    PETAL_MAP = {
        (0, 1): 2,
        (0, 2): 1,
        (0, 3): 0,
        (9, 0): 15,
        (9, 3): 16,
        (8, 1): 5,
        (8, 2): 4,
        (8, 3): 3,
        (7, 0): 17,
        (7, 3): 18,
        (6, 1): 8,
        (6, 2): 7,
        (6, 3): 6,
        (5, 0): 19,
        (5, 3): 20,
        (4, 1): 11,
        (4, 2): 10,
        (4, 3): 9,
        (3, 0): 21,
        (3, 3): 22,
        (2, 1): 14,
        (2, 2): 13,
        (2, 3): 12,
        (1, 0): 23,
        (1, 3): 24,
    }
    MARKER_SIZE = 40

    def _index_for_petal_pad(self, petal, pad):
        if petal >= 10:
            raise ValueError("petal cannot be > 10")

        return self.PETAL_MAP[(petal, pad)]

    def state_for_petal_pad(self, petal, pad):
        ix = self._index_for_petal_pad(petal, pad)
        return self.state()[ix]

    def state_for_petal(self, petal):
        res = False
        if petal % 2 == 0:
            # top
            res = res or self.state_for_petal_pad(petal, 1)
            res = res or self.state_for_petal_pad(petal, 2)
            res = res or self.state_for_petal_pad(petal, 3)
        else:
            # bottom
            res = res or self.state_for_petal_pad(petal, 0)
            res = res or self.state_for_petal_pad(petal, 3)
        return res


class ButtonsInput(Input):
    POSITIONS = [
        (24, 240),
        (56, 240),
        (88, 240),
        (724, 240),
        (756, 240),
        (788, 240),
    ]
    KEYS = [
        pygame.K_LEFT,
        pygame.K_SPACE,
        pygame.K_RIGHT,
        pygame.K_1,
        pygame.K_2,
        pygame.K_3,
    ]
    MARKER_SIZE = 20
    COLOR_HELD = (0x80, 0x80, 0x80, 0xFF)
    COLOR_HOVER = (0x40, 0x40, 0x40, 0xFF)
    COLOR_IDLE = (0x20, 0x20, 0x20, 0xFF)


class Simulation:
    """
    Simulation implements the state and logic of the on-host pygame-based badge
    simulator.
    """

    # Pixel coordinates of each LED. The order is the same as the hardware
    # WS2812 chain, not the order as expected by the micropython API!
    LED_POSITIONS = [
        (404, 0),
        (456, 64),
        (490, 131),
        (511, 186),
        (529, 250),
        (595, 247),
        (663, 250),
        (738, 264),
        (810, 292),
        (755, 371),
        (705, 419),
        (660, 455),
        (608, 490),
        (631, 554),
        (648, 618),
        (659, 690),
        (655, 770),
        (571, 746),
        (502, 711),
        (452, 677),
        (401, 639),
        (352, 680),
        (299, 713),
        (241, 745),
        (151, 771),
        (147, 682),
        (160, 607),
        (176, 549),
        (197, 491),
        (147, 453),
        (98, 416),
        (43, 360),
        (0, 292),
        (64, 267),
        (144, 252),
        (210, 248),
        (276, 249),
        (295, 190),
        (318, 129),
        (351, 65),
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
        self._petal_surface = pygame.Surface(
            (screen_w, screen_h), flags=pygame.SRCALPHA
        )
        self._petal_surface_dirty = True
        self._full_surface = pygame.Surface((screen_w, screen_h), flags=pygame.SRCALPHA)
        self._oled_surface = pygame.Surface(
            (OLED_SIZE, OLED_SIZE), flags=pygame.SRCALPHA
        )

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
                math.sqrt((x - OLED_SIZE // 2) ** 2 + (y - OLED_SIZE // 2) ** 2)
                <= OLED_SIZE // 2
                for x in range(OLED_SIZE)
            ]
            for y in range(OLED_SIZE)
        ]
        # Now, we iterate the mask row-by-row and find the first True bit in
        # it. The offset within that row is our per-row offset for the
        # rendering routine.
        self._oled_offset = [m.index(True) for m in mask]

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

    def _render_oled(self, surface, fb):
        surface.fill((0, 0, 0, 0))
        buf = surface.get_buffer()

        fb = fb[: OLED_SIZE * OLED_SIZE * 4]
        for y in range(OLED_SIZE):
            # Use precalculated row offset to turn OLED disc into square
            # bounded plane.
            offset = self._oled_offset[y]
            start_offs_bytes = y * OLED_SIZE * 4
            start_offs_bytes += offset * 4
            end_offs_bytes = (y + 1) * OLED_SIZE * 4
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
            if not FULL_SCREEN:
                full.blit(background, (0, 0))
            need_overlays = True

        if self._led_surface_dirty:
            if not FULL_SCREEN:
                self._render_leds(self._led_surface)
            self._led_surface_dirty = False
        if need_overlays:
            full.blit(self._led_surface, (0, 0), special_flags=pygame.BLEND_ADD)

        if self._petal_surface_dirty:
            if not FULL_SCREEN:
                self._render_petal_markers(self._petal_surface)
            self._petal_surface_dirty = False
        if need_overlays:
            if not FULL_SCREEN:
                full.blit(self._petal_surface, (0, 0))

        # Always blit oled. Its' alpha blending is designed in a way that it
        # can be repeatedly applied to a dirty _full_surface without artifacts.
        center_x = screen_w // 2
        center_y = screen_h // 2
        if FULL_SCREEN:
            off_x = center_x - (screen_h * OLED_ASPECT // 2)
            off_y = center_y - (screen_h // 2)
            new = pygame.transform.scale(
                self._oled_surface, (screen_h * OLED_ASPECT, screen_h)
            )
            full.blit(new, (off_x, off_y))
        else:
            off_x = center_x - (OLED_SIZE // 2)
            off_y = center_y - (OLED_SIZE // 2)
            full.blit(self._oled_surface, (off_x, off_y))

        screen.blit(full, (0, 0))
        pygame.display.flip()

    def render_gui_lazy(self):
        """
        Render the GUI elements if needed to maintain a responsive 60fps of the
        GUI itself. As with render_gui_now, the OLED surface is not rendered by
        this call.
        """
        target_fps = 60.0
        d = 1 / target_fps

        if self.last_gui_render is None:
            self.render_gui_now()
        elif time.time() - self.last_gui_render > d:
            self.render_gui_now()

    def render_display(self, fb):
        """
        Render the OLED surface from Ctx state.

        Afterwards, render_gui_{lazy,now} should still be called to actually
        present the new OLED surface state to the user.
        """
        self._render_oled(self._oled_surface, fb)

    def set_led_rgb(self, ix, r, g, b):
        self.led_state_buf[ix] = (r, g, b)

    def leds_update(self):
        for i, s in enumerate(_sim.led_state_buf):
            if _sim.led_state[i] != s:
                _sim.led_state[i] = s
                self._led_surface_dirty = True


_sim = Simulation()


import ctx


class FramebufferManager:
    def __init__(self):
        self._free = []
        for _ in range(2):
            fb, c = ctx._wasm.ctx_new_for_framebuffer(OLED_SIZE, OLED_SIZE)
            ctx._wasm.ctx_apply_transform(
                c,
                OLED_SIZE / 240,
                0,
                OLED_SIZE / 2,
                0,
                OLED_SIZE / 240,
                OLED_SIZE / 2,
                0,
                0,
                1,
            )
            self._free.append((fb, c))

    def get(self):
        if len(self._free) == 0:
            return None, None
        fb, ctx = self._free[0]
        self._free = self._free[1:]

        return fb, ctx

    def put(self, fb, ctx):
        self._free.append((fb, ctx))


fbm = FramebufferManager()


def get_ctx():
    dctx = ctx._wasm.ctx_new_drawlist(240, 240)
    return ctx.Context(dctx)


def display_update(subctx):
    _sim.process_events()
    fbp, c = fbm.get()
    if fbp is None:
        return

    ctx._wasm.ctx_render_ctx(subctx._ctx, c)
    ctx._wasm.ctx_destroy(subctx._ctx)

    fb = ctx._wasm._i.exports.memory.uint8_view(fbp)
    _sim.render_display(fb)
    _sim.render_gui_now()

    global SCREENSHOT
    global SCREENSHOT_DELAY
    if SCREENSHOT:
        SCREENSHOT_DELAY -= 1
        if SCREENSHOT_DELAY <= 0:
            path = os.curdir + "/flow3r.png"
            pygame.image.save(screen, path)
            print("Saved screenshot to ", path)
            sys.exit(0)

    fbm.put(fbp, c)


def get_button_state(left):
    _sim.process_events()
    _sim.render_gui_lazy()

    state = _sim.buttons.state()
    if left == 1:
        sub = state[:3]
    elif left == 0:
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
