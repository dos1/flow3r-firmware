import event
import hardware
from synth import tinysynth
import math

def xy_from_polar(r,deg):
	#rad = deg/180*math.pi
	
	return( (
		r * math.sin(deg), #x
		r * math.cos(deg)  #y
	)  )

ctx = hardware.get_ctx()
ctx.text_align = ctx.CENTER
ctx.text_baseline = ctx.MIDDLE

synth = tinysynth(440,1)
synth.decay(25)

popcorn = [9,7,9,5,0,5,-3,999]

def on_step(data):
	note = popcorn[data["step"]]
	if note != 999:
		synth.tone(note)
		synth.start()
	
	
	ctx.rgb(1,1,0).rectangle(-120,-120,240,240).fill()
	(x,y) = xy_from_polar(90,-2*math.pi/8*data["step"]+math.pi)
	size=180
	ctx.rgb(0.8,0.8,0)
	ctx.round_rectangle(
			x-size/2,
			y-size/2,
			size,size,size//2
	).fill()
	ctx.move_to(x,y).rgb(0.5,0.5,0).text("{}".format(data["step"]))
	hardware.display_update()
	
	
	
	
event.Sequence(bpm=160, steps=8, action=on_step, loop=True)
event.the_engine.eventloop()
