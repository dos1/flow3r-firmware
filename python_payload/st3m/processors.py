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
        self.repeat_wait_ms = 800
        self.repeat_ms = 300
        self._update_repeat()

    def _update_repeat(self):
        changed = False
        if self.repeat_wait_ms != settings.num_volume_repeat_wait_ms.value:
            changed = True
        if self.repeat_ms != settings.num_volume_repeat_ms.value:
            changed = True
        if changed:
            self.repeat_wait_ms = settings.num_volume_repeat_wait_ms.value
            self.repeat_ms = settings.num_volume_repeat_ms.value
            self.input.buttons.os.left.repeat_enable(
                self.repeat_wait_ms, self.repeat_ms
            )
            self.input.buttons.os.right.repeat_enable(
                self.repeat_wait_ms, self.repeat_ms
            )

    def think(self, ins: InputState, delta_ms: int) -> None:
        self.input.think(ins, delta_ms)
        os_button = self.input.buttons.os
        event = True
        if os_button.left.pressed:
            audio.adjust_volume_dB(-settings.num_volume_step_db.value)
        elif os_button.right.pressed:
            audio.adjust_volume_dB(settings.num_volume_step_db.value)
        elif os_button.left.repeated:
            audio.adjust_volume_dB(-settings.num_volume_repeat_step_db.value)
        elif os_button.right.repeated:
            audio.adjust_volume_dB(settings.num_volume_repeat_step_db.value)
        else:
            event = False
        if event:
            self._update_repeat()


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
