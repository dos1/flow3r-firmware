from st3m.input import InputState
from st3m.ui import colours
from st3m.ui.view import BaseView, ViewManager
from ctx import Context
from .background import Flow3rView


class RecordView(BaseView):
    def __init__(self) -> None:
        super().__init__()
        self.background = Flow3rView()

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
