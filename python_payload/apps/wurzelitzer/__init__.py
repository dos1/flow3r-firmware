import st3m.run, media, os, ctx
from st3m.application import Application

RADIOSTATIONS = [
    "http://radio-paralax.de:8000/",
    "http://stream6.jungletrain.net:8000/",
    "http://air.doscast.com:8054/livehitradio",
    "http://lyd.nrk.no/nrk_radio_jazz_mp3_l",
    "http://lyd.nrk.no/nrk_radio_mp3_mp3_l",
    # "http://lyd.nrk.no/nrk_radio_alltid_nyheter_mp3_l",
    # "http://pippin.gimp.org/tmp/b71207f10d522d354a001768e21a78fe"
]


class App(Application):
    def __init__(self, app_ctx):
        super().__init__(app_ctx)
        self._streams = RADIOSTATIONS.copy()
        for entry in os.ilistdir("/sd/"):
            if entry[1] == 0x8000:
                if (
                    entry[0].endswith(".mp3")
                    or entry[0].endswith(".mod")
                    or entry[0].endswith(".mpg")
                ):
                    self._streams.append("/sd/" + entry[0])
        if len(self._streams) > len(RADIOSTATIONS):
            # skip radio stations, they are available by going back
            self._stream_no = len(RADIOSTATIONS)
        else:
            self._stream_no = 0

    def load_stream(self):
        media.stop()
        self._filename = self._streams[self._stream_no]
        print("loading " + self._filename)
        media.load(self._filename)

    def think(self, ins, delta_ms):
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
        if self.input.buttons.app.middle.pressed:
            if media.is_playing():
                media.pause()
            else:
                media.play()
        media.think(delta_ms)

    def draw(self, ctx):
        media.draw(ctx)

    def on_enter(self, vm):
        super().on_enter(vm)
        self.load_stream()

    def on_exit(self):
        media.stop()


if __name__ == "__main__":
    st3m.run.run_app(App)
