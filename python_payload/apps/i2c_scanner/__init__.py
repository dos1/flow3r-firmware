# micropython imports
from machine import I2C, Pin

# flow3r imports
from st3m.application import Application, ApplicationContext
from ctx import Context
from st3m.input import InputController, InputState


class I2CScanner(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)
        self.input = InputController()
        self.qwiic = I2C(1, freq=400000)
        self._pending_scan = False

    def scan(self):
        self._devices = self.qwiic.scan()
        print("Found Devices:")
        print(self._devices)

    def on_enter(self, vm):
        super().on_enter(vm)
        self._devices = None

    def draw(self, ctx: Context) -> None:
        # Get the default font
        ctx.font = ctx.get_font_name(1)
        # Draw a black background
        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
        ctx.text_align = ctx.MIDDLE

        ctx.font_size = 16
        ctx.rgb(1, 1, 1)
        ctx.move_to(0, -60).text("I2C Device Scanner")

        ctx.rgb(0.7, 0.7, 0.7)
        ctx.font_size = 14
        yOffset = 18
        yStart = -30
        ctx.move_to(0, yStart).text("Attach a device to the Qwiic port")
        ctx.font_size = 16
        ctx.move_to(0, yStart + yOffset * 1).text("Press OK to Re-Scan")

        if self._devices is None:
            ctx.move_to(0, yStart + (3 * yOffset)).text("Scanning...")
            self._pending_scan = True
        elif len(self._devices) == 0:
            ctx.rgb(0.7, 0.0, 0.0)
            ctx.move_to(0, yStart + (3 * yOffset)).text("No Devices Found")
        else:
            ctx.rgb(0.0, 0.7, 0.0)
            ctx.move_to(0, yStart + (3 * yOffset)).text(
                "Found %d Devices: " % len(self._devices)
            )
            devices_str = ""
            for d in self._devices:
                devices_str += hex(d) + " "

            ctx.move_to(0, yStart + (4 * yOffset)).text(devices_str)

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        if self.input.buttons.app.middle.pressed:
            self._devices = None

        if self._pending_scan and not self.vm.transitioning:
            self.scan()
            self._pending_scan = False


# For running with `mpremote run`:
if __name__ == "__main__":
    import st3m.run

    st3m.run.run_app(I2CScanner)
