import ui

class Control():
    def __init__(self,name, default=0, on_set=None):
        #TODO inheritance from Control()
        self.name=name
        self.on_set = on_set
        self._value = default
        self.ui = ui.IconValue(label=self.name,size=60, value=self.get_value())

    def draw(self):
        self.ui.draw()

    def get_value(self):
        return self._value

    def set_value(self, value, do_trigger=True):
        self._value = value
        self.ui.value = value
        if do_trigger:
            if self.on_set:
                self.on_set(value)
    
    def enter(self):
        pass
    
    def scroll(self,delta):
        pass

    def touch_1d(self,x,z):
        pass
    
class ControlSwitch(Control):
    def enter(self):
        self.set_value(not self.get_value())

class ControlFloat(Control):
    def __init__(self, min=0.0,max=1.0,step=0.1, *args, **kwargs):
        self.min=min
        self.max=max
        self.step=step
        super().__init__(*args, **kwargs)

class ControlKnob(ControlFloat):
    def enter(self):
        #repeat action with current value
        self.set_value(self.get_value())

    def scroll(self,delta):
        v = self.get_value()
        v_new = max(self.min,min(self.max,v+delta*self.step))
        self.set_value(v_new)

class ControlSlide(ControlFloat):
    def __init__(self, do_reset=True, *args, **kwargs):
        self.do_reset=do_reset
        super().__init__(*args, **kwargs)

    def touch_1d(self,x,z):
        if z>0: #Inital Contact
            self._saved_value = self.get_value()

        if z==0: #Continous contact
            v = (self.max-self.min)*x
            self.set_value(v)

        if z<0: #Release
            if self.do_reset:
                self.set_value(self._saved_value)
        