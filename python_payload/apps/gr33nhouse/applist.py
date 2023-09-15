from st3m.goose import Optional, Enum, Any
from st3m.input import InputController, InputState
from st3m.ui import colours
from st3m.ui.view import BaseView, ViewManager
from st3m.ui.interactions import ScrollController
from ctx import Context
from math import sin
import urequests
import time
from .background import Flow3rView
from .confirmation import ConfirmationView


class ViewState(Enum):
    INITIAL = 1
    LOADING = 2
    ERROR = 3
    LOADED = 4


class AppList(BaseView):
    initial_ticks: int = 0
    _scroll_pos: float = 0.0
    _state: ViewState = ViewState.INITIAL

    apps: list[Any] = []

    input: InputController
    background: Flow3rView

    def __init__(self) -> None:
        self.input = InputController()
        self.vm = None
        self.background = Flow3rView()
        self._sc = ScrollController()

    def on_enter(self, vm: Optional[ViewManager]) -> None:
        self.vm = vm
        self.initial_ticks = time.ticks_ms()

    def draw(self, ctx: Context) -> None:
        ctx.move_to(0, 0)

        if self._state == ViewState.INITIAL or self._state == ViewState.LOADING:
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
            ctx.text("Collecting seeds...")
            ctx.restore()
            return

        elif self._state == ViewState.ERROR:
            ctx.rgb(*colours.BLACK)
            ctx.rectangle(
                -120.0,
                -120.0,
                240.0,
                240.0,
            ).fill()

            ctx.save()
            ctx.rgb(*colours.WHITE)
            ctx.gray(1.0)
            ctx.font = "Camp Font 3"
            ctx.font_size = 24
            ctx.text_align = ctx.CENTER
            ctx.text_baseline = ctx.MIDDLE
            ctx.text("Something went wrong")
            ctx.restore()
            return

        elif self._state == ViewState.LOADED:
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

            ctx.move_to(0, 0)
            for idx, app in enumerate(self.apps):
                target = idx == self._sc.target_position()
                if target:
                    ctx.gray(0.0)
                else:
                    ctx.gray(1.0)

                if abs(self._sc.current_position() - idx) <= 5:
                    xpos = 0.0
                    if target and (width := ctx.text_width(app["name"])) > 220:
                        xpos = sin(self._scroll_pos) * (width - 220) / 2
                    ctx.move_to(xpos, offset)
                    ctx.text(app["name"])
                offset += 30

            ctx.restore()
        else:
            raise RuntimeError(f"Invalid view state {self._state}")

    def think(self, ins: InputState, delta_ms: int) -> None:
        self._sc.think(ins, delta_ms)
        if self.initial_ticks == 0 or time.ticks_ms() < self.initial_ticks + 300:
            return

        self.input.think(ins, delta_ms)

        if self._state == ViewState.INITIAL:
            try:
                self._state = ViewState.LOADING
                print("Loading app list...")
                res = urequests.get("https://flow3r.garden/api/apps.json")
                self.apps = res.json()["apps"]

                if self.apps == None:
                    print(f"Invalid JSON or no apps: {res.json()}")
                    self._state = ViewState.ERROR
                    return

                self._state = ViewState.LOADED
                self._sc.set_item_count(len(self.apps))
                print("App list loaded")
            except Exception as e:
                print(f"Load failed: {e}")
                self._state = ViewState.ERROR
            return
        elif self._state == ViewState.LOADING:
            raise RuntimeError(f"Invalid view state {self._state}")
        elif self._state == ViewState.ERROR:
            return

        self.background.think(ins, delta_ms)
        self._scroll_pos += delta_ms / 1000

        if self.input.buttons.app.left.pressed:
            self._sc.scroll_left()
            self._scroll_pos = 0.0
        elif self.input.buttons.app.right.pressed:
            self._sc.scroll_right()
            self._scroll_pos = 0.0
        elif self.input.buttons.app.middle.pressed:
            if self.vm is None:
                raise RuntimeError("vm is None")

            app = self.apps[self._sc.target_position()]
            url = app["rawtarDownloadUrl"]
            url = app["repoUrl"] + "/-/archive/main/" + app["repoUrl"].split("/")[-1] + "-main.tar"
            name = app["name"]
            author = app["author"]
            self.vm.push(
                ConfirmationView(
                    url=url,
                    name=name,
                    author=author,
                )
            )
