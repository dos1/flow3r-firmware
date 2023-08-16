from st3m.input import InputState
from st3m.goose import Optional
import urequests
import gzip
from utarfile import TarFile, DIRTYPE
import io
import os
from st3m.ui.view import BaseView
from ctx import Context


class DownloadView(BaseView):
    response: Optional[urequests.Response]

    def __init__(self, url: str) -> None:
        super().__init__()
        self._state = 1
        self._try = 1
        self._url = url

    def draw(self, ctx: Context) -> None:
        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()

        if self._state == 1 or self._state == 2:
            # Fetching
            ctx.rgb(255, 0, 0).rectangle(-20, -20, 40, 40).fill()
            self._state = 2
        elif self._state == 3 or self._state == 4:
            # Extracting
            ctx.rgb(0, 0, 255).rectangle(-20, -20, 40, 40).fill()
            self._state = 4
        elif self._state == 5:
            # Done
            ctx.rgb(0, 255, 0).rectangle(-20, -20, 40, 40).fill()

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)  # Let BaseView do its thing

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
                if i.type == DIRTYPE:
                    print("dirtype")
                    dirname = "/flash/sys/apps/" + i.name
                    if not os.path.exists(dirname):
                        print("making", dirname)
                        os.mkdir(dirname)
                    else:
                        print("dir", dirname, "exists")
                else:
                    filename = "/flash/sys/apps/" + i.name
                    print("writing to", filename)
                    f = t.extractfile(i)
                    with open(filename, "wb") as of:
                        of.write(f.read())
            self._state = 5

            if self.vm is None:
                raise RuntimeError("vm is None")
            self.vm.pop()
