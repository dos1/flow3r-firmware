import hardware
import random
import math
import time
from math import sin,cos,pi

import gc

WIDTH = 240
HEIGHT = 240

#Define a few RGB (0.0 to 1.0) colors
BLACK = (0,0,0)
RED = (1,0,0)
GREEN = (0,1,0)
BLUE = (0,0,1)
WHITE = (1,1,1)
GREY = (0.5,0.5,0.5)
GO_GREEN = (63/255,255/255,33/53)
PUSH_RED = (251/255,72/255,196/255)

the_ctx = hardware.get_ctx()

# Utility functions
def xy_from_polar(r,phi):
    return (
        r * math.sin(phi), #x
        r * math.cos(phi)  #y
    )

#def ctx_circle(self, x,y, radius, arc_from = -math.pi, arc_to = math.pi):
#    return self.arc(x,y,radius,arc_from,arc_to,True)

#the_ctx.circle = ctx_circle

def randrgb():
    return ((random.random(),random.random(),random.random()))

class UIElement():
    def __init__(self,origin=(0,0)):
        self.children = []
        self.origin = origin
        self.ctx = the_ctx
    
    def draw(self, offset=(0,0)):
        pos = (self.origin[0]+offset[0],self.origin[1]+offset[1])

        self._draw(pos)
        for child in self.children:
            child.draw(pos)

        

    def _draw(self,pos):
        pass

    def add(self, child):
        self.children.append(child)

class Viewport(UIElement):
    def _draw(self,pos):
        self.ctx.rgb(0.3,0.3,0.3).rectangle(-WIDTH/2,-HEIGHT/2,WIDTH,HEIGHT).fill()

class Circle(UIElement):
    def __init__(self,radius,color=PUSH_RED,arc_from=-math.pi, arc_to=math.pi, *args, **kwargs):
        self.radius = radius
        self.color = color
        self.arc_from = arc_from
        self.arc_to = arc_to
        super().__init__()
    
    def _draw(self,pos):
        (x,y)=pos
        self.ctx.move_to(x,y).rgb(*self.color).arc(x,y,self.radius,self.arc_from,self.arc_to,True).fill()


class Text(UIElement):
    def __init__(self,s="foo"):
        self.s = s
        super().__init__()

    def _draw(self, pos):
        self.ctx.text_align = self.ctx.CENTER
        self.ctx.text_baseline = self.ctx.MIDDLE
        self.ctx.font_size = 30
        self.ctx.rgb(1,1,1).move_to(pos[0],pos[1]).text(self.s)

class Icon(UIElement):
    def __init__(self,label="?", size=60):
        self.bg = (random.random(),random.random(),random.random())
        self.fg = 0
        self.label=label
        self.size=size
        self.has_highlight = False
        super().__init__()

    def _draw(self,pos):
        #print("ui.Icon._draw()")
        (x,y) = pos

        self.ctx.text_align = self.ctx.CENTER
        self.ctx.text_baseline = self.ctx.MIDDLE
        self.ctx.font_size = 30

        if self.has_highlight:
            hs = self.size+5;
            self.ctx.move_to(x,y-hs/2).rgb(1,1,1).round_rectangle(
                x-hs/2,
                y-hs/2,
                hs,hs,hs//2
            ).fill()
        
        self.ctx.move_to(x,y).rgb(*self.bg).arc(x,y,self.size/2,-math.pi,math.pi,True).fill()
        #self.ctx.move_to(x,y).rgb(self.bg_r,self.bg_g,self.bg_b).circle(x,y,self.size/2).fill()
        
        self.ctx.rgb(1,1,1).move_to(x,y).text(self.label)

class IconFlower(Icon):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.petal_count= random.randint(4,8)
        self.petal_color = (random.random(),random.random(),random.random())
        self.phi_offset = random.random()
        self.size_offset = random.randint(0,20)
    
    def _draw(self,pos):
        (x,y)=pos
        petal_size=2.3*self.size/self.petal_count+self.size_offset
        self.ctx.font_size=self.size/3

        hs = 3
            
        for i in range(self.petal_count):
            phi = math.pi*2 / self.petal_count * i + self.phi_offset
            r = self.size/2
            (x_,y_) = xy_from_polar(r, phi)
            if self.has_highlight:
                self.ctx.move_to(x+x_,y+y_).rgb(1,1,1).arc(x+x_,y+y_, petal_size/2+hs,-math.pi,math.pi,True).fill()
            self.ctx.move_to(x+x_,y+y_).rgb(*self.petal_color).arc(x+x_,y+y_, petal_size/2,-math.pi,math.pi,True).fill()
        
        #if self.has_highlight:
        #    self.ctx.rgb(1,1,1).arc(x,y, self.size/2+hs,-math.pi,math.pi,True).fill()
        self.ctx.move_to(x,y).rgb(*self.bg).arc(x,y,self.size/2,-math.pi,math.pi,True).fill()

        self.ctx.rgb(*GO_GREEN).move_to(x,y).text(self.label)



class IconValue(Icon):
    def __init__(self, value=0, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.value = value
    
    def _draw(self,pos):
        (x,y) = pos

        if self.has_highlight:
            self.ctx.move_to(x,y).rgb(*GO_GREEN).arc(x,y,self.size/2+5,-pi,pi,True).fill()

        self.ctx.move_to(x,y).rgb(*PUSH_RED).arc(x,y,self.size/2,-pi,pi,True).fill()
        self.ctx.move_to(x,y).rgb(*GO_GREEN).arc(x,y,self.size/2-5,2*pi*self.value,0,1).fill()
        self.ctx.rgb(0,0,0).move_to(x,y).text(self.label)




class GroupStackedVertical(UIElement):
    pass

        
class GroupRing(UIElement):
    def __init__(self,r=100,origin=(0,0),element_center=None):
        self.r = r
        self.angle_offset = 0
        self.element_center=element_center
        super().__init__(origin)

    def draw(self, offset=(0,0)):
        pos = (self.origin[0]+offset[0],self.origin[1]+offset[1])
        self._draw(pos)
        for index in range(len(self.children)):
            #print("child",index)
            child = self.children[index]
            angle = 2*math.pi/len(self.children)*index+self.angle_offset
            x = math.sin(angle)*self.r+pos[0]
            y = -math.cos(angle)*self.r+pos[1]
            #print("pos",(x,y))
            child.draw(offset=(x,y))

    def _draw(self,pos):
        if self.element_center:
            self.element_center._draw(pos)
