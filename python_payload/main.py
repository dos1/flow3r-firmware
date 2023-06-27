import time, gc

ts_start = time.time()

from st3m import logging

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

from st4m.ui.elements.menus import FlowerMenu, SimpleMenu

log.info("import apps done")
log.info(f"free memory: {gc.mem_free()}")
ts_end = time.time()
log.info(f"boot took {ts_end-ts_start} seconds")

# TODO persistent settings
from st3m.system import audio, captouch

log.info("calibrating captouch, reset volume")
captouch.calibration_request()
audio.set_volume_dB(0)

vm = ViewManager(ViewTransitionBlend())

menu_music = SimpleMenu(
    [
        MenuItemBack(),
        MenuItemNoop("Harmonic"),
        MenuItemNoop("Melodic"),
        MenuItemNoop("TinySynth"),
        MenuItemNoop("CrazySynth"),
        MenuItemNoop("Sequencer"),
    ],
    vm,
)

menu_apps = SimpleMenu(
    [
        MenuItemBack(),
        MenuItemNoop("captouch"),
        MenuItemNoop("worms"),
    ],
    vm,
)


menu_main = FlowerMenu(
    [
        MenuItemForeground("MUsic", menu_music),
        MenuItemForeground("Apps", menu_apps),
        MenuItemNoop("Settings"),
    ],
    vm,
)


vm.push(menu_main)

reactor = st4m.Reactor()
reactor.set_top(vm)
reactor.run()
