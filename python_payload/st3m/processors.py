from ctx import Context

from st3m import Responder
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
        # Whether the volume was adjusted. Used to do a second pass.
        adjusted = False
        # Whether the volume is so low that we should enable mute.
        should_mute = False
        if self.input.buttons.os.left.pressed:
            started_at = audio.get_volume_dB()
            if started_at <= -20:
                should_mute = True
            audio.adjust_volume_dB(-5)
            adjusted = True
        if self.input.buttons.os.right.pressed:
            if not audio.get_mute():
                audio.adjust_volume_dB(5)
            adjusted = True
        if adjusted:
            # Clamp lower level to -20dB.
            if audio.get_volume_dB() < -20:
                audio.set_volume_dB(-20)
            # Set audio mute
            if should_mute:
                audio.set_mute(True)
            else:
                audio.set_mute(False)


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
