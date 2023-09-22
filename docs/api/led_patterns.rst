.. py:module:: led_patterns

``st3m.ui.led_patterns`` module
===============================

None of these functions call ``leds.update()``, to actually see the changes you have to do that yourself!

.. py:function:: highlight_petal_rgb(num : int, r : float, g : float, b : float, num_leds : int = 5) -> None

    Sets the LED closest to the petal and num_leds-1 around it to the given rgb color.
    If num_leds is uneven the appearance will be symmetric.

.. py:function:: shift_all_hsv(h : float  = 0, s : float = 0, v : float = 0) -> None

    Shifts all LEDs by the given values. Clips effective ``s`` and ``v``.

.. py:function:: pretty_pattern() -> None

    Generates a random pretty pattern and loads it into the LED buffer.

.. py:function:: set_menu_colors() -> None

    If not disabled in settings: Tries to load LED colors from /flash/menu_leds.json. Else, or in
    case of missing file, call ``pretty_pattern()``. Note: There is no caching, it tries to attempt
    to read the file every time, if you need something faster use ``leds.get_rgb`` to cache it in the
    application.
