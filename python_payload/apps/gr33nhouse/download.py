from st3m.input import InputState
from st3m.goose import Optional, List
from st3m.ui import colours
from st3m.utils import sd_card_plugged
import urequests
import gzip
from utarfile import TarFile, DIRTYPE
import io
import os
import gc
import math
from st3m.ui.view import BaseView
from ctx import Context


class DownloadView(BaseView):
    response: Optional[bytes] = b""
    error_message: str = ""
    _download_instance: Optional["download_file"]

    """
    View state

    1 = Init
    2 = Fetching
    3 = Extracting
    4 = Extracting
    5 = Done
    6 = Error
    """
    _state: int

    def __init__(self, url: str) -> None:
        super().__init__()
        self._state = 1
        self._try = 1
        self._url = url
        self.response = b""
        self._download_instance = None
        self.download_percentage = 0

    def _get_app_folder(self, tar_size: int) -> Optional[str]:
        if sd_card_plugged():
            sd_statvfs = os.statvfs("/sd")
            if tar_size < sd_statvfs[1] * sd_statvfs[3]:
                return "/sd/apps/"

        flash_statvfs = os.statvfs("/flash")
        if tar_size < flash_statvfs[1] * flash_statvfs[3]:
            return "/flash/apps/"

        return None

    def draw(self, ctx: Context) -> None:
        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
        ctx.rgb(*colours.WHITE)

        # Arc strokes should be drawn before move_to for some reason to look nice
        if self.download_percentage and self._state == 2:
            ctx.save()
            ctx.line_width = 3
            ctx.rotate(270 * math.pi / 180)
            ctx.arc(
                0, 0, 117, math.tau * (1 - self.download_percentage), math.tau, 0
            ).stroke()
            ctx.restore()

        ctx.save()
        ctx.move_to(0, 0)
        ctx.font = "Camp Font 3"
        ctx.font_size = 24
        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE

        text_to_draw = ""
        if self._state == 1 or self._state == 2:
            # Fetching
            text_to_draw = "Downloading..."
            if self._download_instance:
                text_to_draw += f"\n{len(self.response)}b"
            self._state = 2
        elif self._state == 3 or self._state == 4:
            # Extracting
            text_to_draw = "Extracting..."
            self._state = 4
        elif self._state == 5:
            # Done
            ctx.move_to(0, -30)
            ctx.text("All done...")
            ctx.gray(0.75)
            ctx.font_size = 22
            text_to_draw = "The app will be\navailable after reboot"
        elif self._state == 6:
            # Errored
            ctx.move_to(0, -30)
            ctx.text("Oops...")
            text_to_draw = self.error_message
            ctx.font_size = 12

        y_offset = 0
        for line in text_to_draw.split("\n"):
            ctx.move_to(0, y_offset)
            ctx.text(line)
            y_offset += int(ctx.font_size * 1.25)

        ctx.restore()

    def download_file(self, url: str, block_size=10240) -> List[bytes]:
        gc.collect()
        req = urequests.get(url)
        total_size = int(req.headers["Content-Length"])

        try:
            while True:
                new_data = req.raw.read(block_size)
                yield new_data, total_size
                if len(new_data) < block_size:
                    break
        finally:
            req.close()

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)  # Let BaseView do its thing

        if self.vm.transitioning:
            return

        if self.input.buttons.app.middle.pressed:
            if self.vm is None:
                raise RuntimeError("vm is None")
            self.vm.pop()

        if self._state == 2:
            fail_reason = ""
            try:
                if not self._download_instance:
                    print("Getting it")
                    self.response = b""
                    self._download_instance = self.download_file(self._url)
                    return
                else:
                    try:
                        new_data, total_size = next(self._download_instance)
                        self.download_percentage = len(self.response) / total_size
                        if new_data:
                            self.response += new_data
                        return
                    except StopIteration:
                        self._download_instance = None

                if self.response is not None:
                    print("Got something")
                    self._state = 3
                    return
                fail_reason = "No content"
            except MemoryError:
                gc.collect()
                self.response = None
                self.error_message = "Out of Memory\n(app too big?)"
                self._state = 6
                return
            except Exception as e:
                fail_reason = f"Exception:\n{str(e)}"
            print(fail_reason)
            self._try += 1
            if self._try >= 3:
                self.response = None
                self.error_message = fail_reason
                self._state = 6

        elif self._state == 4:
            if self.response is None:
                raise RuntimeError("response is None")

            try:
                gc.collect()
                tar = gzip.decompress(self.response)
                self.response = None
                gc.collect()
                t = TarFile(fileobj=io.BytesIO(tar))
            except MemoryError:
                gc.collect()
                self.response = None
                self.error_message = "Out of Memory\n(app too big?)"
                self._state = 6
                return

            app_folder = self._get_app_folder(len(tar))
            if not app_folder:
                gc.collect()
                self.response = None
                self.error_message = f"Not Enough Space\nSD/flash lack:\n{len(tar)}b"
                self._state = 6
                return

            if not os.path.exists(app_folder):
                print(f"making {app_folder}")
                os.mkdir(app_folder)

            for i in t:
                print(i.name)

                if i.type == DIRTYPE:
                    print("dirtype")
                    dirname = app_folder + i.name
                    if not os.path.exists(dirname):
                        print("making", dirname)
                        os.mkdir(dirname)
                    else:
                        print("dir", dirname, "exists")
                else:
                    filename = app_folder + i.name
                    print("writing to", filename)
                    f = t.extractfile(i)
                    with open(filename, "wb") as of:
                        while data := f.read():
                            of.write(data)
            self._state = 5
