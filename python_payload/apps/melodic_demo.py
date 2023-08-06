from bl00mbox import tinysynth
from hardware import *
import captouch
import leds

from st3m.goose import List, Optional
from st3m.input import InputState
from st3m.ui.view import ViewManager
from st3m.ui.ctx import Ctx

octave = 0
synths: List[tinysynth] = []
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


def run(ins: InputState) -> None:
    global scale
    global octave
    global synths
    for i in range(10):
        petal = ins.captouch.petals[i]
        if petal.pressed:
            if i == 4:
                octave = -1
                adjust_playing_field_to_octave()
            elif i == 5:
                octave = 0
                adjust_playing_field_to_octave()
            elif i == 6:
                octave = 1
                adjust_playing_field_to_octave()
            else:
                k = i
                if k > 3:
                    k -= 10
                k = 3 - k
                note = scale[k] + 12 * octave
                synths[0].tone(note)
                synths[0].start()


def init() -> None:
    global synths
    for i in range(1):
        synths += [tinysynth(440)]
    for synth in synths:
        synth.decay_ms(100)


def foreground() -> None:
    adjust_playing_field_to_octave()


from st3m.application import Application


# TODO(q3k): properly port this app
class MelodicApp(Application):
    def __init__(self, name: str) -> None:
        super().__init__(name)
        init()

    def draw(self, ctx: Ctx) -> None:
        ctx.rgb(1, 1, 1).rectangle(-120, -120, 240, 240).fill()
        ctx.rgb(0, 0, 0)
        scope_draw(ctx)
        ctx.fill()

    def on_enter(self, vm: Optional[ViewManager]) -> None:
        foreground()

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)
        run(ins)


app = MelodicApp("melodic")
