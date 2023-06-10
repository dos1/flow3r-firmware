from st3m.application import Application
import ui


class MandelbrotApp(Application):
    def on_init(self):
        pass

    def on_foreground(self):
        print("on foreground")
        ctx = self.ui.ctx

        # center the text horizontally and vertically
        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE

        # ctx.rgb() expects individual values for the channels, so unpack a list/tuple with *
        # operations on ctx can be chained
        # create a blue background
        ctx.rgb(*ui.RED).rectangle(
            -ui.WIDTH / 2, -ui.HEIGHT / 2, ui.WIDTH, ui.HEIGHT
        ).fill()

        # Write some text
        ctx.move_to(0, 0).rgb(*ui.WHITE).text("Mandelbrot")

    def main_forground():
        for x in range(-240, 240):
            for y in range(-240, 240):
                pass


app = MandelbrotApp("Mandelbrot")
app.run()
