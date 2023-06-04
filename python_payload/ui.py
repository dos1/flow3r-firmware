import hardware
import random
import math
import time

class UIElement():
    def __init__(self,origin=(0,0)):
        self.children = []
        self.origin = origin
        self.ctx = hardware.get_ctx()
    
    def draw(self, offset=(0,0)):
        pos = (self.origin[0]+offset[0],self.origin[1]+offset[1])

        self._draw(pos)
        for child in self.children:
            child.draw(pos)

    def _draw(self,pos):
        #print(pos)
        pass

    def add(self, child):
        self.children.append(child)

class Text(UIElement):
    def __init__(self,s="foo"):
        self.s = s
        super().__init__()

    def _draw(self, pos):
        self.ctx.text_align = self.ctx.CENTER
        self.ctx.text_baseline = self.ctx.MIDDLE
        self.ctx.rgb(1,1,1).move_to(pos[0],pos[1]).text(self.s)

class Icon(UIElement):
    def __init__(self,label="?", size=60):
        self.bg_r = random.random()
        self.bg_g = random.random()
        self.bg_b = random.random()
        self.fg = 0
        self.label=label
        self.size = size
        self.has_highlight = False
        super().__init__()

    def _draw(self,pos):
        x = int(pos[0])
        y = int(pos[1])
        width = 55
        height = 40
        self.ctx.text_align = self.ctx.CENTER
        self.ctx.text_baseline = self.ctx.MIDDLE

        if self.has_highlight:
            hs = self.size+5;
            self.ctx.move_to(x,y-hs/2).rgb(1,1,1).round_rectangle(
                x-hs/2,
                y-hs/2,
                hs,hs,hs//2
            ).fill()
        self.ctx.move_to(x,y-self.size/2).rgb(self.bg_r,self.bg_g,self.bg_b).round_rectangle(
            x-self.size/2,
            y-self.size/2,
            self.size,self.size,self.size//2
        ).fill()
        self.ctx.rgb(1,1,1).move_to(x,y).text(self.label)
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
            child = self.children[index]
            angle = 2*math.pi/len(self.children)*index+self.angle_offset
            x = math.sin(angle)*self.r+pos[0]
            y = -math.cos(angle)*self.r+pos[1]
            child.draw(offset=(x,y))

    def _draw(self,pos):
        if self.element_center:
            self.element_center._draw(pos)

def test():
    group = UIElement((10,0))
    group.add(UIElement((10,10)))
    group.add(UIElement((20,20)))
    #group.draw()

    ring = GroupRing(r=80)
    ctx = hardware.get_ctx()
    for i in range(12):
        ring.add(Icon(str(i)))
        hardware.display_update()
    while True:
        ctx.rectangle(0,0,240,240).rgb(1,1,1).fill()
        ring.draw()
        hardware.display_update()
        ring.angle_offset+=2*math.pi/60
        time.sleep(0.01)
        hardware.display_update()

#test()
