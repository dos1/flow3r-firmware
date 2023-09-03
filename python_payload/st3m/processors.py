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
        """
        pass


class AudioProcessor(Processor):
    """
    The AudioProcessor is respondible for managing system-wide audio state, like
    volume control using buttons.
    """

    def __init__(self) -> None:
        super().__init__()
        self.input = InputController()

    def think(self, ins: InputState, delta_ms: int) -> None:
        self.input.think(ins, delta_ms)
        if self.input.buttons.os.left.pressed:
            audio.adjust_volume_dB(-settings.num_volume_step_db.value)
        if self.input.buttons.os.right.pressed:
            audio.adjust_volume_dB(settings.num_volume_step_db.value)


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

    def think(self, ins: InputState, delta_ms: int) -> None:
        for p in self.PROCESSORS:
            p.think(ins, delta_ms)
        self.top.think(ins, delta_ms)

    def draw(self, ctx: Context) -> None:
        self.top.draw(ctx)
