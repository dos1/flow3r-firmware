import bl00mbox

blm = bl00mbox.Channel("Melodic Demo")

import leds

from st3m.goose import List, Optional
from st3m.input import InputState, InputController
from st3m.ui.view import ViewManager
from ctx import Context

octave = 0
synths: List[bl00mbox.patches.tinysynth] = []
scale = [0, 2, 4, 5, 7, 9, 11]


def highlight_bottom_petal(num: int, r: int, g: int, b: int) -> None:
    start = 4 + 8 * num
    for i in range(7):
        leds.set_rgb(((i + start) % 40), r, g, b)


def change_playing_field_color(r: int, g: int, b: int) -> None:
    highlight_bottom_petal(0, r, g, b)
    highlight_bottom_petal(1, r, g, b)
    highlight_bottom_petal(3, r, g, b)
    highlight_bottom_petal(4, r, g, b)
    highlight_bottom_petal(2, 55, 0, 55)
    leds.set_rgb(18, 55, 0, 55)
    leds.set_rgb(19, 55, 0, 55)
    leds.set_rgb(27, 55, 0, 55)
    leds.set_rgb(28, 55, 0, 55)
    leds.update()


def adjust_playing_field_to_octave() -> None:
    global octave
    if octave == -1:
        change_playing_field_color(0, 0, 55)
    elif octave == 0:
        change_playing_field_color(0, 27, 27)
    elif octave == 1:
        change_playing_field_color(0, 55, 0)


def run(input: InputController) -> None:
    global scale
    global octave
    global synths
    any_down = False
    for i in range(10):
        petal = input.captouch.petals[i].whole
        if petal.down:
            any_down = True
        if petal.pressed:
            any_down = True
            if i == 6:
                octave = -1
                adjust_playing_field_to_octave()
            elif i == 5:
                octave = 0
                adjust_playing_field_to_octave()
            elif i == 4:
                octave = 1
                adjust_playing_field_to_octave()
            else:
                k = 10 - i
                if k > 3:
                    k -= 10
                k = 3 - k
                note = scale[k] + 12 * octave
                synths[0].signals.pitch.tone = note
                synths[0].signals.trigger.start()

    if not any_down:
        synths[0].signals.trigger.stop()


def init() -> None:
    global synths
    for i in range(1):
        synth = blm.new(bl00mbox.patches.tinysynth)
        synth.signals.output = blm.mixer
        synths += [synth]
    for synth in synths:
        synth.signals.decay = 100


def foreground() -> None:
    adjust_playing_field_to_octave()


from st3m.application import Application, ApplicationContext


# TODO(q3k): properly port this app
class MelodicApp(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)
        init()

    def draw(self, ctx: Context) -> None:
        ctx.rgb(1, 1, 1).rectangle(-120, -120, 240, 240).fill()
        ctx.rgb(0, 0, 0)
        ctx.scope()
        ctx.fill()

    def on_enter(self, vm: Optional[ViewManager]) -> None:
        super().on_enter(vm)
        foreground()

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)
        run(self.input)
