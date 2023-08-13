from st3m.goose import ABCBase, abstractmethod, List, Optional
from st3m.input import InputState
from ctx import Context

import time
import sys_display
import sys_kernel


class Responder(ABCBase):
    """
    Responder is an interface from the Reactor to any running Micropython code
    that wishes to access input state or draw to the screen.

    A Responder can be a system menu, an application, a graphical widget, etc.

    The Reactor will call think and draw methods at a somewhat-constant pace in
    order to maintain a smooth system-wide update rate and framerate.
    """

    @abstractmethod
    def think(self, ins: InputState, delta_ms: int) -> None:
        """
        think() will be called when the Responder should process the InputState
        and perform internal logic. delta_ms will be set to the number of
        milliseconds elapsed since the last reactor think loop.

        The code must not sleep or block during this callback, as that will
        impact the system tickrate and framerate.
        """
        pass

    @abstractmethod
    def draw(self, ctx: Context) -> None:
        """
        draw() will be called when the Responder should draw, ie. generate a drawlist by performing calls on the given ctx object.

        Depending on what calls the Responder, the ctx might either represent
        the surface of the entire screen, or some composited-subview (eg. an
        application screen that is currently being transitioned out by sliding
        left). Unless specified otherwise by the compositing stack, the screen
        coordinates are +/- 120 in both X and Y (positive numbers towards up and
        right), with 0,0 being the middle of the screen.

        The Reactor will then rasterize and blit the result.

        The code must not sleep or block during this callback, as that will
        impact the system tickrate and framerate.
        """
        pass


class ReactorStats:
    def __init__(self) -> None:
        self.run_times: List[int] = []
        self.render_times: List[int] = []

    def record_run_time(self, ms: int) -> None:
        self.run_times.append(ms)
        delete = len(self.run_times) - 20
        if delete > 0:
            self.run_times = self.run_times[delete:]

    def record_render_time(self, ms: int) -> None:
        self.render_times.append(ms)
        delete = len(self.render_times) - 20
        if delete > 0:
            self.render_times = self.render_times[delete:]


class Reactor:
    """
    The Reactor is the main Micropython scheduler of the st3m system and any
    running payloads.

    It will attempt to run a top Responder with a fixed tickrate a framerate
    that saturates the display rasterization/blitting pipeline.
    """

    __slots__ = (
        "_top",
        "_tickrate_ms",
        "_last_tick",
        "_ctx",
        "_ts",
        "_last_ctx_get",
        "_swap_buttons",
        "stats",
    )

    def __init__(self) -> None:
        self._top: Optional[Responder] = None
        self._tickrate_ms: int = 20
        self._ts: int = 0
        self._last_tick: Optional[int] = None
        self._last_ctx_get: Optional[int] = None
        self._ctx: Optional[Context] = None
        self._swap_buttons = False
        self.stats = ReactorStats()

    def set_top(self, top: Responder) -> None:
        """
        Set top Responder. It will be called by the reactor in a loop once run()
        is called.

        This can be also called after the reactor is started.
        """
        self._top = top

    def run(self) -> None:
        """
        Run the reactor forever, processing the top Responder in a loop.
        """
        while True:
            self._run_once()

    def set_buttons_swapped(self, swapped: bool) -> None:
        self._swap_buttons = swapped

    def _run_once(self) -> None:
        start = time.ticks_ms()
        deadline = start + self._tickrate_ms

        self._run_top(start)

        end = time.ticks_ms()
        elapsed = end - start
        self.stats.record_run_time(elapsed)
        wait = deadline - end
        if wait > 0:
            sys_kernel.freertos_sleep(wait)

    def _run_top(self, start: int) -> None:
        # Skip if we have no top Responder.
        if self._top is None:
            return

        # Calculate delta (default to tickrate if running first iteration).
        delta = self._tickrate_ms
        if self._last_tick is not None:
            delta = start - self._last_tick
        self._last_tick = start

        self._ts += delta

        hr = InputState.gather(self._swap_buttons)

        # Think!
        self._top.think(hr, delta)

        # Draw!
        if self._ctx is None:
            self._ctx = sys_display.get_ctx()
            if self._ctx is not None:
                if self._last_ctx_get is not None:
                    diff = start - self._last_ctx_get
                    self.stats.record_render_time(diff)
                self._last_ctx_get = start

                self._ctx.save()
                self._top.draw(self._ctx)
                self._ctx.restore()
        if self._ctx is not None and not sys_display.pipe_full():
            sys_display.update(self._ctx)
            self._ctx = None
