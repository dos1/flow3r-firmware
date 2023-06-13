.. _Badge Link:

Badge Link
==========

Badge Link is a protocol for digital communication over the 3.5mm ports.

You can configure individual pins (tip, ring) of each of the two 3.5mm ports
(line in, line out) to be used as TX or RX for UART, at any baud rate up to
5mbit in theory. In practice lower baud rates are recommended to avoid data
corruption.

MIDI can be used by configuring the baud rate to 31250. There is no galvanic
isolation, but there are current limiting resistors.

The input protection can handle between +5v and -3v.

Example usage
-------------

On both badges:

.. code-block:: python

   import badge_link
   from machine import UART
   badge_link.enable(badge_link.PIN_MASK_ALL)

On badge 1, connect the cable to line out, and configure uart with tx on tip
(as an example)

.. code-block:: python

   uart = UART(
       1,
       baudrate=115200,
       tx=badge_link.PIN_INDEX_LINE_OUT_TIP,
       rx=badge_link.PIN_INDEX_LINE_OUT_RING
   )

On badge 2, connect the cable to line in, and configure uart with tx on ring:

.. code-block:: python

   uart = UART(
       1,
       baudrate=115200,
       tx=badge_link.PIN_INDEX_LINE_IN_RING,
       rx=badge_link.PIN_INDEX_LINE_IN_TIP
   )

Then write and read from each side:

.. code-block:: python

   uart.write("hiiii")
   uart.read(5)

MIDI
----

You can do raw midi with this.

.. note::
   Good reference for midi commands:
   https://computermusicresource.com/MIDI.Commands.html

.. code-block:: python

   import badge_link
   from machine import UART
   uart = UART(
       1,
       baudrate=31250,
       tx=badge_link.PIN_INDEX_LINE_OUT_TIP,
       rx=badge_link.PIN_INDEX_LINE_OUT_RING
   )

   # note on channel 1, note #60, velocity 127
   uart.write(bytes([144, 60, 127]))

   # note off channel 1, note #60
   uart.write(bytes([144, 60, 127]))

Turn this into usb midi with a cheap adapter or https://github.com/rppicomidi/midi2usbdev

Read incoming events from linux:

.. code-block:: bash

   aseqdump -l
   # use the port below
   aseqdump -p 20:0
