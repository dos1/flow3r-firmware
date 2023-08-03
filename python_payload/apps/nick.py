from st4m.application import Application
from st4m.property import PUSH_RED, GO_GREEN, BLACK
from st4m.goose import Dict, Any
from st4m.ui.ctx import Ctx
from st4m.input import InputState
import leds

import json


class Configuration:
    def __init__(self) -> None:
        self.name = "flow3r"
        self.size: int = 75
        self.font: int = 5

    @classmethod
    def load(cls, path: str) -> "Configuration":
        res = cls()
        try:
            with open(path) as f:
                jsondata = f.read()
            data = json.loads(jsondata)
        except OSError:
            data = {}
        if "name" in data and type(data["name"]) == str:
            res.name = data["name"]
        if "size" in data:
            if type(data["size"]) == float:
                res.size = int(data["size"])
            if type(data["size"]) == int:
                res.size = data["size"]
        if "font" in data and type(data["font"]) == int:
            res.font = data["font"]
        return res

    def save(self, path: str) -> None:
        d = {
            "name": self.name,
            "size": self.size,
            "font": self.font,
        }
        jsondata = json.dumps(d)
        with open(path, "w") as f:
            f.write(jsondata)
            f.close()


class NickApp(Application):
    def __init__(self, name: str) -> None:
        super().__init__(name)
        self._scale = 1.0
        self._dir = 1
        self._led = 0.0
        self._filename = "/flash/nick.json"
        self._config = Configuration.load(self._filename)

    def draw(self, ctx: Ctx) -> None:
        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE
        ctx.font_size = self._config.size
        ctx.font = ctx.get_font_name(self._config.font)

        # TODO (q3k) bug: we have to do this, otherwise we have horrible blinking
        ctx.rgb(1, 1, 1)

        # TODO (q3k) bug: we have to do this two times, otherwise we have horrible blinking
        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
        ctx.rgb(*GO_GREEN)

        ctx.move_to(0, 0)
        ctx.save()
        ctx.scale(self._scale, 1)
        ctx.text(self._config.name)
        ctx.restore()

        leds.set_hsv(int(self._led), abs(self._scale) * 360, 1, 0.2)

        leds.update()
        # ctx.fill()

    def on_exit(self) -> None:
        self._config.save(self._filename)

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        self._scale += delta_ms / 1000 * self._dir

        if abs(self._scale) > 1 and self._scale * self._dir > 0:
            self._dir *= -1

        self._led += delta_ms / 45
        if self._led >= 40:
            self._led = 0


app = NickApp("nick")
