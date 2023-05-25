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
    
    def remove_timed(self,group_id):
        self.events_timed = [event for event in self.events_timed if event.group_id==group_id]
        self._sort_timed()
    
    def _sort_timed(self):
        self.events_timed = sorted(self.events_timed, key = lambda event: event.deadline)

    def add_input(self,event):
        self.events_input.append(event)
    
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
        input_state={
            "b0":(hardware.get_button(0),"button",0),
            "b1":(hardware.get_button(1),"button",1)
        }
        
        for i in range(0,10):
            input_state["c"+str(i)]=(hardware.get_captouch(i),"captouch",i)
            
        
        if not self.last_input_state:
            self.last_input_state=input_state
            return
        
        diff=[]
        for key in input_state:
            if input_state[key][0] != self.last_input_state[key][0]:
                diff.append({
                    "type" : input_state[key][1],
                    "index" : input_state[key][2],
                    "value" : input_state[key][0],
                    "from" : self.last_input_state[key][0],
                    "ticks_ms": time.ticks_ms(),
                    "change": True
                })
            else:
                 diff.append({
                    "type" : input_state[key][1],
                    "index" : input_state[key][2],
                    "value" : input_state[key][0],
                    "ticks_ms": time.ticks_ms(),
                    "change": False
                })
    
        if diff:
            #print(diff)
            for d in diff:
                triggered_events = list(filter(lambda e: e.condition(d),self.events_input))
                #print (triggered_events)
                #map(lambda e: e.trigger(d), triggered_events)
                for e in triggered_events:
                    e.trigger(d)
                    
            
        self.last_input_state=input_state        
        
    def _handle_userloop(self):
        if self.userloop:
            self.userloop()
        
    def _eventloop_single(self):
        self._handle_timed()
        self._handle_input()
        self._handle_userloop()
        
            
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
    def __init__(self,name="unknown",data={},action=None,condition=None,group_id=None):
        #print (action)
        self.name = name
        self.eventtype = None
        self.data = data
        self.action = action
        self.condition = condition
        if not condition:
            self.condition = lambda x: True
        self.group_id=group_id
        the_engine.add(self)
            
        
    def trigger(self,triggerdata={}):
        print ("triggered {} (with {})".format(self.name,triggerdata))
        if not self.action is None:
            triggerdata.update(self.data)
            self.action(triggerdata)
    
    def remove(self):
        if self in the_engine.events_input:
            the_engine.events_input.remove(self)
        if self in the_engine.events_timed:
            the_engine.events_timed.remove(self)

class EventTimed(Event):
    def __init__(self,ms,name="timer", *args, **kwargs):
        #super().__init__(name,data,action)
        self.deadline = time.ticks_add(time.ticks_ms(),ms)
        
        super().__init__(*args, **kwargs)
        self.name=name
        self.type=EVENTTYPE_TIMED
        
    def __repr__(self):
        return ("event on tick {} ({})".format(self.deadline,self.name))


class Sequence():
    def __init__(self,bpm=60,loop=True,steps=16,action=None):
        self.group_id = random.randint(0,100000000)
        self.bpm = bpm
        if not action:
            self.action = lambda data: print("step {}".format(data.get("step")))
        else:
            self.action = action
        stepsize_ms = int(60*1000/bpm)
        for i in range(steps):
            EventTimed(stepsize_ms*i,name="seq{}".format(i),action=self.action, data={'step':i}, group_id=self.group_id)
        
        if loop:
            EventTimed(stepsize_ms*steps,name="loop", group_id=self.group_id, action=lambda data: Sequence(bpm=bpm,loop=loop,steps=steps,action=action))

    def remove(self):
        the_engine.remove_timed(self.group_id)
        
global the_engine
the_engine = Engine()