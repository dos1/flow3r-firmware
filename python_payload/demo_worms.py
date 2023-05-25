import hardware
import random
import time
import math
import event

<<<<<<< HEAD
def xy_from_polar(r,deg):
    #rad = deg/180*math.pi
    
    return( (
        r * math.sin(deg), #x
        r * math.cos(deg)  #y
    )  )

def randrgb():
    return ((random.random(),random.random(),random.random()))
    
    
=======
>>>>>>> events
WIDTH = 240
HEIGHT = 240

#Define a few RGB (0.0 to 1.0) colors
BLACK = (0,0,0)
RED = (1,0,0)
GREEN = (0,1,0)
BLUE = (0,0,1)
WHITE = (1,1,1)
GREY = (0.5,0.5,0.5)

<<<<<<< HEAD
# Get the global context (representing the whole screen)
global ctx

'''
#center the text horizontally and vertically
ctx.text_align = ctx.CENTER
ctx.text_baseline = ctx.MIDDLE

#ctx.rgb() expects individual values for the channels, so unpack a list/tuple with *
#operations on ctx can be chained
#create a blue background
ctx.rgb(*BLUE).rectangle(-WIDTH/2,-HEIGHT/2,WIDTH,HEIGHT).fill()

#Write some text
ctx.move_to(0,0).rgb(*WHITE).text("Hi :)")
hardware.display_update()
'''

class Worm():
    def __init__(self):
        self.color = randrgb()
        self.direction = random.random()*math.pi*2
        self.size = 10
        self.speed = self.size/5
        (x,y) = xy_from_polar(40, self.direction+90)
=======
def xy_from_polar(r,deg):
    #rad = deg/180*math.pi
    
    return( (
        r * math.sin(deg), #x
        r * math.cos(deg)  #y
    )  )

def randrgb():
    return ((random.random(),random.random(),random.random()))
    

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
>>>>>>> events
        self.x = x
        self.y= y
        #(self.dx,self.dy) = xy_from_polar(1,self.direction)
        self._lastdist = 0.0
    
    def draw(self):
<<<<<<< HEAD
        global ctx
=======
>>>>>>> events
        ctx.rgb(*self.color)
        ctx.round_rectangle(
            self.x-self.size/2,
            self.y-self.size/2,
            self.size,self.size,self.size//2
        ).fill()
        
    def mutate(self):
        self.color =  ([max(0,min(1,x+((random.random()-0.5)*0.3))) for x in self.color])
<<<<<<< HEAD
=======
        self.size = 20
>>>>>>> events
        
    
    def move(self):
        dist = math.sqrt(self.x**2+self.y**2)
<<<<<<< HEAD
        self.size = (120-dist)/3
=======
        target_size = (130-dist)/3
        
        if self.size>target_size: self.size-=1
        
        if self.size<target_size: self.size+=1
        
>>>>>>> events
        self.speed = self.size/5
        
        self.direction += (random.random()-0.5)*math.pi/4
        
        (dx,dy) = xy_from_polar(self.speed,self.direction)
        self.x+=dx
        self.y+=dy
        
        
<<<<<<< HEAD
        if dist>110-self.size/2 and dist>self._lastdist:
=======
        if dist>120-self.size/2 and dist>self._lastdist:
>>>>>>> events
            polar_position=math.atan2(self.y,self.x)
            dx=dx*-abs(math.cos(polar_position))
            dy=dy*-abs(math.sin(polar_position))
            self.direction=-math.atan2(dy,dx)
            self.mutate()
        self._lastdist = dist
<<<<<<< HEAD


global worms

def init():
    global worms
    global ctx
    worms = []
    for i in range(23):
        worms.append(Worm())
    ctx = hardware.get_ctx()

def run():
    global worms
    for w in worms:
        w.draw()
        w.move()
    hardware.display_update()
    time.sleep(0.001)


def foreground():
    pass
=======
          

def handle_input(data):
    worms.append(Worm(data.get("index",0)*2*math.pi/10+math.pi ))
    if len(worms)>10:
        worms.pop(0)

>>>>>>> events

def init(data={}):    
    # Get the global context (representing the whole screen)
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
    global worms
    worms = []
    for i in range(0):
        worms.append(Worm())
    
    event.Event(name="worms_control",action=handle_input, 
        condition=lambda data: data.get("type","")=="captouch" and data.get("value")==1 and data["change"])

def loop(data={}):
    for w in worms:
        w.draw()
        w.move()
    
    hardware.display_update()

def run(data={}):
    init()
    event.the_engine.userloop = loop
    event.the_engine.eventloop()    

worms = []  
ctx = hardware.get_ctx()
#Known problems:
#ctx.rotate(math.pi) turns the display black until powercycled
#ctx.clip() causes crashes

