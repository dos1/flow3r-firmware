from st3m.application import Application
from st3m.ui.view import View, ViewTransitionDirection
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
        self.mid_x = 30
        self.num_widgets = 2
        self.overhang = -70
        self.line_height = 24
        self.ctx = None
        self.press = press
        self.focus_pos_limit_min = -60
        self.focus_pos_limit_max = 60
        self.focus_pos_limit_first = -60
        self.focus_pos_limit_last = 80
        self.first_widget_pos = 0
        self.last_widget_pos = 0
        self.focus_widget_pos_min = 0
        self.focus_widget_pos_max = 0
        self._focus_widget = 2
        self._focus_widget_prev = 1

    @property
    def focus_widget(self):
        return self._focus_widget

    @property
    def focus_widget_prev(self):
        return self._focus_widget_prev

    @focus_widget.setter
    def focus_widget(self, val):
        if val < 2:
            val = 2
        if val > self.num_widgets - 1:
            val = self.num_widgets - 1
        self._focus_widget_prev = self._focus_widget
        self._focus_widget = val

    @property
    def at_first_widget(self):
        return self.focus_widget <= 2

    @property
    def at_last_widget(self):
        return self.focus_widget >= (self.num_widgets - 1)

    def draw_heading(
        self, label, col=(0.8, 0.8, 0.8), embiggen=6, top_margin=2, bot_margin=2
    ):
        ctx = self.ctx
        if ctx is None:
            return
        self.widget_no += 1
        self.y += embiggen + top_margin
        if self.widget_no == self.focus_widget:
            if self.focus_widget > self.focus_widget_prev:
                self.focus_widget += 1
            else:
                self.focus_widget -= 1
        ctx.gray(1)
        ctx.move_to(self.mid_x, self.y)
        ctx.save()
        ctx.rgb(*col)
        ctx.move_to(0, self.y)
        ctx.text_align = ctx.CENTER
        ctx.font_size += embiggen
        ctx.text(label)
        ctx.restore()
        self.y += self.line_height + embiggen + bot_margin

    def draw_widget(self, label):
        ctx = self.ctx
        if ctx is None:
            return
        self.widget_no += 1
        if not self.active:
            if self.press.select_pressed and self.focus_widget > 0:
                self.active = True
                self.press.select_pressed = False
            elif self.press.left_pressed:
                self.focus_widget -= 1
                self.press.left_pressed = False
            elif self.press.right_pressed:
                self.focus_widget += 1
                self.press.right_pressed = False
        if self.widget_no == self.focus_widget:
            self.focus_widget_pos_min = self.y
            if not self.active:
                ctx.rectangle(
                    -130, int(self.y - self.font_size * 0.8), 260, self.font_size
                )
                ctx.line_width = 2.0
                ctx.rgba(*colours.GO_GREEN, 1.0)
                ctx.stroke()
            self.focus_widget_pos_max = self.y + self.line_height
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
        if self.widget_no == self.focus_widget and self.active:
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
            if a == no and self.active and self.widget_no == self.focus_widget:
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

    def draw_number(self, label, step_size, no, unit="", val_col=(1.0, 1.0, 1.0)):
        ctx = self.ctx
        if ctx is None:
            return
        self.draw_widget(label)
        ret = no
        if self.widget_no == self.focus_widget and self.active:
            if self.press.left_pressed:
                ret -= step_size
            elif self.press.right_pressed:
                ret += step_size
            elif self.press.select_pressed:
                self.active = False
                self.press.select_pressed = False

        if self.active and self.widget_no == self.focus_widget:
            ctx.save()
            ctx.rgba(*colours.GO_GREEN, 1.0)
            ctx.rectangle(
                ctx.x - 1,
                ctx.y - self.font_size * 0.8,
                ctx.text_width(str(no)[:4]) + 2,
                self.font_size,
            ).stroke()
            ctx.restore()

        ctx.save()
        ctx.rgb(*val_col)
        ctx.text(str(no)[:4] + unit)
        ctx.restore()
        return ret

    def draw_boolean(
        self,
        label,
        value,
        on_str="on",
        off_str="off",
        val_col=(1.0, 1.0, 1.0),
        on_hint=None,
        off_hint=None,
    ):
        ctx = self.ctx
        if ctx is None:
            return
        self.draw_widget(label)
        if self.widget_no == self.focus_widget and self.active:
            value = not value
            self.active = False

        ctx.save()
        ctx.rgb(*val_col)
        if value:
            ctx.text(on_str)
        else:
            ctx.text(off_str)
        ctx.restore()
        if self.widget_no == self.focus_widget:
            if value:
                hint = on_hint
            else:
                hint = off_hint
            if hint is not None:
                ctx.save()
                ctx.font_size -= 4
                ctx.text_align = ctx.CENTER
                ctx.rgb(0.9, 0.9, 0.9)
                lines = hint.split("\n")
                self.y -= 3
                for line in lines:
                    ctx.move_to(0, self.y)
                    ctx.text(line)
                    self.y += self.line_height - 5
                ctx.restore()
                if self.y > 115:
                    self.focus_widget_pos_max = self.y
        return value

    def draw_bg(self):
        ctx = self.ctx
        if ctx is None:
            return
        ctx.gray(1.0)
        ctx.move_to(-100, -80)
        scroll_val = 0
        scroll_speed = 7
        if self.at_last_widget:
            if self.focus_widget_pos_max > self.focus_pos_limit_last:
                scroll_val = -self.focus_widget_pos_max + self.focus_pos_limit_last
        elif self.at_first_widget:
            if self.focus_widget_pos_min < self.focus_pos_limit_first:
                scroll_val = 9999
        elif self.focus_widget_pos_max > self.focus_pos_limit_max:
            scroll_val = -9999
        elif self.focus_widget_pos_min < self.focus_pos_limit_min:
            scroll_val = 9999

        if scroll_val > 0:
            self.overhang += min(scroll_val, scroll_speed)
        else:
            self.overhang += max(scroll_val, -scroll_speed)

        self.y = self.overhang
        self.widget_no = 0
        ctx.rectangle(-120, -120, 240, 240)
        ctx.gray(0)
        ctx.fill()
        ctx.font_size = self.font_size


class Submenu(Drawable):
    def __init__(self, press):
        super().__init__(press)

    def draw(self, ctx):
        self._draw(ctx)

    def _draw(self, ctx):
        # override w specific implementation!
        pass


class SpeakerMenu(Submenu):
    def __init__(self, press):
        super().__init__(press)
        self.num_widgets = 6
        self.overhang = -40
        self.mid_x = 50
        self.focus_pos_limit_min = -100
        self.focus_pos_limit_max = 100
        self.focus_pos_limit_first = -100
        self.focus_pos_limit_last = 100

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
        self.overhang = -40
        self.mid_x = 50
        self.focus_pos_limit_min = -100
        self.focus_pos_limit_max = 100
        self.focus_pos_limit_first = -100
        self.focus_pos_limit_last = 100

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
        self.overhang = -40
        self.mid_x = 25
        self.focus_pos_limit_min = -100
        self.focus_pos_limit_max = 100
        self.focus_pos_limit_first = -100
        self.focus_pos_limit_last = 100

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
        self.num_widgets = 11
        self.overhang = -89
        self.mid_x = 0
        self.focus_pos_limit_max = 80
        self.focus_pos_limit_last = 100

    def _draw(self, ctx):
        self.ctx = ctx
        self.draw_bg()

        avail_col = (0.0, 0.9, 0.6)
        warn_col = (0.9, 0.0, 0.0)
        allow_col = (0.0, 0.7, 0.5)
        not_allow_col = (0.8, 0.3, 0.3)
        not_avail_col = (0.6, 0.6, 0.6)

        self.draw_heading("line in", embiggen=5, top_margin=5, bot_margin=-4)
        if audio.line_in_get_allowed():
            if audio.input_engines_get_source_avail(audio.INPUT_SOURCE_LINE_IN):
                col = avail_col
            else:
                col = allow_col
        else:
            col = not_allow_col
        tmp = self.draw_boolean(
            "line in",
            settings.onoff_line_in_allowed.value,
            on_str="allowed",
            off_str="blocked",
            val_col=col,
        )
        if settings.onoff_line_in_allowed.value != tmp:
            audio.line_in_set_allowed(tmp)
            settings.onoff_line_in_allowed.set_value(tmp)

        tmp = self.draw_number(
            "gain",
            1.5,
            float(settings.num_line_in_gain_db.value),
            unit="dB",
        )
        if settings.num_line_in_gain_db.value != tmp:
            audio.line_in_set_gain_dB(tmp)
            settings.num_line_in_gain_db.set_value(audio.line_in_get_gain_dB())

        self.draw_heading("headset mic", embiggen=5, top_margin=5, bot_margin=-4)

        if audio.headset_mic_get_allowed():
            if audio.input_engines_get_source_avail(audio.INPUT_SOURCE_HEADSET_MIC):
                col = avail_col
            else:
                col = allow_col
        else:
            col = not_allow_col
        tmp = self.draw_boolean(
            "access",
            settings.onoff_headset_mic_allowed.value,
            on_str="allowed",
            off_str="blocked",
            val_col=col,
        )
        if settings.onoff_headset_mic_allowed.value != tmp:
            audio.headset_mic_set_allowed(tmp)
            settings.onoff_headset_mic_allowed.set_value(tmp)

        tmp = self.draw_number(
            "gain",
            1.5,
            float(settings.num_headset_mic_gain_db.value),
            unit="dB",
        )
        if settings.num_headset_mic_gain_db.value != tmp:
            tmp = audio.headset_mic_set_gain_dB(tmp)
            settings.num_headset_mic_gain_db.set_value(tmp)

        self.draw_heading("onboard mic", embiggen=5, top_margin=5, bot_margin=-4)

        if audio.onboard_mic_get_allowed():
            col = avail_col
        else:
            col = not_allow_col
        tmp = self.draw_boolean(
            "access",
            settings.onoff_onboard_mic_allowed.value,
            on_str="allowed",
            off_str="blocked",
            val_col=col,
        )
        if settings.onoff_onboard_mic_allowed.value != tmp:
            audio.onboard_mic_set_allowed(tmp)
            settings.onoff_onboard_mic_allowed.set_value(tmp)

        tmp = self.draw_number(
            "gain",
            1.5,
            float(settings.num_onboard_mic_gain_db.value),
            unit="dB",
        )
        if settings.num_onboard_mic_gain_db.value != tmp:
            tmp = audio.onboard_mic_set_gain_dB(tmp)
            settings.num_onboard_mic_gain_db.set_value(tmp)

        if not audio.onboard_mic_to_speaker_get_allowed():
            col = not_allow_col
        else:
            col = warn_col
        tmp = self.draw_boolean(
            "thru",
            settings.onoff_onboard_mic_to_speaker_allowed.value,
            on_str="allow",
            off_str="phones",
            val_col=col,
            on_hint=" /!\ feedback possible /!\ ",
        )
        if settings.onoff_onboard_mic_to_speaker_allowed.value != tmp:
            audio.onboard_mic_to_speaker_set_allowed(tmp)
            settings.onoff_onboard_mic_to_speaker_allowed.set_value(tmp)


class Press:
    def __init__(self):
        self.right_pressed = False
        self.left_pressed = False
        self.select_pressed = False


class SubView(View):
    def __init__(self, menu, app):
        self.menu = menu
        self.app = app

    def think(self, ins, delta_ms):
        self.app.think(ins, delta_ms)

    def draw(self, ctx):
        self.menu.draw(ctx)


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
                if not self.is_active():
                    self.vm.pop()
                if i < 5:
                    self.vm.push(SubView(self.menus[i // 2], self))
                elif i > 5:
                    self.vm.push(SubView(self.menus[(i // 2) - 1], self))

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
        if self.vm.direction == ViewTransitionDirection.FORWARD:
            settings.load_all()

    def on_exit(self):
        if self.vm.direction == ViewTransitionDirection.BACKWARD:
            settings.save_all()


if __name__ == "__main__":
    from st3m.run import run_app

    run_app(App)
