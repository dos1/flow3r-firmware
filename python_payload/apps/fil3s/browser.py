import os
import uos
import stat
from st3m.goose import Callable, Generator
from st3m.input import InputState
from ctx import Context

from .common.action_view import Action
from .common.action_view import ActionView
from .common import utils
from .common import theme


class Browser(ActionView):
    path: str
    navigate: Callable[[str], None]
    update_path: Callable[[str], None]

    dir_entries: list[tuple[str, str]] = []
    up_enabled: bool = False
    prev_enabled: bool = False
    next_enabled: bool = False
    current_pos = 0
    current_entry: tuple[str, str]

    def __init__(
        self,
        path: str,
        navigate: Callable[[str], None],
        update_path: Callable[[str], None],
    ) -> None:
        super().__init__()

        self.path = path
        self.navigate = navigate
        self.update_path = update_path

        self._update_actions()
        self._scan_path()

    def _on_action(self, index: int) -> None:
        if index == 4:
            if self.current_pos > 0:
                self.current_pos -= 1
                self._update_position()
        if index == 3:
            self._up()
        if index == 2:
            self._select()
        elif index == 1:
            if self.current_pos < len(self.dir_entries) - 1:
                self.current_pos += 1
                self._update_position()

    def draw(self, ctx: Context) -> None:
        utils.fill_screen(ctx, theme.BACKGROUND)
        super().draw(ctx)

        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE
        ctx.font_size = 40
        ctx.font = "Material Icons"
        ctx.rgb(*theme.BACKGROUND_TEXT)
        ctx.move_to(0, -5)
        ctx.text(self.current_entry[1])

        ctx.font_size = 24
        ctx.move_to(0, 20)
        ctx.font = "Camp Font 3"
        ctx.text(self.current_entry[0])

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        for i in range(0, 5):
            if self.input.captouch.petals[i * 2].whole.pressed:
                self._on_action(i)
                return

    def _is_dir(self, path: str) -> bool:
        st_mode = uos.stat(path)[0]  # Can fail with OSError
        return stat.S_ISDIR(st_mode)

    def _get_dir_entry(
        self, names: list[str]
    ) -> Generator[tuple[str, str], None, None]:
        for name in names:
            try:
                if self.path + name == "/flash/sys/st3m":
                    yield (name, "\ue545")
                elif self._is_dir(self.path + name):
                    yield (name, "\ue2c7")
                else:
                    yield (name, "\ue873")
            except Exception as e:
                # This exception is why we're using a generator here
                print(f"Failed to create entry for {name}: {e}")

    def _scan_path(self) -> None:
        self.current_pos = 0

        self.dir_entries = list(self._get_dir_entry(os.listdir(self.path)))

        self._update_position()

    def _change_path(self, path: str) -> None:
        self.path = path
        self._scan_path()
        self.up_enabled = self.path != "/"

        up_action = self.actions[2]
        if up_action is not None:
            up_action.enabled = self.up_enabled

        self.update_path(self.path)

    def _select(self) -> None:
        name = self.dir_entries[self.current_pos][0]

        old_path = self.path
        new_path = self.path + name
        try:
            if self._is_dir(new_path):
                self._change_path(new_path + "/")
            else:
                self.update_path(self.path + name)
                self.navigate("reader")
        except Exception as e:
            # TODO: Create error view
            print(f"Failed to open {new_path}: {e}")
            self._change_path(old_path)

    def _up(self) -> None:
        if not self.up_enabled or len(self.path) <= 1:
            return

        segments = self.path[1:-1].split("/")
        if len(segments) == 1:
            self._change_path("/")
        else:
            segments.pop()
            self._change_path("/" + "/".join(segments) + "/")

    def _update_actions(self) -> None:
        self.actions = [
            Action(icon="\ue3e3", label="Menu", enabled=False),
            Action(icon="\ue409", label="Next", enabled=self.next_enabled),
            Action(icon="\ue876", label="Select"),
            Action(icon="\ue5c4", label="Back", enabled=self.up_enabled),
            Action(icon="\ue5cb", label="Prev", enabled=self.prev_enabled),
        ]

    def _update_position(self) -> None:
        try:
            self.current_entry = self.dir_entries[self.current_pos]
        except:
            self.current_entry = ("\ue002", "No files")

        self.prev_enabled = self.current_pos > 0
        self.next_enabled = self.current_pos < len(self.dir_entries) - 1
        self._update_actions()
