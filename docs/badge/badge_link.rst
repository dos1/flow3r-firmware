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

For the micropython badge link API, see the :ref:`badge_link<py_badge_link>` API docs

