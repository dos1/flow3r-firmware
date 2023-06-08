import hardware
import time 
import math
import random


EVENTTYPE_TIMED = 1
EVENTTYPE_INPUT = 2
#EVENTTYPE_BUTTON = 2
#EVENTTYPE_CAPTOUCH = 3
#EVENTTYPE_CAPCROSS = 4

class Engine():
    def __init__(self):
        self.events_timed = []
        self.events_input = []
        self.next_timed = None
        self.last_input_state = None
        self.userloop = None
        self.is_running = False
    
    def add(self,event):
        if isinstance(event,EventTimed):
            self.add_timed(event)
        elif isinstance(event,Event):
            self.add_input(event)
            
    def add_timed(self,event):
        self.events_timed.append(event)
        self._sort_timed()

    def add_input(self,event):
        self.events_input.append(event)

    def remove(self,group_id):
        self.remove_input(group_id)
        self.remove_timed(group_id)

    def remove_timed(self,group_id):
        #print("before:",len(self.events_timed))
        self.events_timed = [event for event in self.events_timed if event.group_id!=group_id]
        self._sort_timed()
        #print("after",len(self.events_timed))
    
    def remove_input(self,group_id):
        self.events_input = [event for event in self.events_input if event.group_id!=group_id]

    def register_main_loop(self,loop,enable=True):
        if enable:
            #print ("new userloop",loop)
            #loop()
            self.userloop = loop
        elif self.userloop == loop:
            #print ("removed userloop")
            self.userloop = None

    def _sort_timed(self):
        self.events_timed = sorted(self.events_timed, key = lambda event: event.deadline)

    
    def _handle_timed(self):
        if not self.next_timed and self.events_timed:
            self.next_timed = self.events_timed.pop(0)
    
        now = time.ticks_ms()
        
        if self.next_timed:
            diff = time.ticks_diff(self.next_timed.deadline, now)
            if diff <= 0:
                self.next_timed.trigger({"ticks_ms":now, "ticks_delay": -diff})
                self.next_timed = None
                
    def _handle_input(self):
        input_state = []

        #buttons
        input_state.append({
            "type" : "button",
            "index" : 0,
            "value" : hardware.menu_button_get()
        })

        input_state.append({
            "type" : "button",
            "index" : 1,
            "value" : hardware.application_button_get()
        })

        #captouch
        for i in range(0,10):
            input_state.append({
                "type" : "captouch",
                "index" : i,
                "value" : hardware.get_captouch(i),
                "radius" : hardware.captouch_get_petal_rad(i),
                "angle" : hardware.captouch_get_petal_phi(i)/10000
            })

        
        if not self.last_input_state:
            self.last_input_state=input_state
            #tprint (input_state)
            return
        
        for i in range(len(input_state)):
            entry = input_state[i]
            last_entry = self.last_input_state[i]
            
            #update for all
            entry["ticks_ms"] = time.ticks_ms()

            if entry["value"] != last_entry["value"]:
                #update only when value changed
                entry["change"] = True
                entry["from"] = last_entry["value"]
            else:
                #update only when value did not change
                 entry["change"] = False

            #find and trigger the events q
            triggered_events = list(filter(lambda e: e.enabled and e.condition(entry),self.events_input))
            #print (triggered_events)
            #map(lambda e: e.trigger(d), triggered_events)
            for e in triggered_events:
                e.trigger(entry)
            
        self.last_input_state=input_state        
        
    def _handle_userloop(self):
        if not self.userloop is None:
            #print("userloop",self.userloop)
            self.userloop()
        
    def _eventloop_single(self):
        self._handle_timed()
        self._handle_input()
        self._handle_userloop()
        hardware.display_update()
        
            
    def eventloop(self):
        if self.is_running:
            print ("eventloop already running")
            return
        else:
            print("eventloop started")
        self.is_running=True
        while self.is_running:
            self._eventloop_single()

            time.sleep(0.005)
        
class Event():
    def __init__(self,name="unknown",data={},action=None,condition=None,group_id=None,enabled=False):
        #print (action)
        self.name = name
        self.eventtype = None
        self.data = data
        self.action = action
        self.condition = condition
        self.enabled = enabled
        if not condition:
            self.condition = lambda x: True
        self.group_id=group_id
        
        if enabled:
            self.set_enabled()
            
            
        
    def trigger(self,triggerdata={}):
        print ("triggered {} (with {})".format(self.name,triggerdata))
        if not self.action is None:
            triggerdata.update(self.data)
            self.action(triggerdata)
    
    def set_enabled(self,enabled=True):
        self.enabled=enabled
        if enabled:
            the_engine.add(self)
        else:
            self.remove()

    def remove(self):
        print ("remove",self)
        while self in the_engine.events_input:
            print ("from input")
            the_engine.events_input.remove(self)
        while self in the_engine.events_timed:
            print("from timed")
            the_engine.events_timed.remove(self)
            the_engine._sort_timed()

class EventTimed(Event):
    def __init__(self,ms,name="timer", *args, **kwargs):
        #super().__init__(name,data,action)
        self.deadline = time.ticks_add(time.ticks_ms(),ms)
        
        super().__init__(*args, **kwargs)
        self.name=name
        self.type=EVENTTYPE_TIMED
        
    def __repr__(self):
        return ("event on tick {} ({})".format(self.deadline,self.name))


#hack, make this oo
def on_restart(data):
    print("loop sequence")
    obj = data["object"]
    if obj.is_running:
        obj.start()



class Sequence():
    def __init__(self,bpm=60,loop=True,steps=16,action=None):
        self.group_id = random.randint(0,100000000)
        self.bpm = bpm
        self.steps = steps
        self.repeat_event = None
        self.loop = loop
        self.events = []
        self.is_running = False
        
        if not action:
            self.action = lambda data: print("step {}".format(data.get("step")))
        else:
            self.action = action
    
    def start(self):
        if self.is_running: self.stop()
        stepsize_ms = int(60*1000/self.bpm)
        for i in range(self.steps):
            print("adding sequence event ", i)
            self.events.append(EventTimed(stepsize_ms*i,name="seq{}".format(i),action=self.action, data={'step':i}, group_id=self.group_id,enabled=True))
        if self.loop:
            self.repeat_event=EventTimed(stepsize_ms*self.steps,name="loop", group_id=self.group_id, enabled=True, action=on_restart, data={"object":self})
        self.is_running=True
    
    def stop(self):
        #for e in self.events: e.remove()
        print("sequence stop")
        the_engine.remove_timed(group_id=self.group_id)
        self.events = []
        if self.repeat_event: self.repeat_event.remove()
        self.is_running=False
    
        
global the_engine
the_engine = Engine()
