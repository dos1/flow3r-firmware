import hardware
import random
import time
import math

def xy_from_polar(r,deg):
    #rad = deg/180*math.pi

    return (
        r * math.sin(deg), #x
        r * math.cos(deg)  #y
    )

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

# The global context (representing the whole screen)
ctx = None

worms = None


class Worm():
    def __init__(self):
        self.color = randrgb()
        self.direction = random.random()*math.pi*2
        self.size = 10
        self.speed = self.size/5
        (x,y) = xy_from_polar(40, self.direction+90)
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
        
    
    def move(self):
        dist = math.sqrt(self.x**2+self.y**2)
        self.size = (120-dist)/3
        self.speed = self.size/5
        
        self.direction += (random.random()-0.5)*math.pi/4
        
        (dx,dy) = xy_from_polar(self.speed,self.direction)
        self.x+=dx
        self.y+=dy
        
        
        if dist>110-self.size/2 and dist>self._lastdist:
            polar_position=math.atan2(self.y,self.x)
            dx=dx*-abs(math.cos(polar_position))
            dy=dy*-abs(math.sin(polar_position))
            self.direction=-math.atan2(dy,dx)
            self.mutate()
        self._lastdist = dist


def init():
    global worms
    global ctx
    worms = []
    for i in range(23):
        worms.append(Worm())
    ctx = hardware.get_ctx()

# TODO(q3k): factor out frame limiter
last_render = None
target_fps = 30
target_delta = 1000 / target_fps
frame_slack = None
last_report = None
def run():
    global last_render
    global last_report
    global frame_slack
    now = time.ticks_ms()

    if last_render is not None:
        delta = now - last_render
        if frame_slack is None:
            frame_slack = target_delta - delta
        if delta < target_delta:
            return

        if last_report is None or (now - last_report) > 1000:
            fps = 1000/delta
            print(f'fps: {fps:.3}, frame budget slack: {frame_slack:.3}ms')
            last_report = now

    # Simulation is currently locked to FPS.
    global worms
    for w in worms:
        w.draw()
        w.move()
    hardware.display_update()
    last_render = now


def foreground():
    ctx.text_align = ctx.CENTER
    ctx.text_baseline = ctx.MIDDLE

    #ctx.rgb() expects individual values for the channels, so unpack a list/tuple with *
    #operations on ctx can be chained
    #create a blue background
    ctx.rgb(*BLUE).rectangle(-WIDTH/2,-HEIGHT/2,WIDTH,HEIGHT).fill()

    #Write some text
    ctx.move_to(0,0).rgb(*WHITE).text("Hi :)")
    hardware.display_update()

#Known problems:
#ctx.rotate(math.pi) turns the display black until powercycled
#ctx.clip() causes crashes

