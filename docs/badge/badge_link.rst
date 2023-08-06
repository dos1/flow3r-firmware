.. _Badge Link:

Badge Link
==========

.. note::

   See also: :ref:`Micropython API docs<Badge link api>`

Badge Link is a protocol for digital communication over the 3.5mm jacks.

You can configure individual pins (tip, ring) of each of the two 3.5mm jacks
(line in, line out) to be used as anything a GPIO of the ESP32S3 can do with
the following limitations:

 * The input protection can handle between -2V and +3.3V **or** ±10mA max.

   * This means that within that voltage range, you can source or sink higher
     currents
   * And higher voltages will be clamped to a safe range if the external source
     doesn't exceed the current limits

 * The GPIOs have a 100 Ohm resistor in series which limits current output and
   may limit bandwidth depending on cable capacity
 * There is no galvanic isolation (as commonly used in MIDI devices). Ground
   loops can introduce noise in the audio input/output.

Typical use cases include:

 * :ref:`badge-link-uart`
 * :ref:`badge-link-midi`
 * :ref:`badge-link-eurorack-cv`


.. _badge-link-uart:

UART
----

UART can be used with any baud rate up to 5mbit, in theory. In practice lower
baud rates are recommended to avoid data corruption. Longer cables do not
necessarily mean lower baud rates (higher cable capacity)

.. _badge-link-midi:

MIDI
----

MIDI is unidirectional UART with a baud rate of 31250, with one pin acting as
the "current source" (Vcc, an always high GPIO) and one pin acting as the
"current sink" (data, UART tx or rx). Sleeve can be assumed to be always ground.

There are three competing arrangements for this:

 * MIDI Type A: Tip is sink/data, ring is source/Vcc.
 * MIDI Type B: Ring is sink/data, tip is source/Vcc.
 * TS / Non-TRS / Type C: Like type B, but ring and sleeve are shorted. Not
   recommended.

See https://minimidi.world/ for more details.

Type A has been adopted as the standard by the MIDI Manufacturers
Association in 2018, but there are many type B devices too.

On our side, it is up to the application developer to decide how to configure
the pins.

.. _badge-link-eurorack-cv:

Eurorack control voltages
-------------------------

.. warning::

   We haven't tested this.

It's theoretically possible to use the ADC and PWM (LEDC) peripherals of the
ESP32-S3 to interact with eurorack control voltages.

The PWM output will likely need an RC filter to output a smoother signal.
