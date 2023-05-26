import ui

class ControlKnob():
    def __init__(self,name,action=None,default=0.5):
        #TODO inheritance from Control()
        self.name=name
        self.action=action
        self.value = default
        self.ui = ui.IconValue(label=self.name,size=60, value=self.value)
    
    def draw(self):
        self.ui.draw()

    def enter(self,data={}):
        self.value = 0.8
        self.ui.value = self.value
        if self.action:
            self.action(self.value)

    def scroll(self,delta):
        self.value = max(0,min(1,self.value+delta*0.05))
        self.ui.value = self.value
        print (self.value)
        self.draw()
        if self.action:
            self.action(self.value)

class ControlSwitch():
    def __init__(self,name,action,default):
        #TODO inheritance from Control()
        self.name=name
        self.action=action
        self.value = default
        self.ui = ui.IconValue(label=self.name,size=60, value=self.value)
    
    def draw(self):
        self.ui.draw()

    def enter(self):
        self.value = not self.value
        self.ui.value = self.value
        if self.action:
            self.action(self.value)

    def scroll(self,delta):
        pass
