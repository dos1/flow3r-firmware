from st3m import logging
from st3m.application import Application, ApplicationContext
from st3m.input import InputState
from st3m.ui.view import View, ViewManager
from ctx import Context

from .browser import Browser
from .reader import Reader


class Fil3sApp(Application):
    log = logging.Log(__name__, level=logging.INFO)

    path: str = "/"

    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx=app_ctx)

    def on_enter(self, vm: ViewManager | None) -> None:
        super().on_enter(vm)

        if self.vm is None:
            raise RuntimeError("vm is None")

        self.vm.replace(Browser(self.path, self.on_navigate, self.on_update_path))

    def on_navigate(self, view: str) -> None:
        if self.vm is None:
            raise RuntimeError("vm is None")

        if view == "browser":
            self.vm.replace(Browser(self.path, self.on_navigate, self.on_update_path))
        elif view == "reader":
            self.vm.replace(Reader(self.path, self.on_navigate, self.on_update_path))

    def on_update_path(self, path: str) -> None:
        self.path = path

    def draw(self, ctx: Context) -> None:
        pass

    def think(self, ins: InputState, delta_ms: int) -> None:
        pass
