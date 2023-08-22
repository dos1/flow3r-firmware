from st3m.application import Application, ApplicationContext
from st3m.ui.view import ViewManager
from st3m.goose import Optional
from st3m.input import InputState
import st3m.run
import media
import os
from ctx import Context


class JukeBox(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)
        self._streams = [
            "http://radio-paralax.de:8000/",
            "http://stream6.jungletrain.net:8000/",
            "http://air.doscast.com:8054/livehitradio",
            "http://lyd.nrk.no/nrk_radio_jazz_mp3_l",
            "http://lyd.nrk.no/nrk_radio_mp3_mp3_l",
            # "http://lyd.nrk.no/nrk_radio_alltid_nyheter_mp3_l",
            # "http://pippin.gimp.org/tmp/b71207f10d522d354a001768e21a78fe"
        ]
        for entry in os.ilistdir("/sd/"):
            if entry[1] == 0x8000:
                if (
                    entry[0].endswith(".mp3")
                    or entry[0].endswith(".mod")
                    or entry[0].endswith(".mpg")
                ):
                    self._streams.insert(0, "/sd/" + entry[0])
        self._stream_no = 0

    def load_stream(self) -> None:
        media.stop()
        self._filename = self._streams[self._stream_no]
        print("loading " + self._filename)
        media.load(self._filename)

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)
        if self.input.buttons.app.right.pressed:  # or media.get_position() >= 1.0:
            self._stream_no += 1
            if self._stream_no >= len(self._streams):
                self._stream_no = len(self._streams) - 1
            self.load_stream()
        if self.input.buttons.app.left.pressed:
            self._stream_no -= 1
            if self._stream_no < 0:
                self._stream_no = 0
            self.load_stream()
        media.think(delta_ms)

    def draw(self, ctx: Context) -> None:
        media.draw(ctx)

    def on_enter(self, vm: Optional[ViewManager]) -> None:
        super().on_enter(vm)
        self.load_stream()

    def on_exit(self) -> None:
        media.stop()


if __name__ == "__main__":
    st3m.run.run_view(JukeBox(ApplicationContext()))
