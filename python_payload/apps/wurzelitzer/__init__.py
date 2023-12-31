import st3m.run, st3m.wifi, media, uos, ctx
from st3m.application import Application
from st3m.utils import sd_card_plugged

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
        if sd_card_plugged():
            for entry in uos.ilistdir("/sd/"):
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
        self._streaming = False
        self._playing = False
        self._connecting = False
        self._connected = False

    def show_icons(self) -> bool:
        return not media.is_visual() or not media.is_playing()

    def load_stream(self):
        media.stop()
        self._playing = self._streaming = False
        self._filename = self._streams[self._stream_no]
        print("loading " + self._filename)
        if self._filename.startswith("http"):
            self._streaming = True
            if not st3m.wifi.is_connected():
                return
        media.load(self._filename)
        self._playing = True

    def next_source(self):
        self._stream_no += 1
        if self._stream_no >= len(self._streams):
            self._stream_no = len(self._streams) - 1
        self.load_stream()

    def previous_source(self):
        self._stream_no -= 1
        if self._stream_no < 0:
            self._stream_no = 0
        self.load_stream()

    def think(self, ins, delta_ms):
        super().think(ins, delta_ms)
        self._connecting = st3m.wifi.is_connecting()
        self._connected = st3m.wifi.is_connected()

        if self._connected and not self._playing:
            self.load_stream()

        if media.get_position() >= media.get_duration() and media.get_duration() > 0:
            self.next_source()

        if self.input.buttons.app.right.pressed:
            self.right_down_time = 0
        if self.input.buttons.app.right.released:
            if self.right_down_time < 300:
                self.next_source()
        if self.input.buttons.app.right.down:
            self.right_down_time += delta_ms
            if self.right_down_time > 600:
                dur = media.get_duration()
                pos = media.get_position()
                if dur > 1.0:
                    if dur < 300:
                        media.seek((pos / dur) + 0.1)
                    else:
                        media.seek((pos / dur) + 0.05)
                else:
                    media.seek(pos + 0.1)
                self.right_down_time = 300
        if self.input.buttons.app.left.pressed:
            self.left_down_time = 0
        if self.input.buttons.app.left.released:
            if self.left_down_time < 300:
                self.previous_source()
        if self.input.buttons.app.left.down:
            self.left_down_time += delta_ms
            if self.left_down_time > 600:
                dur = media.get_duration()
                pos = media.get_position()
                if dur > 1.0:
                    if dur < 300:
                        media.seek((pos / dur) - 0.1)
                    else:
                        media.seek((pos / dur) - 0.05)
                else:
                    media.seek(pos - 0.1)
                self.left_down_time = 300

        if self.input.buttons.app.middle.pressed:
            if self._streaming and not self._connected:
                st3m.wifi.run_wifi_settings(self.vm)
            elif media.is_playing():
                media.pause()
            else:
                media.play()
        media.think(delta_ms)

    def draw(self, ctx):
        if self._streaming and not self._connected:
            ctx.gray(0)
            ctx.rectangle(-120, -120, 240, 240).fill()
            ctx.gray(1)
            ctx.text_align = ctx.CENTER
            ctx.text_baseline = ctx.MIDDLE
            ctx.font_size = 18
            if st3m.wifi.is_connecting():
                ctx.move_to(0, 0)
                ctx.text("Connecting...")
                return
            ctx.move_to(0, -10)
            ctx.text("Press the app button to")
            ctx.move_to(0, 10)
            ctx.text("enter Wi-Fi settings")
            return

        media.draw(ctx)

    def on_enter(self, vm):
        super().on_enter(vm)
        if not media.is_playing():
            self.load_stream()

    def on_exit(self):
        if media.is_visual() or not media.is_playing():
            media.stop()

    def on_exit_done(self):
        if not st3m.wifi.enabled() and self._streaming:
            media.stop()


if __name__ == "__main__":
    st3m.run.run_app(App)
