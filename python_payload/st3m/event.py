from st3m import logging

log = logging.Log(__name__, level=logging.INFO)
log.info("import")

from st3m.system import hardware, captouch

import kernel
import time
import math
import random


EVENTTYPE_TIMED = 1
EVENTTYPE_INPUT = 2
# EVENTTYPE_BUTTON = 2
# EVENTTYPE_CAPTOUCH = 3
# EVENTTYPE_CAPCROSS = 4


class Engine:
    def __init__(self):
        self.events_timed = []
        self.events_input = []
        self.next_timed = None
        self.last_input_state = None
        self.userloop = None
        self.is_running = False
        self.foreground_app = None
        self.active_menu = None
        self.overlay = None

        self._draw_started = None
        self._draw_ended = None

        self._think_started = None
        self._think_ended = None

        self._last_report = None

    def _report_heap(self):
        st = kernel.heap_stats()
        g = st.general
        d = st.dma
        log.info(
            f"Heap: General: {g.total_free_bytes:d}B free, {g.total_allocated_bytes:d}B allocated, {g.largest_free_block:d}B largest free block"
        )
        log.info(
            f"Heap: DMA: {d.total_free_bytes:d}B free, {d.total_allocated_bytes:d}B allocated, {d.largest_free_block:d}B largest free block"
        )

    def _report(self):
        now = time.ticks_ms()
        if self._last_report is not None and (now - self._last_report) < 1000:
            return

        if self._draw_started is not None and self._draw_ended is not None:
            draw_time = self._draw_ended - self._draw_started
            log.info(f"EventLoop Draw time: {draw_time:.3f}ms")

        if self._think_started is not None and self._think_ended is not None:
            think_time = self._think_ended - self._think_started
            log.info(f"EventLoop Think time: {think_time:.3f}ms")

        self._report_heap()
        self._last_report = now

    def add(self, event):
        if isinstance(event, EventTimed):
            self.add_timed(event)
        elif isinstance(event, Event):
            self.add_input(event)

    def add_timed(self, event):
        self.events_timed.append(event)
        self._sort_timed()

    def add_input(self, event):
        self.events_input.append(event)

    def remove(self, group_id):
        self.remove_input(group_id)
        self.remove_timed(group_id)

    def remove_timed(self, group_id):
        # print("before:",len(self.events_timed))
        self.events_timed = [
            event for event in self.events_timed if event.group_id != group_id
        ]
        self._sort_timed()
        # print("after",len(self.events_timed))

    def remove_input(self, group_id):
        self.events_input = [
            event for event in self.events_input if event.group_id != group_id
        ]

    def _sort_timed(self):
        self.events_timed = sorted(self.events_timed, key=lambda event: event.deadline)

    def _handle_timed(self):
        if not self.next_timed and self.events_timed:
            self.next_timed = self.events_timed.pop(0)

        now = time.ticks_ms()

        if self.next_timed:
            diff = time.ticks_diff(self.next_timed.deadline, now)
            if diff <= 0:
                self.next_timed.trigger({"ticks_ms": now, "ticks_delay": -diff})
                self.next_timed = None

    def _handle_input(self, delta):
        # log.info("input {delta}")
        input_state = []

        # buttons
        input_state.append(
            {"type": "button", "index": 0, "value": hardware.menu_button_get()}
        )

        input_state.append(
            {"type": "button", "index": 1, "value": hardware.application_button_get()}
        )

        # captouch
        cps = captouch.read()
        for i in range(0, 10):
            petal = cps.petals[i]
            (radius, angle) = petal.position
            input_state.append(
                {
                    "type": "captouch",
                    "index": i,
                    "value": petal.pressed,
                    "radius": radius,
                    "angle": angle,
                }
            )

        if not self.last_input_state:
            self.last_input_state = input_state
            return

        for i in range(len(input_state)):
            entry = input_state[i]
            last_entry = self.last_input_state[i]

            # update for all
            entry["ticks_ms"] = time.ticks_ms()
            entry["delta"] = delta

            if entry["value"] != last_entry["value"]:
                # update only when value changed
                entry["change"] = True
                entry["from"] = last_entry["value"]
            else:
                # update only when value did not change
                entry["change"] = False

            # find and trigger the events q
            triggered_events = list(
                filter(lambda e: e.enabled and e.condition(entry), self.events_input)
            )
            # log.info(triggered_events)
            # map(lambda e: e.trigger(d), triggered_events)
            for e in triggered_events:
                e.trigger(entry)

        self.last_input_state = input_state

    def _handle_userloop(self):
        if self.foreground_app:
            self.foreground_app.tick()

    def _handle_draw(self, ctx):
        self._draw_started = time.ticks_ms()

        if self.foreground_app:
            self.foreground_app.draw(ctx)
        if self.active_menu:
            self.active_menu.draw(ctx)
        if self.overlay:
            self.overlay.draw(ctx)

        self._draw_ended = time.ticks_ms()

    def _eventloop_single(self, delta):
        self._think_started = time.ticks_ms()

        self._handle_timed()
        self._handle_input(delta)
        self._handle_userloop()

        self._think_ended = time.ticks_ms()

    def eventloop(self):
        log.info("starting eventloop")
        if self.is_running:
            log.warning("eventloop already running, doing nothing")
            return
        self.is_running = True

        last_eventloop = None

        ctx = None
        while self.is_running:
            start = time.ticks_ms()
            deadline = start + 20
            self._report()

            if last_eventloop is not None:
                delta = (start - last_eventloop) / 1000.0
                if delta >= 0.01:
                    last_eventloop = start
                    self._eventloop_single(delta)
            else:
                last_eventloop = start

            post_think = time.ticks_ms()

            if ctx is None:
                ctx = hardware.get_ctx()
                if ctx is not None:
                    self._handle_draw(ctx)

            post_draw = time.ticks_ms()

            if ctx is not None and not hardware.display_pipe_full():
                hardware.display_update(ctx)
                # log.info("update")
                ctx = None

            post_submit = time.ticks_ms()

            wait = deadline - time.ticks_ms()
            if wait > 0:
                hardware.freertos_sleep(wait)
            else:
                log.warning(f"Application took too long too process! Slack {wait}ms.")
                log.warning(
                    f"Think: {post_think-start}ms, Draw: {post_draw-post_think}ms, Submit: {post_submit-post_draw}ms"
                )
                hardware.freertos_sleep(1)


class Event:
    def __init__(
        self,
        name="unknown",
        data={},
        action=None,
        condition=None,
        group_id=None,
        enabled=False,
    ):
        # print (action)
        self.name = name
        self.eventtype = None
        self.data = data
        self.action = action
        self.condition = condition
        self.enabled = enabled
        if not condition:
            self.condition = lambda x: True
        self.group_id = group_id

        if enabled:
            self.set_enabled()

    def trigger(self, triggerdata={}):
        log.debug("triggered {} (with {})".format(self.name, triggerdata))
        if not self.action is None:
            triggerdata.update(self.data)
            self.action(triggerdata)

    def set_enabled(self, enabled=True):
        self.enabled = enabled
        if enabled:
            the_engine.add(self)
        else:
            self.remove()

    def remove(self):
        log.info(f"remove {self}")
        while self in the_engine.events_input:
            # print ("from input")
            the_engine.events_input.remove(self)
        while self in the_engine.events_timed:
            # print("from timed")
            the_engine.events_timed.remove(self)
            the_engine._sort_timed()


class EventTimed(Event):
    def __init__(self, ms, name="timer", *args, **kwargs):
        # super().__init__(name,data,action)
        self.deadline = time.ticks_add(time.ticks_ms(), ms)

        super().__init__(*args, **kwargs)
        self.name = name
        self.type = EVENTTYPE_TIMED

    def __repr__(self):
        return "event on tick {} ({})".format(self.deadline, self.name)


# hack, make this oo
def on_restart(data):
    print("loop sequence")
    obj = data["object"]
    if obj.is_running:
        obj.start()


class Sequence:
    def __init__(self, bpm=60, loop=True, steps=16, action=None):
        self.group_id = random.randint(0, 100000000)
        self.bpm = bpm
        self.steps = steps
        self.repeat_event = None
        self.loop = loop
        self.events = []
        self.is_running = False

        if not action:
            self.action = lambda data: log.info("step {}".format(data.get("step")))
        else:
            self.action = action

    def start(self):
        if self.is_running:
            self.stop()
        stepsize_ms = int(60 * 1000 / self.bpm)
        for i in range(self.steps):
            log.debug(f"adding sequence event {i}")
            self.events.append(
                EventTimed(
                    stepsize_ms * i,
                    name="seq{}".format(i),
                    action=self.action,
                    data={"step": i},
                    group_id=self.group_id,
                    enabled=True,
                )
            )
        if self.loop:
            self.repeat_event = EventTimed(
                stepsize_ms * self.steps,
                name="loop",
                group_id=self.group_id,
                enabled=True,
                action=on_restart,
                data={"object": self},
            )
        self.is_running = True

    def stop(self):
        # for e in self.events: e.remove()
        log.info("sequence stop")
        the_engine.remove_timed(group_id=self.group_id)
        self.events = []
        if self.repeat_event:
            self.repeat_event.remove()
        self.is_running = False


global the_engine
the_engine = Engine()
