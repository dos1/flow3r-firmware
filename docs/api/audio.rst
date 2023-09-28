.. py:module:: audio

``audio`` module
================

The audio module provides the backbone for handling basic audio bookkeeping such as volume and signal routing.
Actual sound is created by the engines, i.e. bl00mbox and media player at the moment. If you wish to add your
own C-based sound engine to flow3r, please check out the framework we have set up for that in
``components/st3m/st3m_audio.*``.

Jack Detection
--------------

.. py:function:: headset_is_connected() -> bool

   Returns 1 if headphones with microphone were connected to the headphone
   jack at the last call of audio_update_jacksense.

.. py:function:: headphones_are_connected() -> bool

   Returns 1 if headphones with or without microphone were connected to the
   headphone jack at the last call of audio_update_jacksense.

.. py:function:: line_in_is_connected() -> bool

   Returns 1 if the line-in jack was connected at the last call
   of audio_update_jacksense.

Input Sources
-------------

.. note::
    The onboard digital mic turns on an LED on the top board if it receives
    a clock signal which is considered a good proxy for its capability of reading
    data. Access to the onboard mic can be disabled entirely in the audio config
    menu.

The codec can receive data from the line in jack, the headset mic pin in the headphone jack, or the
internal microphone. We distinguish between two use cases: 1) sending the signal to the audio engines
to be mangled with, and 2) directly mix it to the output with a variable volume level. We provide two
different APIs for each use case.

Sources may or may not be available; for line in and the headset mic they might simply not be plugged in,
but also users can configure in the settings which inputs are available in the first place. To handle this
uncertainity gracefully, instead of momentarily trying (and potentially failing) to set up a connection
you set a desired target and the backend will attempt to connect to it continuously until the target is reset
to none. The target states are not cleared when exiting applications, if you don't intend to also configure
the source for other applications please reset them to whatever state you found them in like so:

.. code-block:: python

    on_enter(self, vm):
        # save original target
        self.orig_engine_target_source = audio.input_engine_get_target_source()
        # switch to your preferred one
        audio.input_engine_set_target_source(audio.INPUT_SOURCE_AUTO)

    on_exit(self):
        # restore original target
        audio.input_engine_set_source(self.orig_engine_target_source)

Since the codec can only send data from one source at a time. in case of a disagreement between the engine
source and the thru source, the engine source wins and the through source is temporarily set to none.
For thru to follow the engine source if available and not none use ``audio.input_thru_set_source(audio.INPUT_SOURCE_AUTO)``.

The available sources for both engine and thru are slightly different: The engine only looks for permissions
and hardware state, while thru can not access the onboard mic if playback is happening through the speakers.
This is set up to prevent accidential feedback loops. However the user can give permission to acces this mode
in the user config.


.. py:data:: INPUT_SOURCE_NONE

    No source, datastream suspended.
    
.. py:data:: INPUT_SOURCE_LINE_IN

    Stream data from line in if available.

.. py:data:: INPUT_SOURCE_HEADSET_MIC

    Stream data from headset mic if available and allowed.

.. py:data:: INPUT_SOURCE_ONBOARD_MIC

    Stream data from onboard mic if allowed.

.. py:data:: INPUT_SOURCE_AUTO

    Stream data from available input, ``INPUT_SOURCE_LINE_IN`` is preferred to ``INPUT_SOURCE_HEADSET_MIC``
    is preferred to ``INPUT_SOURCE_ONBOARD_MIC``.

    For ``input_thru_set_source()`` only: matching ``input_engines_get_source()`` unless it is
    ``INPUT_SOURCE_NONE`` has highest preference, and ``INPUT_SOURCE_ONBOARD_MIC`` is never returned
    when speakers are on even if access is permitted.

.. py:function:: input_engines_set_source(source : int) -> int

    Set up a continuous connection query for routing the given source to the input for the audio engines.
    Check for success with ``input_engines_get_source()`` and clean up by passing ``INPUT_SOURCE_NONE``

.. py:function:: input_engines_get_target_source() -> int

    Returns target source last set with i``nput_engines_set_source()``.

.. py:function:: input_engines_get_source() -> int

    Returns source currently connected to the audio engines.

.. py:function:: input_engines_get_source_avail(source : int) -> bool

    Returns true if it is currently possible to connect the audio engines to a given source.
    If given ``INPUT_SOURCE_AUTO`` returns true if any source can be connected to the engines.

.. py:function:: input_thru_set_source(source : int) -> int

    Set up a continuous connection query for routing the given source to the output mixer of the codec.
    Check for success with ``input_thru_get_source()`` and clean up by passing ``INPUT_SOURCE_NONE``

.. py:function:: input_thru_get_target_source() -> int

    Returns target source last set with input_thru_set_source.

.. py:function:: input_thru_get_source() -> int

    Returns the source currently mixed directly to output.

.. py:function:: input_get_source() -> int

    Returns the source the codec is connected to at the moment.

.. py:function:: input_thru_get_source_avail(source : int) -> bool

    Returns true if it is currently possible to a given source to thru.
    If given ``INPUT_SOURCE_AUTO`` returns true if any source can be connected to thru.

.. py:function:: input_thru_set_volume_dB(vol_dB : float)
.. py:function:: input_thru_get_volume_dB() -> float
.. py:function:: input_thru_set_mute(mute : bool)
.. py:function:: input_thru_get_mute() -> bool

    Volume and mute control for input_thru. Please don't use this as a replacement for terminating
    a connection, ``input_thru_set_source(audio.INPUT_SOURCE_NONE)`` instead!

.. py:function:: input_line_in_get_allowed(mute : bool)
.. py:function:: input_headset_mic_get_allowed(mute : bool)
.. py:function:: input_onboard_mic_get_allowed(mute : bool)
.. py:function:: input_onboard_mic_to_speaker_get_allowed(mute : bool)

    Returns if the user has forbidden access to the resource.

OS development
--------------

Many of these functions are available in three variants: headphone volume,
speaker volume, and volume. If :code:`headphones_are_connected()` returns 1
the "headphone" variant is chosen, else the "speaker" variant is chosen.

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

.. py:function:: headset_mic_set_gain_dB(gain_dB : float)
.. py:function:: headset_mic_get_gain_dB() -> float
.. py:function:: onboard_mic_set_gain_dB(gain_dB : float)
.. py:function:: onboard_mic_get_gain_dB() -> float
.. py:function:: line_in_set_gain_dB(gain_dB : float)
.. py:function:: line_in_get_gain_dB() -> float

   Set and get gain for the respective input channels.

.. py:function:: codec_i2c_write(reg : int, data : int)

   Write audio codec register. Obviously very unsafe. Do not use in applications that you
   distribute to users. This can fry your speakers with DC>


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
