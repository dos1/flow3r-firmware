from st3m.application import Application
import math, random, sys_display
from st3m import settings
import leds
import sys_display
from st3m.ui import colours


class App(Application):
    def __init__(self, app_ctx):
        super().__init__(app_ctx)

        self.x = 23
        self.x_vel = 40 / 1000.0

        self.y = 0
        self.font_size = 20
        self.delta_ms = 0
        self.right_pressed = False
        self.left_pressed = False
        self.select_pressed = False
        self.angle = 0
        self.focused_widget = 1
        self.active = False
        self.num_widgets = 4
        self.overhang = -20
        self.line_height = 24
        self.input.buttons.app.left.repeat_enable(800, 300)
        self.input.buttons.app.right.repeat_enable(800, 300)
        self.mid_x = 50
        self.led_accumulator_ms = 0
        self.blueish = False
        self.half_time = 600

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
                if self.focused_widget > self.num_widgets - 1:
                    self.focused_widget = self.num_widgets - 1
                self.right_pressed = False
        if self.widget_no == self.focused_widget and not self.active:
            ctx.rectangle(-130, int(self.y - self.font_size * 0.8), 260, self.font_size)
            ctx.line_width = 2.0
            ctx.rgba(*colours.PUSH_RED, 1.0)
            ctx.stroke()
        ctx.gray(1)
        ctx.move_to(self.mid_x, self.y)
        ctx.save()
        ctx.rgb(0.8, 0.8, 0.8)
        ctx.text_align = ctx.RIGHT
        ctx.text(label + ": ")
        ctx.restore()
        self.y += self.line_height

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
                ctx.rgba(*colours.PUSH_RED, 1.0)
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

    def draw_number(self, label, step_size, no, unit=""):
        ctx = self.ctx
        self.draw_widget(label)
        ret = no
        if self.widget_no == self.focused_widget and self.active:
            if self.left_pressed:
                ret -= step_size
            elif self.right_pressed:
                ret += step_size
            elif self.select_pressed:
                self.active = False
                self.select_pressed = False

        if self.active and self.widget_no == self.focused_widget:
            ctx.save()
            ctx.rgba(*colours.PUSH_RED, 1.0)
            ctx.rectangle(
                ctx.x - 1,
                ctx.y - self.font_size * 0.8,
                ctx.text_width(str(no)[:4]) + 2,
                self.font_size,
            ).stroke()
            ctx.restore()
            ctx.text(str(no)[:4] + unit)
        else:
            ctx.text(str(no)[:4] + unit)
        return ret

    def draw_bg(self):
        ctx = self.ctx
        ctx.gray(1.0)
        ctx.move_to(-100, -80)
        wig = self.focused_widget - 1
        if wig < 2:
            wig = 2
        if wig > self.num_widgets - 3:
            wig = self.num_widgets - 3
        focus_pos = self.overhang + (wig - 0.5) * self.line_height
        if focus_pos > 40:
            self.overhang -= 7
        if focus_pos < -40:
            self.overhang += 7
        self.y = self.overhang
        self.widget_no = 0
        ctx.rectangle(-120, -120, 240, 240)
        ctx.gray(0)
        ctx.fill()
        ctx.save()
        ctx.translate(self.x, -100)
        ctx.font_size = 20
        ctx.gray(0.8)
        ctx.text("audio settings")
        ctx.restore()
        ctx.font_size = self.font_size
        self.x += self.delta_ms * self.x_vel
        if self.x < -50 or self.x > 50:
            self.x_vel *= -1
            self.x += self.delta_ms * self.x_vel

    def draw(self, ctx: Context):
        self.ctx = ctx
        self.draw_bg()

        tmp = self.draw_number(
            "led brightness", 5, int(settings.num_leds_brightness.value)
        )
        if tmp < 5:
            tmp = 5
        elif tmp > 255:
            tmp = 255
        if tmp != settings.num_leds_brightness.value:
            settings.num_leds_brightness.set_value(tmp)
            leds.set_brightness(settings.num_leds_brightness.value)

        tmp = self.draw_number("led speed", 5, int(settings.num_leds_speed.value))
        if tmp < 0:
            tmp = 0
        elif tmp > 255:
            tmp = 255
        if tmp < 120:
            self.half_time = 600 + (120 - tmp) * (120 - tmp) / 2
        else:
            self.half_time = 600
        if tmp != settings.num_leds_speed.value:
            settings.num_leds_speed.set_value(tmp)
            leds.set_slew_rate(settings.num_leds_speed.value)

        tmp = self.draw_number(
            "display brightness",
            5,
            int(settings.num_display_brightness.value),
            unit="%",
        )
        if tmp < 5:
            tmp = 5
        if tmp > 100:
            tmp = 100
        if tmp != settings.num_display_brightness.value:
            settings.num_display_brightness.set_value(tmp)
            sys_display.set_backlight(settings.num_display_brightness.value)

        self.delta_ms = 0
        self.select_pressed = False
        self.left_pressed = False
        self.right_pressed = False

    def think(self, ins, delta_ms):
        super().think(ins, delta_ms)
        self.delta_ms += delta_ms
        if self.focused_widget == 2:
            self.led_accumulator_ms += delta_ms
        if (
            self.input.buttons.app.right.pressed
            or self.input.buttons.app.right.repeated
        ):
            self.right_pressed = True
        if self.input.buttons.app.left.pressed or self.input.buttons.app.left.repeated:
            self.left_pressed = True
        if self.input.buttons.app.middle.pressed:
            self.select_pressed = True

        while self.led_accumulator_ms > self.half_time:
            self.led_accumulator_ms = self.led_accumulator_ms % self.half_time
            self.leds_shift_hue(0.8)

    def leds_shift_hue(self, val):
        for i in range(40):
            rgb = leds.get_rgb(i)
            h, s, v = colours.rgb_to_hsv(*rgb)
            h += val
            leds.set_rgb(i, *colours.hsv_to_rgb(h, s, v))
        leds.update()

    def on_enter(self, vm):
        super().on_enter(vm)
        settings.load_all()

    def on_exit(self):
        settings.save_all()
        super().on_exit()


if __name__ == "__main__":
    from st3m.run import run_app

    run_app(App)
