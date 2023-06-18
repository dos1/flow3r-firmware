.. py:module:: badge_link

``badge_link`` module
=====================

Functions that take a :code:`pin_mask` take a combination of constants from
:code:`badge_link.PIN_MASK_LINE_{IN/OUT}_{TIP/RING}`

The GPIO indices to be used with :code:`machine.UART` are listed in
:code:`badge_link.PIN_INDEX_LINE_{OUT/IN}_{TIP/RING}`.

Functions
---------

.. py:function:: get_active(pin_mask : int) -> int

   Gets active badge link ports. Returns a pin mask.

.. py:function:: enable(pin_mask : int) -> int

   Enables badge link ports.

   Returns the output of :code:`badge_link.get_active()` after execution.

   Do NOT connect headphones to a badge link port. You might hear a ringing for
   a while. Warn user.

.. py:function:: disable(pin_mask : int) -> int

   Disables badge link ports.

   Returns the output of spio_badge_link_get_active after execution.

Pin masks
---------

.. py:data:: PIN_MASK_LINE_IN_TIP
   :type: int
.. py:data:: PIN_MASK_LINE_IN_RING
   :type: int
.. py:data:: PIN_MASK_LINE_OUT_TIP
   :type: int
.. py:data:: PIN_MASK_LINE_OUT_RING
   :type: int
.. py:data:: PIN_MASK_LINE_IN
   :type: int
.. py:data:: PIN_MASK_LINE_OUT
   :type: int
.. py:data:: PIN_MASK_ALL
   :type: int

Pin indices
-----------

These values may vary across prototype revisions.

.. py:data:: PIN_INDEX_LINE_IN_TIP
   :type: int
.. py:data:: PIN_INDEX_LINE_IN_RING
   :type: int
.. py:data:: PIN_INDEX_LINE_OUT_TIP
   :type: int
.. py:data:: PIN_INDEX_LINE_OUT_RING
   :type: int
