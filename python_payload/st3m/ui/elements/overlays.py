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
import st3m.power
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


class Region:
    x0: int
    y0: int
    x1: int
    y1: int

    def __init__(self, *args):
        if not args:
            self.clear()
        else:
            self.set(*args)

    def set(self, x0, y0, x1, y1):
        self.x0 = x0
        self.y0 = y0
        self.x1 = x1
        self.y1 = y1

    def add(self, x0, y0, x1, y1):
        self.x0 = min(self.x0, x0)
        self.y0 = min(self.y0, y0)
        self.x1 = max(self.x1, x1)
        self.y1 = max(self.y1, y1)

    def add_region(self, rect):
        self.add(rect.x0, rect.y0, rect.x1, rect.y1)

    def clear(self):
        self.x0 = self.y0 = self.x1 = self.y1 = -120

    def fill(self, ctx):
        ctx.rectangle(self.x0, self.y0, self.x1 - self.x0, self.y1 - self.y0)
        ctx.fill()

    def is_empty(self):
        return self.x1 == self.x0 or self.y1 == self.y0

    def set_clip(self):
        sys_display.overlay_clip(
            self.x0 + 120, self.y0 + 120, self.x1 + 120, self.y1 + 120
        )

    def copy(self, rect):
        self.x0 = rect.x0
        self.x1 = rect.x1
        self.y0 = rect.y0
        self.y1 = rect.y1

    def __eq__(self, rect):
        return (
            self.x0 == rect.x0
            and self.x1 == rect.x1
            and self.y0 == rect.y0
            and self.y1 == rect.y1
        )

    def __repr__(self):
        return f"({self.x0} x {self.y0} - {self.x1} x {self.y1})"


class Overlay(Responder):
    """
    An Overlay is a Responder with some kind of OverlayKind attached.
    """

    kind: OverlayKind

    def needs_redraw(rect: Region) -> bool:
        rect.add(-120, -120, 120, 120)
        return True


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
        self._last_fps_string = ""
        self._clip_rect = Region()
        self._last_clip = Region()
        self._display_mode = None
        self._enabled: List[Responder] = []
        self._last_enabled: List[Responder] = []
        self._redraw_pending = 0

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
        if (
            sys_display.get_mode() & sys_display.osd == 0
            or settings.onoff_show_fps.value
        ):
            return
        self._enabled = self._enabled_overlays()
        for i in range(len(self._enabled)):
            self._enabled[i].think(ins, delta_ms)

    def draw(self, ctx: Context) -> None:
        self.main.draw(ctx)
        display_mode = sys_display.get_mode()
        redraw = False
        if self._redraw_pending:
            redraw = True
            self._redraw_pending -= 1
        if display_mode != self._display_mode:
            self._redraw_pending = 2
        self._display_mode = display_mode
        if (display_mode & sys_display.osd) == 0:
            return
        if settings.onoff_show_fps.value:
            fps_string = "{0:.1f}".format(sys_display.fps())
            if redraw or fps_string != self._last_fps_string:
                octx = sys_display.ctx(sys_display.osd)
                self._last_fps_string = fps_string
                self._clip_rect.set(-120, -120, 120, -96)
                self._last_clip.add_region(self._clip_rect)
                if self._last_clip != self._clip_rect:
                    octx.save()
                    octx.compositing_mode = octx.CLEAR
                    self._last_clip.fill(octx)
                    octx.restore()
                octx.save()
                octx.rgba(0, 0, 0, 0.5)
                octx.compositing_mode = octx.COPY
                self._clip_rect.fill(octx)
                octx.restore()
                octx.gray(1)
                octx.font_size = 18
                octx.font = "Bold"
                octx.move_to(0, -103)
                octx.text_align = octx.CENTER
                octx.text(fps_string)
                sys_display.update(octx)
                self._clip_rect.set_clip()
                self._last_clip.copy(self._clip_rect)
            self._last_enabled = []
        else:
            self._clip_rect.clear()
            for i in range(len(self._enabled)):
                redraw = (
                    self._enabled[i].needs_redraw(self._clip_rect)
                    or redraw
                    or self._enabled[i] not in self._last_enabled
                )
            for i in range(len(self._last_enabled)):
                redraw = redraw or self._last_enabled[i] not in self._enabled
            self._last_enabled = self._enabled
            if self._clip_rect.is_empty():
                self._clip_rect.set_clip()
                return
            if self._last_clip != self._clip_rect:
                redraw = True
            if not redraw:
                return
            octx = sys_display.ctx(sys_display.osd)
            octx.save()
            octx.compositing_mode = octx.CLEAR
            self._last_clip.add_region(self._clip_rect)
            self._last_clip.fill(octx)
            octx.restore()
            for i in range(len(self._enabled)):
                self._enabled[i].draw(octx)
            sys_display.update(octx)
            self._clip_rect.set_clip()
            self._last_clip.copy(self._clip_rect)

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

    def needs_redraw(self, rect: Region) -> bool:
        rect.add(-120, 80, 120, 92)
        return True


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

    def needs_redraw(self, rect: Region) -> bool:
        rect.add(0, 0, 240, 240)
        return True


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
        self._showing -= delta_ms
        if self._showing < 0:
            self._showing = None

    def draw(self, ctx: Context) -> None:
        if self._showing is None:
            return

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

    def needs_redraw(self, rect: Region) -> bool:
        if self._showing:
            rect.add(-40, -40, 40, 40)
        return self._showing


class Icon(Responder):
    """
    A tray icon that might or might not be shown in the top icon tray.

    Should render into a 240x240 viewport. Will be scaled down by the IconTray
    that contains it.
    """

    WIDTH: int = 25
    _changed: bool = False

    @abstractmethod
    def visible(self) -> bool:
        ...

    def changed(self) -> bool:
        return self._changed


class USBIcon(Icon):
    """
    Found in the bargain bin at an Aldi Süd.

    Might or might not be related to a certain serial bus.
    """

    WIDTH: int = 30

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
    WIDTH: int = 30

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

        self._changed = False

    def think(self, ins: InputState, delta_ms: int) -> None:
        rssi = st3m.wifi.rssi()
        if rssi != self._rssi:
            self._rssi = rssi
            self._changed = True


class BatteryIcon(Icon):
    WIDTH: int = 30

    def __init__(self) -> None:
        super().__init__()
        self._percent = 100.0
        self._charging = False
        self._changed = False

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

        ctx.font = ctx.get_font_name(1)
        ctx.move_to(-72, 32)
        ctx.font_size = 100
        ctx.rgb(255, 255, 255).text(str(self._percent))

    def think(self, ins: InputState, delta_ms: int) -> None:
        self._percent = power.battery_percentage

class ChargingIcon(Icon):
    WIDTH: int = 20
    def __init__(self) -> None:
        super().__init__()
        self._charging = power.battery_charging

    def visible(self) -> bool:
        return self._charging

    def draw(self, ctx: Context) -> None:
            ctx.rgb(255, 255, 255)
            ctx.gray(1)
            ctx.line_width = 20
            ctx.move_to(10, -65)
            ctx.line_to(-30, 20)
            ctx.line_to(30, -20)
            ctx.line_to(-10, 65)
            ctx.line_to(-20, 35)
            ctx.stroke()
            ctx.move_to(-10, 65)
            ctx.line_to(40, 35)
            ctx.stroke()

        self._changed = False

    def think(self, ins: InputState, delta_ms: int) -> None:
        percent = approximate_battery_percentage(power.battery_voltage)
        charging = power.battery_charging
        if percent != self._percent:
            self._percent = percent
            self._changed = True
        if charging != self._charging:
            self._charging = charging
            self._changed = True

class IconTray(Overlay):
    """
    An overlay which renders Icons.
    """

    kind = OverlayKind.Indicators

    def __init__(self) -> None:
        self.icons = [
            ChargingIcon(),
            BatteryIcon(),
            USBIcon(),
            WifiIcon(),
            
        ]
        self.visible: List[Icon] = []
        self.last_visible: List[Icon] = []

    def think(self, ins: InputState, delta_ms: int) -> None:
        self.visible = [i for i in self.icons if i.visible()]
        for i in range(len(self.visible)):
            self.visible[i].think(ins, delta_ms)

    def draw(self, ctx: Context) -> None:
        self.last_visible = self.visible
        if not self.visible:
            return
        width = 0
        for i in range(len(self.visible)):
            width += self.visible[i].WIDTH
        x0 = width / -2 + self.visible[0].WIDTH / 2
        for i in range(len(self.visible)):
            ctx.save()
            ctx.translate(x0, -100)
            ctx.scale(0.13, 0.13)
            self.visible[i].draw(ctx)
            ctx.restore()
            x0 = x0 + self.visible[i].WIDTH

    def needs_redraw(self, rect: Region) -> bool:
        if self.visible:
            rect.add(-40, -120, 40, -88)
            for i in range(len(self.visible)):
                if (
                    self.visible[i].changed()
                    or self.visible[i] not in self.last_visible
                ):
                    return True
            for i in range(len(self.last_visible)):
                if self.last_visible[i] not in self.visible:
                    return True
        return False
