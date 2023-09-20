from st3m.goose import List, Optional
from st3m.input import InputState, InputController
from st3m.ui.view import ViewManager
from st3m.application import Application, ApplicationContext
from ctx import Context
import bl00mbox
import leds
import math


class MelodicApp(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)
        self.synths: List[bl00mbox.patches.tinysynth] = []
        self.base_scale = [0, 2, 4, 5, 7, 9, 11]
        self.mid_point = 7
        self.mid_point_petal = 0
        self.mid_point_lock = False
        self.mid_point_petal_hyst = 3

        self.min_note = -36
        self.max_note = +30
        self.at_min_note = False
        self.at_max_note = False

        self.auto_color = (1, 0.5, 1)
        self.min_hue = 0
        self.max_hue = 0

        self.scale = [0] * 10
        self.prev_note = None
        self.legato = False
        self.blm = None

    def base_scale_get_val_from_mod_index(self, index):
        o = index // len(self.base_scale)
        i = index % len(self.base_scale)
        return 12 * o + self.base_scale[i]

    def base_scale_get_mod_index_from_val(self, val):
        val = int(val)
        index = val
        while True:
            try:
                i = self.base_scale.index(index % 12)
                break
            except:
                index -= 1
        o = val // 12
        return i + len(self.base_scale) * o

    def make_scale(self):
        i = self.base_scale_get_mod_index_from_val(self.mid_point)
        for j in range(-5, 5):
            tone = self.base_scale_get_val_from_mod_index(i + j)
            self.scale[(self.mid_point_petal + j) % 10] = tone

    def draw(self, ctx: Context) -> None:
        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
        ctx.text_align = ctx.CENTER
        ctx.get_font_name(4)
        ctx.font_size = 20
        ctx.rgb(0.8, 0.8, 0.8)
        ctx.move_to(0, -15)
        ctx.text("note shift:")

        ctx.font_size = 25
        ctx.move_to(0, 15)
        if self.mid_point_lock:
            ctx.rgb(0, 0.5, 1)
            ctx.text("manual")
        else:
            ctx.rgb(*self.auto_color)
            ctx.text("auto")
            ctx.rgb(0, 0.5, 1)

        ctx.rotate(math.tau * ((self.mid_point_petal / 10) - 0.05))
        tmp = ctx.line_width
        ctx.line_width = 30
        ctx.arc(0, 0, 105, -math.tau * 0.7, math.tau * 0.2, 0).stroke()
        if not self.mid_point_lock:
            ctx.rgb(*self.auto_color)
            if not self.at_max_note:
                ctx.arc(0, 0, 105, math.tau * 0.05, math.tau * 0.2, 0).stroke()
            if not self.at_min_note:
                ctx.arc(0, 0, 105, -math.tau * 0.7, -math.tau * 0.55, 0).stroke()
        ctx.line_width = tmp

        if self.mid_point_lock:
            ctx.rgb(1, 1, 1)
        else:
            ctx.rgb(0, 0, 0)

        ctx.rotate(-math.tau / 10)
        if not self.at_max_note:
            ctx.move_to(0, 110)
            ctx.text("<<<")
            ctx.move_to(0, 0)
        if not self.at_min_note:
            ctx.rotate(math.tau / 5)
            ctx.move_to(0, 110)
            ctx.text(">>>")

    def _build_synth(self):
        if self.blm is None:
            self.blm = bl00mbox.Channel("melodic demo")
        self.synths = []
        for i in range(1):
            synth = self.blm.new(bl00mbox.patches.tinysynth)
            synth.signals.output = self.blm.mixer
            self.synths += [synth]
        for synth in self.synths:
            synth.signals.decay = 100

    def update_leds(self):
        for i in range(40):
            hue_deg = ((i * 90 / 40) + (self.mid_point * 270 / 60) + 180) % 360
            if i == 0:
                self.hue_min = hue_deg
            if i == 39:
                self.hue_max = hue_deg
            index = i + (self.mid_point_petal - 5) * 4
            leds.set_hsv(index % 40, hue_deg, 1, 1)
        leds.update()

    def on_enter(self, vm: Optional[ViewManager]) -> None:
        super().on_enter(vm)
        if self.blm is None:
            self._build_synth()
        self.blm.foreground = True
        self.make_scale()

    def on_exit(self):
        if self.blm is not None:
            self.blm.free = True
        self.blm = None

    def shift_playing_field_by_num_petals(self, num):
        num_positive = True
        if num < 0:
            num_positive = False
            self.at_max_note = False
        elif num > 0:
            self.at_min_note = False
        num = abs(num)
        while num != 0:
            if num > 3:
                num_part = 3
                num -= 3
            else:
                num_part = num
                num = 0
            if num_positive:
                self.mid_point_petal += num_part
                self.mid_point_petal = self.mid_point_petal % 10
            else:
                self.mid_point_petal -= num_part
                self.mid_point_petal = self.mid_point_petal % 10
            self.mid_point = self.scale[self.mid_point_petal]
            self.make_scale()

        # make sure things stay in bounds
        while max(self.scale) > self.max_note:
            self.mid_point_petal -= 1
            self.mid_point_petal = self.mid_point_petal % 10
            self.mid_point = self.scale[self.mid_point_petal]
            self.make_scale()
            self.at_max_note = True
        while min(self.scale) < self.min_note:
            self.mid_point_petal += 1
            self.mid_point_petal = self.mid_point_petal % 10
            self.mid_point = self.scale[self.mid_point_petal]
            self.make_scale()
            self.at_min_note = True

        self.make_scale()

        if max(self.scale) == self.max_note:
            self.at_max_note = True
        if min(self.scale) == self.min_note:
            self.at_min_note = True

    def think(self, ins: InputState, delta_ms: int) -> None:
        if self.blm is None:
            return
        super().think(ins, delta_ms)

        petals = []

        if self.input.buttons.app.middle.pressed:
            self.mid_point_lock = not self.mid_point_lock
        if self.input.buttons.app.right.pressed:
            self.shift_playing_field_by_num_petals(4)
        if self.input.buttons.app.left.pressed:
            self.shift_playing_field_by_num_petals(-4)

        for i in range(10):
            if ins.captouch.petals[i].pressed:
                petals += [i]

        if len(petals) == 0:
            self.synths[0].signals.trigger.stop()
            self.prev_note = None
        else:
            if (len(petals) == 1) and (not self.mid_point_lock):
                delta = petals[0] - self.mid_point_petal
                if delta > 4:
                    delta -= 10
                if delta < -5:
                    delta += 10
                if delta > 2:
                    self.shift_playing_field_by_num_petals(delta - 2)
                if delta < -3:
                    self.shift_playing_field_by_num_petals(delta + 3)
            avg = 0
            for petal in petals:
                avg += self.scale[petal]
            avg /= len(petals)
            self.synths[0].signals.pitch.tone = avg
            if (self.legato and self.prev_note is None) or (
                (not self.legato) and self.prev_note != avg
            ):
                self.synths[0].signals.trigger.start()
            self.prev_note = avg

        self.update_leds()


# For running with `mpremote run`:
if __name__ == "__main__":
    import st3m.run

    st3m.run.run_app(MelodicApp)
