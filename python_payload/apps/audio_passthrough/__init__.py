from st3m.application import Application, ApplicationContext
from st3m.input import InputState
from st3m.goose import Optional
from st3m.ui.view import ViewManager
from ctx import Context
import audio
import math


# Assume this is an enum
ForceModes = ["AUTO", "FORCE_LINE_IN", "FORCE_LINE_OUT", "FORCE_MIC"]


STATE_TEXT: dict[int, str] = {
    audio.INPUT_SOURCE_AUTO: "auto",
    audio.INPUT_SOURCE_HEADSET_MIC: "headset mic",
    audio.INPUT_SOURCE_LINE_IN: "line in",
    audio.INPUT_SOURCE_ONBOARD_MIC: "onboard mic",
}


class AudioPassthrough(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)
        self._button_0_pressed = False
        self._button_5_pressed = False
        self._force_mode: str = "AUTO"
        self._mute = True
        self._source = None
        self.target_source = audio.INPUT_SOURCE_AUTO

    def on_enter(self, vm: Optional[ViewManager]) -> None:
        super().on_enter(vm)
        self._force_mode = "AUTO"

    def draw(self, ctx: Context) -> None:
        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE
        ctx.font = ctx.get_font_name(8)

        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
        ctx.rgb(1, 1, 1)

        # top button
        ctx.move_to(105, 0)
        ctx.font_size = 15
        ctx.save()
        ctx.rotate((math.pi / 180) * 270)
        ctx.text(">")
        ctx.restore()

        ctx.move_to(0, -90)
        ctx.text("toggle passthrough")

        # middle text
        ctx.font_size = 25
        ctx.move_to(0, 0)
        ctx.save()
        if self._mute:
            # 0xff4500, red
            ctx.rgb(1, 0.41, 0)
        else:
            # 0x3cb043, green
            ctx.rgb(0.24, 0.69, 0.26)
        ctx.text("passthrough off" if self._mute else "passthrough on")
        ctx.restore()

        # bottom text
        ctx.move_to(0, 25)
        ctx.save()
        ctx.font_size = 15
        ctx.text(STATE_TEXT.get(self.target_source, ""))

        ctx.move_to(0, 40)
        if self.source_connected:
            # 0x3cb043, green
            ctx.rgb(0.24, 0.69, 0.26)
        else:
            # 0xff4500, red
            ctx.rgb(1, 0.41, 0)
        if self._mute:
            ctx.text("standby")
        elif self._force_mode == "AUTO":
            src = audio.input_thru_get_source()
            if src != audio.INPUT_SOURCE_NONE:
                ctx.text("connected to")
                ctx.move_to(0, 56)
                ctx.text(STATE_TEXT.get(src, ""))
            else:
                ctx.text("waiting...")
        elif self._force_mode == "FORCE_MIC":
            ctx.text("connected" if self.source_connected else "(headphones only)")
        else:
            ctx.text("connected" if self.source_connected else "waiting...")
        ctx.restore()

        # bottom button
        ctx.move_to(105, 0)
        ctx.font_size = 15
        ctx.save()
        ctx.rotate((math.pi / 180) * 90)
        ctx.text(">")
        ctx.restore()

        ctx.move_to(0, 90)
        ctx.text("next source")

    @property
    def source_connected(self):
        if self.source != audio.INPUT_SOURCE_NONE:
            return self.source == audio.input_thru_get_source()
        else:
            return False

    @property
    def source(self):
        if self._source is None:
            self._source = audio.input_thru_get_source()
        return self._source

    @source.setter
    def source(self, source):
        audio.input_thru_set_source(source)
        self._source = audio.input_thru_get_source()

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        if ins.captouch.petals[0].pressed:
            if not self._button_0_pressed:
                self._button_0_pressed = True
                self._mute = not self._mute
        else:
            self._button_0_pressed = False

        if ins.captouch.petals[5].pressed:
            if not self._button_5_pressed:
                self._button_5_pressed = True
                index = ForceModes.index(self._force_mode)
                index = (index + 1) % 4
                self._force_mode = ForceModes[index]

        else:
            self._button_5_pressed = False

        if self._mute:
            self.source = audio.INPUT_SOURCE_NONE
        else:
            if self._force_mode == "FORCE_MIC":
                self.target_source = audio.INPUT_SOURCE_ONBOARD_MIC
            elif self._force_mode == "AUTO":
                self.target_source = audio.INPUT_SOURCE_AUTO
            elif self._force_mode == "FORCE_LINE_IN":
                self.target_source = audio.INPUT_SOURCE_LINE_IN
            elif self._force_mode == "FORCE_LINE_OUT":
                self.target_source = audio.INPUT_SOURCE_HEADSET_MIC
            self.source = self.target_source


# For running with `mpremote run`:
if __name__ == "__main__":
    import st3m.run

    st3m.run.run_app(AudioPassthrough)
