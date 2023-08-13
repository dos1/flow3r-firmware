from st3m.goose import Optional
from st3m.input import InputController, InputState
from st3m.ui.view import BaseView, ViewManager, ViewTransitionSwipeRight
from ctx import Context

from ..common import utils
from ..common import theme

from math import pi, sin, cos


class Action:
    icon: str
    label: str
    enabled: bool

    def __init__(self, icon: str, label: str, enabled: bool = True) -> None:
        self.icon = icon
        self.label = label
        self.enabled = enabled


class ActionView(BaseView):
    """
    A View with an action on each top petal.

    Each action is represented by a circular icon button and can either be
    enabled, disabled or not assigned.
    """

    actions: list[Optional[Action]] = [
        None,
        None,
        None,
        None,
        None,
    ]

    action_x: list[int] = [0] * 5
    action_y: list[int] = [0] * 5

    input: InputController

    def __init__(self) -> None:
        super().__init__()

        self.input = InputController()

        for i in range(0, 5):
            petal_angle = 2.0 * -pi / 5.0
            self.action_x[i] = int(cos(-petal_angle * float(i) - pi / 2.0) * 100.0)
            self.action_y[i] = int(sin(-petal_angle * float(i) - pi / 2.0) * 100.0)

    def on_enter(self, vm: ViewManager | None) -> None:
        super().on_enter(vm)

    def think(self, ins: InputState, delta_ms: int) -> None:
        self.input.think(ins, delta_ms)

    def draw(self, ctx: Context) -> None:
        for i in range(0, 5):
            action = self.actions[i]

            if action is None:
                continue

            if action.enabled:
                utils.draw_circle(
                    ctx, theme.PRIMARY, self.action_x[i], self.action_y[i], 18
                )
            else:
                utils.draw_circle(
                    ctx, theme.PRIMARY, self.action_x[i], self.action_y[i], 18
                )
                utils.draw_circle(
                    ctx, theme.BACKGROUND, self.action_x[i], self.action_y[i], 17
                )

        ctx.save()
        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.HANGING
        ctx.font_size = 30
        ctx.font = "Material Icons"
        ctx.rgb(*theme.PRIMARY_TEXT)

        for i in range(0, 5):
            action = self.actions[i]
            if action is None:
                continue

            if not action.enabled:
                continue

            ctx.move_to(self.action_x[i], self.action_y[i] - 2)
            ctx.text(action.icon)
        ctx.restore()
