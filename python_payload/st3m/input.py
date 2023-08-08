from st3m.goose import List, Optional, Enum, Tuple

import hardware
import captouch
import imu

import machine


class InputState:
    """
    Current state of inputs from badge user. Passed via think() to every
    Responder.

    If you want to detect edges, use the stateful InputController.
    """

    def __init__(
        self,
        captouch: captouch.CaptouchState,
        left_button: int,
        right_button: int,
    ) -> None:
        # self.petal_pads = petal_pads
        self.captouch = captouch
        self.left_button = left_button
        self.right_button = right_button

    @classmethod
    def gather(cls) -> "InputState":
        """
        Build InputState from current hardware state. Should only be used by the
        Reactor.
        """
        cts = captouch.read()
        left_button = hardware.left_button_get()
        right_button = hardware.right_button_get()

        return InputState(cts, left_button, right_button)


class RepeatSettings:
    def __init__(self, first: float, subsequent: float) -> None:
        self.first = first
        self.subsequent = subsequent


class PressableState(Enum):
    PRESSED = "pressed"
    REPEATED = "repeated"
    RELEASED = "released"
    DOWN = "down"
    UP = "up"


class Pressable:
    """
    A pressable button or button-acting object (like captouch petal in button
    mode).

    Carries information about current and previous state of button, allowing to
    detect edges (pressed/released) and state (down/up). Additionally implements
    button repeating.
    """

    PRESSED = PressableState.PRESSED
    REPEATED = PressableState.REPEATED
    RELEASED = PressableState.RELEASED
    DOWN = PressableState.DOWN
    UP = PressableState.UP

    def __init__(self, state: bool) -> None:
        self._state = state
        self._prev_state = state
        self._repeat: Optional[RepeatSettings] = RepeatSettings(400, 200)

        self._pressed_at: Optional[float] = None
        self._repeating = False
        self._repeated = False

        self._ignoring = 0

    def repeat_enable(self, first: int = 400, subsequent: int = 200) -> None:
        """
        Enable key repeat functionality. Arguments are amount to wait in ms
        until first repeat is emitted and until subsequent repeats are emitted.

        Repeat is enabled by default on Pressables.
        """
        self._repeat = RepeatSettings(first, subsequent)

    def repeat_disable(self) -> None:
        """
        Disable repeat functionality on this Pressable.
        """
        self._repeat = None

    def _update(self, ts: int, state: bool) -> None:
        if self._ignoring > 0:
            self._ignoring -= 1

        self._prev_state = self._state
        self._state = state
        self._repeated = False

        if state == False:
            self._pressed_at = None
            self._repeating = False
        else:
            if self._pressed_at is None:
                self._pressed_at = ts

        repeat = self._repeat
        if state and repeat is not None and self._pressed_at is not None:
            if not self._repeating:
                if ts > self._pressed_at + repeat.first:
                    self._repeating = True
                    self._repeated = True
                    self._prev_state = False
                    self._pressed_at = ts
            else:
                if ts > self._pressed_at + repeat.subsequent:
                    self._prev_state = False
                    self._pressed_at = ts
                    self._repeated = True

    @property
    def state(self) -> PressableState:
        """
        Returns one of PressableState.{UP,DOWN,PRESSED,RELEASED,REPEATED}.
        """
        prev = self._prev_state
        cur = self._state

        if self._ignoring > 0:
            return self.UP

        if self._repeated:
            return self.REPEATED

        if cur and not prev:
            return self.PRESSED
        if not cur and prev:
            return self.RELEASED
        if cur and prev:
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

    def _ignore_pressed(self) -> None:
        """
        Pretend the button isn't being pressed for the next two update
        iterations. Used to prevent spurious presses to be routed to apps that
        have just been foregrounded.
        """
        self._ignoring = 2
        self._repeating = False
        self._repeated = False

    def __repr__(self) -> str:
        return "<Pressable: " + str(self.state) + ">"


class TouchableState(Enum):
    UP = "up"
    BEGIN = "begin"
    RESTING = "resting"
    MOVED = "moved"
    ENDED = "ended"


class Touchable:
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


class PetalState:
    def __init__(self, ix: int) -> None:
        self.ix = ix
        self.whole = Pressable(False)
        self.pressure = 0
        self.gesture = Touchable()

    def _update(self, ts: int, petal: captouch.CaptouchPetalState) -> None:
        self.whole._update(ts, petal.pressed)
        self.pressure = petal.pressure
        self.gesture._update(ts, petal)


class CaptouchState:
    """
    State of capacitive touch petals.

    The petals are indexed from 0 to 9 (inclusive). Petal 0 is above the USB-C
    socket, then the numbering continues counter-clockwise.
    """

    __slots__ = "petals"

    def __init__(self) -> None:
        self.petals = [PetalState(i) for i in range(10)]

    def _update(self, ts: int, ins: InputState) -> None:
        for i, petal in enumerate(self.petals):
            petal._update(ts, ins.captouch.petals[i])

    def _ignore_pressed(self) -> None:
        for petal in self.petals:
            petal.whole._ignore_pressed()


class TriSwitchHandedness(Enum):
    """
    Left or right shoulder button.
    """

    left = "left"
    right = "right"


class TriSwitchState:
    """
    State of a tri-stat shoulder button
    """

    __slots__ = ("left", "middle", "right", "handedness")

    def __init__(self, h: TriSwitchHandedness) -> None:
        self.handedness = h

        self.left = Pressable(False)
        self.middle = Pressable(False)
        self.right = Pressable(False)

    def _update(self, ts: int, hr: InputState) -> None:
        st = (
            hr.left_button
            if self.handedness == TriSwitchHandedness.left
            else hr.right_button
        )
        self.left._update(ts, st == -1)
        self.middle._update(ts, st == 2)
        self.right._update(ts, st == 1)

    def _ignore_pressed(self) -> None:
        self.left._ignore_pressed()
        self.middle._ignore_pressed()
        self.right._ignore_pressed()


class IMUState:
    __slots__ = ("acc", "gyro", "pressure", "temperature")

    def __init__(self) -> None:
        self.acc = (0.0, 0.0, 0.0)
        self.gyro = (0.0, 0.0, 0.0)
        self.pressure = 0.0
        self.temperature = 0.0

    def _update(self, ts: int, hr: InputState) -> None:
        self.acc = imu.acc_read()
        self.gyro = imu.gyro_read()
        self.pressure, self.temperature = imu.pressure_read()


class PowerState:
    __slots__ = ("_ts", "_adc_pin", "_adc", "battery_voltage")

    def __init__(self) -> None:
        self._adc_pin = machine.Pin(9, machine.Pin.IN)
        self._adc = machine.ADC(self._adc_pin, atten=machine.ADC.ATTN_11DB)
        self.battery_voltage = self._battery_voltage_sample()
        self._ts = 0

    def _battery_voltage_sample(self) -> float:
        return self._adc.read_uv() * 2 / 1e6

    def _update(self, ts: int, hr: InputState) -> None:
        if ts >= self._ts + 1000:
            self.battery_voltage = self._battery_voltage_sample()
            self._ts = ts


class InputController:
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
        "left_shoulder",
        "right_shoulder",
        "imu",
        "power",
        "_ts",
    )

    def __init__(self) -> None:
        self.captouch = CaptouchState()
        self.left_shoulder = TriSwitchState(TriSwitchHandedness.left)
        self.right_shoulder = TriSwitchState(TriSwitchHandedness.right)
        self.imu = IMUState()
        self.power = PowerState()
        self._ts = 0

    def think(self, hr: InputState, delta_ms: int) -> None:
        self._ts += delta_ms
        self.captouch._update(self._ts, hr)
        self.left_shoulder._update(self._ts, hr)
        self.right_shoulder._update(self._ts, hr)
        self.imu._update(self._ts, hr)
        self.power._update(self._ts, hr)

    def _ignore_pressed(self) -> None:
        """
        Pretend input buttons aren't being pressed for the next two update
        iterations. Used to prevent spurious presses to be routed to apps that
        have just been foregrounded.
        """
        self.captouch._ignore_pressed()
        self.left_shoulder._ignore_pressed()
        self.right_shoulder._ignore_pressed()
