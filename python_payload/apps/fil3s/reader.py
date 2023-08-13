import os

from ctx import Context
from st3m.goose import Callable
from st3m.input import InputState
from st3m.ui.interactions import CapScrollController

from .common.action_view import Action
from .common.action_view import ActionView
from .common import utils
from .common import theme


class Reader(ActionView):
    path: str
    navigate: Callable[[str], None]
    update_path: Callable[[str], None]

    is_loading: bool = True
    has_error: bool = False
    content: str
    viewport_offset = (0.0, 0.0)
    zoom_enabled = False

    scroll_x: CapScrollController
    scroll_y: CapScrollController

    def __init__(
        self,
        path: str,
        navigate: Callable[[str], None],
        update_path: Callable[[str], None],
    ) -> None:
        super().__init__()

        self.scroll_x = CapScrollController()
        self.scroll_y = CapScrollController()

        self.path = path
        self.navigate = navigate
        self.update_path = update_path

        self.actions = [
            None,
            Action(icon="\ue8d5", label="Scroll Y"),
            Action(icon="\ue8b6", label="Zoom"),
            Action(icon="\ue5c4", label="Back"),
            Action(icon="\ue8d4", label="Scroll X"),
        ]

        # TODO: Buffered reading?
        if self._check_file():
            self._read_file()
            self.is_loading = False
        else:
            self.has_error = True
            self.is_loading = False

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        if self.is_loading:
            return

        if self.input.captouch.petals[6].whole.pressed:
            self._back()
        elif self.input.captouch.petals[4].whole.pressed:
            self.zoom_enabled = not self.zoom_enabled

        # TODO: Use "joystick-style" input for scrolling
        self.scroll_x.update(self.input.captouch.petals[8].gesture, delta_ms)
        self.scroll_y.update(self.input.captouch.petals[2].gesture, delta_ms)
        x = self.scroll_x.position[0] * 0.2
        y = self.scroll_y.position[0] * 0.2
        self.viewport_offset = (x - 80, y - 80)

    def draw(self, ctx: Context) -> None:
        utils.fill_screen(ctx, theme.BACKGROUND)

        if self.is_loading:
            self._draw_loading(ctx)
        elif self.has_error:
            self._draw_not_supported(ctx)
        else:
            self._draw_content(ctx)

    def _draw_loading(self, ctx: Context) -> None:
        ctx.save()
        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE
        ctx.font_size = 40
        ctx.font = "Camp Font 3"
        ctx.rgb(*theme.BACKGROUND_TEXT)
        ctx.move_to(0, 0)
        ctx.text("Loading...")
        ctx.restore()

    def _draw_not_supported(self, ctx: Context) -> None:
        ctx.save()
        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE
        ctx.rgb(*theme.BACKGROUND_TEXT)

        ctx.font_size = 40
        ctx.font = "Material Icons"
        ctx.move_to(0, -40)
        ctx.text("\ue99a")

        ctx.font_size = 24
        ctx.font = "Camp Font 3"
        ctx.move_to(0, -10)
        ctx.text("Can't read file")

        ctx.move_to(0, 20)
        ctx.text("(Not UTF-8?)")

        ctx.restore()

    def _draw_content(self, ctx: Context) -> None:
        ctx.save()
        ctx.text_align = ctx.LEFT
        ctx.rgb(*theme.BACKGROUND_TEXT)
        ctx.font = "Camp Font 3"

        if self.zoom_enabled:
            ctx.font_size = 32
        else:
            ctx.font_size = 16
        ctx.move_to(self.viewport_offset[0], self.viewport_offset[1])
        ctx.text(f"{self.content}")
        ctx.restore()

        super().draw(ctx)

    def _back(self) -> None:
        dir = os.path.dirname(self.path) + "/"
        self.update_path(dir)
        self.navigate("browser")

    def _read_file(self) -> None:
        try:
            with open(self.path, "r", encoding="utf-8") as f:
                self.content = f.read()
        except:
            self.has_error = True

    def _check_file(self) -> bool:
        # Dumb way to check if file is UTF-8 only
        try:
            with open(self.path, "r", encoding="utf-8") as f:
                f.read(100)
            return True
        except UnicodeError:
            return False
