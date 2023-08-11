from ctx import Context
from st3m import InputState, Responder
from st3m.ui.view import BaseView, ViewTransitionSwipeRight
from st3m.utils import tau
from st3m.goose import List, Optional
import math


class Screen(Responder):
    """
    A screen with some text. Just the text part, without background.
    """

    START_Y = -50
    FONT = "Camp Font 3"
    FONT_SIZE = 15

    def __init__(self, text: List[str]) -> None:
        self.text = text

    def draw(self, ctx: Context) -> None:
        ctx.font = self.FONT
        ctx.font_size = self.FONT_SIZE

        y = self.START_Y
        for line in self.text:
            ctx.move_to(0, y)
            ctx.text(line)
            y += self.FONT_SIZE

    def think(self, ins: InputState, delta_ms: int) -> None:
        pass


class HeroScreen(Screen):
    """
    A screen with large text in the middle.
    """

    START_Y = 0
    FONT = "Camp Font 2"
    FONT_SIZE = 30


class IntroScreen(Screen):
    """
    Title card which shows on About app startup.
    """

    START_Y = 25

    def __init__(self) -> None:
        super().__init__(["Chaos", "Communication", "Camp 2023"])

    def draw(self, ctx: Context) -> None:
        ctx.text_align = ctx.MIDDLE

        ctx.font = "Camp Font 2"
        ctx.font_size = 50

        ctx.move_to(0, -33)
        ctx.text("flow3r")
        ctx.move_to(0, -3)
        ctx.text("badge")

        super().draw(ctx)


class About(BaseView):
    """
    A pseudo-app (applet?) which displays an about screen for the badge.
    """

    def __init__(self) -> None:
        self.ts = 0.0
        self.screens: List[Screen] = [
            IntroScreen(),
            HeroScreen(
                [
                    "flow3r.garden",
                ]
            ),
            Screen(
                [
                    "This is Free",
                    "Software based on",
                    "code by numerous",
                    "third parties.",
                    "",
                    "Visit",
                    "flow3r.garden/license",
                    "for more info.",
                ]
            ),
        ]
        self.screen_ix = 0
        self.screen_ix_anim = 0.0
        super().__init__()

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)
        self.ts += delta_ms / 1000

        # Change target screen intent.
        if self.input.buttons.app.left.pressed and self._can_left():
            self.screen_ix -= 1
        if self.input.buttons.app.right.pressed and self._can_right():
            self.screen_ix += 1

        # Calculate animation/transitions.
        diff = self.screen_ix - self.screen_ix_anim
        if abs(diff) < 0.01:
            self.screen_ix_anim = self.screen_ix
        else:
            if diff > 0:
                diff = min(diff, 10 * (delta_ms / 1000))
            else:
                diff = max(diff, -10 * (delta_ms / 1000))
            self.screen_ix_anim += diff

    def _can_left(self, ix: Optional[int] = None) -> bool:
        """
        Returns true if the given screen index (or the current screen index) can
        be decreased.
        """
        if ix is None:
            ix = self.screen_ix
        return ix > 0

    def _can_right(self, ix: Optional[int] = None) -> bool:
        """
        Returns true if the given screen index (or the current screen index) can
        be increased.
        """
        if ix is None:
            ix = self.screen_ix
        return ix < len(self.screens) - 1

    def draw(self, ctx: Context) -> None:
        # Background
        ctx.rectangle(-120, -120, 240, 240)
        ctx.rgb(117 / 255, 255 / 255, 226 / 255)
        ctx.fill()

        ctx.arc(0, 350, 300, 0, tau, 0)
        ctx.rgb(61 / 255, 165 / 255, 30 / 255)
        ctx.fill()

        ctx.save()
        y = -40 + math.cos(self.ts * 2 + 10) * 10
        ctx.translate(0, y)
        for i in range(5):
            a = i * tau / 5 + self.ts
            size = 30 + math.cos(self.ts) * 5
            ctx.save()
            ctx.rotate(a)
            ctx.translate(30, 0)
            ctx.arc(0, 0, size, 0, tau, 0)
            ctx.gray(1)
            ctx.fill()
            ctx.restore()

        ctx.arc(0, 0, 20, 0, tau, 0)
        ctx.rgb(255 / 255, 247 / 255, 46 / 255)
        ctx.fill()
        ctx.restore()

        # Prepare list of screens to draw. We draw at most two screens if we're
        # in a transition, otherwies just one.
        #
        # List of (screen index, screen object, draw offset) tuples.
        draw = []

        diff = self.screen_ix - self.screen_ix_anim
        if diff > 0.01 or diff < -0.01:
            ix = math.floor(self.screen_ix_anim)
            draw = [
                (ix, self.screens[ix], self.screen_ix_anim - ix),
                (ix + 1, self.screens[ix + 1], (self.screen_ix_anim - ix) - 1),
            ]
        else:
            draw = [
                (self.screen_ix, self.screens[self.screen_ix], 0),
            ]

        # Draw currently visible screens.
        for ix, screen, offs in draw:
            ctx.save()
            ctx.translate(-240 * offs, 0)

            # Opaque circle
            ctx.start_group()
            ctx.global_alpha = 0.5
            ctx.rgb(0, 0, 0)
            ctx.arc(0, 0, 90, 0, tau, 0)
            ctx.fill()

            # Draw arrows.
            if self._can_left(ix):
                ctx.move_to(-105, 20)
                ctx.font = "Material Icons"
                ctx.font_size = 30
                ctx.text_align = ctx.MIDDLE
                ctx.text("\ue5c4")
            if self._can_right(ix):
                ctx.move_to(105, 20)
                ctx.font = "Material Icons"
                ctx.font_size = 30
                ctx.text_align = ctx.MIDDLE
                ctx.text("\ue5c8")
            ctx.end_group()

            # Text
            ctx.start_group()
            ctx.global_alpha = 1.0
            ctx.gray(1)

            ctx.text_align = ctx.MIDDLE

            screen.draw(ctx)

            ctx.end_group()
            ctx.restore()
