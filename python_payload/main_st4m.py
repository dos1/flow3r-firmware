"""
Experimental/Research UI/UX framework (st4m).

To run, rename this file to main.py.

See st4m/README.md for more information.
"""


import st4m

from st4m.goose import Optional, List, ABCBase, abstractmethod
from st4m.ui.view import View, ViewManager, ViewTransitionBlend
from st4m.ui.menu import (
    MenuItem,
    MenuController,
    MenuItemBack,
    MenuItemForeground,
    MenuItemNoop,
)
from st4m import Responder, InputState, Ctx

import math, hardware

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


class USBIcon(Responder):
    """
    Found in the bargain bin at an Aldi Süd.
    """

    def draw(self, ctx: Ctx) -> None:
        ctx.gray(1.0)
        ctx.arc(-90, 0, 20, 0, 6.28, 0).fill()
        ctx.line_width = 10.0
        ctx.move_to(-90, 0).line_to(90, 0).stroke()
        ctx.move_to(100, 0).line_to(70, 15).line_to(70, -15).line_to(100, 0).fill()
        ctx.move_to(-50, 0).line_to(-10, -40).line_to(20, -40).stroke()
        ctx.arc(20, -40, 15, 0, 6.28, 0).fill()
        ctx.move_to(-30, 0).line_to(10, 40).line_to(40, 40).stroke()
        ctx.rectangle(40 - 15, 40 - 15, 30, 30).fill()

    def think(self, ins: InputState, delta_ms: int) -> None:
        pass


class REPLIcon(Responder):
    def draw(self, ctx: Ctx) -> None:
        ctx.gray(1.0)
        ctx.line_width = 10.0
        for i in range(3):
            x = i * 40
            ctx.move_to(-60 + x, -60).line_to(0 + x, 0).line_to(-60 + x, 60).stroke()

    def think(self, ins: InputState, delta_ms: int) -> None:
        pass


class Sun(Responder):
    """
    A rotating sun widget.
    """

    def __init__(self) -> None:
        self.x = 0.0
        self.y = 0.0
        self.size = 50.0
        self.ts = 1.0

    def think(self, ins: InputState, delta_ms: int) -> None:
        self.ts += delta_ms
        pass

    def draw(self, ctx: Ctx) -> None:
        nrays = 10
        angle_per_ray = 6.28 / nrays
        for i in range(nrays):
            angle = i * angle_per_ray + self.ts / 4000
            angle %= 3.14159 * 2

            if angle > 2 and angle < 4:
                continue

            ctx.save()
            ctx.rgb(0.5, 0.5, 0)
            ctx.line_width = 30
            ctx.translate(-120, 0).rotate(angle)
            ctx.move_to(20, 0)
            ctx.line_to(260, 0)
            ctx.stroke()
            ctx.restore()

        ctx.save()
        ctx.rgb(0.92, 0.89, 0)
        ctx.translate(-120, 0)

        ctx.arc(self.x, self.y, self.size, 0, 6.29, 0)
        ctx.fill()
        ctx.restore()


class MainMenu(MenuController):
    """
    A circular menu with a rotating sun.
    """

    __slots__ = (
        "_ts",
        "_sun",
    )

    def __init__(self, items: List[MenuItem], vm: ViewManager) -> None:
        self._ts = 0
        self._sun = Sun()
        super().__init__(items, vm)

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)
        self._sun.think(ins, delta_ms)
        self._ts += delta_ms

    def _draw_text_angled(
        self, ctx: Ctx, text: str, angle: float, activity: float
    ) -> None:
        size = lerp(20, 40, activity)
        color = lerp(0, 1, activity)
        if color < 0.01:
            return

        ctx.save()
        ctx.translate(-120, 0).rotate(angle).translate(140, 0)
        ctx.font_size = size
        ctx.rgba(1.0, 1.0, 1.0, color).move_to(0, 0).text(text)
        ctx.restore()

    def draw(self, ctx: Ctx) -> None:
        ctx.gray(0)
        ctx.rectangle(-120, -120, 240, 240).fill()

        self._sun.draw(ctx)

        ctx.font_size = 40
        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE

        angle_per_item = 0.4

        current = self._scroll_controller.current_position()

        for ix, item in enumerate(self._items):
            rot = (ix - current) * angle_per_item
            self._draw_text_angled(ctx, item.label(), rot, 1 - abs(rot))


class SimpleMenu(MenuController):
    """
    A simple line-by-line menu.
    """

    def draw(self, ctx: Ctx) -> None:
        ctx.gray(0)
        ctx.rectangle(-120, -120, 240, 240).fill()

        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE

        current = self._scroll_controller.current_position()

        ctx.gray(1)
        for ix, item in enumerate(self._items):
            offs = (ix - current) * 30
            size = lerp(30, 20, abs(offs / 20))
            ctx.font_size = size
            ctx.move_to(0, offs).text(item.label())


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


class Viewport(Responder):
    def __init__(self, vm: ViewManager) -> None:
        self.vm = vm
        self.usb = USBIcon()
        self.repl = REPLIcon()
        self.visible: List[Responder] = []

    def think(self, ins: InputState, delta_ms: int) -> None:
        self.vm.think(ins, delta_ms)

        self.visible = []
        usb = hardware.usb_connected()
        console = hardware.usb_console_active()
        if not usb:
            console = False
        if usb:
            self.visible.append(self.usb)
        # if console:
        #    self.visible.append(self.repl)

        for v in self.visible:
            v.think(ins, delta_ms)

    def draw(self, ctx: Ctx) -> None:
        self.vm.draw(ctx)

        nicons = len(self.visible)
        dist = 20
        width = (nicons - 1) * dist
        x0 = width / -2
        for i, v in enumerate(self.visible):
            x = x0 + i * dist
            ctx.save()
            ctx.translate(x, -100)
            ctx.scale(0.1, 0.1)
            v.draw(ctx)
            ctx.restore()


reactor = st4m.Reactor()
reactor.set_top(Viewport(vm))
reactor.run()
