import os
import media

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
    is_media: bool = False
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

        # TODO: Buffered reading?
        if self._is_media():
            self.is_loading = False
            self.is_media = True
            media.load(self.path)
            if (
                not media.has_video()
                and not media.has_audio()
                and not media.is_visual()
            ):
                self.has_error = True
        elif self._check_file():
            self._read_file()
            self.is_loading = False
        else:
            self.has_error = True
            self.is_loading = False

        self._update_actions()

    def _update_actions(self):
        controls = not self.has_error

        if self.is_media:
            self.actions = [
                None,
                Action(
                    icon="\ue034" if media.is_playing() else "\ue037",
                    label="Pause",
                    enabled=controls,
                ),
                Action(icon="\ue8b6", label="Zoom", enabled=False),
                Action(icon="\ue5c4", label="Back"),
                Action(icon="\ue042", label="Rewind", enabled=controls),
            ]
            return

        self.actions = [
            None,
            Action(icon="\ue8d5", label="Scroll Y", enabled=controls),
            Action(icon="\ue8b6", label="Zoom", enabled=controls),
            Action(icon="\ue5c4", label="Back"),
            Action(icon="\ue8d4", label="Scroll X", enabled=controls),
        ]

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        if self.is_loading:
            return

        if self.input.captouch.petals[6].whole.pressed:
            self._back()
        elif self.input.captouch.petals[4].whole.pressed:
            self.zoom_enabled = not self.zoom_enabled

        if self.is_media:
            if self.input.captouch.petals[2].whole.pressed:
                if media.is_playing():
                    media.pause()
                else:
                    media.play()
                self._update_actions()
            elif self.input.captouch.petals[8].whole.pressed:
                media.seek(0)

        # TODO: Use "joystick-style" input for scrolling
        if not self.is_media:
            self.scroll_x.update(self.input.captouch.petals[8].gesture, delta_ms)
            self.scroll_y.update(self.input.captouch.petals[2].gesture, delta_ms)
            x = self.scroll_x.position[0] * 0.2
            y = self.scroll_y.position[0] * 0.2
            self.viewport_offset = (x - 80, y - 80)

    def draw(self, ctx: Context) -> None:
        utils.fill_screen(ctx, theme.BACKGROUND)

        if self.is_loading:
            self._draw_loading(ctx)
            return
        elif self.has_error:
            self._draw_not_supported(ctx)
        elif self.is_media:
            self._draw_media(ctx)
        else:
            self._draw_content(ctx)

        super().draw(ctx)

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

        if not self.is_media:
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

        line_height = ctx.font_size

        for i, line in enumerate(self.content):
            x, y = self.viewport_offset[0], self.viewport_offset[1] + i * line_height
            if y > 120 + line_height or y < -120 - line_height:
                continue
            ctx.move_to(x, y)
            ctx.text(line[:10240])

        ctx.restore()

    def _draw_media(self, ctx: Context) -> None:
        if media.is_visual():
            media.draw(ctx)
        elif media.has_audio():
            ctx.gray(1.0)
            ctx.scope()

    def _back(self) -> None:
        if self.is_media:
            media.stop()
        dir = os.path.dirname(self.path) + "/"
        self.update_path(dir, os.path.basename(self.path))
        self.navigate("browser")

    def _read_file(self) -> None:
        try:
            with open(self.path, "r", encoding="utf-8") as f:
                self.content = f.readlines()
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

    def _is_media(self) -> bool:
        for ext in [".mp3", ".mod", ".mpg", ".gif"]:
            if self.path.lower().endswith(ext):
                return True
        return False

    def on_exit(self):
        if self.is_media:
            media.stop()
