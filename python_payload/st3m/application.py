from st3m import logging
log = logging.Log(__name__,level=logging.INFO)
log.info("import")
from . import ui,event,menu

STATE_OFF = 0
STATE_INIT = 10
STATE_BACKGROUND = 200
STATE_FOREGROUND = 300
STATE_ERROR = 500

log.info("setting up application")
class Application():
    def __init__(self,title="badge23 app", author="someone@earth", exit_on_menu_enter=True):
        log.info(f"__init__ app '{title}'")
        self.title = title
        self.author = author
        self.state = STATE_OFF
        self.has_background = False
        self.has_foreground = True
        self._events_background = []
        self._events_foreground = []
        self.ui = ui.Viewport()
        self.icon = ui.Icon(label=self.title, size=100)
        self.engine = event.the_engine
        self.menu = None
        if exit_on_menu_enter:
            self.add_event(event.Event(
                    name="exit",
                action=self.exit,
                condition=lambda e: e["type"]=="button" and e.get("from")==2 and e["change"]
            ))
        
    def __repr__(self):
        return "App "+self.title
    def init(self):
        log.info(f"init app '{self.title}'")
        self.state = STATE_INIT
        self.on_init()
        if self.has_background:
            if self._events_background:
                self._set_events(self._events_background,True)
            engine.register_service_loop(self.main_always,True)


    def run(self):
        log.info(f"run app '{self.title}' from state {self.state}")
        if self.state == STATE_OFF:
            log.info("from STATE_OFF, doing init()")
            self.init()
        if self.has_foreground:
            self._to_foreground()
        elif self.has_background:
            self._to_background()
        else:
            log.warning(f"App {self.title} has neither foreground nor background, not doing anything")

        #start the eventloop if it is not already running
        if not event.the_engine.is_running:
            log.info("eventloop not yet running, starting")
            event.the_engine.eventloop()

        
    def exit(self,data={}):
        log.info(f"exit app '{self.title}' from state {self.state}")
        self.on_exit()
        if self.state == STATE_FOREGROUND:
            self._to_background()
        
    
    def kill(self):
        #disable all events
        log.info(f"kill app '{self.title}' from state {self.state}")
        engine.register_service_loop(self.main_always,False)
        
        engine.foreground_app = None
        self._set_events(self._events_background,False)
        self._set_events(self._events_forground,False)
        self.state = STATE_OFF

    def draw(self):
        self.ui.draw()
        self.on_draw()
    
    def tick(self):
        self.main_foreground()

    def add_event(self,event,is_background=False):
        if not is_background:
            self._events_foreground.append(event)
        else:
            self._events_background.append(event)

    def is_foreground(self):
        return self.state == STATE_FOREGROUND

    def _to_foreground(self):
        log.info(f"to_foreground app '{self.title}' from state {self.state}")
        if not self.has_foreground:
            log.error(f"app has no foreground!")
            return
        
        if self._events_background:
            self._set_events(self_events_background,False)
        self.state = STATE_FOREGROUND
        
        if self._events_foreground:
            self._set_events(self._events_foreground,True)
        self.on_foreground()
        
        self.engine.foreground_app = self
          
    def _to_background(self):
        log.info(f"to_background app '{self.title}' from state {self.state}")
        self.state = STATE_BACKGROUND
        if self._events_foreground:
            self._set_events(self._events_foreground,False)

        self.engine.foreground_app = None
        if self.has_background:
            self.state = STATE_BACKGROUND
        self.on_background()
        
    def _set_events(self,events,enabled=True):
        for e in events:
            e.set_enabled(enabled)

    def on_init(self):
        log.info(f"app {self.title}: on_init()")

    def on_foreground(self):
        log.info(f"app {self.title}: on_foreground()")

    def on_background(self):
        log.info(f"app {self.title}: on_background()")
    
    def on_exit(self):
        log.info(f"app {self.title}: on_exit()")

    def on_kill(self): 
        log.info(f"app {self.title}: on_kill()")

    def on_draw(self):
        #self.ui.ctx.rgb(*ui.RED).rectangle(-120,-120,120,120).fill()
        #self.icon.draw()
        log.debug(f"app {self.title}: on_draw()")

    def main_foreground(self):
        pass
    
    def main_always(self):
        pass


    def main(self):
        self.main_foreground()
