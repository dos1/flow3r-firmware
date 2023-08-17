from st3m.goose import Enum
from st3m.application import Application, ApplicationContext
from st3m.input import InputController, InputState
from st3m.ui.interactions import ScrollController
from st3m.ui import colours
from st3m.ui.view import ViewManager
from ctx import Context
import network
from .applist import AppList
from .background import Flow3rView
from .record import RecordView
from .manual import ManualInputView


class ViewState(Enum):
    CONTENT = 1
    NO_INTERNET = 2


class Gr33nhouseApp(Application):
    items = ["Browse apps", "Record flow3r seed", "Enter flow3r seed"]

    input: InputController
    background: Flow3rView
    state: ViewState

    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx=app_ctx)

        self.input = InputController()
        self.background = Flow3rView()
        self._sc = ScrollController()
        self._sc.set_item_count(3)

        self.state = ViewState.CONTENT

    def on_enter(self, vm: ViewManager | None) -> None:
        super().on_enter(vm)

        if self.vm is None:
            raise RuntimeError("vm is None")

    def draw(self, ctx: Context) -> None:
        if self.state == ViewState.NO_INTERNET:
            ctx.move_to(0, 0)
            ctx.rgb(*colours.BLACK)
            ctx.rectangle(
                -120.0,
                -120.0,
                240.0,
                240.0,
            ).fill()

            ctx.save()
            ctx.rgb(*colours.WHITE)
            ctx.font = "Camp Font 3"
            ctx.font_size = 24
            ctx.text_align = ctx.CENTER
            ctx.text_baseline = ctx.MIDDLE

            ctx.move_to(0, -15)
            ctx.text("No internet")

            ctx.move_to(0, 15)
            ctx.text("Check settings")

            ctx.restore()
            return

        self.background.draw(ctx)
        ctx.save()

        ctx.gray(1.0)
        ctx.rectangle(
            -120.0,
            -15.0,
            240.0,
            30.0,
        ).fill()

        ctx.translate(0, -30 * self._sc.current_position())

        offset = 0

        ctx.font = "Camp Font 3"
        ctx.font_size = 24
        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE

        for idx, item in enumerate(self.items):
            if idx == self._sc.target_position():
                ctx.gray(0.0)
            else:
                ctx.gray(1.0)

            ctx.move_to(0, offset)
            ctx.text(item)
            offset += 30

        ctx.restore()

    def think(self, ins: InputState, delta_ms: int) -> None:
        self.input.think(ins, delta_ms)
        self._sc.think(ins, delta_ms)

        if self.vm is None:
            raise RuntimeError("vm is None")

        if not network.WLAN(network.STA_IF).isconnected():
            self.state = ViewState.NO_INTERNET
            return
        else:
            self.state = ViewState.CONTENT

        self.background.think(ins, delta_ms)

        if self.input.buttons.app.left.pressed:
            self._sc.scroll_left()
        elif self.input.buttons.app.right.pressed:
            self._sc.scroll_right()
        elif self.input.buttons.app.middle.pressed:
            pos = self._sc.target_position()
            if pos == 0:
                self.vm.push(AppList())
            elif pos == 1:
                self.vm.push(RecordView())
            elif pos == 2:
                self.vm.push(ManualInputView())