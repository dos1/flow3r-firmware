import time, gc

ts_start = time.time()

from st3m import logging

log = logging.Log(__name__, level=logging.INFO)
log.info(f"starting main")
log.info(f"free memory: {gc.mem_free()}")

import st3m

from st3m.goose import Optional, List, ABCBase, abstractmethod

from st3m import settings

settings.load_all()

from st3m.ui.view import View, ViewManager, ViewTransitionBlend
from st3m.ui.menu import (
    MenuItemBack,
    MenuItemForeground,
    MenuItemNoop,
)

from st3m.ui.elements.menus import FlowerMenu, SimpleMenu, SunMenu
from st3m.ui.elements import overlays

log.info("import apps done")
log.info(f"free memory: {gc.mem_free()}")
ts_end = time.time()
log.info(f"boot took {ts_end-ts_start} seconds")

# TODO persistent settings
from st3m.system import audio, captouch

log.info("calibrating captouch, reset volume")
captouch.calibration_request()
audio.set_volume_dB(0)

import leds

leds.set_rgb(0, 255, 0, 0)

vm = ViewManager(ViewTransitionBlend())


# Preload all applications.
# TODO(q3k): only load these on demand.
from apps.demo_worms4 import app as worms
from apps.harmonic_demo import app as harmonic
from apps.melodic_demo import app as melodic
from apps.nick import app as nick
from apps.cap_touch_demo import app as captouch_demo


# Set the view_managers for the apps, otherwise leaving the app (back) will not work
worms._view_manager = vm
harmonic._view_manager = vm
melodic._view_manager = vm
nick._view_manager = vm
captouch_demo._view_manager = vm

# Build menu structure

menu_settings = settings.build_menu(vm)

menu_music = SimpleMenu(
    [
        MenuItemBack(),
        MenuItemForeground("Harmonic", harmonic),
        MenuItemForeground("Melodic", melodic),
        # MenuItemNoop("TinySynth"),
        # MenuItemNoop("CrazySynth"),
        MenuItemNoop("NOOP Sequencer"),
    ],
    vm,
)

menu_apps = SimpleMenu(
    [
        MenuItemBack(),
        MenuItemForeground("captouch", captouch_demo),
        MenuItemForeground("worms", worms),
    ],
    vm,
)


menu_badge = SimpleMenu(
    [
        MenuItemBack(),
        MenuItemForeground("nick", nick),
    ],
    vm,
)

from st3m import InputState, Ctx
from st3m.ui.view import ViewWithInputState
from st3m.utils import tau
import math


class About(ViewWithInputState):
    def __init__(self):
        self.ts = 0.0
        super().__init__()

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)
        self.ts += delta_ms / 1000

        if self.input.left_shoulder.middle.pressed:
            print("exiting")

    def draw(self, ctx: Ctx) -> None:
        ctx.rectangle(-120, -120, 240, 240)
        ctx.rgb(117 / 255, 255 / 255, 226 / 255)
        ctx.fill()

        ctx.arc(0, 350, 300, 0, tau, 0)
        ctx.rgb(61 / 255, 165 / 255, 30 / 255)
        ctx.fill()

        ctx.save()
        y = -40 + math.cos(self.ts * 2 + 10) * 10
        ctx.translate(0, y)
        for i in range(5):
            a = i * tau / 5 + self.ts
            size = 30 + math.cos(self.ts) * 5
            ctx.save()
            ctx.rotate(a)
            ctx.translate(30, 0)
            ctx.arc(0, 0, size, 0, tau, 0)
            ctx.gray(1)
            ctx.fill()
            ctx.restore()

        ctx.arc(0, 0, 20, 0, tau, 0)
        ctx.rgb(255 / 255, 247 / 255, 46 / 255)
        ctx.fill()
        ctx.restore()

        ctx.start_group()
        ctx.global_alpha = 0.5
        ctx.rgb(0, 0, 0)
        ctx.arc(0, 0, 80, 0, tau, 0)
        ctx.fill()
        ctx.end_group()

        ctx.start_group()
        ctx.global_alpha = 1.0
        ctx.gray(1)

        ctx.text_align = ctx.MIDDLE

        ctx.font = "Camp Font 2"
        ctx.font_size = 50

        ctx.move_to(0, -33)
        ctx.text("flow3r")
        ctx.move_to(0, -3)
        ctx.text("badge")

        ctx.font = "Camp Font 3"
        ctx.font_size = 15

        ctx.move_to(0, 25)
        ctx.text("Chaos")
        ctx.move_to(0, 40)
        ctx.text("Communication")
        ctx.move_to(0, 55)
        ctx.text("Camp 2023")

        ctx.end_group()


menu_system = SimpleMenu(
    [
        MenuItemBack(),
        MenuItemForeground("Settings", menu_settings),
        MenuItemNoop("Disk Mode"),
        MenuItemForeground("About", About()),
        MenuItemNoop("Reboot"),
    ],
    vm,
)

menu_main = SunMenu(
    [
        MenuItemForeground("Badge", menu_badge),
        MenuItemForeground("Music", menu_music),
        MenuItemForeground("Apps", menu_apps),
        MenuItemForeground("System", menu_system),
    ],
    vm,
)


vm.push(menu_main)

reactor = st3m.Reactor()

# Set up top-level compositor (for combining viewmanager with overlays).
compositor = overlays.Compositor(vm)


# Tie compositor's debug overlay to setting.
def _onoff_debug_update() -> None:
    compositor.enabled[overlays.OverlayKind.Debug] = settings.onoff_debug.value


_onoff_debug_update()
settings.onoff_debug.subscribe(_onoff_debug_update)

# Configure debug overlay fragments.
debug = overlays.OverlayDebug()
debug.add_fragment(overlays.DebugReactorStats(reactor))
compositor.add_overlay(debug)

reactor.set_top(compositor)
reactor.run()
