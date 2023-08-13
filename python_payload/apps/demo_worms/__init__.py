# python imports
import random
import time
import math

# flow3r imports
from st3m import InputState
from st3m.application import Application, ApplicationContext
from st3m.ui.colours import BLUE, WHITE
from st3m.goose import Optional
from st3m.utils import xy_from_polar
from st3m.ui.view import ViewManager
from ctx import Context


tau = 2 * math.pi


# Subclass Application
class AppWorms(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)

        # HACK: we work against double buffering by keeping note of how many
        # times on_draw got called.
        #
        # We know there's two buffers, so if we render the same state twice in a
        # row we'll be effectively able to keep a persistent framebuffer, like
        # with the old API.
        #
        # When bufn is in [0, 1] we render the background image.
        # When bufn is in [2, ...) we render the worms.
        # When bufn is > 3, we enable updating the worm state.
        #
        # TODO(q3k): allow apps to request single-buffered graphics for
        # persistent framebuffers.
        self.bufn = 0

        self.worms = []
        for i in range(0):
            self.worms.append(Worm())

        self.just_shown = True

    def on_enter(self, vm: Optional[ViewManager]) -> None:
        # print("on foreground")
        super().on_enter(vm)
        self.just_shown = True

    def draw(self, ctx: Context) -> None:
        if self.bufn <= 5:
            ctx.rgb(*BLUE).rectangle(-120, -120, 240, 240).fill()

            ctx.text_align = ctx.CENTER
            ctx.text_baseline = ctx.MIDDLE
            ctx.move_to(0, 0).rgb(*WHITE).text("touch me :)")
            self.bufn += 1

            return

        for w in self.worms:
            w.draw(ctx)
        self.bufn += 1

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        # Simulation is currently locked to FPS.
        if self.bufn > 7:
            for w in self.worms:
                w.move()
            self.bufn = 6
        for index, petal in enumerate(self.input.captouch.petals):
            if petal.whole.pressed or petal.whole.repeated:
                self.worms.append(Worm(-tau * index / 10 + math.pi))
        while len(self.worms) > 10:
            self.worms.pop(0)


def randrgb() -> tuple[float, float, float]:
    return (random.random(), random.random(), random.random())


class Worm:
    def __init__(self, direction: Optional[float] = None) -> None:
        self.color = randrgb()

        if direction is not None:
            self.direction = direction
        else:
            self.direction = random.random() * math.pi * 2

        self.size = 50
        self.speed = self.size / 5
        (x, y) = xy_from_polar(100, self.direction)
        self.x = x
        self.y = y
        # (self.dx,self.dy) = xy_from_polar(1,self.direction)
        self._lastdist = 0.0

    def draw(self, ctx: Context) -> None:
        ctx.rgb(*self.color)
        ctx.round_rectangle(
            self.x - self.size / 2,
            self.y - self.size / 2,
            self.size,
            self.size,
            self.size // 2,
        ).fill()

    def mutate(self) -> None:
        self.color = (
            max(0, min(1, self.color[0] + ((random.random() - 0.5) * 0.3))),
            max(0, min(1, self.color[1] + ((random.random() - 0.5) * 0.3))),
            max(0, min(1, self.color[2] + ((random.random() - 0.5) * 0.3))),
        )

    def move(self) -> None:
        dist = math.sqrt(self.x**2 + self.y**2)
        target_size = (130 - dist) / 3

        if self.size > target_size:
            self.size -= 1

        if self.size < target_size:
            self.size += 1

        self.speed = self.size / 5

        self.direction += (random.random() - 0.5) * math.pi / 4

        (dx, dy) = xy_from_polar(self.speed, self.direction)
        self.x += dx
        self.y += dy

        if dist > 120 - self.size / 2 and dist > self._lastdist:
            polar_position = math.atan2(self.y, self.x)
            dx = dx * -abs(math.cos(polar_position))
            dy = dy * -abs(math.sin(polar_position))
            self.direction = -math.atan2(dy, dx)
            self.mutate()
        self._lastdist = dist
