from st3m.application import Application, ApplicationContext
from st3m.ui.colours import PUSH_RED, GO_GREEN, BLACK
from st3m.goose import Dict, Any, Tuple
from st3m.input import InputState
from ctx import Context
import leds
import json
import math


class Configuration:
    def __init__(self) -> None:
        self.name = "flow3r"
        self.size: int = 75
        self.font: int = 5
        self.pronouns: list[str] = []
        self.pronouns_size: int = 25
        self.color = "0x40ff22"
        self.mode = 0
        self._write = True

    @classmethod
    def load(cls, path: str) -> "Configuration":
        res = cls()
        try:
            with open(path) as f:
                jsondata = f.read()
            data = json.loads(jsondata)
        except OSError:
            data = {}
        except json.decoder.JSONDecodeError:
            data = {"name": "json error"}
            res._write = False
        if "name" in data and type(data["name"]) == str:
            res.name = data["name"]
        if "size" in data:
            if type(data["size"]) == float:
                res.size = int(data["size"])
            if type(data["size"]) == int:
                res.size = data["size"]
        if "font" in data and type(data["font"]) == int:
            res.font = data["font"]
        # type checks don't look inside collections
        if (
            "pronouns" in data
            and type(data["pronouns"]) == list
            and set([type(x) for x in data["pronouns"]]) == {str}
        ):
            res.pronouns = data["pronouns"]
        if "pronouns_size" in data:
            if type(data["pronouns_size"]) == float:
                res.pronouns_size = int(data["pronouns_size"])
            if type(data["pronouns_size"]) == int:
                res.pronouns_size = data["pronouns_size"]
        if (
            "color" in data
            and type(data["color"]) == str
            and data["color"][0:2] == "0x"
            and len(data["color"]) == 8
        ):
            res.color = data["color"]
        if "mode" in data:
            if type(data["mode"]) == float:
                res.mode = int(data["mode"])
            if type(data["mode"]) == int:
                res.mode = data["mode"]
        return res

    def save(self, path: str) -> None:
        d = {
            "name": self.name,
            "size": self.size,
            "font": self.font,
            "pronouns": self.pronouns,
            "pronouns_size": self.pronouns_size,
            "color": self.color,
            "mode": self.mode,
        }
        jsondata = json.dumps(d)
        with open(path, "w") as f:
            f.write(jsondata)
            f.close()

    def to_normalized_tuple(self) -> Tuple[float, float, float]:
        return (
            int(self.color[2:4], 16) / 255.0,
            int(self.color[4:6], 16) / 255.0,
            int(self.color[6:8], 16) / 255.0,
        )


class NickApp(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)
        self._scale_name = 1.0
        self._scale_pronouns = 1.0
        self._led = 0.0
        self._phase = 0.0
        self._filename = "/flash/nick.json"
        self._config = Configuration.load(self._filename)
        self._pronouns_serialized = " ".join(self._config.pronouns)
        self._angle = 0.0

    def draw(self, ctx: Context) -> None:
        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE
        ctx.font_size = self._config.size
        ctx.font = ctx.get_font_name(self._config.font)

        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
        ctx.rgb(*self._config.to_normalized_tuple())

        ctx.move_to(0, 0)
        ctx.save()
        if self._config.mode == 0:
            ctx.scale(self._scale_name, 1)
        elif self._config.mode == 1:
            ctx.rotate(self._angle)
        ctx.text(self._config.name)
        ctx.restore()

        if self._pronouns_serialized:
            ctx.move_to(0, -60)
            ctx.font_size = self._config.pronouns_size
            ctx.save()
            if self._config.mode == 0:
                ctx.scale(self._scale_pronouns, 1)
            elif self._config.mode == 1:
                ctx.rotate(self._angle)
            ctx.text(self._pronouns_serialized)
            ctx.restore()

        leds.set_hsv(int(self._led), abs(self._scale_name) * 360, 1, 0.2)

        leds.update()
        # ctx.fill()

    def on_exit(self) -> None:
        if self._config._write:
            self._config.save(self._filename)

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        self._phase += delta_ms / 1000
        self._scale_name = math.sin(self._phase)
        self._scale_pronouns = math.cos(self._phase)

        iy = ins.imu.acc[0] * delta_ms / 10.0
        ix = ins.imu.acc[1] * delta_ms / 10.0
        ang = math.atan2(ix, iy)
        d_ang = self._angle + (ang + math.pi / 8 * math.sin(self._phase))
        self._angle -= d_ang / 20

        self._led += delta_ms / 45
        if self._led >= 40:
            self._led = 0


# For running with `mpremote run`:
if __name__ == "__main__":
    import st3m.run

    st3m.run.run_view(NickApp(ApplicationContext()))
