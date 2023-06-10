from st3m import logging

log = logging.Log(__name__, level=logging.INFO)
log.info("import")

from st3m.system import ctx
import random
import math
import time
from math import sin, cos, pi

import gc

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

the_ctx = ctx


# Utility functions
def xy_from_polar(r, phi):
    return (r * math.sin(phi), r * math.cos(phi))  # x  # y


# def ctx_circle(self, x,y, radius, arc_from = -math.pi, arc_to = math.pi):
#    return self.arc(x,y,radius,arc_from,arc_to,True)

# the_ctx.circle = ctx_circle


def randrgb():
    return (random.random(), random.random(), random.random())


class UIElement:
    def __init__(self, origin=(0, 0)):
        self.children = []
        self.origin = origin
        self.ctx = the_ctx

    def draw(self, offset=(0, 0)):
        pos = (self.origin[0] + offset[0], self.origin[1] + offset[1])

        self._draw(pos)
        for child in self.children:
            child.draw(pos)

    def _draw(self, pos):
        pass

    def add(self, child):
        self.children.append(child)


class Viewport(UIElement):
    def _draw(self, pos):
        pass
        # self.ctx.rgb(0.3,0.3,0.3).rectangle(-WIDTH/2,-HEIGHT/2,WIDTH,HEIGHT).fill()


class Circle(UIElement):
    def __init__(
        self, radius, color=PUSH_RED, arc_from=-math.pi, arc_to=math.pi, *args, **kwargs
    ):
        self.radius = radius
        self.color = color
        self.arc_from = arc_from
        self.arc_to = arc_to
        super().__init__()

    def _draw(self, pos):
        (x, y) = pos
        self.ctx.move_to(x, y).rgb(*self.color).arc(
            x, y, self.radius, self.arc_from, self.arc_to, True
        ).fill()


class Text(UIElement):
    def __init__(self, s="foo"):
        self.s = s
        super().__init__()

    def _draw(self, pos):
        self.ctx.text_align = self.ctx.CENTER
        self.ctx.text_baseline = self.ctx.MIDDLE
        self.ctx.font_size = 30
        self.ctx.rgb(1, 1, 1).move_to(pos[0], pos[1]).text(self.s)


class Icon(UIElement):
    def __init__(self, label="?", size=60):
        self.bg = (random.random(), random.random(), random.random())
        self.fg = 0
        self.label = label
        self.size = size
        self.has_highlight = False
        super().__init__()

    def _draw(self, pos):
        self.ctx.text_align = self.ctx.CENTER
        self.ctx.text_baseline = self.ctx.MIDDLE
        self.ctx.font_size = self.size / 3

        (x, y) = pos
        hs = 5

        if self.has_highlight:
            self.ctx.rgb(*GO_GREEN).arc(
                x, y, self.size / 2 + hs, -math.pi, math.pi, True
            ).fill()
        self.ctx.move_to(x, y).rgb(*self.bg).arc(
            x, y, self.size / 2, -math.pi, math.pi, True
        ).fill()

        y += self.size / 3
        width = max(self.size - 10, self.ctx.text_width(self.label)) + 10
        height = self.size / 3 + 8
        if self.has_highlight:
            self.ctx.rgb(*BLACK).move_to(x, y - height / 2).round_rectangle(
                x - width / 2, y - height / 2, width, height, width // 2
            ).fill()
            self.ctx.rgb(*GO_GREEN).move_to(x, y).text(self.label)
        else:
            self.ctx.rgb(*PUSH_RED).move_to(x, y - height / 2).round_rectangle(
                x - width / 2, y - height / 2, width, height, width // 2
            ).fill()
            self.ctx.rgb(*BLACK).move_to(x, y).text(self.label)


class IconLabel(Icon):
    def _draw(self, pos):
        self.ctx.text_align = self.ctx.CENTER
        self.ctx.text_baseline = self.ctx.MIDDLE
        self.ctx.font_size = self.size / 2

        (x, y) = pos
        hs = 5
        width = self.ctx.text_width(self.label) + 10
        height = self.size / 2
        if self.has_highlight:
            self.ctx.rgb(*GO_GREEN).move_to(x, y - height / 2).round_rectangle(
                x - width / 2, y - height / 2, width, height, width // 2
            ).fill()
            self.ctx.rgb(*BLACK).move_to(x, y).text(self.label)
        else:
            self.ctx.rgb(*PUSH_RED).move_to(x, y - height / 2).round_rectangle(
                x - width / 2, y - height / 2, width, height, width // 2
            ).fill()
            self.ctx.rgb(*BLACK).move_to(x, y).text(self.label)


class IconFlower(Icon):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.petal_count = random.randint(3, 5)
        # self.petal_count = 0
        self.petal_color = (random.random(), random.random(), random.random())
        self.phi_offset = random.random()
        self.size_offset = random.randint(0, 20)
        # self.bg=PUSH_RED

    def _draw(self, pos):
        self.ctx.text_align = self.ctx.CENTER
        self.ctx.text_baseline = self.ctx.MIDDLE
        self.ctx.font_size = self.size / 3

        (x, y) = pos
        petal_size = 0
        if self.petal_count:
            petal_size = 2.3 * self.size / self.petal_count + self.size_offset

        hs = 5

        for i in range(self.petal_count):
            phi = math.pi * 2 / self.petal_count * i + self.phi_offset
            r = self.size / 2
            (x_, y_) = xy_from_polar(r, phi)
            size_rnd = random.randint(-1, 1)
            if self.has_highlight:
                self.ctx.move_to(x + x_, y + y_).rgb(*GO_GREEN).arc(
                    x + x_,
                    y + y_,
                    petal_size / 2 + hs + size_rnd,
                    -math.pi,
                    math.pi,
                    True,
                ).fill()
            self.ctx.move_to(x + x_, y + y_).rgb(*self.petal_color).arc(
                x + x_, y + y_, petal_size / 2 + size_rnd, -math.pi, math.pi, True
            ).fill()

        if self.has_highlight:
            self.ctx.rgb(*GO_GREEN).arc(
                x, y, self.size / 2 + hs, -math.pi, math.pi, True
            ).fill()
        self.ctx.move_to(x, y).rgb(*self.bg).arc(
            x, y, self.size / 2, -math.pi, math.pi, True
        ).fill()

        y += self.size / 3
        width = max(self.size, self.ctx.text_width(self.label) + 10)
        height = self.size / 3 + 8
        if self.has_highlight:
            self.ctx.rgb(*BLACK).move_to(x, y - height / 2).round_rectangle(
                x - width / 2, y - height / 2, width, height, width // 2
            ).fill()
            self.ctx.rgb(*GO_GREEN).move_to(x, y).text(self.label)
        else:
            self.ctx.rgb(*PUSH_RED).move_to(x, y - height / 2).round_rectangle(
                x - width / 2, y - height / 2, width, height, width // 2
            ).fill()
            self.ctx.rgb(*BLACK).move_to(x, y).text(self.label)


class IconValue(Icon):
    def __init__(self, value=0, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.value = value
        self.get_value = None

    def _draw(self, pos):
        (x, y) = pos

        v = self.value
        if self.get_value:
            v = self.get_value()
            self.value = v

        self.ctx.text_align = self.ctx.CENTER
        self.ctx.text_baseline = self.ctx.MIDDLE
        self.ctx.font_size = self.size / 3

        if self.has_highlight:
            self.ctx.move_to(x, y).rgb(*WHITE).arc(
                x, y, self.size / 2 + 5, -pi, pi, True
            ).fill()

        self.ctx.move_to(x, y).rgb(*PUSH_RED).arc(
            x, y, self.size / 2, -pi, pi, True
        ).fill()
        self.ctx.move_to(x, y).rgb(*GO_GREEN).arc(
            x, y, self.size / 2 - 5, v * 2 * pi, 0, 1
        ).fill()
        self.ctx.rgb(0, 0, 0).move_to(x, y).text(self.label)


class GroupStackedVertical(UIElement):
    pass


class GroupRing(UIElement):
    def __init__(self, r=100, origin=(0, 0), element_center=None):
        self.r = r
        self.angle_offset = 0
        self.element_center = element_center
        super().__init__(origin)

    def draw(self, offset=(0, 0)):
        pos = (self.origin[0] + offset[0], self.origin[1] + offset[1])
        self._draw(pos)
        for index in range(len(self.children)):
            # print("child",index)
            child = self.children[index]
            if not child:
                continue
            angle = 2 * math.pi / len(self.children) * index + self.angle_offset
            # print(angle,self.r,pos[0])
            x = -math.sin(angle) * self.r + pos[0]
            y = -math.cos(angle) * self.r + pos[1]
            # print("pos",(x,y))
            child.draw(offset=(x, y))

    def _draw(self, pos):
        if self.element_center:
            self.element_center.has_highlight = False
            self.element_center._draw(pos)


class GroupPetals(GroupRing):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.children = [None for i in range(10)]
