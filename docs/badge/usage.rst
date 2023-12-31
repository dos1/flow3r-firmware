.. include:: <isonum.txt>

Usage
=====

Hold your Flow3r with the pink part facing towards you, and the USB port facing
upwards

.. image:: overview.svg
  :width: 700px

.. _usage_power:

Powering your Flow3r
--------------------

The Flow3r needs electricity to run - either from a battery or over its USB port.

Once it has power available, you can turn it on by moving the right-hand side
power switch (next to the 'flow3r' label on the front of the badge) towards the
right.

You should then see the badge spring to life and display 'Starting...' on the screen.

.. _usage_menu:

Navigating the Menu
-------------------

The app shoulder button (left shoulder unless swapped in settings) is used to
navigate the menus of the badge. Moving it left and right selects an option
in the menu. Pressing it down selects a menu option.

The OS shoulder button (right shoulder unless swapped in settings) can be
pressed down to quickly return 'back', either in a menu or an app.

.. _usage_audio:

Dealing with Audio
------------------

The badge has two built-in speakers. Their loudness can always be adjusted by
using the OS shoulder button (right shoulder unless swapped in settings), left
for lowering the volume and right for making it louder.

You can plug in a pair of headphones to the 3.5mm jack on the bottom-left petal.
The built-in speakers will then turn off and audio will go out through the
headphones. You can adjust their volume in the same way.

.. _usage_nick:

Showing your nick and pronouns
------------------------------

You can navigate to Badge |rarr| Nick to display your nick and pronouns. If
your nick is ``flow3r``, and you have no pronouns, congratulations! You're
ready to go. Otherwise, you'll have to connect your badge to a computer and
edit a file to change your nick and pronouns.

From the main menu, navigate to System |rarr| Disk Mode (Flash). Connect your
badge to a computer, and it will appear as a mass storage device (a.k.a.
pendrive). Open the file ```nick.json`` in a text editor and change your nick,
pronouns, font sizes for nick and pronouns, and whatever else you wish. Please
note that ``pronouns`` is a list, and should be formatted as such. for example:
``"pronouns": ["aa/bb", "cc/dd"],``

For the ``nick.json`` file to appear, you must have started the Nick app at
least once.

Use ``"color": "0xffffff",`` to color your name and pronouns.

Use ``"mode": "1",`` to use a different animation mode rotating your nick based on badge orientation.

When you're done editing, unmount/eject the badge from your computer
(``umount`` on Linux is enough) and press the OS shoulder button (right shoulder unless swapped in
settings) to exit Disk Mode. Then, go to Badge |rarr| Nick to see your changes!

If the ``nick.json`` file is unparseable or otherwise gets corrupted, it will be
overwritten with the default contents on next nick app startup.

.. _usage_music:

Playing Music
-------------

We ship some noise-making apps by default: 

shoegaze
^^^^^^^^

Electric guitar simulator with fuzz and reverb. Tilt for wiggle stick. Top petals play notes in chord, bottom petals change chord. App button left turns delay on and off, app button right checks for chords in the savefile of chord organ and toggles between them if found.

Otamatone
^^^^^^^^^

The highlighted blue petal makes noise. The highlighted green petal modulates the a resonant lowpass filter.

gay drums
^^^^^^^^^^

A simple step sequencer. Four groups of four steps, six tracks.

Hold one of the four left petals to select one of the four groups of steps, then press one of the four right petals to toggle the state of the sequencer. You can press more on each side at the same time too to set entire groups!

Use the top petal to set BPM (tap a rhythm) or long-press to start/stop. If gay drums is not stopped when exited it will continue playing in the background indefinitely, allowing you to play other instruments over a beat you made.

Cycle through tracks by tapping the bottom petal. You can also go back with a long-press!

Left/right on the app button sets the step length of the sequencer. If it's a single solid line, the sequencer will be reset each
time when passing it. If it's two lines, they will alternate between being solid and missing a piece in the middle; if solid, the sequencer will
be reset when passing it, if they're "open" they let the sequencer pass through once and switch to solid in the next pass. This is very useful for creating "early reset" type beats and hiding special moments in the last group or so :D.

Your beat is saved in flash at ``/sys/gay_drums.json`` when exiting gay drums! Make sure to wait until the menu screen appears before turning the power off though :D!

chord organ
^^^^^^^^^^^

A chord organ! The top petals always play the 5 notes of the selected chord.

Chord selection is done in 3 different mode, cycle through them with left/right on the app button:

- Mode 1 (default) ``Chord Switcher``: harmonic demo starts in this screen. 5 chords are available to be picked with the bottom petals.

- Mode 2 ``Chord Root Shifter``: In this mode the chord that was last selected in ``Chord Switcher`` can be shifted up and down, either by octaves or by semitone steps as indicated by the labels. The number behind the note name indicates the octave.

- Mode 3 ``Chord Selector``: In this mode the internal makeup of the chord that was last selected in ``Chord Switcher`` can be modified. Petal 3 switches through basic triads, i.e. minor, major, diminished, augmented, sus2, sus4. Petal 7 and 9 set higher-order tensions to give chords more flavor. Petal 1 selects an alternate lower voicing with the 3rd in the bass.

If you find yourself lost in ``Chord Selector``, note that intervals from root are color coded on the screen, meaning you can just
try out what setting changes which petal and feel out how what difference it makes in context of the others!

Your chords are saved in flash at ``/sys/harmonic_demo.json`` when exiting harmonic demo! Make sure to wait until the menu screen appears before turning the power off though :D!

tiny sampler
^^^^^^^^^^^^

5-slot sampler. Each slot records up to 4s at 12kHz. There are 4 page groups, cycle through the pages with left/right on the app button:

- ``Record and Play`` (default): Hold the bottom petals to record samples into their respective slots. Hold the corresponding top petals as indicated by the screen to replay them. This mode is actually up to 3 pages depending on how many input sources are available, such as headset mic or line in. If you experience clipping or very low signal you might wanna try to readjust input gain in the audio settings for the respective source.

- ``Play with/without passthrough``: If you have global passthrough for any source enabled one of the play buttons mutes passthrough while the button is pressed.

- ``Save and Load``: Tap a bottom petal to save a sample into the SD card if recording is available in the slot. Tap a top petal to load the corresponding sample from the SD card if available there. Not functioning if there is no SD card.

- ``Pitch Shift``: Tap a top petal to increase replay speed of the sample by a semitone, tap a bottom one to decrease it. You can also hold the petals to directly replay the results.

Samples are saved at ``/sd/tiny_sampler/tiny_sample_*.wav``.

.. _usage_apps:

Apps
------------

Audio passthrough
^^^^^^^^^^^^^^^^^

Make audio from the line in, onboard mic or headset mic appear on the speakers or
headphone output.

captouch demo
^^^^^^^^^^^^^

Visualizes the positional data from the captouch pads. Unfortunately petal 5 is not fully functional with the current driver so it performs slightly worse than all the others. Press app button in any direction to enter captouch calibration mode. *Note: calibration happens during every boot, the data is not permanently stored at this point in time. If you experience a captouch malfunction during runtime and do not wish to lose your runtime data this is useful. In the future we will probably move to calibrating more explicitely and storing the data.*

Files
^^^^^

File manager. Can read and delete system files.

IMU Demo
^^^^^^^^

Tilt to accelerate a circle with gravity.

LED Painter
^^^^^^^^^^^

Use the petals to set a color and the application button to move and lift the brush to paint on the LEDs! On exit the LED data is stored in flash at ``/menu_leds.json``. This file sets the default LED color pattern for the main menus and is not specific to LED Painter, other applications may read and write to it.

Scroll Demo
^^^^^^^^^^^

Use petal 2 to scroll a reel up and down.

Mandelbrot
^^^^^^^^^^

Generates a  Mandelbrot fractal on screen.

Worms
^^^^^

Touch petals for worms!

sensors
^^^^^^^

Shows output of several system sensors. LED hue changes with smoothed relative altitude and goes full circle on a 1m difference. Screen rotates to an upright position if flow3r is not in a horizontal position. The sensor data shown on the display is purposefully unfiltered to reflect the expected noise floor when using the low level API calls. If no battery detected the battery voltage field shows garbage.

.. _usage_system:

System
------

Settings
^^^^^^^^

Menu for setting various system parameters.

- ``WiFi``: Enter SSID and password here to connect to WiFi. Note: WiFi consumes a lot of system resources, if you feel like an application is struggling try turning off WiFi. Petal 0 toggles WiFi, a bottom bar shows you the status. If WiFi is active and networks have been found, they are shown in a list. To connect, select one with the app button. The keyboard works similar to T9, with multiple presses on the top petals selecting characters from their displayed list with a timeout and the bottom petals performing additional state switching and text operations. Confirm by pressing the app button down. If the connection is saved, it is shown in yellow, while connecting in blue, and when connected in green.
- ``Enable WiFi on Boot``: Will attempt to connect to known WiFi networks at boot time.
- ``Show Icons``: Displays battery voltage and USB connection status overlay in menu screens.
- ``Swap buttons``: Use right button as app button and left button as os button instead of the other way around.
- ``Show FPS``: Displays FPS overlay.
- ``Debug: ftop``: Prints a task cpu load and memory report every 5 seconds on the USB serial port.
- ``Touch Overlay``: If a petal is pressed the positional output is displayed in an overlay.
- ``Restore Defaults``: Restores default settings.

A settings file with more options including headphone and speaker max/min volume and volume
adjust step is on the flash filesystem at ``/settings.json``.

Graphics Mode
^^^^^^^^^^^^^

Various graphics settings. If ``lock`` is enabled applications can not override these,
else they can set it to their individual preferences at runtime.

Get Apps
^^^^^^^^

Enter the app store. Requires WiFi connection.

Disk Mode (Flash)
^^^^^^^^^^^^^^^^^

Make the flash filesystem accessible as a block device via USB. Reboots on exit.

Disk Mode (SD)
^^^^^^^^^^^^^^^^^

Make the SD card filesystem accessible as a block device via USB. Reboots on exit.

Yeet local changes
^^^^^^^^^^^^^^^^^^

Restores python payload to the state of the last firmware updatei and reboots. This excludes
settings and files not present in the original payload.

Reboot
^^^^^^

Reboot flow3r.

.. _usage_troubleshooting:

Troubleshooting
---------------

Captouch doesn't register inputs
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

At this point in time the captouch driver is silently calibrated once during every boot. This calibration expects no petals to be pressed, so if you hold flow3r in such a way that a press should be registered during calibration the petal will function poorly during this boot cycle. From experimental data we have found that similar effects are present when flow3r is face-up on a couch or similar surface. This is all in all not ideal, we're planning to add explicit calibration and persistent data storage in a future firmware release. For now we personally find it easiest to boot the badge while holding it from the back with one hand or while it is lying on a non-couchy surface. Apologies for the inconvenience. PS: Fingertips just so curling around the edge of the badge to get a proper grip is usually not an issue.

Captouch of petal 5 seems to be only working in the lower half
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This not a defect but a driver peculiarity and will be fixed in the future.
