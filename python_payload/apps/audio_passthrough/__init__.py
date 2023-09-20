from st3m.application import Application, ApplicationContext
from st3m.input import InputState
from st3m.goose import Optional
from st3m.ui.view import ViewManager
from ctx import Context
import audio
import math


# Assume this is an enum
ForceModes = ["AUTO", "FORCE_LINE_IN", "FORCE_LINE_OUT", "FORCE_MIC", "FORCE_NONE"]


STATE_TEXT: dict[int, str] = {
    audio.INPUT_SOURCE_HEADSET_MIC: "using headset mic (line out)",
    audio.INPUT_SOURCE_LINE_IN: "using line in",
    audio.INPUT_SOURCE_ONBOARD_MIC: "using onboard mic",
    audio.INPUT_SOURCE_NONE: "plug cable to line in/out",
}


class AudioPassthrough(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)
        self._button_0_pressed = False
        self._button_5_pressed = False
        self._force_mode: str = "AUTO"

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
        if audio.input_thru_get_mute():
            # 0xff4500, red
            ctx.rgb(1, 0.41, 0)
        else:
            # 0x3cb043, green
            ctx.rgb(0.24, 0.69, 0.26)
        ctx.text("passthrough off" if audio.input_thru_get_mute() else "passthrough on")
        ctx.restore()

        # bottom text
        ctx.move_to(0, 25)
        ctx.save()
        ctx.font_size = 15
        ctx.text(STATE_TEXT.get(audio.input_get_source(), ""))

        # have red text when sleep mode isn't auto
        if self._force_mode != "AUTO":
            # 0xff4500, red
            ctx.rgb(1, 0.41, 0)

        ctx.move_to(0, 40)
        ctx.text("(auto)" if self._force_mode == "AUTO" else "(forced)")

        # mic has a loopback risk so has precautions
        # so we warn users about it to not confuse them
        if self._force_mode == "FORCE_MIC":
            ctx.move_to(0, 55)
            ctx.text("headphones only")
            ctx.move_to(0, 70)
            ctx.text("will not persist app exit")
        ctx.restore()

        # bottom button
        ctx.move_to(105, 0)
        ctx.font_size = 15
        ctx.save()
        ctx.rotate((math.pi / 180) * 90)
        ctx.text(">")
        ctx.restore()

        ctx.move_to(0, 90)
        ctx.text("force line in/out")

    def on_exit(self) -> None:
        # Mic passthrough has a loopback risk
        if self._force_mode == "FORCE_MIC":
            self._force_mode = "FORCE_NONE"
            audio.input_set_source(audio.INPUT_SOURCE_NONE)
            audio.input_thru_set_mute(True)

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        headset_connected = audio.headset_is_connected()
        if self._force_mode == "FORCE_MIC":
            audio.input_set_source(audio.INPUT_SOURCE_ONBOARD_MIC)
        elif (
            audio.line_in_is_connected() and self._force_mode == "AUTO"
        ) or self._force_mode == "FORCE_LINE_IN":
            audio.input_set_source(audio.INPUT_SOURCE_LINE_IN)
        elif headset_connected or self._force_mode == "FORCE_LINE_OUT":
            audio.input_set_source(audio.INPUT_SOURCE_HEADSET_MIC)
        else:
            audio.input_set_source(audio.INPUT_SOURCE_NONE)

        if ins.captouch.petals[0].pressed:
            if not self._button_0_pressed:
                self._button_0_pressed = True
                audio.input_thru_set_mute(not audio.input_thru_get_mute())
        else:
            self._button_0_pressed = False

        if ins.captouch.petals[5].pressed:
            if not self._button_5_pressed:
                self._button_5_pressed = True
                self._force_mode = ForceModes[ForceModes.index(self._force_mode) + 1]
                if ForceModes.index(self._force_mode) >= ForceModes.index("FORCE_NONE"):
                    self._force_mode = "AUTO"
        else:
            self._button_5_pressed = False

        if self._force_mode == "FORCE_MIC" and not audio.headphones_are_connected():
            self._force_mode = "AUTO"


# For running with `mpremote run`:
if __name__ == "__main__":
    import st3m.run

    st3m.run.run_view(AudioPassthrough(ApplicationContext()))
