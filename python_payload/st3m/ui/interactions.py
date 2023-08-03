import st3m

from st3m.input import InputState
from st3m.ui.ctx import Ctx
from st3m import Responder


class ScrollController(st3m.Responder):
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
        "_nitems",
        "_target_position",
        "_current_position",
        "_velocity",
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
        return round(self._current_position, 4)

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


# class GestureScrollController(ScrollController):
#    """
#    GestureScrollController extends ScrollController to also react to swipe gestures
#    on a configurable petal.
#
#    #TODO (iggy): rewrite both to extend a e.g. "PhysicsController"
#    """
#
#    def __init__(self, petal_index, wrap=False):
#        super().__init__(wrap)
#        self._petal = PetalController(petal_index)
#        self._speedbuffer = [0.0]
#        self._ignore = 0
#
#    def scroll_left(self) -> None:
#        """
#        Call when the user wants to scroll left by discrete action (eg. button
#        press).
#        """
#        # self._target_position -= 1
#        self._speedbuffer = []
#        self._velocity = -1
#        self._current_position -= 0.3
#
#    def scroll_right(self) -> None:
#        """
#        Call when the user wants to scroll right by discrete action (eg. button
#        press).
#        """
#        # self._target_position += 1
#        self._speedbuffer = []
#        self._velocity = 1
#        self._current_position += 0.3
#
#    def think(self, ins: InputState, delta_ms: int) -> None:
#        # super().think(ins, delta_ms)
#
#        self._petal.think(ins, delta_ms)
#
#        if self._ignore:
#            self._ignore -= 1
#            return
#
#        dphi = self._petal._input._dphi
#        phase = self._petal._input.phase()
#
#        self._speedbuffer.append(self._velocity)
#
#        while len(self._speedbuffer) > 3:
#            self._speedbuffer.pop(0)
#
#        speed = sum(self._speedbuffer) / len(self._speedbuffer)
#
#        speed = min(speed, 0.008)
#        speed = max(speed, -0.008)
#
#        if phase == self._petal._input.ENDED:
#            # self._speedbuffer = [0 if abs(speed) < 0.005 else speed]
#            while len(self._speedbuffer) > 3:
#                print("sb:", self._speedbuffer)
#                self._speedbuffer.pop()
#        elif phase == self._petal._input.UP:
#            pass
#        elif phase == self._petal._input.BEGIN:
#            self._ignore = 5
#            # self._speedbuffer = [0.0]
#        elif phase == self._petal._input.RESTING:
#            self._speedbuffer.append(0.0)
#        elif phase == self._petal._input.MOVED:
#            impulse = -dphi / delta_ms
#            self._speedbuffer.append(impulse)
#
#        if abs(speed) < 0.0001:
#            speed = 0
#
#        self._velocity = speed
#
#        self._current_position = self._current_position + self._velocity * delta_ms
#
#        if self.wrap:
#            self._current_position = self._current_position % self._nitems
#        elif round(self._current_position) < 0:
#            self._current_position = 0
#        elif round(self._current_position) >= self._nitems:
#            self._current_position = self._nitems - 1
#
#        if phase != self._petal._input.UP:
#            return
#
#        pos = round(self._current_position)
#        microstep = round(self._current_position) - self._current_position
#        # print("micro:", microstep)
#        # print("v", self._velocity)
#        # print("pos", self._current_position)
#
#        if (
#            abs(microstep) < 0.1
#            and abs(self._velocity) < 0.001
#            # and abs(self._velocity)
#        ):
#            self._velocity = 0
#            self._speedbuffer.append(0)
#            self._current_position = round(self._current_position)
#            self._target_position = self._current_position
#            # print("LOCK")
#            return
#
#        if abs(self._velocity) > 0.001:
#            self._speedbuffer.append(-self._velocity)
#            # print("BREAKING")
#            return
#
#        if self._velocity >= 0 and microstep > 0:
#            self._speedbuffer.append(max(self._velocity, 0.01) * microstep * 10)
#            # print("1")
#        elif self._velocity < 0 and microstep > 0:
#            self._speedbuffer.append(-self._velocity)
#            # print("2")
#        elif self._velocity > 0 and microstep < 0:
#            self._speedbuffer.append(-self._velocity * 0.5)
#            # print("3")
#
#        elif self._velocity <= 0 and microstep < 0:
#            self._speedbuffer.append(min(self._velocity, -0.01) * abs(microstep) * 10)
#            # print("4")
#
