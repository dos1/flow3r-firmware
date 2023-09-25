from st3m.application import Application
import math, random, sys_display
from st3m import settings
import audio
from st3m.ui import colours


class Drawable:
    def __init__(self, press):
        self.x = 23
        self.x_vel = 40 / 1000.0

        self.y = 0
        self.font_size = 20
        self.active = False
        # override these w ur own val :3
        self.focused_widget = 2
        self.mid_x = 30
        self.num_widgets = 2
        self.overhang = -70
        self.line_height = 24
        self.ctx = None
        self.press = press

    def draw_heading(self, label):
        ctx = self.ctx
        if ctx is None:
            return
        self.widget_no += 1
        if not self.active:
            if self.press.select_pressed and self.focused_widget > 0:
                self.active = True
                self.press.select_pressed = False
            elif self.press.left_pressed:
                self.focused_widget -= 1
                if self.focused_widget < 2:
                    self.focused_widget = 2
                self.press.left_pressed = False
            elif self.press.right_pressed:
                self.focused_widget += 1
                if self.focused_widget > self.num_widgets - 1:
                    self.focused_widget = self.num_widgets - 1
                self.press.right_pressed = False
        if self.widget_no == self.focused_widget and not self.active:
            ctx.rectangle(-130, int(self.y - self.font_size * 0.8), 260, self.font_size)
            ctx.line_width = 2.0
            ctx.rgba(*colours.GO_GREEN, 1.0)
            ctx.stroke()
        ctx.gray(1)
        ctx.move_to(self.mid_x, self.y)
        ctx.save()
        ctx.rgb(0.8, 0.8, 0.8)
        ctx.move_to(0, self.y)
        ctx.text_align = ctx.CENTER
        ctx.font_size += 6
        ctx.text(label)
        ctx.restore()
        self.y += self.line_height + 8

    def draw_widget(self, label):
        ctx = self.ctx
        if ctx is None:
            return
        self.widget_no += 1
        if not self.active:
            if self.press.select_pressed and self.focused_widget > 0:
                self.active = True
                self.press.select_pressed = False
            elif self.press.left_pressed:
                self.focused_widget -= 1
                if self.focused_widget < 2:
                    self.focused_widget = 2
                self.press.left_pressed = False
            elif self.press.right_pressed:
                self.focused_widget += 1
                if self.focused_widget > self.num_widgets - 1:
                    self.focused_widget = self.num_widgets - 1
                self.press.right_pressed = False
        if self.widget_no == self.focused_widget and not self.active:
            ctx.rectangle(-130, int(self.y - self.font_size * 0.8), 260, self.font_size)
            ctx.line_width = 2.0
            ctx.rgba(*colours.GO_GREEN, 1.0)
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
        if ctx is None:
            return
        self.draw_widget(label)
        if self.widget_no == self.focused_widget and self.active:
            if self.press.left_pressed:
                no -= 1
                if no < 0:
                    no = 0
            elif self.press.right_pressed:
                no += 1
                if no >= len(choices):
                    no = len(choices) - 1
            elif self.press.select_pressed:
                self.active = False
                self.press.select_pressed = False
        for a in range(len(choices)):
            if a == no and self.active and self.widget_no == self.focused_widget:
                ctx.save()
                ctx.rgba(*colours.GO_GREEN, 1.0)
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
        if ctx is None:
            return
        self.draw_widget(label)
        ret = no
        if self.widget_no == self.focused_widget and self.active:
            if self.press.left_pressed:
                ret -= step_size
            elif self.press.right_pressed:
                ret += step_size
            elif self.press.select_pressed:
                self.active = False
                self.press.select_pressed = False

        if self.active and self.widget_no == self.focused_widget:
            ctx.save()
            ctx.rgba(*colours.GO_GREEN, 1.0)
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

    def draw_boolean(self, label, value, on_str="on", off_str="off"):
        ctx = self.ctx
        if ctx is None:
            return
        self.draw_widget(label)
        if self.widget_no == self.focused_widget and self.active:
            value = not value
            self.active = False

        if value:
            ctx.text(on_str)
        else:
            ctx.text(off_str)
        return value

    def draw_bg(self):
        ctx = self.ctx
        if ctx is None:
            return
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
        ctx.font_size = 20
        ctx.gray(0.8)
        ctx.restore()
        ctx.font_size = self.font_size


class Submenu(Drawable):
    def __init__(self, press):
        super().__init__(press)
        self.submenu_active = False

    def draw(self, ctx):
        if self.submenu_active:
            self._draw(ctx)

    def _draw(self, ctx):
        # override w specific implementation!
        pass


class SpeakerMenu(Submenu):
    def __init__(self, press):
        super().__init__(press)
        self.num_widgets = 6
        self.focused_widget = 2
        self.overhang = -40
        self.mid_x = 50

    def _draw(self, ctx):
        self.ctx = ctx
        self.draw_bg()
        self.draw_heading("speaker")
        tmp = self.draw_number(
            "startup volume",
            1,
            int(settings.num_speaker_startup_volume_db.value),
            unit="dB",
        )
        if tmp < -60:
            tmp = -60
        if tmp > 14:
            tmp = 14
        settings.num_speaker_startup_volume_db.set_value(tmp)

        tmp = self.draw_number(
            "minimum volume", 1, int(settings.num_speaker_min_db.value), unit="dB"
        )
        if settings.num_speaker_min_db.value != tmp:
            audio.speaker_set_minimum_volume_dB(tmp)
            settings.num_speaker_min_db.set_value(audio.speaker_get_minimum_volume_dB())

        tmp = self.draw_number(
            "maximum volume", 1, int(settings.num_speaker_max_db.value), unit="dB"
        )
        if settings.num_speaker_max_db.value != tmp:
            audio.speaker_set_maximum_volume_dB(tmp)
            settings.num_speaker_max_db.set_value(audio.speaker_get_maximum_volume_dB())

        tmp = self.draw_boolean(
            "equalizer", settings.onoff_speaker_eq_on.value, "soft", "off"
        )
        if settings.onoff_speaker_eq_on.value != tmp:
            audio.speaker_set_eq_on(tmp)
            settings.onoff_speaker_eq_on.set_value(tmp)


class HeadphonesMenu(Submenu):
    def __init__(self, press):
        super().__init__(press)
        self.num_widgets = 5
        self.focused_widget = 2
        self.overhang = -40
        self.mid_x = 50

    def _draw(self, ctx):
        self.ctx = ctx
        self.draw_bg()

        self.draw_heading("headphones")
        tmp = self.draw_number(
            "startup volume",
            1,
            int(settings.num_headphones_startup_volume_db.value),
            unit="dB",
        )
        if tmp < -70:
            tmp = -70
        if tmp > 3:
            tmp = 3
        settings.num_headphones_startup_volume_db.set_value(tmp)

        tmp = self.draw_number(
            "minimum volume", 1, int(settings.num_headphones_min_db.value), unit="dB"
        )
        if settings.num_headphones_min_db.value != tmp:
            audio.headphones_set_minimum_volume_dB(tmp)
            settings.num_headphones_min_db.set_value(
                audio.headphones_get_minimum_volume_dB()
            )

        tmp = self.draw_number(
            "maximum volume", 1, int(settings.num_headphones_max_db.value), unit="dB"
        )
        if settings.num_headphones_max_db.value != tmp:
            audio.headphones_set_maximum_volume_dB(tmp)
            settings.num_headphones_max_db.set_value(
                audio.headphones_get_maximum_volume_dB()
            )


class VolumeControlMenu(Submenu):
    def __init__(self, press):
        super().__init__(press)
        self.num_widgets = 6
        self.focused_widget = 2
        self.overhang = -40
        self.mid_x = 25

    def _draw(self, ctx):
        self.ctx = ctx
        self.draw_bg()
        self.draw_heading("volume control")
        tmp = self.draw_number(
            "step", 0.5, float(settings.num_volume_step_db.value), unit="dB"
        )
        if tmp < 0.5:
            tmp = 0.5
        if tmp > 5:
            tmp = 5
        settings.num_volume_step_db.set_value(tmp)

        tmp = self.draw_number(
            "repeat step",
            0.5,
            float(settings.num_volume_repeat_step_db.value),
            unit="dB",
        )
        if tmp < 0:
            tmp = 0
        if tmp > 5:
            tmp = 5
        settings.num_volume_repeat_step_db.set_value(tmp)

        tmp = self.draw_number(
            "repeat wait",
            50,
            int(settings.num_volume_repeat_wait_ms.value),
            unit="ms",
        )
        if tmp < 100:
            tmp = 100
        if tmp > 1000:
            tmp = 1000
        settings.num_volume_repeat_wait_ms.set_value(tmp)

        tmp = self.draw_number(
            "repeat", 50, int(settings.num_volume_repeat_ms.value), unit="ms"
        )
        if tmp < 100:
            tmp = 100
        if tmp > 1000:
            tmp = 1000
        settings.num_volume_repeat_ms.set_value(tmp)


class InputMenu(Submenu):
    def __init__(self, press):
        super().__init__(press)
        self.num_widgets = 6
        self.focused_widget = 2
        self.overhang = -40
        self.mid_x = 50

    def _draw(self, ctx):
        self.ctx = ctx
        self.draw_bg()
        self.draw_heading("inputs")
        tmp = self.draw_number(
            "headset gain", 1, int(settings.num_headset_gain_db.value), unit="dB"
        )
        if settings.num_headset_gain_db.value != tmp:
            audio.headset_set_gain_dB(int(tmp))
            settings.num_headset_gain_db.set_value(audio.headset_get_gain_dB())


class Press:
    def __init__(self):
        self.right_pressed = False
        self.left_pressed = False
        self.select_pressed = False


class App(Application):
    def __init__(self, app_ctx):
        super().__init__(app_ctx)

        self.font_size = 20

        # only the main app handles inputs
        self.input.buttons.app.left.repeat_enable(800, 300)
        self.input.buttons.app.right.repeat_enable(800, 300)
        self.press = Press()

        # submenus
        self.menus = []
        self.menus += [VolumeControlMenu(self.press)]
        self.menus += [InputMenu(self.press)]
        self.menus += [SpeakerMenu(self.press)]
        self.menus += [HeadphonesMenu(self.press)]

    def draw_bg(self):
        ctx = self.ctx
        if ctx is None:
            return
        ctx.rectangle(-120, -120, 240, 240)
        ctx.gray(0)
        ctx.fill()
        ctx.font_size = self.font_size
        ctx.gray(1)

    def draw(self, ctx):
        self.ctx = ctx

        main_menu_active = True
        for menu in self.menus:
            menu.draw(self.ctx)
            if menu.submenu_active:
                main_menu_active = False

        if main_menu_active:
            self.draw_bg()
            ctx.save()
            ctx.rgb(*colours.GO_GREEN)
            ctx.rotate(math.tau / 10)
            for i in range(5):
                if i == 2:
                    ctx.rotate(math.tau / 5)
                    continue
                ctx.round_rectangle(-40, -110, 80, 45, 6).stroke()
                ctx.rotate(math.tau / 5)
            ctx.restore()

            ctx.text_align = ctx.CENTER
            ctx.rotate(math.tau / 10)
            ctx.move_to(0, -91)
            ctx.text("volume")
            ctx.move_to(0, -74)
            ctx.text("control")
            ctx.move_to(0, 0)
            ctx.rotate(math.tau * (1 / 5 + 1 / 2))
            ctx.move_to(0, 92)
            ctx.text("inputs")
            ctx.move_to(0, 0)
            ctx.rotate(math.tau * 2 / 5)
            ctx.move_to(0, 92)
            ctx.text("speaker")
            ctx.move_to(0, 0)
            ctx.rotate(math.tau * (1 / 5 + 1 / 2))
            ctx.move_to(0, -91)
            ctx.text("head")
            ctx.move_to(0, -74)
            ctx.text("phones")
            ctx.rotate(math.tau / 10)
            ctx.move_to(0, 0)
            ctx.text("audio config")
            ctx.rgb(0.7, 0.7, 0.7)
            ctx.font_size = 18
            ctx.move_to(0, 20)
            ctx.text("exit to save")

        self.press.select_pressed = False
        self.press.left_pressed = False
        self.press.right_pressed = False

    def think(self, ins, delta_ms):
        super().think(ins, delta_ms)
        for i in range(1, 10, 2):
            if self.input.captouch.petals[i].whole.pressed:
                for menu in self.menus:
                    menu.submenu_active = False
                if i < 5:
                    self.menus[i // 2].submenu_active = True
                elif i > 5:
                    self.menus[(i // 2) - 1].submenu_active = True

        if (
            self.input.buttons.app.right.pressed
            or self.input.buttons.app.right.repeated
        ):
            self.press.right_pressed = True
        if self.input.buttons.app.left.pressed or self.input.buttons.app.left.repeated:
            self.press.left_pressed = True
        if self.input.buttons.app.middle.pressed:
            self.press.select_pressed = True

    def on_enter(self, vm):
        super().on_enter(vm)
        settings.load_all()

    def on_exit(self):
        settings.save_all()
        super().on_exit()


if __name__ == "__main__":
    from st3m.run import run_app

    run_app(App)
