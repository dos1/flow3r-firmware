.. py:module:: audio

``audio`` module
================

(XXX when it says bool in the python api that's often an int, should change
that)

.. py:function:: headset_is_connected() -> bool
.. c:function:: uint8_t audio_headset_is_connected()

   Returns 1 if headphones with microphone were connected to the headphone
   jack at the last call of audio_update_jacksense.

.. py:function:: headphones_are_connected() -> bool
.. c:function:: uint8_t audio_headphones_are_connected()

   Returns 1 if headphones with or without microphone were connected to the
   headphone jack at the last call of audio_update_jacksense.

.. py:function:: headphones_detection_override(enable : bool)
.. c:function:: void audio_headphones_detection_override(uint8_t enable)

   If a sleeve contact mic doesn't pull the detection pin low enough the
   codec's built in headphone detection might fail. Calling this function
   with 'enable = 1' overrides the detection and assumes there's headphones
   plugged in. Call with 'enable = 0' to revert to automatic detection.

.. py:function:: headphones_set_volume_dB(vol_dB : float) -> float
.. c:function:: float audio_headphones_set_volume_dB(float vol_dB);
.. py:function:: speaker_set_volume_dB(vol_dB : float) -> float
.. c:function:: float audio_speaker_set_volume_dB(float vol_dB);
.. py:function:: set_volume_dB(vol_dB : float) -> float
.. c:function:: float audio_set_volume_dB(float vol_dB);

   Attempts to set target volume for the headphone output/onboard speakers
   respectively, clamps/rounds if necessary and returns the actual volume.
   Absolute reference arbitrary.
   Does not unmute, use :code:`audio_{headphones_/speaker_/}set_mute` as
   needed.
   Enters fake mute if requested volume is below the value set by
   :code:`audio_{headphones/speaker}_set_minimum_volume_user`.

   Note: This function uses a hardware PGA for the coarse value and software
   for the fine value. These two methods are as of yet not synced so that
   there may be a transient volume "hiccup". "p1" badges only use software
   volume. The unspecified variant automatically chooses the adequate channel
   (**).


.. py:function:: headphones_adjust_volume_dB
.. py:function:: speaker_adjust_volume_dB
.. py:function:: adjust_volume_dB

.. py:function:: headphones_get_volume_dB
.. py:function:: speaker_get_volume_dB
.. py:function:: get_volume_dB

.. py:function:: headphones_get_mute
.. py:function:: speaker_get_mute
.. py:function:: get_mute

.. py:function:: headphones_set_mute
.. py:function:: speaker_set_mute
.. py:function:: set_mute

.. py:function:: headphones_set_minimum_volume_dB
.. py:function:: speaker_set_minimum_volume_dB
.. py:function:: headphones_set_maximum_volume_dB
.. py:function:: speaker_set_maximum_volume_dB

.. py:function:: headphones_get_minimum_volume_dB
.. py:function:: speaker_get_minimum_volume_dB
.. py:function:: headphones_get_maximum_volume_dB
.. py:function:: speaker_get_maximum_volume_dB

.. py:function:: headphones_get_volume_relative
.. py:function:: speaker_get_volume_relative
.. py:function:: get_volume_relative
