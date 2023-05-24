import hardware
import random
import time
import math
import event

def xy_from_polar(r,deg):
	#rad = deg/180*math.pi
	
	return( (
		r * math.sin(deg), #x
		r * math.cos(deg)  #y
	)  )

def randrgb():
	return ((random.random(),random.random(),random.random()))
	
	
WIDTH = 240
HEIGHT = 240

#Define a few RGB (0.0 to 1.0) colors
BLACK = (0,0,0)
RED = (1,0,0)
GREEN = (0,1,0)
BLUE = (0,0,1)
WHITE = (1,1,1)
GREY = (0.5,0.5,0.5)

# Get the global context (representing the whole screen)
global ctx
ctx = hardware.get_ctx()

#center the text horizontally and vertically
ctx.text_align = ctx.CENTER
ctx.text_baseline = ctx.MIDDLE

#ctx.rgb() expects individual values for the channels, so unpack a list/tuple with *
#operations on ctx can be chained
#create a blue background
ctx.rgb(*BLUE).rectangle(-WIDTH/2,-HEIGHT/2,WIDTH,HEIGHT).fill()

#Write some text
ctx.move_to(0,0).rgb(*WHITE).text("touch me :)")
hardware.display_update()


class Worm():
	def __init__(self,direction=None):
		self.color = randrgb()
		
		if direction:
			self.direction = direction
		else:
			self.direction = random.random()*math.pi*2
		
			
		self.size = 50
		self.speed = self.size/5
		(x,y) = xy_from_polar(100, self.direction)
		self.x = x
		self.y= y
		#(self.dx,self.dy) = xy_from_polar(1,self.direction)
		self._lastdist = 0.0
	
	def draw(self):
		ctx.rgb(*self.color)
		ctx.round_rectangle(
			self.x-self.size/2,
			self.y-self.size/2,
			self.size,self.size,self.size//2
		).fill()
		
	def mutate(self):
		self.color =  ([max(0,min(1,x+((random.random()-0.5)*0.3))) for x in self.color])
		self.size = 20
		
	
	def move(self):
		dist = math.sqrt(self.x**2+self.y**2)
		target_size = (130-dist)/3
		
		if self.size>target_size: self.size-=1
		
		if self.size<target_size: self.size+=1
		
		self.speed = self.size/5
		
		self.direction += (random.random()-0.5)*math.pi/4
		
		(dx,dy) = xy_from_polar(self.speed,self.direction)
		self.x+=dx
		self.y+=dy
		
		
		if dist>120-self.size/2 and dist>self._lastdist:
			polar_position=math.atan2(self.y,self.x)
			dx=dx*-abs(math.cos(polar_position))
			dy=dy*-abs(math.sin(polar_position))
			self.direction=-math.atan2(dy,dx)
			self.mutate()
		self._lastdist = dist
		
worms = []

for i in range(0):
	worms.append(Worm())
	

def handle_input(data):
	if data.get("type","")=="captouch" and data.get("to")==1:
		print(data)
		worms.append(Worm(data.get("index",0)*2*math.pi/10+math.pi ))
		if len(worms)>10:
			worms.pop(0)

engine = event.Engine()
engine.add_input(event.Event(name="control",action=handle_input, 
	condition=lambda data: data.get("type","")=="captouch" and data.get("to")==1))

while True:
	for w in worms:
		w.draw()
		w.move()
	
	hardware.display_update()
	engine._eventloop_single()
	time.sleep(0.001)
	

#Known problems:
#ctx.rotate(math.pi) turns the display black until powercycled
#ctx.clip() causes crashes

