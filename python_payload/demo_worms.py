#python
import random
import time
import math

#badge23
import event
import application
import ui

# Subclass Application
class AppWorms(application.Application):

    def on_init(self):
        print("on init")
        self.worms = []
        for i in range(0):
            worms.append(Worm())
    
    def on_foreground(self):
        print("on foreground")
        ctx = app.ui.ctx

        #center the text horizontally and vertically
        ctx.text_align = ctx.CENTER
        ctx.text_baseline = ctx.MIDDLE

        #ctx.rgb() expects individual values for the channels, so unpack a list/tuple with *
        #operations on ctx can be chained
        #create a blue background
        ctx.rgb(*ui.BLUE).rectangle(-ui.WIDTH/2,-ui.HEIGHT/2,ui.WIDTH,ui.HEIGHT).fill()

        #Write some text
        ctx.move_to(0,0).rgb(*ui.WHITE).text("touch me :)")

    def main_foreground(self):
        #print("worms")
        for w in self.worms:
            w.draw()
            w.move()

app = AppWorms("worms")

def handle_input(data):
    worms = app.worms
    worms.append(Worm(data.get("index",0)*2*math.pi/10+math.pi ))
    if len(worms)>10:
        worms.pop(0)

app.add_event(event.Event(
    name="worms_control",
    action=handle_input, 
    condition=lambda data: data.get("type","")=="captouch" and data.get("value")==1 and data["change"],
    )
)

app.add_event(event.Event(
    name="worms_exit",
    action=app.exit,
    condition=lambda e: e["type"]=="button" and e.get("from")==2 and e["change"]
))

class Worm():
    def __init__(self,direction=None):
        self.color = ui.randrgb()
    
        if direction:
            self.direction = direction
        else:
            self.direction = random.random()*math.pi*2
        
        self.size = 50
        self.speed = self.size/5
        (x,y) = ui.xy_from_polar(100, self.direction)
        self.x = x
        self.y= y
        #(self.dx,self.dy) = xy_from_polar(1,self.direction)
        self._lastdist = 0.0
    
    def draw(self):
        app.ui.ctx.rgb(*self.color)
        app.ui.ctx.round_rectangle(
            self.x-self.size/2,
            self.y-self.size/2,
            self.size,self.size,self.size//2
        ).fill()

    def mutate(self):
        self.color =  ([max(0,min(1,x+((random.random()-0.5)*0.3))) for x in self.color])
        
    
    def move(self):
        dist = math.sqrt(self.x**2+self.y**2)
        target_size = (130-dist)/3
        
        if self.size>target_size: self.size-=1
        
        if self.size<target_size: self.size+=1
        
        self.speed = self.size/5
        
        self.direction += (random.random()-0.5)*math.pi/4
        
        (dx,dy) = ui.xy_from_polar(self.speed,self.direction)
        self.x+=dx
        self.y+=dy
        
        
        if dist>120-self.size/2 and dist>self._lastdist:
            polar_position=math.atan2(self.y,self.x)
            dx=dx*-abs(math.cos(polar_position))
            dy=dy*-abs(math.sin(polar_position))
            self.direction=-math.atan2(dy,dx)
            self.mutate()
        self._lastdist = dist


#app.run()
#app.engine.eventloop()

#Known problems:
#ctx.rotate(math.pi) turns the display black until powercycled
#ctx.clip() causes crashes