"""
Factory test/QC file.
"""

import st4m

from st4m.goose import Optional, List, ABCBase, abstractmethod, Tuple
from st4m.input import InputController
from st4m.ui.view import View, ViewManager, ViewTransitionBlend
from st4m import Responder, InputState, Ctx

import audio
import badge_link

import math, time, hardware, leds


vm = ViewManager(ViewTransitionBlend())


def lerp(a: float, b: float, v: float) -> float:
    if v <= 0:
        return a
    if v >= 1.0:
        return b
    return a + (b - a) * v


class LED:
    def __init__(self, ix, rgb=None, hsv=None):
        self._ix = ix
        self._rgb = rgb
        self._hsv = hsv

    def apply(self):
        if self._rgb is not None:
            leds.set_rgb(self._ix, *self._rgb)
        elif self._hsv is not None:
            leds.set_hsv(self._ix, *self._hsv)


class Test:
    NAME = 'unknown'
    COLOR = (0, 0, 0)
    COLOR_PASS = (0, 255, 0)
    NO_LEDS = False
    def passed(self) -> bool:
        ...

    def think(self, ins: InputState, delta_ms: int) -> None:
        ...

    def leds(self) -> List[LED]:
        ...


class SplashScreen(Responder):
    def __init__(self) -> None:
        self._time_ms = 0

    def draw(self, ctx: Ctx) -> None:
        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE

        ctx.gray(0.8)
        ctx.move_to(0, -30).font_size = 20
        ctx.text("Please visit")

        ctx.gray(1)
        ctx.move_to(0, 0).font_size = 40
        ctx.text("flow3r.garden")

        ctx.gray(0.8).move_to(0, 30).font_size = 20
        ctx.text("in a web browser to")
        ctx.move_to(0, 50).text("set up your badge.")

class TouchTest(Test):
    NAME = 'captouch'
    def __init__(self) -> None:
        self.top_petals_seen = [False for _ in range(5 * 3)]
        self.top_petals_seen_early = [False for _ in range(5 * 3)]
        self.bot_petals_seen = [False for _ in range(5 * 2)]
        self.bot_petals_seen_early = [False for _ in range(5 * 2)]
        self.bot_petals_seen[4] = True
        self.input = InputController()
        self._checked_early = False

    def think(self, ins: InputState, delta_ms: int) -> None:
        self.input.think(ins, delta_ms)
        for i in range(10):
            petal = self.input.captouch.petals[i]
            if i % 2 == 0:
                self.top_petals_seen[i // 2 * 3 + 0] |= petal.base.pressed
                self.top_petals_seen[i // 2 * 3 + 1] |= petal.cw.pressed
                self.top_petals_seen[i // 2 * 3 + 2] |= petal.ccw.pressed
            else:
                self.bot_petals_seen[i // 2 * 2 + 0] |= petal.base.pressed
                self.bot_petals_seen[i // 2 * 2 + 1] |= petal.tip.pressed

        if not self._checked_early:
            self._checked_early = True
            self.top_petals_seen_early = self.top_petals_seen[:]
            self.bot_petals_seen_early = self.bot_petals_seen[:]
            self.bot_petals_seen_early[4] = False


    def passed(self) -> bool:
        return all(self.bot_petals_seen) and all(self.top_petals_seen) \
            and not any(self.bot_petals_seen_early) and not any(self.top_petals_seen_early)

    def leds(self) -> List[LED]:
        res = []
        # top petals
        for i in range(5):
            seen = all(self.top_petals_seen[i*3:i*3+3])
            seen_early = any(self.top_petals_seen_early[i*3:i*3+3])
            if seen_early:
                res.append(LED(8 * i, rgb=(255, 0, 0)))
            elif seen:
                res.append(LED(8 * i, rgb=(0, 255, 0)))
        # bottom petals
        for i in range(5):
            seen = all(self.bot_petals_seen[i*2:i*2+2])
            seen_early = any(self.bot_petals_seen_early[i*2:i*2+2])
            if seen_early:
                res.append(LED(4 + 8 * i, rgb=(255, 0, 0)))
            elif seen:
                res.append(LED(4 + 8 * i, rgb=(0, 255, 0)))
        return res


class ButtonTest(Test):
    NAME = 'buttons'
    def __init__(self):
        self.seen = [False for _ in range(6)]
        self.seen_early = [False for _ in range(6)]
        self.input = InputController()
        self._checked_early = False

    def think(self, ins: InputState, delta_ms: int) -> None:
        self.input.think(ins, delta_ms)
        self.seen[0] |= self.input.left_shoulder.left.pressed
        self.seen[1] |= self.input.left_shoulder.middle.pressed
        self.seen[2] |= self.input.left_shoulder.right.pressed
        self.seen[3] |= self.input.right_shoulder.left.pressed
        self.seen[4] |= self.input.right_shoulder.middle.pressed
        self.seen[5] |= self.input.right_shoulder.right.pressed

        if not self._checked_early:
            self._checked_early = True
            self.seen_early = self.seen[:]

    def passed(self) -> bool:
        return all(self.seen) and not any(self.seen_early)

    def leds(self) -> List[LED]:
        res = []
        for ix_seen, ix_led in [
            (0, 7), (1, 6), (2, 5),
            (3, 35), (4, 34), (5, 33),
        ]:
            if self.seen_early[ix_seen]:
                res.append(LED(ix_led, rgb=(255, 0, 0)))
            elif self.seen[ix_seen]:
                res.append(LED(ix_led, rgb=(0, 255, 0)))
        return res


class DispatchApp(Responder):
    def __init__(self):
        self._ix = 0
        self._selftest_error = set()
        self._selftest_ran = False
        self._splash = SplashScreen()
        self._tests = [
            TouchTest(),
            ButtonTest(),
        ]
        self._tests_passed = [False for _ in self._tests]
        self._selftest_leds = []
        self._passed = False
        self._input = InputController()
        self._ms = 0

    def _fail(self, ix: int, msg: str) -> None:
        print('SELF-TEST FAILED: ' + msg)
        self._selftest_error.add(msg)
        self._selftest_leds.append(ix)

    def selftest(self, ins: InputState) -> None:
        if self._selftest_ran:
            return
        self._selftest_ran = True
        self.selftest_i2c()
        self.selftest_badge_link()

        if not self._selftest_error:
            print('SELF-TEST PASSED')
    
    def selftest_i2c(self):
        devices = hardware.i2c_scan()
        if 0x10 not in devices:
            self._fail(17, "i2c:codec")
        if 0x2c not in devices:
            self._fail(18, "i2c:touch-top")
        if 0x2d not in devices:
            self._fail(19, "i2c:touch-bot")
        if 0x6e not in devices:
            self._fail(21, "i2c:portexp-0")
        if 0x6d not in devices:
            self._fail(22, "i2c:portexp-1")

    def selftest_badge_link(self):
        badge_link.right.enable()
        badge_link.left.enable()

        rt = badge_link.right.tip
        rr = badge_link.right.ring

        lt = badge_link.left.tip
        lr = badge_link.left.ring

        rt.pin.init(mode=rt.pin.OUT)
        rr.pin.init(mode=rr.pin.OUT)

        lt.pin.init(mode=lt.pin.IN)
        lr.pin.init(mode=lr.pin.IN)

        rt.pin.on()
        rr.pin.on()

        time.sleep(0.1)

        if not lt.pin.value():
            self._fail(23, "tip stuck 0")
        if not lr.pin.value():
            self._fail(24, "ring stuck 0")

        rt.pin.off()
        rr.pin.off()

        time.sleep(0.1)

        if lt.pin.value():
            self._fail(23, "tip stuck 1")
        if lr.pin.value():
            self._fail(24, "ring stuck 1")

        badge_link.right.disable()
        badge_link.left.disable()

    def draw(self, ctx: Ctx) -> None:
        ctx.save()
        ctx.rectangle(-120, -120, 240, 240).gray(0).fill()

        if self._selftest_error:
            ctx.text_align = ctx.CENTER
            ctx.text_baseline = ctx.MIDDLE
            ctx.rgb(1, 0, 0)
            ctx.move_to(0, -20).font_size = 40
            ctx.text("SELF-TEST")
            ctx.move_to(0, 20).font_size = 40
            ctx.text("FAILED")
            ctx.move_to(0, 60).font_size = 20
            ctx.text(' '.join(sorted(self._selftest_error)))
            ctx.restore()
            return

        if self._passed:
            ctx.text_align = ctx.CENTER
            ctx.text_baseline = ctx.MIDDLE
            ctx.rgb(0, 1, 0)
            ctx.move_to(0, 0).font_size = 80
            ctx.text("PASS")
        else:
            self._splash.draw(ctx)
        ctx.restore()

    def think(self, ins: InputState, delta_ms: int) -> None:
        self._input.think(ins, delta_ms)
        self.selftest(ins)
        self._ms += delta_ms

        for i, test in enumerate(self._tests):
            test.think(ins, delta_ms)

            if test.passed() and not self._tests_passed[i]:
                self._tests_passed[i] = True
                print('Test ' + test.NAME + ' passed')

        passed = all(t.passed() for t in self._tests)
        if passed and not self._passed and not self._selftest_error:
            self._passed = passed
            print('ALL PASSED')
            audio.input_set_source(audio.INPUT_SOURCE_ONBOARD_MIC)

        if self._passed:
            rgb = int(self._ms / 1000) % 4
            rgb = [
                (255, 0, 0),
                (0, 255, 0),
                (0, 0, 255),
                (255, 255, 255),
            ][rgb]
            leds.set_all_rgb(*rgb)
            leds.update()
        else:
            leds.set_all_rgb(0, 0, 0)
            leds.set_rgb(1, 0, 255, 0)
            for test in self._tests:
                for led in test.leds():
                    led.apply()
            for ix in self._selftest_leds:
                leds.set_rgb(ix, 255, 0, 0)
            leds.update()


leds.set_all_rgb(0, 0, 0)
leds.update()

reactor = st4m.Reactor()
reactor.set_top(DispatchApp())
reactor.run()
