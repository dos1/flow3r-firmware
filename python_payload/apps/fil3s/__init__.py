from st3m import logging
from st3m.application import Application, ApplicationContext
from st3m.goose import Optional
from st3m.input import InputState
from st3m.ui.view import View, ViewManager
from ctx import Context

from .browser import Browser
from .reader import Reader


class Fil3sApp(Application):
    log = logging.Log(__name__, level=logging.INFO)

    path: str = "/"
    selected: Optional[str] = None

    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx=app_ctx)
        self.view = "browser"

    def on_enter(self, vm: ViewManager | None) -> None:
        super().on_enter(vm)

        if self.vm is None:
            raise RuntimeError("vm is None")

        if self.view == "browser":
            self.vm.replace(
                Browser(self.path, self.on_navigate, self.on_update_path, self.selected)
            )
        elif self.view == "reader":
            self.vm.replace(Reader(self.path, self.on_navigate, self.on_update_path))

    def on_navigate(self, view: str) -> None:
        if self.vm is None:
            raise RuntimeError("vm is None")

        self.view = view

        if view == "browser":
            self.vm.replace(
                Browser(self.path, self.on_navigate, self.on_update_path, self.selected)
            )
        elif view == "reader":
            self.vm.replace(Reader(self.path, self.on_navigate, self.on_update_path))

    def on_update_path(self, path: str, selected: str = None) -> None:
        self.path = path
        self.selected = selected

    def draw(self, ctx: Context) -> None:
        pass

    def think(self, ins: InputState, delta_ms: int) -> None:
        pass
