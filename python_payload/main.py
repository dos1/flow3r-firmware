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

from apps.demo_worms4 import app as worms

worms._view_manager = vm
menu_apps = SimpleMenu(
    [
        MenuItemBack(),
        MenuItemNoop("captouch"),
        MenuItemForeground("worms", worms),
    ],
    vm,
)


menu_main = FlowerMenu(
    [
        MenuItemForeground("Music", menu_music),
        MenuItemForeground("Apps", menu_apps),
        MenuItemNoop("Settings"),
    ],
    vm,
)


from st4m.input import PetalController
import math


from st3m.system import hardware


class PetalResponder(st4m.Responder):
    def __init__(self):
        self.petal = PetalController(0)
        self.ts = 0.0
        self._last = ""
        self.speed = 0.0
        self.value = 0.0
        self.speedbuffer = [0.0]
        self._ignore = 0

    def think(self, ins: InputState, delta_ms: int) -> None:
        self.ts += delta_ms
        self.petal.think(ins, delta_ms)

        if self._ignore:
            self._ignore -= 1
            return

        dx = self.petal._input._dx
        dphi = self.petal._input._dphi

        phase = self.petal._input.phase()
        speed = sum(self.speedbuffer) / len(self.speedbuffer)
        if abs(speed) < 0.00001:
            speed = 0

        max = 50

        if speed > max:
            speed = max
        if speed < -max:
            speed = -max

        if len(self.speedbuffer) > 10:
            self.speedbuffer.pop(0)

        if phase == self.petal._input.ENDED:
            self.speedbuffer = [0 if abs(speed) < 0.001 else speed]
        elif phase == self.petal._input.UP:
            self.speedbuffer.append(speed * 0.85)
        elif phase == self.petal._input.BEGIN:
            self._ignore = 5
            self.speedbuffer = [0.0]
        elif phase == self.petal._input.RESTING:
            self.speedbuffer.append(0.0)
        elif phase == self.petal._input.MOVED:
            self.speedbuffer.append(dphi / delta_ms)
            # self.speed *= 0.3

        # print(speed, phase, self.speedbuffer)

        self.value += speed * delta_ms
        self.speed = speed

    def draw(self, ctx: Ctx) -> None:
        tau = math.pi * 2

        dx = self.petal._input._dx
        dy = self.petal._input._dy

        sum = abs(dx) + abs(dy)
        pos = self.petal._input._pos
        # ang = -math.atan2(*pos) - tau / 6 * 2
        ang = self.value
        p = self.petal._input.phase()
        self._last = p
        # if p == "up" and self._last != "up":
        #    print(p)
        # if p != "up" and p != "resting":
        #    print(p, dx, dy, sum, ang)

        ctx.gray(0)
        ctx.rectangle(-120, -120, 240, 240).fill()

        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE
        ctx.font_size = 20
        ctx.gray(1)
        # ctx.rectangle(-20, -20, 40, 40).fill()
        ctx.move_to(0, 0).text(p)
        ctx.move_to(0, 60).text(f"value: {self.value:.2f}")

        dir = "stop"
        if self.speed > 0:
            dir = "ccw"
        if self.speed < 0:
            dir = "cw"

        ctx.move_to(0, 90).text(f"{dir}: {self.speed*1000:.2f}")

        # if p != "up":
        ctx.rectangle(math.sin(ang) * 100, math.cos(ang) * 100, 20, 20).fill()

        (r, phi) = self.petal._input._polar
        p = (math.sin(phi) * r, math.cos(phi) * r)

        ctx.rgb(1, 0, 0).rectangle(p[0], p[1], 10, 10).fill()


pr = PetalResponder()
vm.push(menu_main)

reactor = st4m.Reactor()
reactor.set_top(vm)
# reactor.set_top(pr)
reactor.run()
