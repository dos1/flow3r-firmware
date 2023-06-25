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
    MenuItem,
    MenuController,
    MenuItemBack,
    MenuItemForeground,
    MenuItemNoop,
)

from st4m import Responder, InputState, Ctx

# from apps import flow3r

log.info("import apps done")
log.info(f"free memory: {gc.mem_free()}")
ts_end = time.time()
log.info(f"boot took {ts_end-ts_start} seconds")

# TODO persistent settings
from st3m.system import audio, captouch

log.info("calibrating captouch, reset volume")
captouch.calibration_request()
audio.set_volume_dB(0)

import math, random
from st3m.ui import xy_from_polar

WIDTH = 240
HEIGHT = 240

# Define a few RGB (0.0 to 1.0) colors
BLACK = (0, 0, 0)
RED = (1, 0, 0)
GREEN = (0, 1, 0)
BLUE = (0, 0, 1)
WHITE = (1, 1, 1)
GREY = (0.5, 0.5, 0.5)
GO_GREEN = (63 / 255, 255 / 255, 33 / 53)
PUSH_RED = (251 / 255, 72 / 255, 196 / 255)

tau = 2 * math.pi

vm = ViewManager(ViewTransitionBlend())


def lerp(a: float, b: float, v: float) -> float:
    if v <= 0:
        return a
    if v >= 1.0:
        return b
    return a + (b - a) * v


class GroupRing(Responder):
    def __init__(self, r=100, x=0, y=0):
        self.r = r
        self.x = x
        self.y = y
        self.items_ring = []
        self.item_center = None
        self.angle_offset = 0
        self.ts = 0.0

    def think(self, ins: InputState, delta_ms: int) -> None:
        self.ts += delta_ms
        self.item_center.think(ins, delta_ms)
        for item in self.items_ring:
            item.think(ins, delta_ms)

    def draw(self, ctx: Ctx) -> None:
        if self.item_center:
            self.item_center.has_highlight = False
            self.item_center.draw(ctx)

        for index, item in enumerate(self.items_ring):
            if item is None:
                continue
            angle = tau / len(self.items_ring) * index + self.angle_offset
            (x, y) = xy_from_polar(self.r, angle)
            ctx.save()
            ctx.translate(self.x + x, self.y + y)
            item.draw(ctx)
            ctx.restore()


class FlowerIcon(Responder):
    """
    A flower icon
    """

    def __init__(self, label="?") -> None:
        self.x = 0.0
        self.y = 0.0
        self.size = 50.0
        self.ts = 1.0
        self.bg = (random.random(), random.random(), random.random())
        self.label = label

        self.highlighted = False
        self.rotation_time = 0.0

        self.petal_count = random.randint(2, 3)
        self.petal_color = (random.random(), random.random(), random.random())
        self.phi_offset = random.random()
        self.size_offset = random.randint(0, 20)

    def think(self, ins: InputState, delta_ms: int) -> None:
        self.ts += delta_ms
        pass

    def draw(self, ctx: Ctx) -> None:
        x = self.x
        y = self.y
        petal_size = 0
        if self.petal_count:
            petal_size = 2.3 * self.size / self.petal_count + self.size_offset

        hs = 5
        # print(self.ts)
        ctx.save()
        ctx.move_to(x, y)
        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE
        ctx.font_size = self.size / 3
        ctx.line_width = 10
        if self.rotation_time:
            phi_rotate = tau * ((self.ts % self.rotation_time) / self.rotation_time)
        else:
            phi_rotate = 0
        for i in range(self.petal_count):
            ctx.save()

            phi = (tau / self.petal_count * i + self.phi_offset + phi_rotate) % tau
            r = self.size / 2
            (x_, y_) = xy_from_polar(r, phi)

            size_offset = abs(math.pi - (phi + math.pi) % tau) * 5
            ctx.move_to(x + x_ + petal_size / 2 + size_offset + 5, y + y_)
            if self.highlighted:
                # ctx.move_to(x + x_ - petal_size / 2 - size_offset - 5, y + y_)
                ctx.arc(x + x_, y + y_, petal_size / 2 + size_offset + 1, 0, tau, 0)
                ctx.rgb(*GO_GREEN).stroke()

            ctx.arc(x + x_, y + y_, petal_size / 2 + size_offset, 0, tau, 0)
            ctx.rgb(*self.petal_color).fill()
            ctx.restore()
        if self.highlighted:
            ctx.arc(x, y, self.size / 2 + 5, 0, tau, 1)
            ctx.rgb(*GO_GREEN).stroke()

        ctx.arc(x, y, self.size / 2, 0, tau, 0)
        ctx.rgb(*self.bg).fill()

        # label
        # y += self.size / 3
        w = max(self.size, ctx.text_width(self.label) + 10)
        h = self.size / 3 + 8
        if False and self.highlighted:
            ctx.rgb(*BLACK).move_to(x, y - height / 2).round_rectangle(
                x - width / 2, y - height / 2, width, height, width // 2
            ).fill()
            ctx.rgb(*GO_GREEN).move_to(x, y).text(self.label)
        else:
            ctx.save()
            ctx.translate(0, self.size / 3)
            ctx.rgb(*PUSH_RED).round_rectangle(
                x - w / 2, y - h / 2, w, h, w // 2
            ).fill()

            ctx.rgb(*BLACK).move_to(x, y).text(self.label)
            ctx.restore()

        ctx.restore()


class FlowerMenu(MenuController):
    """
    A circular menu with flowers.
    """

    __slots__ = (
        "_ts",
        "_sun",
    )

    def __init__(self, items: List[MenuItem], vm: ViewManager, name="flow3r") -> None:
        self._ts = 0
        self.name = name
        self.ui = GroupRing(r=80)
        for item in items:
            self.ui.items_ring.append(FlowerIcon(label=item.label()))
        super().__init__(items, vm)
        self._scroll_controller.wrap = True

        self.icon = FlowerIcon(label=self.name)
        self.icon.rotation_time = -5000
        self.ui.item_center = self.icon

        self.angle = 0
        self.angle_step = 0.2

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)
        self.ui.think(ins, delta_ms)
        self._ts += delta_ms

    def draw(self, ctx: Ctx) -> None:
        ctx.gray(0)
        ctx.rectangle(-120, -120, 240, 240).fill()
        for item in self.ui.items_ring:
            item.highlighted = False
            item.rotation_time = 0
        current = self._scroll_controller.current_position()
        self.ui.items_ring[int(current)].highlighted = True
        self.ui.items_ring[int(current)].rotation_time = 3000
        self.ui.angle_offset = math.pi - (tau * current / len(self.ui.items_ring))

        self.ui.draw(ctx)
        # print("here")
        # ctx.font_size = 40
        # ctx.text_align = ctx.CENTER
        # ctx.text_baseline = ctx.MIDDLE

        # angle_per_item = 0.4

        # current = self._scroll_controller.current_position()

        # for ix, item in enumerate(self._items):
        #    rot = (ix - current) * angle_per_item
        #    self._draw_text_angled(ctx, item.label(), rot, 1 - abs(rot))

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
