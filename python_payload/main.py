import time, gc

ts_start = time.time()

from st4m import logging

log = logging.Log(__name__, level=logging.INFO)
log.info(f"starting main")
log.info(f"free memory: {gc.mem_free()}")

import st4m

from st4m.goose import Optional, List, ABCBase, abstractmethod
from st4m.ui.view import View, ViewManager, ViewTransitionBlend
from st4m.ui.menu import (
    MenuItemBack,
    MenuItemForeground,
    MenuItemNoop,
)

from st4m.ui.elements.menus import FlowerMenu, SimpleMenu, SunMenu

log.info("import apps done")
log.info(f"free memory: {gc.mem_free()}")
ts_end = time.time()
log.info(f"boot took {ts_end-ts_start} seconds")

# TODO persistent settings
from st4m.system import audio, captouch

log.info("calibrating captouch, reset volume")
captouch.calibration_request()
audio.set_volume_dB(0)

import leds

leds.set_rgb(0, 255, 0, 0)

vm = ViewManager(ViewTransitionBlend())


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
menu_main = SunMenu(
    [
        MenuItemForeground("Badge", menu_badge),
        MenuItemForeground("Music", menu_music),
        MenuItemForeground("Apps", menu_apps),
        MenuItemNoop("Settings"),
    ],
    vm,
)

vm.push(menu_main)

reactor = st4m.Reactor()
reactor.set_top(vm)
# reactor.set_top(pr)
reactor.run()
