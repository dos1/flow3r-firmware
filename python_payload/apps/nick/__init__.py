from st3m.application import Application, ApplicationContext
from st3m.goose import Tuple, Any
from st3m.input import InputState
from ctx import Context
import leds
import json
import math

CONFIG_SCHEMA: dict[str, dict[str, Any]] = {
    "name": {"types": [str]},
    "size": {"types": [int, float], "cast_to": int},
    "font": {"types": [int, float], "cast_to": int},
    "pronouns": {"types": ["list_of_str"]},
    "pronouns_size": {"types": [int, float], "cast_to": int},
    "color": {"types": ["hex_color"]},
    "mode": {"types": [int, float], "cast_to": int},
}


class Configuration:
    def __init__(self) -> None:
        self.name = "flow3r"
        self.size: int = 75
        self.font: int = 5
        self.pronouns: list[str] = []
        self.pronouns_size: int = 25
        self.color = "0x40ff22"
        self.mode = 0
        self.config_errors: list[str] = []
        self.config_loaded: bool = False

    @classmethod
    def load(cls, path: str) -> "Configuration":
        res = cls()
        try:
            with open(path) as f:
                jsondata = f.read()
            data = json.loads(jsondata)
            res.config_loaded = True
        except OSError:
            data = {}
        except ValueError:
            res.config_errors = ["nick.json decode failed!"]
            data = {}

        # verify the config format and generate an error message
        config_type_errors: list[str] = []
        for config_key, type_data in CONFIG_SCHEMA.items():
            if config_key not in data.keys():
                continue
            key_type_valid = False
            for allowed_type in type_data["types"]:
                if isinstance(allowed_type, type):
                    if isinstance(data[config_key], allowed_type):
                        key_type_valid = True
                        break
                elif allowed_type == "list_of_str":
                    if isinstance(data[config_key], list) and (
                        len(data[config_key]) == 0
                        or set([type(x) for x in data[config_key]]) == {str}
                    ):
                        key_type_valid = True
                        break
                elif allowed_type == "hex_color":
                    if (
                        isinstance(data[config_key], str)
                        and data[config_key][0:2] == "0x"
                        and len(data[config_key]) == 8
                    ):
                        key_type_valid = True
                        break

            if not key_type_valid:
                config_type_errors.append(config_key)
            else:
                # Cast to relevant type if needed
                if type_data.get("cast_to"):
                    data[config_key] = type_data["cast_to"](data[config_key])
                setattr(res, config_key, data[config_key])

        if config_type_errors:
            res.config_errors += [
                "data types wrong",
                "in nick.json for:",
                "",
            ] + config_type_errors

        return res

    def save(self, path: str) -> None:
        d = {
            config_key: getattr(self, config_key) for config_key in CONFIG_SCHEMA.keys()
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
        if not self._config.config_loaded and not self._config.config_errors:
            self._config.save(self._filename)

    def draw(self, ctx: Context) -> None:
        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE
        ctx.font_size = self._config.size
        ctx.font = ctx.get_font_name(self._config.font)

        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
        ctx.rgb(*self._config.to_normalized_tuple())

        if self._config.config_errors:
            draw_y = (-20 * len(self._config.config_errors)) / 2
            ctx.move_to(0, draw_y)
            ctx.font_size = 20
            # 0xff4500, red
            ctx.rgb(1, 0.41, 0)
            ctx.font = ctx.get_font_name(8)
            for config_error in self._config.config_errors:
                draw_y += 20
                ctx.move_to(0, draw_y)
                ctx.text(config_error)
            return

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

    st3m.run.run_app(NickApp)
