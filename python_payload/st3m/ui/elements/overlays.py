"""
Composition/overlay system.

This is different from a menu system. Instead of navigation, it should be used
for persistent, anchored symbols like charging symbols, toasts, debug overlays,
etc.
"""

from st3m import Responder, Ctx, InputState, Reactor
from st3m.goose import Dict, Enum, List, ABCBase, abstractmethod


class OverlayKind(Enum):
    # Battery, USB, ...
    Indicators = 0
    # Naughty debug developers for naughty developers and debuggers.
    Debug = 1


_all_kinds = [
    OverlayKind.Indicators,
    OverlayKind.Debug,
]


class Overlay(Responder):
    """
    An Overlay is a Responder with some kind of OverlayKind attached.g
    """

    kind: OverlayKind


class Compositor(Responder):
    """
    A Compositor blends together some main Responder (usually a ViewManager)
    alongside with active Overlays. Overlays can be enabled/disabled by kind.
    """

    def __init__(self, main: Responder):
        self.main = main
        self.overlays: Dict[OverlayKind, List[Responder]] = {}
        self.enabled: Dict[OverlayKind, bool] = {
            OverlayKind.Indicators: True,
            OverlayKind.Debug: True,
        }

    def _enabled_overlays(self) -> List[Responder]:
        res: List[Responder] = []
        for kind in _all_kinds:
            if not self.enabled.get(kind, False):
                continue
            for overlay in self.overlays.get(kind, []):
                res.append(overlay)
        return res

    def think(self, ins: InputState, delta_ms: int) -> None:
        self.main.think(ins, delta_ms)
        for overlay in self._enabled_overlays():
            overlay.think(ins, delta_ms)

    def draw(self, ctx: Ctx) -> None:
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

    def draw(self, ctx: Ctx) -> None:
        ctx.save()
        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE
        ctx.font = ctx.get_font_name(0)
        ctx.font_size = 10
        ctx.gray(0).rectangle(-120, 80, 240, 12).fill()
        ctx.gray(1).move_to(0, 86).text(self.text())
        ctx.restore()
