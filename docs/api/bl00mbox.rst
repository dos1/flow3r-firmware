.. py:module:: bl00mbox


``bl00mbox`` module
===================

This module provides the `tinysynth` class. With it notes can be defined and in turn played.

.. py:class:: tinysynth

   A `tinysynth` is defined by its pitch (in Hz), waveform (sine, triangle, ...  see below) and :ref:`envelopes<env_phase_constants-label>` (adsr, see below).

   .. py:method:: __init__(freq:int)

      :param freq: Frequency in Hz
      :returns: a ``tinysynth`` object

      Create a tinysynth and provide a frequency.

      Default values:

      waveform: TRAD_OSC_WAVE_TRI.

      attack: 20ms

      decay: 500ms

      sustain: 0 (basically skip sustain and release)

      release: 500ms

   .. py:method:: start()

      Start the `tinysynth`. Essentially put it into attack phase.

   .. py:method:: stop()

      Stop the `tinysynth`. Starts the release.

   .. py:method:: freq(freq)

      :param freq: Frequency in Hz

      Set the frequency of the `tinysynth` to `freq`.

   .. py:method:: tone(tone)

      :param tone: Semitone-distance from 440 Hz.

      Set the pitch in semitone-distance from 440 Hz.

   .. py:method:: waveform(waveform)

      :param waveform: Waveform of the sound. One of ``TRAD_OSC_WAVE_{SINE/FAKE_SINE/TRI/SAW/SQUARE/PULSE/BLIP/NES_LONG/NES_SHORT}``. (:ref:`See below<wave_forms-label>`.)

      Set the waveform.

   .. py:method:: attack(duration)

      :param duration: Duration in milliseconds.

      Set the attack of the ``tinysynth`` to ``duration``.

   .. py:method:: decay(duration)

      :param duration: Duration in milliseconds.

      Set the attack of the ``tinysynth`` to ``duration``.

   .. py:method:: release(duration)

      :param duration: Duration in milliseconds.

      Set the attack of the ``tinysynth`` to ``duration``.

   .. py:method:: sustain(level)

      :param level: Level to be sustained as float between 0 an 1.

      Set the attack of the ``tinysynth`` to ``level``.

   .. py:method:: volume(volume)

      .. WARNING:: TODO what are valid inputs? I guess float between 0 and 1?

      :param volume: Volume.

      Set the overall volume of the ``tinysynth`` to ``volume``.

   .. py:method:: __del__()

      Deletes the `tinysynth`.

.. _env_phase_constants-label:

Envelope Phases and Model
-------------------------

Envelopes are used to shape notes over time.

The phases of a ``tinysynth`` envelope are (in this order):

 #. ATTACK
 #. DECAY
 #. SUSTAIN
 #. RELEASE

The attack phase is used to 'ramp up' the sound from 0 to maximum, then it
decays to the sustain level, which is sustained until the sound ends (ex: a
pressed key is released) and goes back to 0 in the release phase.

.. py:data:: TRAD_ENV_PHASE_OFF
.. py:data:: TRAD_ENV_PHASE_ATTACK
.. py:data:: TRAD_ENV_PHASE_DECAY
.. py:data:: TRAD_ENV_PHASE_SUSTAIN
.. py:data:: TRAD_ENV_PHASE_RELEASE

.. _wave_forms-label:

Wave Form Constants
-------------------

.. py:data:: TRAD_OSC_WAVE_SINE

   Sine wave.

.. py:data:: TRAD_OSC_WAVE_FAKE_SINE

   .. WARNING:: TODO document

   Fake sine wave.

.. py:data:: TRAD_OSC_WAVE_TRI

   Triangular wave.

.. py:data:: TRAD_OSC_WAVE_SAW

   Sawtooth wave.

.. py:data:: TRAD_OSC_WAVE_SQUARE

   Square wave.

.. py:data:: TRAD_OSC_WAVE_PULSE

   .. WARNING:: TODO document

   Pulses.

.. py:data:: TRAD_OSC_WAVE_BLIP

   .. WARNING:: TODO document

   ?

.. py:data:: TRAD_OSC_WAVE_NES_LONG

   .. WARNING:: TODO document

   ?

.. py:data:: TRAD_OSC_WAVE_NES_SHORT

   .. WARNING:: TODO document

   ?



.. _examples-label:

Example Usage
-------------

.. code-block:: python

   import bl00mbox
   s = bl00mbox.tinysynth(440)
   s.start()
   s.stop() // no need to stop as the default sustain level 'deactivates' the sustain and release phases.

