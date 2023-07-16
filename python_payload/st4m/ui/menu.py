import st4m

from st4m import Responder
from st4m.goose import ABCBase, abstractmethod, List, Optional
from st4m.input import InputState, InputController
from st4m.ui.view import (
    ViewWithInputState,
    View,
    ViewManager,
    ViewTransitionSwipeLeft,
    ViewTransitionSwipeRight,
)
from st4m.ui.interactions import ScrollController, GestureScrollController
from st4m.ui.ctx import Ctx


class MenuItem(ABCBase):
    """
    An abstract MenuItem to be implemented by concrete impementations.

    A MenuItem implementation can be added to a MenuController
    """

    @abstractmethod
    def press(self, vm: Optional[ViewManager]) -> None:
        """
        Called when the menu item is 'pressed'/'activated'.

        vm will be set the the active ViewManager if the menu is used in
        ViewManager context.
        """
        pass

    @abstractmethod
    def label(self) -> str:
        """
        Return the printable label of the menu item.
        """
        pass


class MenuItemForeground(MenuItem):
    """
    A MenuItem which, when activated, navigates to the given View.
    """

    def __init__(self, label: str, r: View) -> None:
        self._r = r
        self._label = label

    def press(self, vm: Optional[ViewManager]) -> None:
        if vm is not None:
            vm.push(self._r, ViewTransitionSwipeLeft())

    def label(self) -> str:
        return self._label


class MenuItemNoop(MenuItem):
    """
    A MenuItem which does nothing.
    """

    def __init__(self, label: str) -> None:
        self._label = label

    def press(self, vm: Optional[ViewManager]) -> None:
        pass

    def label(self) -> str:
        return self._label


class MenuItemBack(MenuItem):
    """
    A MenuItem which, when activatd, navigates back in history.
    """

    def press(self, vm: Optional[ViewManager]) -> None:
        if vm is not None:
            vm.pop(ViewTransitionSwipeRight())

    def label(self) -> str:
        return "Back"


class MenuController(ViewWithInputState):
    """
    Base class for menus. Reacts to canonical inputs (left shoulder button) to
    move across and select actions from the menu.

    Implementers must implement draw() and use self._items and
    self._scroll_controller to display menu items accordingly.
    """

    __slots__ = (
        "_items",
        "_scroll_controller",
        "_view_manager",
    )

    def __init__(self, items: List[MenuItem], vm: Optional[ViewManager]) -> None:
        self._items = items
        self._scroll_controller = GestureScrollController(0)
        self._scroll_controller.set_item_count(len(items))
        self._view_manager = vm

        super().__init__()

    def _parse_state(self) -> None:
        left = self.input.left_shoulder.left
        right = self.input.left_shoulder.right

        if left.pressed:
            self._scroll_controller.scroll_left()
            return
        if right.pressed:
            self._scroll_controller.scroll_right()
            return

        if not self._scroll_controller.at_left_limit() and left.repeated:
            self._scroll_controller.scroll_left()
            return

        if not self._scroll_controller.at_right_limit() and right.repeated:
            self._scroll_controller.scroll_right()
            return

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        self._scroll_controller.think(ins, delta_ms)

        if self.input.left_shoulder.middle.pressed:
            self.select()
        self._parse_state()

    def draw(self, ctx: Ctx) -> None:
        pass

    def select(self) -> None:
        """
        Call to activate the currently selected item.

        Automatically called on canonical user input.
        """
        self._items[self._scroll_controller.target_position()].press(self._view_manager)
