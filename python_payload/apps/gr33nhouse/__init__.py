from st3m.application import Application, ApplicationContext
from st3m.input import InputController, InputState
from st3m.ui import colours
from st3m.ui.view import ViewManager
from ctx import Context
from .applist import AppList
from .background import Flow3rView
from .record import RecordView
from .manual import ManualInputView


class Gr33nhouseApp(Application):
    items = ["Browse apps", "Record App Seed", "Enter App Seed"]
    selection = 0

    input: InputController
    background: Flow3rView

    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx=app_ctx)

        self.input = InputController()
        self.background = Flow3rView()

    def on_enter(self, vm: ViewManager | None) -> None:
        super().on_enter(vm)

        if self.vm is None:
            raise RuntimeError("vm is None")

    def draw(self, ctx: Context) -> None:
        self.background.draw(ctx)
        ctx.save()

        ctx.gray(1.0)
        ctx.rectangle(
            -120.0,
            -15.0,
            240.0,
            30.0,
        ).fill()

        ctx.translate(0, -30 * self.selection)

        offset = 0

        ctx.font = "Camp Font 3"
        ctx.font_size = 24
        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE

        for idx, item in enumerate(self.items):
            if idx == self.selection:
                ctx.gray(0.0)
            else:
                ctx.gray(1.0)

            ctx.move_to(0, offset)
            ctx.text(item)
            offset += 30

        ctx.restore()

    def think(self, ins: InputState, delta_ms: int) -> None:
        if self.vm is None:
            raise RuntimeError("vm is None")

        self.background.think(ins, delta_ms)
        self.input.think(ins, delta_ms)

        if self.input.buttons.app.left.pressed:
            if self.selection > 0:
                self.selection -= 1

        elif self.input.buttons.app.right.pressed:
            if self.selection < len(self.items) - 1:
                self.selection += 1

        elif self.input.buttons.app.middle.pressed:
            if self.selection == 0:
                self.vm.push(AppList())
            elif self.selection == 1:
                self.vm.push(RecordView())
            elif self.selection == 2:
                self.vm.push(ManualInputView())
