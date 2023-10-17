from st3m.application import Application
import math, random, sys_display


class App(Application):
    def __init__(self, app_ctx):
        super().__init__(app_ctx)

        self.x = 23
        self.x_vel = 40 / 1000.0

        self.y = -53
        self.font_size = 16
        self.delta_ms = 0
        self.right_pressed = False
        self.left_pressed = False
        self.select_pressed = False
        self.angle = 0
        self.focused_widget = 1
        self.active = False

    def draw_widget(self, label):
        ctx = self.ctx
        self.widget_no += 1
        if not self.active:
            if self.select_pressed and self.focused_widget > 0:
                self.active = True
                self.select_pressed = False
            elif self.left_pressed:
                self.focused_widget -= 1
                if self.focused_widget < 1:
                    self.focused_widget = 1
                self.left_pressed = False
            elif self.right_pressed:
                self.focused_widget += 1
                if self.focused_widget > 9:
                    self.focused_widget = 9
                self.right_pressed = False
        if self.widget_no == self.focused_widget and not self.active:
            ctx.rectangle(-130, int(self.y - self.font_size * 0.8), 260, self.font_size)
            ctx.line_width = 2.0
            ctx.rgba(0.8, 0.6, 0.1, 1.0)
            ctx.stroke()
        ctx.gray(1)
        ctx.move_to(-95, self.y)
        self.y += self.font_size
        ctx.text(label + ": ")

    def draw_choice(self, label, choices, no):
        ctx = self.ctx
        self.draw_widget(label)
        if self.widget_no == self.focused_widget and self.active:
            if self.left_pressed:
                no -= 1
                if no < 0:
                    no = 0
            elif self.right_pressed:
                no += 1
                if no >= len(choices):
                    no = len(choices) - 1
            elif self.select_pressed:
                self.active = False
                self.select_pressed = False

        for a in range(len(choices)):
            if a == no and self.active and self.widget_no == self.focused_widget:
                ctx.save()
                ctx.rgba(0.8, 0.6, 0.1, 1.0)
                ctx.line_width = 2.0
                ctx.rectangle(
                    ctx.x - 1,
                    ctx.y - self.font_size * 0.8,
                    ctx.text_width(choices[a]) + 2,
                    self.font_size,
                ).stroke()
                ctx.restore()
                ctx.text(choices[a] + " ")
            elif a == no:
                ctx.save()
                ctx.gray(1)
                ctx.rectangle(
                    ctx.x - 1,
                    ctx.y - self.font_size * 0.8,
                    ctx.text_width(choices[a]) + 2,
                    self.font_size,
                ).fill()
                ctx.gray(0)
                ctx.text(choices[a] + " ")
                ctx.restore()
            else:
                ctx.text(choices[a] + " ")
        return no

    def draw_boolean(self, label, value):
        ctx = self.ctx
        self.draw_widget(label)
        if self.widget_no == self.focused_widget and self.active:
            value = not value
            self.active = False

        if value:
            ctx.text(" on")
        else:
            ctx.text(" off")
        return value

    def draw_bg(self):
        ctx = self.ctx
        ctx.gray(1.0)
        ctx.font_size = self.font_size
        ctx.move_to(-100, -50)
        self.y = -50
        self.widget_no = 0
        ctx.rectangle(-120, -120, 240, 240)
        ctx.gray(0)
        ctx.fill()
        ctx.save()
        ctx.translate(self.x, -80)
        ctx.logo(0, 0, 40)
        ctx.restore()
        self.x += self.delta_ms * self.x_vel
        if self.x < -50 or self.x > 50:
            self.x_vel *= -1
            self.x += self.delta_ms * self.x_vel

    def draw(self, ctx: Context):
        curmode = sys_display.get_mode()
        low_latency = (curmode & sys_display.low_latency) != 0
        direct_ctx = (curmode & sys_display.direct_ctx) != 0
        lock = (curmode & sys_display.lock) != 0
        osd = (curmode & sys_display.osd) != 0
        think_per_draw = (curmode & sys_display.EXPERIMENTAL_think_per_draw) != 0
        smart_redraw = (curmode & sys_display.smart_redraw) != 0
        scale = 0
        if (curmode & sys_display.x4) == sys_display.x2:
            scale = 1
        elif (curmode & sys_display.x4) == sys_display.x3:
            scale = 2
        elif (curmode & sys_display.x4) == sys_display.x4:
            scale = 3

        bpp = curmode & 63
        palette = 0
        if bpp == 9:
            palette = 0
            bpp = 0
        elif bpp == 8:
            palette = 1
            bpp = 0
        elif bpp == 10:
            palette = 2
            bpp = 0
        elif bpp == 11:
            palette = 3
            bpp = 0
        elif bpp == 16:
            bpp = 1
        elif bpp == 24:
            bpp = 2
        elif bpp == 32:
            bpp = 3
        elif bpp == 1:
            bpp = 4
        elif bpp == 2:
            bpp = 5
        elif bpp == 4:
            bpp = 6
        else:
            bpp = 0

        self.ctx = ctx
        self.draw_bg()

        bpp = self.draw_choice("bpp", ["8", "16", "24", "32", "1", "2", "4"], bpp)
        if bpp > 0:
            palette = 0
        palette = self.draw_choice("palette", ["RGB", "gray", "sepia", "cool"], palette)
        scale = self.draw_choice("scale", ["1x", "2x", "3x", "4x"], scale)
        low_latency = self.draw_boolean("low latency", low_latency)
        direct_ctx = self.draw_boolean("direct ctx", direct_ctx)
        think_per_draw = self.draw_boolean("think per draw", think_per_draw)
        smart_redraw = self.draw_boolean("smart redraw", smart_redraw)
        osd = self.draw_boolean("osd", osd)
        lock = self.draw_boolean("lock", lock)
        if direct_ctx:
            low_latency = True
        if palette != 0:
            bpp = 0

        if bpp == 0:
            if palette == 0:
                mode = 9
            elif palette == 1:
                mode = 8
            elif palette == 2:
                mode = 10
            elif palette == 3:
                mode = 11
        elif bpp == 1:
            mode = 16
        elif bpp == 2:
            mode = 24
        elif bpp == 3:
            mode = 32
        elif bpp == 4:
            mode = 1
        elif bpp == 5:
            mode = 2
        elif bpp == 6:
            mode = 4

        mode += osd * sys_display.osd
        mode += low_latency * sys_display.low_latency
        mode += direct_ctx * sys_display.direct_ctx
        mode += lock * sys_display.lock
        mode += think_per_draw * sys_display.EXPERIMENTAL_think_per_draw
        mode += smart_redraw * sys_display.smart_redraw

        if scale == 1:
            mode += sys_display.x2
        elif scale == 2:
            mode += sys_display.x3
        elif scale == 3:
            mode += sys_display.x4

        if mode != curmode:
            sys_display.set_default_mode(mode)

        ################################################################

        self.delta_ms = 0
        self.select_pressed = False
        self.left_pressed = False
        self.right_pressed = False

    def think(self, ins, delta_ms):
        super().think(ins, delta_ms)
        self.delta_ms += delta_ms
        if (
            self.input.buttons.app.right.pressed
            or self.input.buttons.app.right.repeated
        ):
            self.right_pressed = True
        if self.input.buttons.app.left.pressed or self.input.buttons.app.left.repeated:
            self.left_pressed = True
        if self.input.buttons.app.middle.pressed:
            self.select_pressed = True


if __name__ == "__main__":
    from st3m.run import run_app

    run_app(App)
