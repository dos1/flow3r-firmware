from st3m.input import InputController, InputState
from st3m.ui import colours
from st3m.ui.view import BaseView, ViewManager
from ctx import Context
from .background import Flow3rView
from .download import DownloadView


class ConfirmationView(BaseView):
    background: Flow3rView
    input: InputController

    url: str
    name: str
    author: str

    def __init__(self, url: str, name: str, author: str) -> None:
        self.input = InputController()
        self.vm = None
        self.background = Flow3rView()

        self.url = url
        self.name = name
        self.author = author

    def on_enter(self, vm: ViewManager | None) -> None:
        super().on_enter(vm)

        if self.vm is None:
            raise RuntimeError("vm is None")

    def draw(self, ctx: Context) -> None:
        ctx.move_to(0, 0)

        self.background.draw(ctx)

        ctx.save()
        ctx.rgb(*colours.WHITE)
        ctx.rectangle(
            -120.0,
            -80.0,
            240.0,
            160.0,
        ).fill()

        ctx.rgb(*colours.BLACK)
        ctx.font = "Camp Font 3"
        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE

        ctx.font_size = 16
        ctx.move_to(0, -60)
        ctx.text("Install")

        ctx.font_size = 24
        ctx.move_to(0, -30)
        ctx.text(self.name)

        ctx.font_size = 16
        ctx.move_to(0, 0)
        ctx.text("by")

        ctx.font_size = 24
        ctx.move_to(0, 30)
        ctx.text(self.author)

        ctx.font_size = 16
        ctx.move_to(0, 60)
        ctx.text("(OS shoulder to abort)")

        ctx.restore()

    def think(self, ins: InputState, delta_ms: int) -> None:
        self.input.think(ins, delta_ms)
        self.background.think(ins, delta_ms)

        if self.vm is None:
            raise RuntimeError("vm is None")

        if self.input.buttons.app.middle.pressed:
            self.vm.replace(
                DownloadView(
                    url=self.url,
                )
            )
