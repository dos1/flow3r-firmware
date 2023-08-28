from ctx import Context

from st3m import Responder, settings
from st3m.input import InputState, InputController
from st3m.goose import ABCBase, abstractmethod, List

import audio


class Processor(ABCBase):
    """
    Processors are background-running controllers not tied to any particular
    view.
    """

    @abstractmethod
    def think(self, ins: InputState, delta_ms: int) -> None:
        """
        Called every tick.
        Or maybe less? ;D
        """
        pass


class AudioProcessor(Processor):
    """
    The AudioProcessor is respondible for managing system-wide audio state, like
    volume control using buttons.
    """

    def __init__(self) -> None:
        super().__init__()
        self.repeat_after_ms = 800
        self.repeat_rate_ms = 300
        self.repeat_step_mult = 1.5

    def vol_steps(self, steps):
        audio.adjust_volume_dB(steps * settings.num_volume_step_db.value)

    def vol_steps_repeat(self, switch, step_mult):
        if switch.press_event:
            self.vol_steps(step_mult)
        elif switch.is_pressed:
            if switch.pressed_since_ms > self.repeat_after_ms:
                self.vol_steps(self.repeat_step_mult * step_mult)
                switch.pressed_since_ms -= self.repeat_rate_ms

    def think(self, ins: InputState, delta_ms: int) -> None:
        self.vol_steps_repeat(ins.buttons.os.left, -1)
        self.vol_steps_repeat(ins.buttons.os.right, 1)


class ProcessorMidldeware(Responder):
    """
    Combines system processors and runs them together with some kind of
    top-level foreground responder.
    """

    PROCESSORS: List[Processor] = [
        AudioProcessor(),
    ]

    def __init__(self, top: Responder) -> None:
        self.top = top
        self._os2 = None

    def think(self, ins: InputState, delta_ms: int) -> None:
        for p in self.PROCESSORS:
            p.think(ins, delta_ms)
        # TODO: Deprecate copying, this is just for backwards compatibility
        input_copy = ins.copy()
        # TODO: Give the OS and Apps their own set of timers in the backend
        if self._os2 is None:
            self._os2 = ins.buttons.os.copy()
        input_copy.buttons.os = self._os2
        self.top.think(input_copy, delta_ms)

    def draw(self, ctx: Context) -> None:
        self.top.draw(ctx)
