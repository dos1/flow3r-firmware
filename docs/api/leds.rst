``leds`` module
===============

The flow3r badge features a background task that helps creating smooth LED transitions. Users who choose
to not expose themselves to blinking lights may do so by reducing the maximum slew rate in the system menu.
This is the slew rate applications will default to on_enter(). There are rare occassions in which it is necessary
for an application to function properly to increase this number, a good example is LED Painter which becomes
unusable at too low of a slew rate.

Please consider carefully if you wish to go above the user setting.

.. code-block:: python

    import leds
    import math
    from st3m.ui import colour

    def on_enter(self, vm):
        # mellow slew rate but never greater than user value
        leds.set_slew_rate(min(leds.get_slew_rate(), 180))
        leds.set_all_rgba(*colour.hsv_to_rgb(math.tau/3, 1, 1), 0.5)
        leds.update()

    def on_enter(self, vm):
        # for response time critical applications like LED Painter: set practical minimum
        leds.set_slew_rate(max(leds.get_slew_rate(), 200))
        leds.set_all_rgba(*colour.hsv_to_rgb(math.tau/3, 1, 1), 0.5)
        leds.update()


.. automodule:: leds
   :members:
   :undoc-members:
   :member-order: bysource
