.. include:: <isonum.txt>

Usage
=====

Hold your Flow3r with the pink part facing towards you, and the USB port facing
upwards

.. image:: overview.svg
  :width: 700px

Powering your Flow3r
--------------------

The Flow3r needs electricity to run - either from a battery or over its USB port.

Once it has power available, you can turn it on by moving the right-hand side
power switch (next to the 'flow3r' label on the front of the badge) towards the
right.

You should then see the badge spring to life and display 'Starting...' on the screen.

Navigating the Menu
-------------------

The left shoulder button is used to navigate the menus of the badge. Moving it
left and right selects an option in the menu. Pressing it down selects a menu
option.

The right shoulder button can be pressed down to quickly return 'back', either
in a menu or an app.

Dealing with Audio
------------------

The badge has two built-in speakers. Their loudness can always be adjusted by
using the right shoulder button, left for lowering the volume and right for
making it louder.

You can plug in a pair of headphones to the 3.5mm jack on the bottom-left petal.
The built-in speakers will then turn off and audio will go out through the
headphones. You can adjust their volume in the same way.

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

When you're done editing, unmount/eject the badge from your computer
(``umount`` on Linux is enough) and press the left shoulder button to exit Disk
Mode. Then, go to Badge |rarr| Nick to see your changes!

If the ``nick.json`` file is unparseable or otherwise gets corrupted, it will be
overwritten with the default contents on next nick app startup.

Playing Music
-------------

We ship some noise-making apps by default: 

shoegaze
^^^^^^^^

*TODO*

Otamatone
^^^^^^^^^

The highlighted blue petal makes noise.

Simple Drums
^^^^^^^^^^^^

A simple gay step sequencer. Four groups of four steps, six insturments.

Hold one of the four left petals to select one of the four groups of steps, then short-press one of the four right petals to toggle the state of the sequencer.

Use the top petal to set BPM (tap a rhythm) or long-press to start/stop.

Use the bottom petal to switch instruments.

Melodic
^^^^^^^

Toy synthesizer. Bottom three petals select octave, Top 7 petals play a note from that octave.

Harmonic
^^^^^^^^

Toy synthesizer. White petals select chord, pink petals play note.
