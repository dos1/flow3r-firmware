.. py:module:: bl00mbox


``bl00mbox`` module
===================

This module provides the `tinysynth` class. With it notes can be defined and in turn played.

.. py:class:: tinysynth

   A `tinysynth` is defined by its pitch (in Hz), waveform (sine, triangle, ...  see below) and envelope (adsr, see below).

   .. py:method:: __init__(freq:int)

      Create a tinysynth and provide a frequency.
      Default values:
      waveform: TRAD_OSC_WAVE_TRI.
      attack: 20ms
      decay: 500ms
      sustain: 0
      release: 500ms

   .. py:method:: start()

      Start the `tinysynth`. Essentially put it into attack phase.

   .. py:method:: stop()

      Stop the `tinysynth`. Essentially put it into release phase (which will be followed by the 'off phase').

   .. py:method:: freq(freq)

      Set the frequency of the `tinysynth` to `freq`.

   .. py:method:: tone(tone)

      Set semitone TODO

   .. py:method:: waveform(waveform)

      Set the waveform to one of :code:`TRAD_OSC_WAVE_{SINE/FAKE_SINE/TRI/SAW/SQUARE/PULSE/BLIP/NES_LONG/NES_SHORT}`. (See below.)

   .. py:method:: attack(duration)

      Set the attack of the `tinysynth` to `duration` in milliseconds.

   .. py:method:: decay(duration)

      Set the attack of the `tinysynth` to `duration` in milliseconds.

   .. py:method:: release(duration)

      Set the attack of the `tinysynth` to `duration` in milliseconds.

   .. py:method:: sustain(level)

      Set the attack of the `tinysynth` to `level`.
      TODO what is the valid value range, what does it represent?

   .. py:method:: volume(volume)

      Set the attack of the `tinysynth` to `volume`.
      TODO details!

   .. py:method:: __del__()

      Deletes the `tinysynth`.


Event Phase Constants
---------------------

The phases a particular `tinysynth` can be in.

(ATTACK -> DECAY -> SUSTAIN -> RELEASE)

.. py:data:: TRAD_ENV_PHASE_OFF
.. py:data:: TRAD_ENV_PHASE_ATTACK
.. py:data:: TRAD_ENV_PHASE_DECAY
.. py:data:: TRAD_ENV_PHASE_SUSTAIN
.. py:data:: TRAD_ENV_PHASE_RELEASE

Wave Form Constants
-------------------

.. py:data:: TRAD_OSC_WAVE_SINE
.. py:data:: TRAD_OSC_WAVE_FAKE_SINE
.. py:data:: TRAD_OSC_WAVE_TRI
.. py:data:: TRAD_OSC_WAVE_SAW
.. py:data:: TRAD_OSC_WAVE_SQUARE
.. py:data:: TRAD_OSC_WAVE_PULSE
.. py:data:: TRAD_OSC_WAVE_BLIP
.. py:data:: TRAD_OSC_WAVE_NES_LONG
.. py:data:: TRAD_OSC_WAVE_NES_SHORT


Example Usage
-------------

.. code-block:: python

   import bl00mbox
   s = bl00mbox.tinysynth(440)
   s.start()
   s.stop()

