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
        self._devices = self.qwiic.scan()
        print("Found Devices:")
        print(self._devices)

    def on_enter(self, vm):
        super().on_enter(vm)

    def draw(self, ctx: Context) -> None:
        # Get the default font
        ctx.font = ctx.get_font_name(1)
        # Draw a black background
        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()

        ctx.font_size = 16

        ctx.rgb(0.7, 0.7, 0.7)
        ctx.text_align = ctx.MIDDLE
        ctx.move_to(0, -62).text("Press OK to Re-Scan")
        ctx.move_to(0, -42).text("Found Devices:")
        for i, d in enumerate(self._devices):
            ctx.move_to(0, -24 + (i * 12)).text(hex(d))

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        if self.input.buttons.app.middle.pressed:
            self._devices = self.qwiic.scan()


# For running with `mpremote run`:
if __name__ == "__main__":
    import st3m.run

    st3m.run.run_app(I2CScanner)
