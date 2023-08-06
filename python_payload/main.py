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
from apps.scroll_demo import app as scroll_demo

# Build menu structure

menu_settings = settings.build_menu()

menu_music = SimpleMenu(
    [
        MenuItemBack(),
        MenuItemForeground("Harmonic", harmonic),
        MenuItemForeground("Melodic", melodic),
        # MenuItemNoop("TinySynth"),
        # MenuItemNoop("CrazySynth"),
        MenuItemNoop("NOOP Sequencer"),
    ],
)

menu_apps = SimpleMenu(
    [
        MenuItemBack(),
        MenuItemForeground("captouch", captouch_demo),
        MenuItemForeground("worms", worms),
        MenuItemForeground("cap scroll", scroll_demo),
    ],
)


menu_badge = SimpleMenu(
    [
        MenuItemBack(),
        MenuItemForeground("nick", nick),
    ],
)

menu_system = SimpleMenu(
    [
        MenuItemBack(),
        MenuItemForeground("Settings", menu_settings),
        MenuItemNoop("Disk Mode"),
        MenuItemNoop("About"),
        MenuItemNoop("Reboot"),
    ],
)

menu_main = SunMenu(
    [
        MenuItemForeground("Badge", menu_badge),
        MenuItemForeground("Music", menu_music),
        MenuItemForeground("Apps", menu_apps),
        MenuItemForeground("System", menu_system),
    ],
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

debug_touch = overlays.OverlayCaptouch()


# Tie compositor's debug touch overlay to setting.
def _onoff_debug_touch_update() -> None:
    compositor.enabled[overlays.OverlayKind.Touch] = settings.onoff_debug_touch.value


_onoff_debug_touch_update()
settings.onoff_debug_touch.subscribe(_onoff_debug_touch_update)
compositor.add_overlay(debug_touch)

reactor.set_top(compositor)
reactor.run()
