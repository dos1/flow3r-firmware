from st3m.goose import List, Optional, Enum, Tuple

import sys_buttons
import captouch
import imu
from st3m.power import Power

import math

power = Power()


class Accelerometer(tuple):
    @property
    def radius(self):
        return sum([self[i] ** 2 for i in range(3)]) ** 0.5

    @property
    def inclination(self):
        x, y, z = self[0], self[1], self[2]
        if z > 0:
            return math.atan((((x**2) + (y**2)) ** 0.5) / z)
        elif z < 0:
            return math.tau / 2 + math.atan((((x**2) + (y**2)) ** 0.5) / z)
        return math.tau / 4

    @property
    def azimuth(self):
        x, y, z = self[0], self[1], self[2]
        if x > 0:
            return math.atan(y / x)
        elif x > 0:
            if y < 0:
                return math.atan(y / x) - math.tau / 2
            else:
                return math.atan(y / x) + math.tau / 2
        elif y < 0:
            return -math.tau / 4
        return math.tau / 4


class IMUState:
    """
    State of the Inertial Measurement Unit

    Acceleration in m/s**2, roation rate in deg/s, pressure in Pascal
    """

    __slots__ = ("acc", "gyro", "pressure")

    def __init__(
        self,
        acc: Accelerometer[float, float, float],
        gyro: Tuple[float, float, float],
        pressure: float,
    ) -> None:
        self.acc = acc
        self.gyro = gyro
        self.pressure = pressure

    def copy(self):
        return IMUState(self.acc, self.gyro, self.pressure)


class MomentarySwitch:
    def __init__(self, copyfrom=None) -> None:
        # _*: unstable, do not use for application development
        # these mirror exposed properties but bypass self._ignore for debug purposes
        if copyfrom is None:
            self._is_pressed = False
            self._press_event = False
            self._release_event = False
            self._pressed_since_ms = None

            self._is_pressed_prev = False
            self._ignore = True
            self._ignore_stop = False
        else:
            self._is_pressed = copyfrom._is_pressed
            self._press_event = copyfrom._press_event
            self._release_event = copyfrom._release_event
            self._pressed_since_ms = copyfrom._pressed_since_ms

            self._is_pressed_prev = copyfrom._is_pressed_prev
            self._ignore = copyfrom._ignore
            self._ignore_stop = copyfrom._ignore_stop

    def copy(self):
        # nice
        return type(self)(self)

    def _update(self, delta_t_ms: int, is_pressed: bool) -> None:
        self._is_pressed_prev = self._is_pressed
        self._is_pressed = is_pressed

        # wait a cycle to unignore to filter out release event
        if self._ignore_stop:
            self._ignore = False
            self._ignore_stop = False

        if self._is_pressed:
            self._press_event = not self._is_pressed_prev
            self._release_event = False
            if self._press_event:
                self._pressed_since_ms = 0
            else:
                self._pressed_since_ms += delta_t_ms
        else:
            self._press_event = False
            self._release_event = self._is_pressed_prev
            self._pressed_since_ms = None
            self._ignore_stop = True

    def _ignore_pressed(self) -> None:
        self._ignore = True
        self._ignore_stop = False

    def clear_press(self):
        """
        if switch is pressed: switch will report no events and pretend to not be pressed.
        this status is removed after a release_event occurred. triggered on view transition.
        """
        self._ignore_pressed()

    @property
    def is_pressed(self):
        """
        True if button/surface is being pressed
        """
        if self._ignore:
            return False
        return self._is_pressed

    @property
    def press_event(self):
        """
        True on a positive edge of is_pressed
        """
        if self._ignore:
            return False
        return self._press_event

    @property
    def release_event(self):
        """
        True on a negative edge of is_pressed
        """
        if self._ignore:
            return False
        return self._release_event

    @property
    def pressed_since_ms(self):
        """
        Time in ms since last press_event happened
        """
        if not self.is_pressed:
            return None
        return self._pressed_since_ms

    @pressed_since_ms.setter
    def pressed_since_ms(self, val):
        if self.is_pressed:
            self._pressed_since_ms = val

    def __repr__(self) -> str:
        ret = "<Pressable: "
        if self.press_event:
            ret += "press event"
        elif self.release_event:
            ret += "release event"
        elif self.is_pressed:
            ret += "is pressed since "
            ret += str(self.pressed_since_ms)
            ret += "ms"
        else:
            ret += "is not pressed"
        return ret + ">"


class InputButtonState:
    """
    State of the tri-state switches/buttons on the shoulders of the badge.

    If you want to detect edges, use the stateful InputController.

    By default, the left shoulder button is the 'app' button and the right
    shoulder button is the 'os' button. The user can switch this behaviour in
    the settings menu.

    The 'app' button can be freely used by applicaton code. The 'os' menu has
    fixed functions: volume up/down and back.

    In cases you want to access left/right buttons independently of app/os
    mapping (for example in applications where the handedness of the user
    doesn't matter), then you can use _left and _right to access their state
    directly.

    'app_is_left' is provided to let you figure out on which side of the badge
    the app button is, eg. for use when highlighting buttons on the screen or
    with LEDs.
    """

    PRESSED_LEFT = sys_buttons.PRESSED_LEFT
    PRESSED_RIGHT = sys_buttons.PRESSED_RIGHT
    PRESSED_DOWN = sys_buttons.PRESSED_DOWN
    NOT_PRESSED = sys_buttons.NOT_PRESSED

    def __init__(self, copyfrom=None):
        if copyfrom is None:
            self.app = IntTriButton()
            self.os = IntTriButton()
            self._update(0)
        else:
            self.app = copyfrom.app.copy()
            self.os = copyfrom.os.copy()
            self.app_button_is_left = copyfrom.app_button_is_left

    def _update(self, delta_t_ms):
        self.app_button_is_left = sys_buttons.app_is_left()
        self.app.is_left_button = self.app_button_is_left
        self.os.is_left_button = not self.app_button_is_left
        self.app._update(delta_t_ms, sys_buttons.get_app())
        self.os._update(delta_t_ms, sys_buttons.get_os())

    def copy(self):
        return InputButtonState(self)


class PetalPad:
    def __init__(self, copyfrom=None):
        if copyfrom is None:
            self._raw_pressure = 0
            self._is_pressed = False
        else:
            self._raw_pressure = copyfrom._raw_pressure
            self._is_pressed = copyfrom._is_pressed

    def _update(self, delta_t_ms, is_pressed, raw=0):
        self._is_pressed = is_pressed
        self._raw_pressure = raw

    @property
    def touch_area(self):
        # magic number
        return min(1.0, self._raw_pressure / 35000.0)

    @property
    def is_pressed(self):
        return self._is_pressed

    def copy(self):
        return PetalPad(self)


class PetalPads:
    def __init__(self, is_top, copyfrom=None):
        if copyfrom is None:
            self._is_top = is_top
            self.base = PetalPad()
            if self._is_top:
                self.ccw = PetalPad()
                self.cw = PetalPad()
            else:
                self.tip = PetalPad()
        else:
            self._is_top = copyfrom._is_top
            self.base = copyfrom.base.copy()
            if self._is_top:
                self.ccw = copyfrom.ccw.copy()
                self.cw = copyfrom.cw.copy()
            else:
                self.tip = copyfrom.tip.copy()

    def update(self, delta_t_ms, petal_state):
        pads = petal_state.pads
        if self._is_top:
            self.base._update(delta_t_ms, pads.base, pads._base_raw)
            self.ccw._update(delta_t_ms, pads.ccw, pads._ccw_raw)
            self.cw._update(delta_t_ms, pads.cw, pads._cw_raw)
        else:
            self.base._update(delta_t_ms, pads.base, pads._base_raw)
            self.tip._update(delta_t_ms, pads.tip, pads._tip_raw)

    def copy(self):
        return PetalPads(self._is_top, self)


class Petal(MomentarySwitch):
    def __init__(self, num, copyfrom=None):
        super().__init__(copyfrom)
        if copyfrom is None:
            self._num = num
            self.pads = PetalPads(self.is_top)
            self._raw_position = 0
            self._raw_pressure = 0
        else:
            self._num = copyfrom._num
            self.pads = copyfrom.pads.copy()
            self._raw_position = copyfrom._raw_position
            self._raw_pressure = copyfrom._raw_pressure

    def _update(self, delta_t_ms, petal_state):
        super()._update(delta_t_ms, petal_state.pressed)
        self._raw_position = petal_state.position
        self._raw_pressure = petal_state.position
        self.pads.update(delta_t_ms, petal_state)

    def copy(self):
        return type(self)(self._num, self)

    @property
    def index(self):
        return self._num

    @property
    def is_top(self):
        return (self._num % 2) == 0

    @property
    def at_angle(self):
        return self._num * (math.tau / 10)

    @property
    def touch_radius(self):
        # magic numbers
        ret = (self._raw_position[0] + 35000.0) / 80000.0
        return max(0, min(1, ret))

    @property
    def touch_area(self):
        # magic number
        return min(1.0, self._raw_pressure / 35000.0)

    @property
    def touch_angle_cw(self):
        if self.is_top:
            # magic number
            ret = self._raw_position[1] / 45000.0
            ret = max(-1, min(1, ret))
            return ret * (math.tau / 10)
        else:
            return 0


class PetalDeprecate(Petal):
    # TODO: DEPRECATE

    @property
    def pressed(self):
        return self.is_pressed

    @property
    def position(self):
        return self._raw_position

    @property
    def pressure(self):
        return self._raw_pressure

    @property
    def bottom(self):
        return not self.is_top

    @property
    def top(self):
        return self.is_top


class CaptouchRotaryDial:
    def __init__(self, petals):
        self._petals = petals

    @property
    def touch_angle_cw(self):
        active_petals = []
        for petal in self._petals:
            if petal.is_top and petal.is_pressed:
                if len(active_petals) == 2:
                    return None
                active_petals += [petal]
        if len(active_petals) == 0:
            return None
        if len(active_petals) == 1:
            petal = active_petals[0]
            return petal.touch_angle_cw + petal.at_angle
        if len(active_petals) == 2:
            lo = active_petals[0]
            hi = active_petals[1]
            lo_angle = lo.touch_angle_cw + lo.at_angle
            hi_angle = hi.touch_angle_cw + hi.at_angle
            at_angle = (hi.at_angle + lo.at_angle) / 2
            if lo.index == 0:
                if hi.index == 8:
                    lo_angle += math.tau
                    at_angle += math.tau / 2
                elif hi.index != 2:
                    return None
            elif abs(hi.index - lo.index) > 2:
                return None
            lo_raw = lo.pads.base._raw_pressure + 1
            hi_raw = hi.pads.base._raw_pressure + 1
            if hi_raw == lo_raw:
                return at_angle
            else:
                at_raw = 1000 / abs(hi_raw - lo_raw)
            ret = 0
            ret += hi_angle * hi_raw
            ret += lo_angle * lo_raw
            ret += at_angle * at_raw
            ret /= lo_raw + hi_raw + at_raw
            return ret
        return None


class Captouch:
    def __init__(self, copyfrom=None):
        if copyfrom is None:
            self.petals = [PetalDeprecate(i) for i in range(10)]
            self._update(0)
            self._rotary_dial = CaptouchRotaryDial(self.petals)
        else:
            self.petals = [None] * 10
            for i, petal in enumerate(copyfrom.petals):
                self.petals[i] = petal.copy()
            self._rotary_dial = CaptouchRotaryDial(self.petals)

    def _update(self, delta_t_ms):
        captouch_state = captouch.read()
        for i, petal in enumerate(self.petals):
            petal._update(delta_t_ms, captouch_state.petals[i])

    @property
    def rotary_dial(self):
        return self._rotary_dial

    def _ignore_pressed(self):
        for petal in self.petals:
            petal._ignore_pressed()

    def copy(self):
        return Captouch(self)


class InputState:
    """
    Current state of inputs from badge user. Passed via think() to every
    Responder.
    """

    def __init__(self, copyfrom=None) -> None:
        if copyfrom is None:
            self.captouch = Captouch()
            self.buttons = InputButtonState()
            self._imu = None
            self._pressure = None
            self._battery_voltage = None
            self._temperature = None
        else:
            self.buttons = copyfrom.buttons.copy()
            self.captouch = copyfrom.captouch.copy()
            if copyfrom.imu is None:
                self._imu = None
            else:
                self._imu = copyfrom.imu.copy()
            self._pressure = copyfrom.pressure
            self._battery_voltage = copyfrom._battery_voltage
            self._temperature = copyfrom.temperature

    def update(self, delta_t_ms):
        self._pressure = None  # bundles temperature
        self._battery_voltage = None
        self._imu = None
        self.buttons._update(delta_t_ms)
        self.captouch._update(delta_t_ms)

    def copy(self):
        return InputState(self)

    @property
    def battery_voltage(self):
        if self._battery_voltage is None:
            self._battery_voltage = power.battery_voltage
        return self._battery_voltage

    @property
    def imu(self):
        if self._imu is None:
            self._acc = Accelerometer(imu.acc_read())
            self._gyro = imu.gyro_read()
            self._imu = IMUState(self._acc, self._gyro, self.pressure)
        return self._imu

    @property
    def pressure(self):
        if self._pressure is None:
            self._pressure, self._temperature = imu.pressure_read()
        return self._pressure

    @property
    def temperature(self):
        if self._pressure is None:
            self._pressure, self._temperature = imu.pressure_read()
        return self._temperature

    @property
    def meters_above_sea(self):
        # p = 101325 * (1 - 2.25577 * (10**-5) m)**5.256
        # (p/101325)**(1/5.256) = 1 - 2.25577 * (10**-5) m
        # p = 101325 * (1 - 2.25577 * (10**-5) m)**5.256
        return (1 - (self.pressure / 101325) ** (1 / 5.256)) * (100000 / 2.2557)


class RepeatSettings:
    # TODO: DEPRECATE
    def __init__(self, first: float, subsequent: float) -> None:
        self.first = first
        self.subsequent = subsequent

    def copy(self):
        return RepeatSettings(self.first, self.subsequent)


class Pressable(MomentarySwitch):
    # TODO: DEPRECATE
    PRESSED = "pressed"
    REPEATED = "repeated"
    RELEASED = "released"
    DOWN = "down"
    UP = "up"

    def __init__(self, copyfrom=None) -> None:
        super().__init__(copyfrom)
        if copyfrom is None:
            self._is_pressed = False
            self._repeat: Optional[RepeatSettings] = RepeatSettings(400, 200)
            self._pressed_or_repeated_since_ms: Optional[float] = None
            self._first_repeat_happened = False
            self._repeat_event = False
        else:
            self._is_pressed = copyfrom._is_pressed
            self._repeat = copyfrom._repeat.copy()
            self._pressed_or_repeated_since_ms = copyfrom._pressed_or_repeated_since_ms
            self._first_repeat_happened = copyfrom._first_repeat_happened
            self._repeat_event = copyfrom._repeat_event

    def _update(self, delta_t_ms: int, is_pressed: bool) -> None:
        super()._update(delta_t_ms, is_pressed)
        if self.is_pressed:
            if self.press_event:
                self._pressed_or_repeated_since_ms = 0
            else:
                self._pressed_or_repeated_since_ms += delta_t_ms
        else:
            self._pressed_or_repeated_since_ms = None
            self._first_repeat_happened = False

        repeat_event = False
        if (
            self.is_pressed
            and self._repeat is not None
            and self._pressed_or_repeated_since_ms is not None
        ):
            repeat_time = self._repeat.subsequent
            if not self._first_repeat_happened:
                repeat_time = self._repeat.first
            if self._pressed_or_repeated_since_ms > repeat_time:
                self._first_repeat_happened = True
                self._pressed_or_repeated_since_ms = 0
                repeat_event = True
        self._repeat_event = repeat_event

    def _ignore_pressed(self) -> None:
        super()._ignore_pressed()
        self._first_repeat_happened = False
        self._repeat_event = False

    @property
    def state(self):
        """
        Returns one of self.{UP,DOWN,PRESSED,RELEASED,REPEATED}.
        """
        if self._repeat_event:
            return self.REPEATED
        if self.press_event:
            return self.PRESSED
        if self.release_event:
            return self.RELEASED
        if self.is_pressed:
            return self.DOWN
        return self.UP

    @property
    def pressed(self) -> bool:
        """
        True if the button has just been pressed.
        """
        return self.state == self.PRESSED

    @property
    def repeated(self) -> bool:
        """
        True if the button has been held long enough that a virtual 'repeat'
        press should be acted upon.
        """
        return self.state == self.REPEATED

    @property
    def released(self) -> bool:
        """
        True if the button has just been released.
        """
        return self.state == self.RELEASED

    @property
    def down(self) -> bool:
        """
        True if the button is held down, after first being pressed.
        """
        return self.state == self.DOWN

    @property
    def up(self) -> bool:
        """
        True if the button is currently not being held down.
        """
        return self.state == self.UP

    def repeat_enable(self, first: int = 400, subsequent: int = 200) -> None:
        """
        Enable key repeat functionality. Arguments are amount to wait in ms
        until first repeat is emitted and until subsequent repeats are emitted.

        Repeat is enabled by default on Pressables.
        """
        self._repeat = RepeatSettings(first, subsequent)

    def repeat_disable(self) -> None:
        """
        TODO: DEPRECATE
        Disable repeat functionality on this Pressable.
        """
        self._repeat = None


class TouchableState(Enum):
    # TODO: DEPRECATE
    UP = "up"
    BEGIN = "begin"
    RESTING = "resting"
    MOVED = "moved"
    ENDED = "ended"


class Touchable:
    # TODO: DEPRECATE
    """
    A Touchable processes incoming captouch positional state into higher-level
    simple gestures.

    The Touchable can be in one of four states:

        UP: not currently being interacted with
        BEGIN: some gesture has just started
        MOVED: a gesture is continuing
        ENDED: a gesture has just ended

    The state can be retrieved by calling phase().

    The main API for consumers is current_gesture(), which returns a
    Touchable.Gesture defining the current state of the gesture, from the
    beginning of the touch event up until now (or until the end, if the current
    phase is ENDED).

    Under the hood, the Touchable keeps a log of recent samples from the
    captouch petal position, and processes them to eliminate initial noise from
    the beginning of a gesture.

    All positional output state is the same format/range as in the low-level
    CaptouchState.
    """

    UP = TouchableState.UP
    BEGIN = TouchableState.BEGIN
    MOVED = TouchableState.MOVED
    ENDED = TouchableState.ENDED

    class Entry:
        """
        A Touchable's log entry, containing some position measurement at some
        timestamp.
        """

        __slots__ = ["ts", "phi", "rad"]

        def __init__(self, ts: int, phi: float, rad: float) -> None:
            self.ts = ts
            self.phi = phi
            self.rad = rad

        def __repr__(self) -> str:
            return f"{self.ts}: ({self.rad}, {self.phi})"

    class Gesture:
        # TODO: DEPRECATE
        """
        A simple captouch gesture, currently definined as a movement between two
        points: the beginning of the gesture (when the user touched the petal)
        and to the current state. If the gesture is still active, the current
        state is averaged/filtered to reduce noise. If the gesture has ended,
        the current state is the last measured position.
        """

        def __init__(self, start: "Touchable.Entry", end: "Touchable.Entry") -> None:
            self.start = start
            self.end = end

        @property
        def distance(self) -> Tuple[float, float]:
            """
            Distance traveled by this gesture.
            """
            delta_rad = self.end.rad - self.start.rad
            delta_phi = self.end.phi - self.start.phi
            return (delta_rad, delta_phi)

        @property
        def velocity(self) -> Tuple[float, float]:
            """
            Velocity vector of this gesture.
            """
            delta_rad = self.end.rad - self.start.rad
            delta_phi = self.end.phi - self.start.phi
            if self.end.ts == self.start.ts:
                return (0, 0)
            delta_s = (self.end.ts - self.start.ts) / 1000
            return (delta_rad / delta_s, delta_phi / delta_s)

    def __init__(self, pos: tuple[float, float] = (0.0, 0.0)) -> None:
        # Entry log, used for filtering.
        self._log: List[Touchable.Entry] = []

        # What the beginning of the gesture is defined as. This is ampled a few
        # entries into the log as the initial press stabilizes.
        self._start: Optional[Touchable.Entry] = None
        self._start_ts: int = 0

        # Current and previous 'pressed' state from the petal, used to begin
        # gesture tracking.
        self._pressed = False
        self._prev_pressed = self._pressed

        self._state = self.UP

        # If not nil, amount of update() calls to wait until the gesture has
        # been considered as started. This is part of the mechanism which
        # eliminates early parts of a gesture while the pressure on the sensor
        # grows and the user's touch contact point changes.
        self._begin_wait: Optional[int] = None

        self._last_ts: int = 0

    def _append_entry(self, ts: int, petal: captouch.CaptouchPetalState) -> None:
        """
        Append an Entry to the log based on a given CaptouchPetalState.
        """
        (rad, phi) = petal.position
        entry = self.Entry(ts, phi, rad)
        self._log.append(entry)
        overflow = len(self._log) - 10
        if overflow > 0:
            self._log = self._log[overflow:]

    def _update(self, ts: int, petal: captouch.CaptouchPetalState) -> None:
        """
        Called when the Touchable is being processed by an InputController.
        """
        self._last_ts = ts
        self._prev_pressed = self._pressed
        self._pressed = petal.pressed

        if not self._pressed:
            if not self._prev_pressed:
                self._state = self.UP
            else:
                self._state = self.ENDED
            return

        self._append_entry(ts, petal)

        if not self._prev_pressed:
            # Wait 5 samples until we consider the gesture started.
            # TODO(q3k): do better than hardcoding this. Maybe use pressure data?
            self._begin_wait = 5
        elif self._begin_wait is not None:
            self._begin_wait -= 1
            if self._begin_wait < 0:
                self._begin_wait = None
                # Okay, the gesture has officially started.
                self._state = self.BEGIN
                # Grab latest log entry as gesture start.
                self._start = self._log[-1]
                self._start_ts = ts
                # Prune log.
                self._log = self._log[-1:]
        else:
            self._state = self.MOVED

    def phase(self) -> TouchableState:
        """
        Returns the current phase of a gesture as tracked by this Touchable (petal).
        """
        return self._state

    def current_gesture(self) -> Optional[Gesture]:
        if self._start is None:
            return None

        assert self._start_ts is not None
        delta_ms = self._last_ts - self._start_ts

        first = self._start
        last = self._log[-1]
        # If this gesture hasn't ended, grab last 5 log entries for average of
        # current position. This filters out a bunch of noise.
        if self.phase() != self.ENDED:
            log = self._log[-5:]
            phis = [el.phi for el in log]
            rads = [el.rad for el in log]
            phi_avg = sum(phis) / len(phis)
            rad_avg = sum(rads) / len(rads)
            last = self.Entry(last.ts, phi_avg, rad_avg)

        return self.Gesture(first, last)


class TriButton:
    """
    State of a tri-stat shoulder button
    """

    def __init__(self, copyfrom=None) -> None:
        if copyfrom is None:
            self.left = Pressable()
            self.middle = Pressable()
            self.right = Pressable()
            self._is_left_button_prev = True
            self.is_left_button = False
        else:
            self.left = copyfrom.left.copy()
            self.middle = copyfrom.middle.copy()
            self.right = copyfrom.right.copy()
            self._is_left_button_prev = copyfrom._is_left_button_prev
            self.is_left_button = copyfrom.is_left_button

    @property
    def is_left_button(self):
        return self._is_left_button

    @is_left_button.setter
    def is_left_button(self, val):
        self._is_left_button = val
        self._update_ward()

    def _update(self, delta_t_ms: int, st: int) -> None:
        self.left._update(delta_t_ms, st == -1)
        self.middle._update(delta_t_ms, st == 2)
        self.right._update(delta_t_ms, st == 1)

    def _update_ward(self):
        if self.is_left_button:
            self.outward = self.left
            self.inward = self.right
        else:
            self.outward = self.right
            self.inward = self.left
        if self._is_left_button_prev != self.is_left_button:
            self._ignore_pressed()
            self._is_left_button_prev = self.is_left_button

    def _ignore_pressed(self) -> None:
        self.left._ignore_pressed()
        self.middle._ignore_pressed()
        self.right._ignore_pressed()

    def copy(self):
        return TriButton(self)


class IntTriButton(int, TriButton):
    # TODO: DEPRECATE
    pass


class ButtonsState:
    # TODO: DEPRECATE
    """
    Edge-trigger detection for input button state.

    See  InputButtonState for more information about the meaning of app, os,
    _left, _right and app_is_left.
    """

    def __init__(self) -> None:
        self.app = TriButton()
        self.os = TriButton()

        # Defaults. Real data coming from _update will change this to the
        # correct values from an InputState.
        self.app_is_left = True

        self._left = self.app
        self._right = self.os

    @property
    def app_is_left(self):
        return self._app_is_left

    @app_is_left.setter
    def app_is_left(self, val):
        self._app_is_left = val

    def _update(self, delta_t_ms: int) -> None:
        self.app._update(delta_t_ms, sys_buttons.get_app())
        self.os._update(delta_t_ms, sys_buttons.get_os())

    def _ignore_pressed(self) -> None:
        self.app._ignore_pressed()
        self.os._ignore_pressed()


class PetalPressableDeprecate:
    # TODO: DEPRECATE
    def __init__(self, num, copyfrom=None):
        self.whole = Pressable()
        self._num = num

    @property
    def ix(self):
        return self._num


class CaptouchPressable:
    # TODO: DEPRECATE
    def __init__(self):
        self.petals = [PetalPressableDeprecate(i) for i in range(10)]
        self._update(0)

    def _update(self, delta_t_ms):
        captouch_state = captouch.read()
        for i, petal in enumerate(self.petals):
            petal.whole._update(delta_t_ms, captouch_state.petals[i].pressed)

    def _ignore_pressed(self):
        for petal in self.petals:
            petal.whole._ignore_pressed()


class InputController:
    # TODO: DEPRECATE
    """
    A stateful input controller. It accepts InputState updates from the Reactor
    and allows a Responder to detect input events, like a button having just
    been pressed.

    To use, instantiate within a Responder and call think() from your
    responder's think().

    Then, access the captouch/left_shoulder/right_shoulder fields.
    """

    __slots__ = (
        "captouch",
        "buttons",
    )

    def __init__(self) -> None:
        self.buttons = ButtonsState()
        self.captouch = CaptouchPressable()

    def think(self, hr: InputState, delta_t_ms: int) -> None:
        self.buttons._update(delta_t_ms)
        self.captouch._update(delta_t_ms)

    def _ignore_pressed(self) -> None:
        # TODO: DEPRECATE
        """
        Pretend input buttons aren't being pressed for the next two update
        iterations. Used to prevent spurious presses to be routed to apps that
        have just been foregrounded.
        """
        self.captouch._ignore_pressed()
        self.buttons._ignore_pressed()
