"""
Composition/overlay system.

This is different from a menu system. Instead of navigation, it should be used
for persistent, anchored symbols like charging symbols, toasts, debug overlays,
etc.
"""

from st3m import Responder, InputState, Reactor
from st3m.goose import Dict, Enum, List, ABCBase, abstractmethod, Optional
from st3m.utils import tau
from st3m.ui.view import ViewManager
from st3m.input import power
from ctx import Context

import math
import audio
import sys_kernel


class OverlayKind(Enum):
    # Battery, USB, ...
    Indicators = 0
    # Naughty debug developers for naughty developers and debuggers.
    Debug = 1
    Touch = 2
    Toast = 3


_all_kinds = [
    OverlayKind.Indicators,
    OverlayKind.Debug,
    OverlayKind.Touch,
    OverlayKind.Toast,
]


class Overlay(Responder):
    """
    An Overlay is a Responder with some kind of OverlayKind attached.g
    """

    kind: OverlayKind


class Compositor(Responder):
    """
    A Compositor blends together some ViewManager alongside with active
    Overlays. Overlays can be enabled/disabled by kind.
    """

    def __init__(self, main: ViewManager):
        self.main = main
        self.overlays: Dict[OverlayKind, List[Responder]] = {}
        self.enabled: Dict[OverlayKind, bool] = {
            OverlayKind.Indicators: True,
            OverlayKind.Debug: True,
            OverlayKind.Toast: True,
        }

    def _enabled_overlays(self) -> List[Responder]:
        res: List[Responder] = []
        for kind in _all_kinds:
            if not self.enabled.get(kind, False):
                continue
            if kind == OverlayKind.Indicators and not self.main.wants_icons():
                continue
            for overlay in self.overlays.get(kind, []):
                res.append(overlay)
        return res

    def think(self, ins: InputState, delta_ms: int) -> None:
        self.main.think(ins, delta_ms)
        for overlay in self._enabled_overlays():
            overlay.think(ins, delta_ms)

    def draw(self, ctx: Context) -> None:
        self.main.draw(ctx)
        for overlay in self._enabled_overlays():
            overlay.draw(ctx)

    def add_overlay(self, ov: Overlay) -> None:
        """
        Add some Overlay to the Compositor. It will be drawn if enabled.
        """
        if ov.kind not in self.overlays:
            self.overlays[ov.kind] = []
        self.overlays[ov.kind].append(ov)


class DebugFragment(ABCBase):
    """
    Something which wishes to provide some text to the OverlayDebug.
    """

    @abstractmethod
    def text(self) -> str:
        ...


class DebugReactorStats(DebugFragment):
    """
    DebugFragment which provides the OverlayDebug with information about the
    active reactor.
    """

    def __init__(self, reactor: Reactor):
        self._stats = reactor.stats

    def text(self) -> str:
        res = []

        rts = self._stats.run_times
        if len(rts) > 0:
            avg = sum(rts) / len(rts)
            res.append(f"tick: {int(avg)}ms")

        rts = self._stats.render_times
        if len(rts) > 0:
            avg = sum(rts) / len(rts)
            res.append(f"fps: {int(1/(avg/1000))}")

        return " ".join(res)


class DebugBattery(DebugFragment):
    """
    DebugFragment which provides the OverlayDebug with information about the
    battery voltage.
    """

    def text(self) -> str:
        v = power.battery_voltage
        return f"bat: {v:.2f}V"


class OverlayDebug(Overlay):
    """
    Overlay which renders a text bar at the bottom of the screen with random
    bits of trivia.
    """

    kind = OverlayKind.Debug

    def __init__(self) -> None:
        self.fragments: List[DebugFragment] = []

    def add_fragment(self, f: DebugFragment) -> None:
        self.fragments.append(f)

    def think(self, ins: InputState, delta_ms: int) -> None:
        pass

    def text(self) -> str:
        text = [f.text() for f in self.fragments if f.text() != ""]
        return " ".join(text)

    def draw(self, ctx: Context) -> None:
        ctx.save()
        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE
        ctx.font = ctx.get_font_name(0)
        ctx.font_size = 10
        ctx.gray(0).rectangle(-120, 80, 240, 12).fill()
        ctx.gray(1).move_to(0, 86).text(self.text())
        ctx.restore()


class OverlayCaptouch(Overlay):
    kind = OverlayKind.Touch

    class Dot(Responder):
        def __init__(self, ix: int) -> None:
            self.ix = ix
            self.phi = 0.0
            self.rad = 0.0
            self.pressed = False

        def think(self, s: InputState, delta_ms: int) -> None:
            self.pressed = s.captouch.petals[self.ix].pressed
            (rad, phi) = s.captouch.petals[self.ix].position
            self.phi = phi
            self.rad = rad

        def draw(self, ctx: Context) -> None:
            if not self.pressed:
                return

            a = (tau / 10) * self.ix
            ctx.rotate(a)
            ctx.translate(0, -80)

            offs_x = self.phi / 1000
            offs_y = -self.rad / 1000
            ctx.rectangle(-5 + offs_x, -5 + offs_y, 10, 10)
            ctx.rgb(1, 0, 1)
            ctx.fill()

    def __init__(self) -> None:
        self.dots = [self.Dot(i) for i in range(10)]

    def think(self, ins: InputState, delta_ms: int) -> None:
        for dot in self.dots:
            dot.think(ins, delta_ms)

    def draw(self, ctx: Context) -> None:
        for dot in self.dots:
            ctx.save()
            dot.draw(ctx)
            ctx.restore()


class OverlayVolume(Overlay):
    """
    Renders a black square with icon and volume slider.

    Icon depends on whether headphones are plugged in or not.
    """

    kind = OverlayKind.Toast

    def __init__(self) -> None:
        self._showing: Optional[int] = None
        self._volume = 0.0
        self._headphones = False
        self._muted = False

        self._started = False
        self._prev_volume = self._volume
        self._prev_headphones = self._headphones
        self._prev_muted = self._muted

    def changed(self) -> bool:
        """
        Returns true if there was a system volume change warranting re-drawing
        the overlay.
        """
        if self._prev_volume != self._volume:
            return True
        if self._prev_headphones != self._headphones:
            return True
        if self._prev_muted != self._muted:
            return True
        return False

    def think(self, ins: InputState, delta_ms: int) -> None:
        self._volume = audio.get_volume_dB()
        self._headphones = audio.headphones_are_connected()
        self._muted = audio.get_mute()
        if self._headphones:
            # -20 ... +3dB
            self._volume += 20
            self._volume /= 20 + 3
        else:
            # -20 ... +14dB
            self._volume += 20
            self._volume /= 20 + 14

        if self._started:
            if self.changed():
                self._showing = 1000
        else:
            # Ignore first run cycle, to not show volume slider on startup.
            self._started = True

        self._prev_volume = self._volume
        self._prev_headphones = self._headphones
        self._prev_muted = self._muted

        if self._showing is None:
            return
        self._showing -= delta_ms
        if self._showing < 0:
            self._showing = None

    def draw(self, ctx: Context) -> None:
        if self._showing is None:
            return

        opacity = self._showing / 200
        opacity = min(opacity, 0.8)

        ctx.start_group()
        ctx.global_alpha = opacity

        # Background
        ctx.gray(0)
        ctx.round_rectangle(-40, -40, 80, 80, 5)
        ctx.fill()

        ctx.end_group()

        # Foreground
        opacity = self._showing / 200
        opacity = min(opacity, 1)
        ctx.start_group()
        ctx.global_alpha = opacity

        muted = self._muted
        if muted:
            ctx.gray(0.5)
        else:
            ctx.gray(1)

        # Icon
        if self._headphones:
            ctx.round_rectangle(-20, -10, 5, 20, 3)
            ctx.fill()
            ctx.round_rectangle(15, -10, 5, 20, 3)
            ctx.fill()
            ctx.line_width = 3
            ctx.arc(0, -10, 17, tau / 2, tau, 0)
            ctx.stroke()
        else:
            ctx.move_to(-10, -10)
            ctx.line_to(0, -10)
            ctx.line_to(10, -20)
            ctx.line_to(10, 10)
            ctx.line_to(0, 0)
            ctx.line_to(-10, 0)
            ctx.fill()

        ctx.gray(1)

        # Volume slider
        ctx.round_rectangle(-30, 20, 60, 10, 3)
        ctx.line_width = 2
        ctx.stroke()

        v = self._volume
        v = min(max(v, 0), 1)

        width = 60 * v
        ctx.round_rectangle(-30, 20, width, 10, 3)
        ctx.fill()

        ctx.end_group()


class Icon(Responder):
    """
    A tray icon that might or might not be shown in the top icon tray.

    Should render into a 240x240 viewport. Will be scaled down by the IconTray
    that contains it.
    """

    @abstractmethod
    def visible(self) -> bool:
        ...


class USBIcon(Icon):
    """
    Found in the bargain bin at an Aldi Süd.

    Might or might not be related to a certain serial bus.
    """

    def visible(self) -> bool:
        return sys_kernel.usb_connected()

    def draw(self, ctx: Context) -> None:
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


class IconTray(Overlay):
    """
    An overlay which renders Icons.
    """

    kind = OverlayKind.Indicators

    def __init__(self) -> None:
        self.icons = [
            USBIcon(),
        ]
        self.visible: List[Icon] = []

    def think(self, ins: InputState, delta_ms: int) -> None:
        self.visible = [i for i in self.icons if i.visible()]
        for v in self.visible:
            v.think(ins, delta_ms)

    def draw(self, ctx: Context) -> None:
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
