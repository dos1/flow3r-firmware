from st3m import logging,menu
log = logging.Log(__name__,level=logging.INFO)
log.info("import")

from . import ui

class Control():
    def __init__(self,name, default=0, on_set=None, on_get=None, on_mod=None):
        #TODO inheritance from Control()
        self.name=name
        self.on_set = on_set
        self.on_get = on_get
        self.on_mod = on_mod

        if not self.on_get:
            self._value = default
        self.ui = ui.IconValue(label=self.name,size=60, value=self.get_value())
        self.ui.get_value = self.get_normal_value

        self.menu = menu.MenuControl(self)

    def draw(self):
        self.ui.value = self.get_value()
        self.ui.draw()

    def get_normal_value(self):
        v = self.get_value()
        n = min(1,max(0,v))
        return n

    def get_value(self,update=True):
        if update and self.on_get:
            self._value = self.on_get()
        return self._value

    def set_value(self, value, do_trigger=True):
        self._value = value
        if do_trigger:
            if self.on_set:
                self.on_set(value)
    
    def mod_value(self,delta):
        self._value = self.on_mod(delta)

    
    def enter_menu(self):
        menu.submenu_push(self.menu)
    
    def scroll(self,delta):
        pass

    def touch_1d(self,x,z):
        pass
    
class ControlSwitch(Control):
    def enter(self):
        self.set_value(not self.get_value())
        self.draw()

    def scroll(self,delta):
        self.enter()

    def touch_1d(self,x,z):
        if z<0: #Release
            self.enter()


class ControlFloat(Control):
    def __init__(self, min=0.0,max=1.0,step=0.1, *args, **kwargs):
        self.min=min
        self.max=max
        self.step=step
        super().__init__(*args, **kwargs)

    def get_normal_value(self):
        v = self.get_value()
        n = (v-self.min)/(self.max-self.min)
        return n

class ControlKnob(ControlFloat):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._x_init = 0
        self._touch_started_here = False

    def enter(self):
        #repeat action with current value
        self.set_value(self.get_value())
        self.draw()

    def scroll(self,delta):
        if self.on_mod:
            #print("control: on mod")
            self.on_mod(delta)

        elif self.on_set:
            v = self.get_value()
            v_new = max(self.min,min(self.max,v+delta*self.step))
            self.set_value(v_new)
        self.draw()

    def touch_1d(self,x,z):
        if z>0: #Inital Contact
            self._x_init = x
            self._touch_started_here=True

        if z==0: #Continous contact
            if not self._touch_started_here: 
                return
            diff = self._x_init-x
            self.scroll(5*diff)
            
        if z<0: #Release
            self._touch_started_here = False


class ControlSlide(ControlFloat):
    def __init__(self, do_reset=True, *args, **kwargs):
        self.do_reset=do_reset
        self._saved_value = None
        super().__init__(*args, **kwargs)

    def touch_1d(self,x,z):
        if z>0: #Inital Contact
            self._saved_value = self.get_value()

        if self._saved_value is None: return


        if z==0: #Continous contact
            
            v = (self.max-self.min)*x+self.min
            self.set_value(v)
            #print("c",x,v,self.max,self.min)

        if z<0: #Release
            if self.do_reset:
                self.set_value(self._saved_value)
            self._saved_value = None
        self.draw()

class ControlString(Control):
    pass

class ControlTextField():
    pass