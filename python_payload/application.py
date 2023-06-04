import ui
import event
import menu

STATE_OFF = 0
STATE_INIT = 10
STATE_BACKGROUND = 200
STATE_FOREGROUND = 300
STATE_ERROR = 500


class Application():
    def __init__(self,title="badge23 app", author="someone@earth"):
        self.title = title
        self.author = author
        self.state = STATE_OFF
        self.has_background = False
        self.has_foreground = True
        self._events_background = []
        self._events_foreground = []
        self.ui = ui.Viewport()
        self.engine = event.the_engine

        self.add_event(event.Event(
                name="exit",
            action=self.exit,
            condition=lambda e: e["type"]=="button" and e.get("from")==2 and e["change"]
        ))

        self.ui.add(ui.Icon(label=self.title))
        
    def __repr__(self):
        return "App "+self.title
    def init(self):
        print("INIT")
        self.state = STATE_INIT
        print("before on_init")
        self.on_init()
        print("after on_init")
        if self.has_background:
            if self._events_background:
                self._set_events(self._events_background,True)
            engine.register_service_loop(self.main_always,True)


    def run(self):
        print ("RUN from",self.state)
        if self.state == STATE_OFF:
            print("go init")
            self.init()
        
        if self.has_foreground:
            self._to_foreground()
        elif self.has_background:
            self._to_background()
            print ("App {} has no foreground, running in background".format(self.title))
        else:
            print("App has neither foreground nor background, not doing anything")

        #start the eventloop if it is not already running
        event.the_engine.eventloop()

        
    def exit(self,data={}):
        print("EXIT")
        self.on_exit()
        if self.state == STATE_FOREGROUND:
            self._to_background()
        
    
    def kill(self):
        #disable all events
        engine.register_service_loop(self.main_always,False)
        engine.register_main_loop(self.main_forground,False)
        self._set_events(self._events_background,False)
        self._set_events(self._events_forground,False)
        self.state = STATE_OFF

    def add_event(self,event,is_background=False):
        if not is_background:
            self._events_foreground.append(event)
        else:
            self._events_background.append(event)

    def is_foreground(self):
        return self.state == STATE_FOREGROUND

    def _to_foreground(self):
        print ("to foreground", self)
        if not self.has_foreground:
            #TODO log
            return
        
        if self._events_background:
            self._set_events(self_events_background,False)
        self.state = STATE_FOREGROUND
        
        if self._events_foreground:
            self._set_events(self._events_foreground,True)
        #TODO should this really happen here??
        menu.menu_disable()
        if self.ui:
            self.ui.draw()
        self.on_foreground()
        
        self.engine.register_main_loop(self.main_foreground,True)
        
          
    def _to_background(self):
        self.state = STATE_BACKGROUND
        if self._events_foreground:
            print("baz")
            self._set_events(self._events_foreground,False)

        self.engine.register_main_loop(self.main_foreground,False)
        if self.has_background:
            self.state = STATE_BACKGROUND
        self.on_background()
        
    def _set_events(self,events,enabled=True):
        for e in events:
            e.set_enabled(enabled)

    def on_init(self):
        print("nothing to init")
        pass

    def on_foreground(self):
        pass

    def on_background(self):
        pass
    
    def on_exit(self):
        pass

    def on_kill(self): 
        pass

    def main_foreground(self):
        #print("nothing")
        pass
    
    def main_always(self):
        pass


    def main(self):
        self.main_foreground()
