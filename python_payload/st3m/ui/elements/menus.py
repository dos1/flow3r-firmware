from st3m.goose import Optional, List, ABCBase, abstractmethod
from st3m.ui.view import ViewManager
from st3m.ui.elements.visuals import Sun, GroupRing, FlowerIcon
from st3m.ui.menu import MenuController, MenuItem

from st3m import InputState

from st3m.utils import lerp, tau
import math
from ctx import Context


class SimpleMenu(MenuController):
    """
    A simple line-by-line menu.
    """

    SIZE_LARGE = 30
    SIZE_SMALL = 20

    def draw(self, ctx: Context) -> None:
        ctx.gray(0)
        ctx.rectangle(-120, -120, 240, 240).fill()

        current = self._scroll_controller.current_position()

        ctx.gray(1)
        for ix, item in enumerate(self._items):
            offs = (ix - current) * self.SIZE_LARGE
            ctx.save()
            ctx.move_to(0, 0)
            ctx.font_size = self.SIZE_LARGE
            ctx.text_align = ctx.CENTER
            ctx.text_baseline = ctx.MIDDLE
            ctx.translate(0, offs)
            scale = lerp(
                1, self.SIZE_SMALL / self.SIZE_LARGE, abs(offs / self.SIZE_SMALL)
            )
            ctx.scale(scale, scale)
            item.draw(ctx)
            ctx.restore()


class SunMenu(MenuController):
    """
    A circular menu with a rotating sun.
    """

    __slots__ = (
        "_ts",
        "_sun",
    )

    def __init__(self, items: List[MenuItem]) -> None:
        self._ts = 0
        self._sun = Sun()
        super().__init__(items)

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)
        self._sun.think(ins, delta_ms)
        self._ts += delta_ms

    def _draw_item_angled(
        self, ctx: Context, item: MenuItem, angle: float, activity: float
    ) -> None:
        size = lerp(20, 40, activity)
        color = lerp(0, 1, activity)
        if color < 0.01:
            return

        ctx.save()
        ctx.translate(-120, 0).rotate(angle).translate(140, 0)
        ctx.font_size = size
        ctx.rgba(1.0, 1.0, 1.0, color).move_to(0, 0)
        item.draw(ctx)
        ctx.restore()

    def draw(self, ctx: Context) -> None:
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
            self._draw_item_angled(ctx, item, rot, 1 - abs(rot))


class FlowerMenu(MenuController):
    """
    A circular menu with flowers.
    """

    __slots__ = (
        "_ts",
        "_sun",
    )

    def __init__(self, items: List[MenuItem], name: str = "flow3r") -> None:
        self._ts = 0
        self.name = name
        self.ui = GroupRing(r=80)
        for item in items:
            self.ui.items_ring.append(FlowerIcon(label=item.label()))
        super().__init__(items)

        self.icon = FlowerIcon(label=self.name)
        self.icon.rotation_time = -5000
        self.ui.item_center = self.icon

        self.angle = 0
        self.angle_step = 0.2

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)
        self.ui.think(ins, delta_ms)
        self._ts += delta_ms

    def draw(self, ctx: Context) -> None:
        ctx.gray(0)
        ctx.rectangle(-120, -120, 240, 240).fill()
        # for item in self.ui.items_ring:
        #    item.highlighted = False
        #    item.rotation_time = 10000
        current = self._scroll_controller.current_position()
        current_int = round(current) % len(self._items)
        # print("current", current, current_int)
        # self.ui.items_ring[current_int].highlighted = True
        # self.ui.items_ring[current_int].rotation_time = 3000
        self.ui.angle_offset = math.pi - (tau * current / len(self.ui.items_ring))

        self.ui.draw(ctx)
