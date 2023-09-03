from st3m.input import InputController, InputState
from st3m.goose import Optional
from st3m.ui import colours
from urllib.urequest import urlopen
import gzip
from utarfile import TarFile, DIRTYPE
import io
import os
import gc
from st3m.ui.view import BaseView
from ctx import Context


class DownloadView(BaseView):
    response: Optional[bytes]
    error_message: str = ""

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

    input: InputController

    def __init__(self, url: str) -> None:
        super().__init__()
        self._state = 1
        self._try = 1
        self._url = url

        self.input = InputController()

    def draw(self, ctx: Context) -> None:
        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()

        ctx.save()
        ctx.move_to(0, 0)
        ctx.rgb(*colours.WHITE)
        ctx.font = "Camp Font 3"
        ctx.font_size = 24
        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE

        text_to_draw = ""
        if self._state == 1 or self._state == 2:
            # Fetching
            text_to_draw = "Downloading..."
            self._state = 2
        elif self._state == 3 or self._state == 4:
            # Extracting
            text_to_draw = "Extracting..."
            self._state = 4
        elif self._state == 5:
            # Done
            ctx.move_to(0, -30)
            ctx.text("All done...")
            text_to_draw = "The app will be\navailable after reboot"
        elif self._state == 6:
            # Errored
            ctx.move_to(0, -30)
            ctx.text("Oops...")
            text_to_draw = self.error_message

        y_offset = 0
        for line in text_to_draw.split("\n"):
            ctx.move_to(0, y_offset)
            ctx.text(line)
            y_offset += 30

        ctx.restore()

    def download_file(self, url: str, block_size=40960) -> Optional[bool]:
        gc.collect()
        req = urlopen(url)
        self.response = b""

        try:
            while True:
                new_data = req.read(block_size)
                self.response += new_data
                if len(new_data) < block_size:
                    break
        finally:
            req.close()
        return True

    def think(self, ins: InputState, delta_ms: int) -> None:
        # super().think(ins, delta_ms)  # Let BaseView do its thing
        self.input.think(ins, delta_ms)

        if self._state == 2:
            fail_reason = ""
            try:
                print("Getting it")
                download_result = self.download_file(self._url)

                if download_result and self.response is not None:
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

            for i in t:
                print(i.name)

                if not os.path.exists("/flash/apps/"):
                    print("making /flash/apps/")
                    os.mkdir("/flash/apps/")

                if i.type == DIRTYPE:
                    print("dirtype")
                    dirname = "/flash/apps/" + i.name
                    if not os.path.exists(dirname):
                        print("making", dirname)
                        os.mkdir(dirname)
                    else:
                        print("dir", dirname, "exists")
                else:
                    filename = "/flash/apps/" + i.name
                    print("writing to", filename)
                    f = t.extractfile(i)
                    with open(filename, "wb") as of:
                        of.write(f.read())
            self._state = 5

        if self.input.buttons.app.middle.pressed:
            if self.vm is None:
                raise RuntimeError("vm is None")
            self.vm.pop()
