from st4m.ui.view import ViewWithInputState, ViewTransitionSwipeRight, ViewManager
from st4m.input import InputState
from st4m.goose import Optional


class Application(ViewWithInputState):
    def __init__(self, name: str = __name__) -> None:
        self._name = name
        self._view_manager: Optional[ViewManager] = None
        super().__init__()

    def on_exit(self) -> None:
        pass

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        if self.input.left_shoulder.middle.pressed:
            if self._view_manager is not None:
                self.on_exit()
                self._view_manager.pop(ViewTransitionSwipeRight())
