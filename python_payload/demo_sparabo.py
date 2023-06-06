#python
import math

#badge23
import event
import hardware
from synth import tinysynth
import application
import ui

import audio

popcorn = [9,7,9,5,0,5,-3,999]

def on_step(data):
    ctx = app.ui.ctx
    synth = app.synth
    note = popcorn[data["step"]]
    if note != 999:
        synth.tone(note)
        synth.start()
    
    if not app.is_foreground(): return

    ctx.text_align = ctx.CENTER
    ctx.text_baseline = ctx.MIDDLE

    ctx.rgb(1,1,0).rectangle(-120,-120,240,240).fill()
    (x,y) = ui.xy_from_polar(90,-2*math.pi/8*data["step"]+math.pi)
    size=180
    ctx.rgb(0.8,0.8,0)
    ctx.round_rectangle(
            x-size/2,
            y-size/2,
            size,size,size//2
    ).fill()
    ctx.move_to(x,y).rgb(0.5,0.5,0).text("{}".format(data["step"]))

class AppSparabo(application.Application):
    def on_init(self):
        audio.set_volume_dB(0)
        
        self.synth = tinysynth(440,1)
        self.synth.decay(25)

        print ("here")
        self.sequencer = event.Sequence(bpm=160, steps=8, action=on_step, loop=True)
        self.sequencer.start()
        if self.sequencer.repeat_event:
            self.add_event(self.sequencer.repeat_event)
    
    def on_foreground(self):
        self.sequencer.start()

    def on_exit(self):
        self.sequencer.stop()

    
app = AppSparabo("sequencer")
