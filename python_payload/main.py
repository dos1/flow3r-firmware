from machine import Pin

p = Pin(0, Pin.IN)

while (p.value() == 1):
    pass

import cap_touch_demo
cap_touch_demo.cap_touch_demo_start()
