"""
Composition/overlay system.

This is different from a menu system. Instead of navigation, it should be used
for persistent, anchored symbols like charging symbols, toasts, debug overlays,
etc.
"""

from st3m import Responder, InputState, Reactor, settings
from st3m.goose import Dict, Enum, List, ABCBase, abstractmethod, Optional
from st3m.utils import tau
from st3m.ui.view import ViewManager
from st3m.input import power
from st3m.power import approximate_battery_percentage
from ctx import Context
import st3m.wifi

import math
import audio
import sys_kernel
import sys_display
import network


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

_clip_x0 = 120
_clip_x1 = 120
_clip_y0 = 0
_clip_y1 = 0


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
        self._frame_skip = 0

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
        if sys_display.get_mode() != 0:
            return
        if self._frame_skip <= 0:
            if not settings.onoff_show_fps.value and not sys_display.get_mode() != 0:
                for overlay in self._enabled_overlays():
                    overlay.think(ins, delta_ms)

    def draw(self, ctx: Context) -> None:
        global _clip_x0
        global _clip_y0
        global _clip_x1
        global _clip_y1
        self.main.draw(ctx)
        if sys_display.get_mode() != 0:
            return
        if self._frame_skip <= 0:
            octx = sys_display.ctx(256)  # XXX add symbolic name for overlay
            if settings.onoff_show_fps.value:
                _clip_x0 = 110
                _clip_y1 = 0
                _clip_x1 = 130
                _clip_y1 = 7
                octx.save()
                octx.compositing_mode = octx.CLEAR
                octx.rectangle(
                    _clip_x0 - 120,
                    _clip_y0 - 120,
                    _clip_x1 - _clip_x0 + 1,
                    _clip_y1 - _clip_y0 + 1,
                ).fill()
                octx.restore()
                octx.gray(1)
                octx.font_size = 11
                octx.font = "Bold"
                octx.move_to(0, -113)
                octx.text_align = octx.CENTER
                octx.text("{0:.1f}".format(sys_display.fps()))
            else:
                _clip_x0 = 80
                _clip_y0 = 0
                _clip_x1 = 160
                _clip_y1 = 0
                octx.save()
                octx.compositing_mode = octx.CLEAR
                octx.rectangle(-120, -120, 240, 240).fill()
                octx.restore()
                for overlay in self._enabled_overlays():
                    overlay.draw(octx)
            self._frame_skip = 4
            sys_display.overlay_clip(_clip_x0, _clip_y0, _clip_x1, _clip_y1)
            sys_display.update(octx)
        self._frame_skip -= 1

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
            ctx.fill()

    def __init__(self) -> None:
        self.dots = [self.Dot(i) for i in range(10)]

    def think(self, ins: InputState, delta_ms: int) -> None:
        for dot in self.dots:
            dot.think(ins, delta_ms)

    def draw(self, ctx: Context) -> None:
        ctx.rgb(1, 0, 1)
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
        self._volume = audio.get_volume_relative()
        self._headphones = audio.headphones_are_connected()
        self._muted = (self._volume == 0) or audio.get_mute()

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
        self._showing -= delta_ms * 4
        if self._showing < 0:
            self._showing = None

    def draw(self, ctx: Context) -> None:
        global _clip_y1
        if self._showing is None:
            return
        _clip_y1 = max(_clip_y1, 161)

        opacity = self._showing / 200
        opacity = min(opacity, 0.8)

        # Background
        ctx.rgba(0, 0, 0, opacity)
        ctx.round_rectangle(-40, -40, 80, 80, 5)
        ctx.fill()

        # Foreground
        opacity = self._showing / 200
        opacity = min(opacity, 1)

        muted = self._muted
        if muted:
            ctx.rgba(0.5, 0.5, 0.5, opacity)
        else:
            ctx.rgba(1.0, 1.0, 1.0, opacity)

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

        ctx.rgba(1.0, 1.0, 1.0, opacity)

        # Volume slider
        ctx.round_rectangle(-30, 20, 60, 10, 3)
        ctx.line_width = 2
        ctx.stroke()

        width = 60 * self._volume
        ctx.round_rectangle(-30, 20, width, 10, 3)
        ctx.fill()


class Icon(Responder):
    """
    A tray icon that might or might not be shown in the top icon tray.

    Should render into a 240x240 viewport. Will be scaled down by the IconTray
    that contains it.
    """

    WIDTH: int = 25

    @abstractmethod
    def visible(self) -> bool:
        ...


class USBIcon(Icon):
    """
    Found in the bargain bin at an Aldi Süd.

    Might or might not be related to a certain serial bus.
    """

    WIDTH: int = 20

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


class WifiIcon(Icon):
    WIDTH: int = 15

    def __init__(self) -> None:
        super().__init__()
        self._rssi: float = -120

    def visible(self) -> bool:
        return st3m.wifi.enabled()

    def draw(self, ctx: Context) -> None:
        ctx.gray(1.0)
        ctx.line_width = 10.0
        w = 1.5
        a = -w / 2 - 3.14 / 2
        b = w / 2 - 3.14 / 2

        r = self._rssi
        ctx.gray(1.0 if r > -75 else 0.2)
        ctx.arc(0, 65, 100, a, b, 0).stroke()
        ctx.gray(1.0 if r > -85 else 0.2)
        ctx.arc(0, 65, 70, a, b, 0).stroke()
        ctx.gray(1.0 if r > -95 else 0.2)
        ctx.arc(0, 65, 40, a, b, 0).stroke()

    def think(self, ins: InputState, delta_ms: int) -> None:
        self._rssi = st3m.wifi.rssi()


class BatteryIcon(Icon):
    def __init__(self) -> None:
        super().__init__()
        self._percent = 100.0
        self._charging = False

    def visible(self) -> bool:
        return True

    def draw(self, ctx: Context) -> None:
        if self._percent > 30:
            ctx.rgb(0.17, 0.55, 0.04)
        else:
            ctx.rgb(0.52, 0.04, 0.17)

        height = 160 * self._percent / 100
        ctx.rectangle(-80, -50, height, 100)
        ctx.fill()

        ctx.gray(0.8)
        ctx.line_width = 10.0
        ctx.rectangle(80, -50, -160, 100)
        ctx.stroke()
        ctx.rectangle(100, -30, -20, 60)
        ctx.fill()

        if self._charging:
            ctx.gray(1)
            ctx.line_width = 20
            ctx.move_to(10, -65 - 10)
            ctx.line_to(-30, 20 - 10)
            ctx.line_to(30, -20 - 10)
            ctx.line_to(-10, 65 - 10)
            ctx.line_to(-20, 35 - 10)
            ctx.stroke()
            ctx.move_to(-10, 65 - 10)
            ctx.line_to(40, 35 - 10)
            ctx.stroke()

    def think(self, ins: InputState, delta_ms: int) -> None:
        self._percent = approximate_battery_percentage(power.battery_voltage)
        self._charging = power.battery_charging


class IconTray(Overlay):
    """
    An overlay which renders Icons.
    """

    kind = OverlayKind.Indicators

    def __init__(self) -> None:
        self.icons = [
            BatteryIcon(),
            USBIcon(),
            WifiIcon(),
        ]
        self.visible: List[Icon] = []

    def think(self, ins: InputState, delta_ms: int) -> None:
        self.visible = [i for i in self.icons if i.visible()]
        for v in self.visible:
            v.think(ins, delta_ms)

    def draw(self, ctx: Context) -> None:
        global _clip_y1
        if len(self.visible) < 1:
            return
        _clip_y1 = max(_clip_y1, 32)
        width = 0
        for icon in self.visible:
            width += icon.WIDTH
        x0 = width / -2 + self.visible[0].WIDTH / 2
        for i, v in enumerate(self.visible):
            ctx.save()
            ctx.translate(x0, -100)
            ctx.scale(0.1, 0.1)
            v.draw(ctx)
            ctx.restore()
            x0 = x0 + v.WIDTH
