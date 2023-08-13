from st3m.application import Application, ApplicationContext
#from st3m.property import PUSH_RED, GO_GREEN, BLACK
from st3m.goose import Dict, Any
from st3m.input import InputState
from ctx import Context
import leds
import hardware
from st3m.ui.menu import (
    MenuItem,
    MenuItemBack,
    MenuItemForeground,
    MenuItemNoop,
    MenuItemAction,
    MenuItemLaunchPersistentView,
)
from st3m.ui.elements.menus import SimpleMenu
from st3m.ui.view import View, ViewManager, ViewTransitionBlend
from st3m.reactor import Reactor, Responder
from st3m.ui.elements import overlays
from st3m import settings, logging, processors

import json
import math

LEFT = -1
ENTER = 0
RIGHT = 1

class LessSimpleMenu(SimpleMenu):

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        rbut = None
        if self.input.buttons.os.left.pressed:
            rbut = -1
        elif self.input.buttons.os.right.pressed:
            rbut = 1
        elif self.input.buttons.os.middle.pressed:
            rbut = 0
        if rbut is not None:
            self.action(rbut)

#        dotime=self._fractime+delta_ms
#        self._fractime=dotime%self._timescale
#        if dotime >= self._timescale:
#            for s in range(dotime//self._timescale):
#                update_leds()
#            for idx, led in enumerate(rgb):
#                leds.set_rgb(idx,*rgb[idx])
#            leds.update()

    def action(self, rbut) -> None:
        """
        Do an (right-button) action on a MenuItem
        """
        self._items[self._scroll_controller.target_position()].action(self.vm, rbut)


class ActionableMenuItemForeground(MenuItemForeground):
    def action(self, vm, rbut) -> None:
        print("I got a right action", rbut)
        pass

class ActionableMenuItemNoop(MenuItemNoop):
    def action(self, vm, rbut) -> None:
        pass

class ActionableMenuItem(MenuItem):
    """
    A MenuItem which does something.
    """

    def __init__(self, label: str) -> None:
        self._label = label

    def press(self, vm) -> None:
        pass

    def action(self, vm, rbut) -> None:
        print("I got a right action", rbut)
        pass

    def label(self) -> str:
        return self._label

    def think(self, ins: InputState, delta_ms: int) -> None:
#        print("thinking")
        pass

class ActionableMenuItemCounter(ActionableMenuItem):
    def __init__(self, label: str, method) -> None:
        self._label = label
        self._method = method

    def press(self, vm) -> None:
        pass

    def draw(self, ctx: Context) -> None:
        super().draw(ctx)
        ctx.text(" : ")
        ctx.text(self._method.get())

    def action(self, vm, rbut) -> None:
        self._method.add(rbut)
        pass

class wrapobjattr:
    def __init__(self, obj, attr):
        self._obj=obj
        self._attr=attr
        print("Wrapped",obj, attr)

    def set(self, val):
        self._obj.__dict__[self._attr]=val

    def get(self):
        return self._obj.__dict__[self._attr]

    def add(self, val):
        self.set(self.get()+val)


class ActionableMenuItemRGB(ActionableMenuItem):
    def __init__(self, label: str, r, g, b) -> None:
        self._label = label
        self._rgb = (r,g,b)

    def press(self, vm) -> None:
        pass

    def action(self, vm, rbut) -> None:
        print("I got a right action", rbut)
        pass

    def draw(self, ctx: Context) -> None:
        ctx.save()
        ctx.text(self.label()+":")
#        ctx.rgb(*self._rgb).rectangle(-10,-10,20,20).fill()
        ctx.rgb(*self._rgb).rectangle(25,-10,20,20).fill()
        ctx.restore()

rgb=[(0,0,0)]*40 # Number of LEDs


class Configuration:
    def __init__(self) -> None:
        self.name = "flow3r"
        self.size: int = 75
        self.font: int = 5

    @classmethod
    def load(cls, path: str) -> "Configuration":
        res = cls()
        try:
            with open(path) as f:
                jsondata = f.read()
            data = json.loads(jsondata)
        except OSError:
            data = {}
        if "name" in data and type(data["name"]) == str:
            res.name = data["name"]
        if "size" in data:
            if type(data["size"]) == float:
                res.size = int(data["size"])
            if type(data["size"]) == int:
                res.size = data["size"]
        if "font" in data and type(data["font"]) == int:
            res.font = data["font"]
        return res

    def save(self, path: str) -> None:
        d = {
            "name": self.name,
            "size": self.size,
            "font": self.font,
        }
        jsondata = json.dumps(d)
        with open(path, "w") as f:
            f.write(jsondata)
            f.close()


class NickApp(Application):
    def __init__(self, app_ctx: ApplicationContext) -> None:
        super().__init__(app_ctx)
        self._filename = "/flash/nick.json"
        self._config = Configuration.load(self._filename)
        self._fractime = 0
        self._timescale = 20
        self.sl=0
        self.inhibit=False
        self._action=False


        mine=waldos[0]

        #submenu
        mact=mine.__dict__["action"]
        settings=[MenuItemBack()]
        settings.append(ActionableMenuItemNoop(mact.__class__.__name__))
        for x in mact.cfg:
            if x == 'action':
                settings.append(ActionableMenuItemNoop(x+": "+str(mact.__dict__[x].__class__.__name__)))
            elif x in ['src', 'dst']:
                settings.append(ActionableMenuItemRGB(x, *mact.__dict__[x]))
            else:
                settings.append(ActionableMenuItemNoop(x+": "+str(mact.__dict__[x])))
        menu_sub=LessSimpleMenu(settings)

        #normalmenu
        settings=[]
        settings.append(ActionableMenuItemNoop(mine.__class__.__name__))
        for x in mine.cfg:
            if x == 'action':
#                settings.append(MenuItemNoop(x+": "+str(mine.__dict__[x].__class__.__name__)))
                settings.append(ActionableMenuItemForeground(str(mine.__dict__[x].__class__.__name__), menu_sub))
            elif x in ['steps', 'count', 'src']:
                settings.append(ActionableMenuItemCounter(x, wrapobjattr(mine, x)))
            else:
                settings.append(ActionableMenuItem(x+": "+str(mine.__dict__[x])))
        self._menu=LessSimpleMenu(settings)

        vm = ViewManager(ViewTransitionBlend())
        vm.push(self._menu)
        reactor = Reactor()
        compositor = overlays.Compositor(vm)

        top = processors.ProcessorMidldeware(compositor)
        reactor.set_top(top)
        reactor.run()


    def draw(self, ctx: Context) -> None:
#        ctx.text_align = ctx.RIGHT
#        ctx.text_baseline = ctx.MIDDLE
#        ctx.font_size = self._config.size
#        ctx.font = ctx.get_font_name(self._config.font)

        ctx.rgb(0, 0, 0).rectangle(-120, -120, 240, 240).fill()
        ctx.rgb(0,1,0)

        btn = hardware.application_button_get()

#        ctx.move_to(-100, 0)
#        ctx.save()
#        ctx.scale(self._scale, 1)
        eidx=0
        mine=waldos[0]

        lno=-1
        lh=22
        if self._action:
            mine=mine.__dict__["action"]
            ctx.move_to(-55, -45+lno*lh)
            ctx.text("Action[%d]\n"%eidx)
            lno+=1
            ctx.move_to(-90, -45+lno*lh)
            if self.sl==lno:
                ctx.text(">")
            ctx.text(mine.__class__.__name__)
            ctx.text("\n")
            for x in mine.cfg:
                lno+=1
                ctx.move_to(-90, -45+lno*lh)
                if self.sl==lno:
                    ctx.text(">")
                ctx.text(x)
                ctx.text(": ")
                if x in ['src', 'dst']:
                    ctx.save()
#                    ctx.rgb(*mine.__dict__[x]).move_to(0,-45+(lno-1)*lh+lh).rectangle(0, 0, 22, 22).fill()
                    ctx.rgb(*mine.__dict__[x]).rectangle(0, -45+lno*lh-lh+2, lh-2, lh-2).fill()
                    ctx.restore()
                else:
                    ctx.text(mine.__dict__[x])

        else:
            ctx.move_to(-55, -45+lno*lh)
            ctx.text("Waldo[%d]\n"%eidx)
            lno+=1
            ctx.move_to(-90, -45+lno*lh)
            if self.sl==lno:
                ctx.text(">")
            ctx.text(mine.__class__.__name__)
            ctx.text("\n")
            for x in mine.cfg:
                lno+=1
                ctx.move_to(-90, -45+lno*lh)
                if self.sl==lno:
                    ctx.text(">")
                ctx.text(x)
                ctx.text(": ")
                if x == "action":
                    ctx.text(mine.__dict__[x].__class__.__name__)
                else:
                    ctx.text(mine.__dict__[x])
        lno+=1
        if self.inhibit:
            match btn:
                case 0:
                    self.inhibit=False
        else:
            match btn:
                case -1: #hardware.BUTTON_PRESSED_LEFT:
                    self.sl-=1
                    self.inhibit=True
                case 1: # hardware.BUTTON_PRESSED_RIGHT:
                    self.sl+=1
                    self.inhibit=True
                case 2: # hardware.BUTTON_PRESSED_DOWN:
                    if self.sl==1 and not self._action:
                        self._action=True
                    elif self.sl==0 and self._action:
                        self._action=False
        ctx.move_to(-90, -45+lno*lh)
        ctx.text("%d %d"%(btn, self.sl))
#        ctx.restore()

        # ctx.fill()

    def on_exit(self) -> None:
        self._config.save(self._filename)

    def think(self, ins: InputState, delta_ms: int) -> None:
        super().think(ins, delta_ms)

        dotime=self._fractime+delta_ms
        self._fractime=dotime%self._timescale
        if dotime >= self._timescale:
            for s in range(dotime//self._timescale):
                update_leds()
            for idx, led in enumerate(rgb):
                leds.set_rgb(idx,*rgb[idx])
            leds.update()


from copy import deepcopy
def steps(start, end, no):
    d = (end-start)/no
    return [ int(start+d*x) for x in range(0,no+1) ]

# Tracker enum
class Action:
  class Solid:
    def __init__(self, rgb):
        self.color=rgb
    def start(self):
        return self
    def run(self):
        return self.color

  class Fade:
    cfg = ['src','dst','steps']
    def __init__(self, src, dst, step):
        self.src = src
        self.dst = dst
        self.steps=step
        self.idx = 0
        self.list = list(zip(
                steps(src[0], dst[0], step),
                steps(src[1], dst[1], step),
                steps(src[2], dst[2], step),
                ))
        second=self.list[1:self.steps]
        self.list += second[::-1]
#        print("fade:",self.list)
    def start(self):
        print("Fader started",self)
        self.idx = -1
        return self
    def run(self):
        self.idx += 1
        self.idx %= self.steps*2
        return self.list[self.idx]

  class Fade1:
    cfg = ['src','dst','steps']
    def __init__(self, src, dst, step):
        self.src = src
        self.dst = dst
        self.steps=step
        self.idx = 0
        self.list = list(zip(
                steps(src[0], dst[0], step),
                steps(src[1], dst[1], step),
                steps(src[2], dst[2], step),
                ))
    def start(self):
        self.idx = -1
        return self
    def run(self):
        self.idx += 1
        if self.idx >= self.steps:
            return self.list[-1]
        return self.list[self.idx]
  class Fade2:
    cfg = ['src','dst','steps']
    def __init__(self, src, dst, step):
        self.src = src
        self.dst = dst
        self.steps=step
        self.idx = 0
        self.list = list(zip(
                steps(src[0], dst[0], step),
                steps(src[1], dst[1], step),
                steps(src[2], dst[2], step),
                ))
        second=self.list[1:self.steps]
        self.list += second[::-1]
    def start(self):
        self.idx = -1
        return self
    def run(self):
        self.idx += 1
        if self.idx >= self.steps*2:
            return self.list[0]
        return self.list[self.idx]

class Waldo:
    class Mover:
        def __init__(self, startt, action, src, step, cnt):
            self.action = action
            self.src = src
            self.steps=step
            self.count=cnt
            self.cfg = ['action','src','steps','count']

            self.idx=-1-startt
            print("New Mover:",self.__dict__)

        def step(self):
            self.idx += 1
            if self.idx > reset:
                self.idx=0
            print("Step: ",self.__dict__)
            if self.idx < self.count and self.idx>=0:
                return ((self.src+self.steps*self.idx)%len(rgb), deepcopy(self.action).start())
            else:
                print("Waldo ran out")
                return(None,None)

def update_leds():
    global step

    print("=============> Step:", step)

    for waldo in waldos:
        (idx, action)= waldo.step()
        if idx is not None:
            current[idx]=action

    for idx, todo in enumerate(current):
        if todo is None:
            continue
        rgb[idx]=todo.run()
        print(idx,"=>",rgb[idx])

    step += 1


# Tracker
step=0
current=[None]*len(rgb)

#waldos=[
#        Waldo.Mover(0,Action.Fade2((255,255,255),(0,0,255),10),0,+1,40),
#        Waldo.Mover(50,Action.Fade2((255,255,255),(0,0,255),10),39,-1,40)
#        ]

waldos=[
        Waldo.Mover(0,Action.Fade1((0,0,255),(255,255,255),20),0,+1,41),
        Waldo.Mover(42,Action.Fade1((0,0,255),(255,255,255),20),0,-1,41),
        ]

reset=84
