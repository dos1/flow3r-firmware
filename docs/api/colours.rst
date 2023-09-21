.. py:module:: colours

``st3m.ui.colours`` module
==========================

Colour data is expressed in floats. ``r``, ``g``, ``b``, ``v``, ``s`` range from [0..1] and clamp beyond, ``h`` ranges from [0..math.tau] and overflows gracefully for values within [-100..100].

Example (note the asterisk expanding the returned tuple):

.. code-block:: python

    import leds
    import math
    from st3m.ui import colours
    leds.set_all_rgba(*colours.hsv_to_rgb(math.tau*5/6, 1, 1), 0.5)
    leds.update()

.. py:function:: hsv_to_rgb(h : float, s : float, v : float) -> Tuple[r : float, g: float, b: float]

    Returns RGB tuple corresponding to the HSV input parameters.

.. py:function:: rgb_to_hsv(r : float, g : float, b : float) -> Tuple[h : float, s: float, v: float]

    Returns HSV tuple corresponding to the RGB input parameters.

This module also provides some color constants:

.. py:data:: BLACK
.. py:data:: RED
.. py:data:: GREEN
.. py:data:: BLUE
.. py:data:: WHITE
.. py:data:: GREY
.. py:data:: GO_GREEN
.. py:data:: PUSH_RED
