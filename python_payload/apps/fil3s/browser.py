import os
import uos
import stat
import math
from st3m.goose import Callable, Generator, Optional
from st3m.input import InputState
from st3m.utils import reload_app_list
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
    delete_enabled: bool = True
    select_enabled: bool = True
    current_pos = 0
    current_entry: tuple[str, str]
    app_list_outdated: bool = False

    def __init__(
        self,
        path: str,
        navigate: Callable[[str], None],
        update_path: Callable[[str], None],
        selected: Optional[str] = None,
    ) -> None:
        super().__init__()

        self._delete_held_for = 0.0
        self._delete_hold_time = 1.5
        self._delete_require_release = False

        self._scroll_pos = 0.0

        self.path = path
        self.selected = selected
        self.navigate = navigate
        self.update_path = update_path

        self._update_actions()
        self._scan_path()

    def _on_action(self, index: int) -> None:
        if index == 4:
            if self.current_pos > 0:
                self.current_pos -= 1
                self._update_position()
        elif index == 3:
            self._up()
        elif index == 2:
            self._select()
        elif index == 1:
            if self.current_pos < len(self.dir_entries) - 1:
                self.current_pos += 1
                self._update_position()
        elif index == 0:
            self._delete()

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
        ctx.font = "Camp Font 3"
        xpos = 0.0
        if (width := ctx.text_width(self.current_entry[0])) > 220:
            xpos = math.sin(self._scroll_pos) * (width - 220) / 2
        ctx.move_to(xpos, 20)
        ctx.text(self.current_entry[0])

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        self._scroll_pos += delta_ms / 1000

        # Handle delete petal being held down
        if ins.captouch.petals[0].pressed:
            if not self._delete_require_release:
                self._delete_held_for += delta_ms / 1000
                if self._delete_held_for > self._delete_hold_time:
                    self._delete_held_for = self._delete_hold_time
                    self._delete_require_release = True
                    self._on_action(0)
        else:
            self._delete_held_for = 0.0
            self._delete_require_release = False
        self.actions[0].progress = self._delete_held_for / self._delete_hold_time

        for i in range(1, 5):
            if self.input.captouch.petals[i * 2].whole.pressed:
                self._on_action(i)
                return

    def _get_dir_entry(
        self, names: list[str]
    ) -> Generator[tuple[str, str], None, None]:
        for name in names:
            try:
                if self.path + name == "/flash/sys/st3m":
                    yield (name, "\ue545")
                elif utils.is_dir(self.path + name):
                    yield (name, "\ue2c7")
                else:
                    yield (name, "\ue873")
            except Exception as e:
                # This exception is why we're using a generator here
                print(f"Failed to create entry for {name}: {e}")

    def _scan_path(self) -> None:
        dir = os.listdir(self.path)

        self.current_pos = 0
        if self.selected and self.selected in dir:
            self.current_pos = dir.index(self.selected)

        self.dir_entries = list(self._get_dir_entry(dir))

        self._update_position()

    def _change_path(self, path: str, selected: Optional[str] = None) -> None:
        self.path = path
        self.selected = selected
        self._scan_path()
        self.up_enabled = self.path != "/"

        up_action = self.actions[2]
        if up_action is not None:
            up_action.enabled = self.up_enabled

        self.update_path(self.path, selected)
        self._update_actions()

    def _select(self) -> None:
        if not self.select_enabled:
            return

        name = self.dir_entries[self.current_pos][0]

        old_path = self.path
        new_path = self.path + name
        try:
            if utils.is_dir(new_path):
                self._change_path(new_path + "/")
            else:
                self.update_path(self.path + name)
                self.navigate("reader")
        except Exception as e:
            # TODO: Create error view
            print(f"Failed to open {new_path}: {e}")
            self._change_path(old_path)

    def on_exit(self) -> None:
        if self.app_list_outdated:
            reload_app_list(self.vm)
        super().on_exit()

    def _delete(self) -> None:
        if not self.delete_enabled:
            return

        name = self.dir_entries[self.current_pos][0]
        path = self.path + name

        if self.path in [
            "/flash/sys/apps/",
            "/flash/apps/",
            "/sd/apps/",
        ]:
            self.app_list_outdated = True

        try:
            if utils.is_dir(path):
                utils.rmdirs(path)
            else:
                os.remove(path)
                print(f"deleted file: {path}")

            # refresh dir listing
            self.current_pos = max(self.current_pos - 1, 0)
            self._scan_path()
        except Exception as e:
            # TODO: Create error view
            print(f"Failed to delete {path}: {e}")

    def _up(self) -> None:
        if not self.up_enabled or len(self.path) <= 1:
            return

        segments = self.path[1:-1].split("/")
        if len(segments) == 1:
            self._change_path("/", segments[-1])
        else:
            selected = segments.pop()
            self._change_path("/" + "/".join(segments) + "/", selected)

    def _update_actions(self) -> None:
        self.actions = [
            Action(icon="\ue92b", label="Delete", enabled=self.delete_enabled),
            Action(icon="\ue409", label="Next", enabled=self.next_enabled),
            Action(icon="\ue876", label="Select", enabled=self.select_enabled),
            Action(icon="\ue5c4", label="Back", enabled=self.up_enabled),
            Action(icon="\ue5cb", label="Prev", enabled=self.prev_enabled),
        ]

    def _update_position(self) -> None:
        try:
            self.current_entry = self.dir_entries[self.current_pos]
            self.select_enabled = True
        except Exception:
            self.current_entry = ("\ue002", "No files")
            self.select_enabled = False

        self.up_enabled = self.path != "/"
        self.prev_enabled = self.current_pos > 0
        self.next_enabled = self.current_pos < len(self.dir_entries) - 1
        # disallow deleting st3m folder
        name = self.current_entry[0]
        self.delete_enabled = (
            self.path + name
            not in [
                "/flash",
                "/flash/sys",
                "/flash/sys/st3m",
                "/sd",
            ]
            and self.select_enabled
        )
        self._scroll_pos = math.pi / 2

        self._update_actions()
