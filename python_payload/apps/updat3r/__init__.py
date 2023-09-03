from st3m.application import Application, ApplicationContext
from st3m.input import InputState
from st3m.goose import Optional
from ctx import Context
import os
import sys_kernel
import urequests
from st3m.ui.view import ViewManager
import st3m.wifi
from urllib.urequest import urlopen

# from .helpers import sd_card_plugged


def sd_card_plugged() -> bool:
    try:
        os.listdir("/sd")
        return True
    except OSError:
        # OSError: [Errno 19] ENODEV
        return False


class UpdaterApp(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)

    def draw(self, ctx: Context) -> None:
        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE
        ctx.font = ctx.get_font_name(1)

        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
        ctx.rgb(1, 1, 1)

        ctx.font_size = 15
        ctx.move_to(0, -90)
        ctx.text("you are running...")
        ctx.font_size = 25
        ctx.move_to(0, -70)
        ctx.text(self._firmware_version)

        ctx.font_size = 15
        state_lines = self._state_text.split("\n")
        y_offset = max(-((len(state_lines) - 1) * 15 / 2), -30)
        for line in self._state_text.split("\n"):
            ctx.move_to(0, y_offset)
            ctx.text(line)
            y_offset += 15

    def on_enter(self, vm: Optional[ViewManager]):
        super().on_enter(vm)
        print(self._wifi_preference)
        self._firmware_version = sys_kernel.firmware_version()
        self.latest_version = None
        self.download_instance = None
        self.filename = None

        st3m.wifi.setup_wifi()
        self.wlan_instance = st3m.wifi.iface
        print(self.wlan_instance.active(), self.wlan_instance.isconnected())

        self._sd_present = sd_card_plugged()
        self._sd_failed = False

        if self._sd_present:
            self._state_text = "getting latest version..."
        else:
            self._state_text = "no SD card detected!\n\nif you have one in there\nturn off and on flow3r power (ha)\nthen try to reattempt\ndownloading the update"

    def on_exit(self) -> None:
        super().on_exit()

    def download_file(self, url: str, path: str, block_size=40960) -> None:
        # start by removing the file
        path_fd = None

        try:
            if os.path.exists(path):
                os.remove(path)

            path_fd = open(path, "wb")
        except OSError as e:
            if "EIO" in str(e):
                self._sd_failed = True
            return

        req = urlopen(url)

        try:
            while True:
                new_data = req.read(block_size)
                path_fd.write(new_data)
                yield path_fd.tell()
                if len(new_data) < block_size:
                    break
        finally:
            path_fd.close()
            req.close()

    def download_file_no_yield(self, url: str, path: str, block_size=10240) -> None:
        # start by deleting the file
        if os.path.exists(path):
            os.delete(path)

        req = urlopen(url)
        print("opened url")
        path_fd = open(path, "wb")
        print("opened file")
        while True:
            new_data = req.read(block_size)
            path_fd.write(new_data)
            print(path_fd.tell())
            if len(new_data) < block_size:
                break
        path_fd.close()
        print("closed file")
        req.close()
        print("closed req")

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        # TODO: version comparison
        # TODO: verify hash

        if self._sd_failed:
            self._state_text = "don't panic, but...\na weird SD bug happened D:\nturn off and on flow3r power (ha)\nthen try to reattempt\ndownloading the update\n\nyou got this."
            return

        if not self._sd_present:
            return

        if self.latest_version is None and self.wlan_instance.isconnected():
            req = urequests.get("https://flow3r.garden/api/releases.json")
            self.latest_version = req.json()[0]
            req.close()
            self._state_text = f"latest version: {self.latest_version['name']}\n\npress app shoulder button\nto start downloading"

        if self.download_instance is not None:
            try:
                download_state = next(self.download_instance)
                self._state_text = (
                    f"downloading...\nDON'T TURN OFF YOUR BADGE!\n\n{download_state}b"
                )
            except StopIteration:
                self.download_instance = None
                self._state_text = f'downloaded to\n{self.filename}\n\nhold right shoulder when\nbooting then pick\n"Flash Firmware Image"\nto continue'

        if self.latest_version and self.input.buttons.app.middle.pressed:
            self._state_text = "downloading...\nDON'T TURN OFF YOUR BADGE!\n\n"
            url = ""
            for partition in self.latest_version["partitions"]:
                if partition["name"] == "flow3r":
                    url = partition["url"]
                    break

            self.filename = f"/sd/flow3r_{self.latest_version['tag']}.bin"

            self.download_instance = self.download_file(url, self.filename)


# For running with `mpremote run`:
if __name__ == "__main__":
    import st3m.run

    st3m.run.run_view(UpdaterApp(ApplicationContext()))
