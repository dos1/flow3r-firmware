# python imports
import random
import time
import math

# flow3r imports
from st3m import event, application, ui


# Subclass Application
class AppWorms(application.Application):
    def on_init(self):
        print("on init")

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

        self.add_event(
            event.Event(
                name="worms_control",
                action=self.handle_input,
                condition=lambda data: data.get("type", "") == "captouch"
                and data.get("value") == 1
                and data["change"],
            )
        )

        self.worms = []
        for i in range(0):
            self.worms.append(Worm())

        self.just_shown = True

    def on_foreground(self):
        print("on foreground")
        self.just_shown = True

    def on_draw(self, ctx):
        if self.bufn == 0 or self.bufn == 1:
            ctx.rgb(*ui.BLUE).rectangle(
                -ui.WIDTH / 2, -ui.HEIGHT / 2, ui.WIDTH, ui.HEIGHT
            ).fill()
            ctx.text_align = ctx.CENTER
            ctx.text_baseline = ctx.MIDDLE
            ctx.move_to(0, 0).rgb(*ui.WHITE).text("touch me :)")
            self.bufn += 1
            return

        for w in self.worms:
            w.draw(ctx)
        self.bufn += 1

    def main_foreground(self):
        now = time.ticks_ms()

        # Simulation is currently locked to FPS.
        if self.bufn > 3:
            for w in self.worms:
                w.move()
            self.bufn = 2

        self.last_render = now

    def handle_input(self, data):
        worms = self.worms
        worms.append(Worm(data.get("index", 0) * 2 * math.pi / 10 + math.pi))
        if len(worms) > 10:
            worms.pop(0)


class Worm:
    def __init__(self, direction=None):
        self.color = ui.randrgb()

        if direction:
            self.direction = direction
        else:
            self.direction = random.random() * math.pi * 2

        self.size = 50
        self.speed = self.size / 5
        (x, y) = ui.xy_from_polar(100, self.direction)
        self.x = x
        self.y = y
        # (self.dx,self.dy) = xy_from_polar(1,self.direction)
        self._lastdist = 0.0

    def draw(self, ctx):
        ctx.rgb(*self.color)
        ctx.round_rectangle(
            self.x - self.size / 2,
            self.y - self.size / 2,
            self.size,
            self.size,
            self.size // 2,
        ).fill()

    def mutate(self):
        self.color = [
            max(0, min(1, x + ((random.random() - 0.5) * 0.3))) for x in self.color
        ]

    def move(self):
        dist = math.sqrt(self.x**2 + self.y**2)
        target_size = (130 - dist) / 3

        if self.size > target_size:
            self.size -= 1

        if self.size < target_size:
            self.size += 1

        self.speed = self.size / 5

        self.direction += (random.random() - 0.5) * math.pi / 4

        (dx, dy) = ui.xy_from_polar(self.speed, self.direction)
        self.x += dx
        self.y += dy

        if dist > 120 - self.size / 2 and dist > self._lastdist:
            polar_position = math.atan2(self.y, self.x)
            dx = dx * -abs(math.cos(polar_position))
            dy = dy * -abs(math.sin(polar_position))
            self.direction = -math.atan2(dy, dx)
            self.mutate()
        self._lastdist = dist


app = AppWorms("worms")

# To run standalone:
# app.run()
# app.engine.eventloop()

# Known problems:
# ctx.rotate(math.pi) turns the display black until powercycled
# ctx.clip() causes crashes
