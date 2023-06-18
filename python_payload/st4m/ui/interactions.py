import st4m

from st4m.input import InputState
from st4m.ui.ctx import Ctx
from st4m import Responder


class ScrollController(st4m.Responder):
    """
    ScrolLController is a controller for physically scrollable one-dimensional
    lists.

    It allows navigating with explicit 'left'/'right' commands (triggered by eg.
    buttons). In the future it will also allow navigating by captouch gestures.

    Its output is the information about a 'target position' (integer from 0 to
    Nitems-1) a 'current position' (float from -inf to +inf).

    The target position is the user intent, ie. what the user likely selected.
    This value should be used to interpret what the user has currently selected.

    The current position is the animated 'current' position, which includes
    effects like acceleration and past-end-of-list bounce-back. This value
    should be used to render the current state of the scrolling list.
    """
    __slots__ = (
        '_nitems',
        '_target_position',
        '_current_position',
        '_velocity',
    )

    def __init__(self) -> None:
        self._nitems = 0
        self._target_position = 0
        self._current_position = 0.0
        self._velocity: float = 0.0
    
    def set_item_count(self, count: int) -> None:
        """
        Set how many items this scrollable list contains. Currently, updating
        the item count does not gracefully handle current/target position
        switching - it will be clamped to the new length. A different API should
        provide the ability to remove a [start,end) range of items from the list
        and update positions gracefully according to that.
        """
        if count < 0:
            count = 0
        self._nitems = count

    def scroll_left(self) -> None:
        """
        Call when the user wants to scroll left by discrete action (eg. button
        press).
        """
        self._target_position -= 1
        self._velocity = -10

    def scroll_right(self) -> None:
        """
        Call when the user wants to scroll right by discrete action (eg. button
        press).
        """
        self._target_position += 1
        self._velocity = 10

    def think(self, ins: InputState, delta_ms: int) -> None:
        if self._nitems == 0:
            self._target_position = 0
            self._current_position = 0
            self._velocity = 0
            return

        if self._target_position < 0:
            self._target_position = 0
        if self._target_position >= self._nitems:
            self._target_position = self._nitems - 1

        self._physics_step(delta_ms / 1000.0)

    def draw(self, ctx: Ctx) -> None:
        pass

    def current_position(self) -> float:
        """
        Return current position, a float from -inf to +inf.

        In general, this value will be within [O, Nitems), but might go out of
        bounds when rendering the bounceback effect.

        With every tick, this value moves closer and closer to target_position().

        Use this value to animate the scroll list.
        """
        return self._current_position

    def target_position(self) -> int:
        """
        Return user-selected 'target' position, an integer in [0, Nitems).

        Use this value to interpret the user selection when eg. a 'select'
        button is pressed.
        """
        return self._target_position

    def at_left_limit(self) -> bool:
        """
        Returns true if the scrollable list is at its leftmost (0) position.
        """
        return self._target_position <= 0
    
    def at_right_limit(self) -> bool:
        """
        Returns true if the scrollable list is as its rightmost (Nitems-1)
        position.
        """
        return self._target_position >= self._nitems - 1

    def _physics_step(self, delta: float) -> None:
        diff = float(self._target_position) - self._current_position
        max_velocity = 500
        velocity = self._velocity

        if abs(diff) > 0.1:
            # Apply force to reach target position.
            if diff > 0:
                velocity += 80 * delta
            else:
                velocity -= 80 * delta

            # Clamp velocity.
            if velocity > max_velocity:
                velocity = max_velocity
            if velocity < -max_velocity:
                velocity = -max_velocity
            self._velocity = velocity
        else:
            # Try to snap to target position.
            pos = self._velocity > 0 and diff > 0
            neg = self._velocity < 0 and diff < 0
            if pos or neg:
                self._current_position = self._target_position
                self._velocity = 0

        self._physics_integrate(delta)

    def _physics_integrate(self, delta: float) -> None:
        self._velocity -= self._velocity * delta * 10
        self._current_position += self._velocity * delta


