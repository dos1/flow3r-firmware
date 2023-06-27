"""
Experimental/Research UI/UX framework (st4m).

To run, rename this file to main.py.

See st4m/README.md for more information.
"""


import st4m

from st4m.goose import Optional, List, ABCBase, abstractmethod
from st4m.ui.view import View, ViewManager, ViewTransitionBlend
from st4m.ui.menu import (
    MenuItemBack,
    MenuItemForeground,
    MenuItemNoop,
)

from st4m.ui.elements.menus import SunMenu, SimpleMenu

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

menu_main = SunMenu(
    [
        MenuItemForeground("Music", menu_music),
        MenuItemForeground("Apps", menu_apps),
        MenuItemNoop("Settings"),
    ],
    vm,
)

vm.push(menu_main)

reactor = st4m.Reactor()
reactor.set_top(vm)
reactor.run()
