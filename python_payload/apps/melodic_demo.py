import bl00mbox
blm = bl00mbox.Channel()
from bl00mbox_patches import tinysynth_fm

from hardware import *
import leds

octave = 0
synths = []
scale = [0, 2, 4, 5, 7, 9, 11]


def highlight_bottom_petal(num, r, g, b):
    start = 4 + 8 * num
    for i in range(7):
        leds.set_rgb(((i + start) % 40), r, g, b)


def change_playing_field_color(r, g, b):
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


def adjust_playing_field_to_octave():
    global octave
    if octave == -1:
        change_playing_field_color(0, 0, 55)
    elif octave == 0:
        change_playing_field_color(0, 27, 27)
    elif octave == 1:
        change_playing_field_color(0, 55, 0)


def run():
    global scale
    global octave
    global synths
    for i in range(10):
        if get_captouch(i):
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


def init():
    global synths
    for i in range(1):
        synths += [blm.new_patch(tinysynth_fm)]
    for synth in synths:
        synth.decay(100)


def foreground():
    adjust_playing_field_to_octave()


from st3m.application import Application


class MelodicApp(Application):
    def on_init(self):
        init()

    def on_draw(self, ctx):
        ctx.rgb(1, 1, 1).rectangle(-120, -120, 240, 240).fill()
        ctx.rgb(0, 0, 0)
        scope_draw(ctx)
        ctx.fill()

    def on_foreground(self):
        foreground()

    def main_foreground(self):
        run()


app = MelodicApp("melodic")
