from st3m.input import InputController, InputState
from st3m.ui import colours
from st3m.ui.view import BaseView, ViewManager
from ctx import Context
from .background import Flow3rView


class RecordView(BaseView):
    input: InputController

    def __init__(self) -> None:
        self.input = InputController()
        self.vm = None
        self.background = Flow3rView()

    def on_enter(self, vm: ViewManager | None) -> None:
        super().on_enter(vm)

        if self.vm is None:
            raise RuntimeError("vm is None")

    def draw(self, ctx: Context) -> None:
        ctx.move_to(0, 0)
        ctx.save()
        ctx.rgb(*colours.BLACK)
        ctx.rectangle(
            -120.0,
            -120.0,
            240.0,
            240.0,
        ).fill()

        ctx.rgb(*colours.WHITE)
        ctx.font = "Camp Font 3"
        ctx.font_size = 24
        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE
        ctx.text("Coming soon")
        ctx.restore()

    def think(self, ins: InputState, delta_ms: int) -> None:
        self.input.think(ins, delta_ms)
