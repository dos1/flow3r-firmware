from st3m.input import InputController, InputState
from st3m.goose import Optional
from st3m.ui import colours
import urequests
import gzip
from utarfile import TarFile, DIRTYPE
import io
import os
from st3m.ui.view import BaseView
from ctx import Context


class DownloadView(BaseView):
    response: Optional[urequests.Response]

    """
    View state

    1 = Init
    2 = Fetching
    3 = Extracting
    4 = Extracting
    5 = Done
    """
    state: int

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
        if self._state == 1 or self._state == 2:
            # Fetching
            ctx.rgb(*colours.WHITE)
            ctx.font = "Camp Font 3"
            ctx.font_size = 24
            ctx.text_align = ctx.CENTER
            ctx.text_baseline = ctx.MIDDLE
            ctx.text("Downloading...")

            self._state = 2
        elif self._state == 3 or self._state == 4:
            # Extracting
            ctx.rgb(*colours.WHITE)
            ctx.font = "Camp Font 3"
            ctx.font_size = 24
            ctx.text_align = ctx.CENTER
            ctx.text_baseline = ctx.MIDDLE
            ctx.text("Extracting...")

            self._state = 4
        elif self._state == 5:
            # Done
            ctx.move_to(0, -30)

            ctx.rgb(*colours.WHITE)
            ctx.font = "Camp Font 3"
            ctx.font_size = 24
            ctx.text_align = ctx.CENTER
            ctx.text_baseline = ctx.MIDDLE
            ctx.text("All done!")

            ctx.move_to(0, 0)
            ctx.text("The app will be")

            ctx.move_to(0, 30)
            ctx.text("available after reboot")

        ctx.restore()

    def think(self, ins: InputState, delta_ms: int) -> None:
        # super().think(ins, delta_ms)  # Let BaseView do its thing
        self.input.think(ins, delta_ms)

        if self._state == 2:
            try:
                print("Getting it")
                self.response = urequests.get(self._url)

                if self.response is not None and self.response.content is not None:
                    print("Got something")
                    self._state = 3
                    return
                print("no content...")
            except:
                print("Exception")
            print("Next try")
            self._try += 1

        elif self._state == 4:
            if self.response is None:
                raise RuntimeError("response is None")

            tar = gzip.decompress(self.response.content)
            self.response = None
            t = TarFile(fileobj=io.BytesIO(tar))
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
