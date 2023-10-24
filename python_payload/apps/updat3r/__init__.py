from st3m.application import Application, ApplicationContext
from st3m.input import InputState
from st3m.goose import Optional
from st3m.utils import sd_card_plugged
from ctx import Context
import sys_kernel
import urequests
import math
import sys
from st3m.ui.view import ViewManager
import st3m.wifi


class UpdaterApp(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)

    def draw(self, ctx: Context) -> None:
        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE
        ctx.line_width = 3
        ctx.font = ctx.get_font_name(1)

        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()

        ctx.rgb(1, 1, 1)
        if self.download_percentage:
            ctx.save()
            ctx.rotate(270 * math.pi / 180)
            ctx.arc(
                0, 0, 117, math.tau * (1 - self.download_percentage), math.tau, 0
            ).stroke()
            ctx.restore()

        ctx.font_size = 15
        ctx.move_to(0, -90)
        ctx.text("you are running")
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

        ctx.gray(0.75)
        if (
            not st3m.wifi.is_connected()
            and not st3m.wifi.is_connecting()
            and not self.fetched_version
            and not self.vm.transitioning
        ):
            ctx.move_to(0, 40)
            ctx.text("press the app button to")
            ctx.move_to(0, 55)
            ctx.text("enter Wi-Fi settings")

    def version_to_number(self, version: str):
        if "dev" in version:
            return 0
        major, minor, patch = version.split(".")
        version_number = (int(major) * 1000000) + (int(major) * 1000) + int(patch)
        return version_number

    def on_enter(self, vm: Optional[ViewManager]):
        super().on_enter(vm)
        self._firmware_version = sys_kernel.firmware_version()
        self._firmware_version_number = self.version_to_number(self._firmware_version)
        self.fetched_version = []
        self.selected_version = None
        self.download_instance = None
        self.filename = None
        self.use_dev_version = False
        self.download_percentage = 0

        self._download_error = False

        self._sd_present = sd_card_plugged()
        self._sd_failed = False

        if self._sd_present:
            self._state_text = "getting latest version..."

            if not st3m.wifi.enabled() or (
                not st3m.wifi.is_connected() and not st3m.wifi.is_connecting()
            ):
                self._state_text = "no connection"
        else:
            self._state_text = "no SD card detected!\n\nif you have one in there\nturn off and on flow3r power (ha)\nthen try to reattempt\ndownloading the update"

    def on_exit(self) -> None:
        super().on_exit()

    def download_file(self, url: str, path: str, block_size=40960) -> None:
        path_fd = None

        try:
            path_fd = open(path, "wb")
        except OSError as e:
            if "EIO" in str(e):
                self._sd_failed = True
            return

        req = urequests.get(url)
        total_size = int(req.headers["Content-Length"])

        try:
            while True:
                new_data = req.raw.read(block_size)
                path_fd.write(new_data)
                yield path_fd.tell(), total_size
                if len(new_data) < block_size:
                    break
        finally:
            path_fd.close()
            req.close()

    def download_file_no_yield(self, url: str, path: str, block_size=10240) -> None:
        req = urequests.get(url)
        print("opened url")
        path_fd = open(path, "wb")
        print("opened file")
        while True:
            new_data = req.raw.read(block_size)
            path_fd.write(new_data)
            print(path_fd.tell())
            if len(new_data) < block_size:
                break
        path_fd.close()
        print("closed file")
        req.close()
        print("closed req")

    def change_selected_version(self):
        if self.fetched_version and self.use_dev_version:
            self.selected_version = self.fetched_version[-1]

            self._state_text = "latest dev build\n\npress app shoulder button\nto start downloading\n\n(tilt shoulder left to\nswitch to latest version)"
        elif self.fetched_version:
            self.selected_version = self.fetched_version[0]
            latest_version_number = self.version_to_number(
                self.selected_version["tag"].replace("v", "")
            )
            self._state_text = f"latest version: {self.selected_version['name']}"
            if latest_version_number > self._firmware_version_number:
                self._state_text += (
                    "\n\npress app shoulder button\nto start downloading"
                )
            else:
                self._state_text += "\n\nyou are up to date :)"

            self._state_text += "\n\n(tilt shoulder right to\nswitch to dev version)"

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        # TODO: verify hash
        # TODO: download percentage

        if self.input.buttons.app.right.pressed:
            self.use_dev_version = True
            self.change_selected_version()
        elif self.input.buttons.app.left.pressed:
            self.use_dev_version = False
            self.change_selected_version()

        if not self._sd_present:
            return

        if self._sd_failed:
            self._state_text = "don't panic, but...\na weird SD bug happened D:\nturn off and on flow3r power (ha)\nthen try to reattempt\ndownloading the update\n\nyou got this."
            return

        if not st3m.wifi.is_connected() and not st3m.wifi.is_connecting():
            if self.input.buttons.app.middle.pressed:
                st3m.wifi.run_wifi_settings(self.vm)
            return

        if (
            not self.fetched_version
            and st3m.wifi.is_connected()
            and not self.vm.transitioning
            and not self._download_error
        ):
            try:
                req = urequests.get("https://flow3r.garden/api/releases.json")
                self.fetched_version = req.json()
                req.close()
                self.change_selected_version()
            except Exception as e:
                self._state_text = "download error :("
                self._download_error = True
                sys.print_exception(e)

        if self.download_instance is not None:
            try:
                download_state, total_size = next(self.download_instance)
                self.download_percentage = download_state / total_size
                self._state_text = f"downloading...\nDON'T TURN OFF YOUR BADGE!\n\n{download_state}/{total_size}b"
            except StopIteration:
                self.download_instance = None
                self.download_percentage = 0
                self._state_text = f'downloaded to\n{self.filename}\n\nhold right shoulder when\nbooting then pick\n"Flash Firmware Image"\nto continue'

        if self.selected_version and self.input.buttons.app.middle.pressed:
            self._state_text = "downloading...\nDON'T TURN OFF YOUR BADGE!\n\n"
            url = ""
            for partition in self.selected_version["partitions"]:
                if partition["name"] == "flow3r":
                    url = partition["url"]
                    break

            self.filename = f"/sd/flow3r_{self.selected_version['tag']}.bin"

            self.download_instance = self.download_file(url, self.filename)


# For running with `mpremote run`:
if __name__ == "__main__":
    import st3m.run

    st3m.run.run_view(UpdaterApp(ApplicationContext()))
