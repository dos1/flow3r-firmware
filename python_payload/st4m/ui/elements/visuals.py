from st4m.utils import xy_from_polar
from st4m.color import PUSH_RED, GO_GREEN, BLACK
from st4m import Responder, Ctx, InputState


import random
import math
from st4m.vector import tau


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
            # self.items_ring[0].draw(ctx)

        # ctx.save()
        for index, item in enumerate(self.items_ring):
            if item is None:
                continue
            angle = tau / len(self.items_ring) * index + self.angle_offset
            (x, y) = xy_from_polar(self.r, angle)
            ctx.save()
            # ctx.translate(self.x + x, self.y + y)
            ctx.rotate(-angle).translate(0, self.r).rotate(math.pi)
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

        self.petal_count = random.randint(3, 5)
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
        ctx.line_width = 5
        ctx.font = ctx.get_font_name(6)
        if self.rotation_time:
            phi_rotate = tau * ((self.ts % self.rotation_time) / self.rotation_time)
        else:
            phi_rotate = 0
        for i in range(self.petal_count):
            # continue
            # ctx.save()

            phi = (tau / self.petal_count * i + self.phi_offset + phi_rotate) % tau
            r = self.size / 2
            (x_, y_) = xy_from_polar(r, phi)

            size_offset = abs(math.pi - (phi + math.pi) % tau) * 5
            ctx.move_to(x + x_, y + y_)
            if self.highlighted:
                # ctx.move_to(x + x_ - petal_size / 2 - size_offset - 5, y + y_)
                ctx.arc(x + x_, y + y_, petal_size / 2 + size_offset + 1, 0, tau, 0)
                ctx.rgb(*GO_GREEN).stroke()

            ctx.arc(x + x_, y + y_, petal_size / 2 + size_offset, 0, tau, 0)
            ctx.rgb(*self.petal_color).fill()
            # ctx.restore()
        ctx.move_to(x, y)
        if self.highlighted:
            ctx.arc(x, y, self.size / 2, 0, tau, 1)
            ctx.rgb(*GO_GREEN).stroke()

        ctx.arc(x, y, self.size / 2, 0, tau, 0)
        ctx.rgb(*self.bg).fill()

        # label
        # y += self.size / 3

        w = max(self.size, ctx.text_width(self.label) + 10)
        h = self.size / 3 + 8
        # ctx.save()
        ctx.translate(0, self.size / 3)

        if self.highlighted:
            bg = BLACK
            fg = GO_GREEN
        else:
            bg = PUSH_RED
            fg = BLACK

        ctx.rgb(*bg).round_rectangle(x - w / 2, y - h / 2, w, h, w // 2).fill()
        ctx.rgb(*fg).move_to(x, y).text(self.label)

        ctx.restore()
