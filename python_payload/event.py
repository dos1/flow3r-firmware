import hardware
import time 
import math


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
	
	def add_timed(self,event):
		self.events_timed.append(event)
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
					"to" : input_state[key][0],
					"from" : self.last_input_state[key][0],
					"ticks_ms": time.ticks_ms()
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
		
		
		
	def _eventloop_single(self):
		self._handle_timed()
		self._handle_input()
			
	def eventloop(self):
		while True:
			self._eventloop_single()
			time.sleep(0.005)
		
class Event():
	def __init__(self,name="unknown",data={},action=None,condition=None):
		#print (action)
		self.name = name
		self.eventtype = None
		self.data = data
		self.action = action
		self.condition = condition
		if not condition:
			self.condition = lambda x: True
			
		#print (data)
		
	def trigger(self,triggerdata={}):
		print ("triggered {} (with {})".format(self.name,triggerdata))
		if not self.action is None:
			triggerdata.update(self.data)
			#print("trigger")
			self.action(triggerdata)
	

class EventTimed(Event):
	def __init__(self,ms,name="timer", *args, **kwargs):
		#super().__init__(name,data,action)
		super().__init__(*args, **kwargs)
		self.name=name
		self.deadline = time.ticks_add(time.ticks_ms(),ms)
		self.type=EVENTTYPE_TIMED
		
	def __repr__(self):
		return ("event on tick {} ({})".format(self.deadline,self.name))
		
e = Engine()
e.add_timed(EventTimed(200,name="bar",action=lambda data: print("GNANGNAGNA")))
e.add_timed(EventTimed(100,name="foo"))
e.add_input(Event(name="baz",
	action=lambda data: print(data),
	condition=lambda data: data.get('type')=="captouch")
)

print (e.events_timed)
#e.eventloop()
