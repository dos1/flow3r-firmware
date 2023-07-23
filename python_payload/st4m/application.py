from st4m.ui.view import ViewWithInputState, ViewTransitionSwipeRight
from st4m.input import InputState


class Application(ViewWithInputState):
    def __init__(self, name: str = __name__):
        self._name = name
        self._view_manager = None
        super().__init__()

    def on_exit(self):
        pass

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        if self.input.left_shoulder.middle.pressed:
            if self._view_manager is not None:
                self.on_exit()
                self._view_manager.pop(ViewTransitionSwipeRight())
