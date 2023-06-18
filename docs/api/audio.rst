.. py:module:: audio

``audio`` module
================

Many of these functions are available in three variants: headphone volume,
speaker volume, and volume. If :code:`headphones_are_connected()` returns 1
the "headphone" variant is chosen, else the "speaker" variant is chosen.

.. py:function:: headset_is_connected() -> bool

   Returns 1 if headphones with microphone were connected to the headphone
   jack at the last call of audio_update_jacksense.

.. py:function:: headphones_are_connected() -> bool

   Returns 1 if headphones with or without microphone were connected to the
   headphone jack at the last call of audio_update_jacksense.

.. py:function:: headphones_detection_override(enable : bool)

   If a sleeve contact mic doesn't pull the detection pin low enough the
   codec's built in headphone detection might fail. Calling this function
   with 'enable = 1' overrides the detection and assumes there's headphones
   plugged in. Call with 'enable = 0' to revert to automatic detection.

.. py:function:: headphones_set_volume_dB(vol_dB : float) -> float
.. py:function:: speaker_set_volume_dB(vol_dB : float) -> float
.. py:function:: set_volume_dB(vol_dB : float) -> float

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
   volume.

.. py:function:: headphones_adjust_volume_dB(vol_dB : float) -> float
.. py:function:: speaker_adjust_volume_dB(vol_dB : float) -> float
.. py:function:: adjust_volume_dB(vol_dB : float) -> float

   Like the :code:`audio_{headphones_/speaker_/}set_volume` family but changes
   relative to last volume value.

.. py:function:: headphones_get_volume_dB() -> float
.. py:function:: speaker_get_volume_dB() -> float
.. py:function:: get_volume_dB() -> float

   Returns volume as set with :code:`audio_{headphones/speaker}_set_volume_dB`.

.. py:function:: headphones_get_mute() -> int
.. py:function:: speaker_get_mute() -> int
.. py:function:: get_mute() -> int

   Returns 1 if channel is muted, 0 if channel is unmuted.

.. py:function:: headphones_set_mute(mute : int)
.. py:function:: speaker_set_mute(mute : int)
.. py:function:: set_mute(mute : int)

   Mutes (mute = 1) or unmutes (mute = 0) the specified channel.

   Note: Even if a channel is unmuted it might not play sound depending on
   the return value of audio_headphone_are_connected. There is no override for
   this (see HEADPHONE PORT POLICY below).

.. py:function:: headphones_set_minimum_volume_dB(vol_dB : float) -> float
.. py:function:: speaker_set_minimum_volume_dB(vol_dB : float) -> float
.. py:function:: headphones_set_maximum_volume_dB(vol_dB : float) -> float
.. py:function:: speaker_set_maximum_volume_dB(vol_dB : float) -> float

   Set the minimum and maximum allowed volume levels for speakers and headphones
   respectively. Clamps with hardware limitations. Maximum clamps below the minimum
   value, minimum clamps above the maximum. Returns clamped value.

.. py:function:: headphones_get_minimum_volume_dB() -> float
.. py:function:: speaker_get_minimum_volume_dB() -> float
.. py:function:: headphones_get_maximum_volume_dB() -> float
.. py:function:: speaker_get_maximum_volume_dB() -> float

   Returns the minimum and maximum allowed volume levels for speakers and headphones
   respectively. Change with
   :code:`audio_{headphones/speaker}_set_{minimum/maximum}_volume_dB`.

.. py:function:: headphones_get_volume_relative() -> float
.. py:function:: speaker_get_volume_relative() -> float
.. py:function:: get_volume_relative() -> float

   Syntactic sugar for drawing UI: Returns channel volume in a 0..1 range,
   scaled into a 0.01..1 range according to the values set with
   :code:`audio_{headphones_/speaker_/}set_{maximum/minimum}_volume_` and 0 if
   in a fake mute condition.


Headphone port policy
---------------------

Under normal circumstances it is an important feature to have a reliable speaker
mute when plugging in headphones. However, since the headphone port on the badge
can also be used for badge link, there are legimate cases where it is desirable to
have the speakers unmuted while a cable is plugged into the jack.

As a person who plugs in the headphones on the tram, doesn't put them on, turns on
music to check if it's not accidentially playing on speakers and then finally puts
on headphones (temporarily, of course, intermittent checks if the speakers didn't
magically turn on are scheduled according to our general anxiety level) we wish to
make it difficult to accidentially have sound coming from the speakers.

Our proposed logic is as follows (excluding boot conditions):

1) Badge link TX cannot be enabled for any of the headphone jack pins without a
   cable detected in the jack. This is to protect users from plugging in headphones
   while badge link is active and receiving a short but potentially very loud burst
   of digital data before the software can react to the state change.

2) If the software detects that the headphone jack has changed from unplugged to
   plugged it *always* turns off speakers, no exceptions.

3) If a user wishes to TX on headphone badge link, they must confirm a warning that
   having headphones plugged in may potentially cause hearing damage *every time*.

4) If a user wishes to RX or TX on headphone badge link while playing sound on the
   onboard speakers, they must confirm a warning *every time*.

We understand that these means seem extreme, but we find them to be a sensible
default configuration to make sure people can safely operate the device without
needing to refer to a manual.

(TX here means any state that is not constantly ~GND with whatever impedance.
While there are current limiting resistors (value TBD at the time of writing, but
presumably 100R-470R) in series with the GPIOs, they still can generate quite some
volume with standard 40Ohm-ish headphones. Ideally the analog switch will never
switch to the GPIOs without a cable plugged in.)
