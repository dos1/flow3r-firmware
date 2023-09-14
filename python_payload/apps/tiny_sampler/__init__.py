import bl00mbox
import captouch
import leds
import audio

from st3m.application import Application, ApplicationContext
from st3m.input import InputState
from st3m.goose import Tuple, Iterator, Optional, Callable, List, Any, TYPE_CHECKING
from ctx import Context
from st3m.ui.view import View, ViewManager


class TinySampler(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)
        self.blm = bl00mbox.Channel("tiny sampler")

        self.samplers: List[bl00mbox.patches._Patch | Any] = [None] * 5
        self.line_in = self.blm.new(bl00mbox.plugins.bl00mbox_line_in)
        self.blm.volume = (
            30000  # TODO: increase onboard mic line in gain and remove this
        )
        self.line_in.signals.gain = 30000
        for i in range(5):
            self.samplers[i] = self.blm.new(bl00mbox.patches.sampler, 1000)
            self.samplers[i].signals.output = self.blm.mixer
            self.samplers[i].signals.rec_in = self.line_in.signals.right
        self.is_recording = [False] * 5
        audio.input_set_source(audio.INPUT_SOURCE_ONBOARD_MIC)

        self.ct_prev = captouch.read()

    def _highlight_petal(self, num: int, r: int, g: int, b: int) -> None:
        for i in range(7):
            leds.set_rgb((4 * num - i + 3) % 40, r, g, b)

    def draw(self, ctx: Context) -> None:
        dist = 90
        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()

        ctx.font = ctx.get_font_name(0)
        ctx.text_align = ctx.MIDDLE
        ctx.font_size = 24
        for i in range(5):
            ctx.rgb(0.8, 0.8, 0.8)
            ctx.move_to(0, -dist)
            ctx.text("play" + str(i))
            ctx.move_to(0, 0)
            ctx.rotate(6.28 / 10)
            ctx.move_to(0, -dist)
            if self.is_recording[i]:
                ctx.rgb(1, 0, 0)
            ctx.text("rec" + str(i))
            ctx.move_to(0, 0)
            ctx.rotate(6.28 / 10)

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        leds.set_all_rgb(0, 0, 0)
        for i in range(5):
            if self.is_recording[i]:
                self._highlight_petal(i * 2, 255, 0, 0)
            else:
                self._highlight_petal(i * 2, 0, 255, 0)
        leds.update()

        ct = captouch.read()
        for i in range(5):
            if ct.petals[i * 2].pressed and not self.ct_prev.petals[i * 2].pressed:
                if not self.is_recording[i]:
                    self.samplers[i].signals.trigger.start()

        for i in range(5):
            if (
                ct.petals[(i * 2) + 1].pressed
                and not self.ct_prev.petals[(i * 2) + 1].pressed
            ):
                if not self.is_recording[i]:
                    self.samplers[i].signals.rec_trigger.start()
                    self.is_recording[i] = True
            if (
                not ct.petals[(i * 2) + 1].pressed
                and self.ct_prev.petals[(i * 2) + 1].pressed
            ):
                if self.is_recording[i]:
                    self.samplers[i].signals.rec_trigger.stop()
                    self.is_recording[i] = False

        self.ct_prev = ct

    def on_enter(self, vm) -> None:
        super().on_enter(vm)
        self.blm.foreground = True

    def on_exit(self) -> None:
        super().on_exit()
        for i in range(5):
            if self.is_recording[i]:
                self.samplers[i].signals.rec_trigger.stop()
                self.is_recording[i] = False
        audio.input_set_source(audio.INPUT_SOURCE_NONE)
        self.blm.foreground = False
