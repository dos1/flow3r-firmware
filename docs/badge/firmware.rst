Firmware
========

Introduction
------------

The flow3r badge firmware is composed of a number of parts:

ESP-IDF
	The ESP-IDF is the SDK provided by Espressif for its ESP32 series of
	devices. This SDK is written in C and heavily based around FreeRTOS.

flow3r-bsp
	The lowest layer of our own code, the Board Support Package is the driver
	layer for interacting with the badge hardware. It's implemented as a
	standard ESP-IDF components, so you can use it in your own ESP-IDF based
	projects to quickly create fully custom experiences for the badge.

micropython
	Micropython is a interpreter for a subset of the Python programming
	language. It runs as a FreeRTOS task and provides the main runtime for
	flow3r-specific code, including user (your own!) applications. This is the
	main subsystem you'll be interacting with when developing applications.

.. _st3m:

st3m
	st3m (pronounced: stem) is the main framework and standard library for code
	running on the badge. Its implementation is split into a C part (another
	ESP-IDF component, sometimes call C-st3m) and Micropython code (which lives
	in ``/flash/sys`` on the badge and in ``/python_payload`` in the repository,
	somtimes called Py-st3m).

recovery
	The `Recovery Mode`_ firmware is a smaller version of the default firmware, and
	doesn't contain an audio stack or a Python interpreter. It provides enough
	functionality to allow you to recover from bricks, or to update your
	firmware.

Filesystem
----------

The badge has a single UNIX-like filesystem. This filesystem is visible to both
C and Micropython software:

+------------+---------------+------------------------------------+
| Mountpoint | Filesystem    | Description                        |
+============+===============+====================================+
| ``/flash`` | FAT32         | 10MiB partition on internal Flash. |
|            | with WL [#WL]_| Contains st3m Python code in sys/. |
+------------+---------------+------------------------------------+
| ``/sd``    | FAT32         | External SD card, if available.    |
+------------+---------------+------------------------------------+

.. _partition:

SPI Flash Partitions
--------------------

This is the SPI flash partition layout we use:

+--------------+--------+---------------------------------------+
| Name         | Size   | Description                           |
+==============+========+=======================================+
| n/a          | 32KiB  | Bootloader (``bootloader.bin``).      |
+--------------+--------+---------------------------------------+
| ``nvs``      | 24KiB  | Non-Volatile Storage.                 |
+--------------+--------+---------------------------------------+
| ``phy_init`` | 4KiB   | Unused PHY data partition.            |
+--------------+--------+---------------------------------------+
| ``recovery`` | 512KiB | Recovery firmare (``recovery.bin``).  |
+--------------+--------+---------------------------------------+
| ``factory``  | 3MiB   | Main badge firmware (``flow3r.bin``). |
+--------------+--------+---------------------------------------+
| ``vfs``      | 12MiB  | FAT32 filesystem (with [#WL]_ layer). |
+--------------+--------+---------------------------------------+



Setup USB access rules
-------------------------

.. warning::
   **Did you set up USB access rules already?**

   To let ``mpremote`` to work properly your user needs to have access rights to ttyACM.

   Quick fix: ``sudo chmod a+rw /dev/ttyACM[Your Device Id here]```

   More sustainable fix: Setup an systemd rule to automatically allow the logged in user to access ttyUSB

	    1. To use this, add the following to /etc/udev/rules.d/60-extra-acl.rules: ``KERNEL=="ttyACM[0-9]*", TAG+="udev-acl", TAG+="uaccess"``
	    2. Reload ``udevadm control --reload-rules && udevadm trigger``
	


If the badge is running correctly, you can access the filesystem over the micropython REPL, using tools like mpremote.

::

	$ mpremote
	MicroPython c48f94151-dirty on 1980-01-01; flow3r with ESP32S3
	Type "help()" for more information.
	>>> import os
	>>> os.listdir('/')
	['flash']
	>>> os.listdir('/flash/sys')
	['main.py', 'st3m', '.sys-installed']
	>>> 

	$ mpremote ls :flash/sys
	ls :flash/sys
	           0 main.py
	           0 st3m
	           0 .sys-installed


You can also put the badge into :ref:`Disk Mode` to make it appear as a USB pendrive
on your PC. However, only one of the two FAT32 block devices (internal flash
partition or SD card) can be mounted at once.

Startup
-------

The badge boot process is the following:

1. The ESP32S3 ROM starts. If the BOOT0 (the left shoulder button) is pressed
   down, it will boot into serial mode, from which esptool.py can be used to
   flash_ the firmware.

2. The ESP32S3 ROM loads and executes the second stage bootloader from SPI
   Flash. The bootloader is currently a stock ESP-IDF bootloader, but this will
   likely change.

3. The bootloader loads the partition table definition from SPI flash, loads and
   runs the ``factory`` partition_ from SPI flash.

4. The badge firmware starts and the display shows 'Starting...'

5. The badge firmware initializes (ie. formats) the FAT32 filesystem on the internal flash if necessary. The filesystem is then mounted on ``/flash``.

6. The badge firmware checks if the badge has a ``/flash/sys/.sys-installed`` file. If not, st3m_ Micropython files are extracted there.

7. The badge starts Micropython which then loads ``/flash/sys/main.py``.



When Things Break
-----------------

If you brick your badge by corrupting or messing up the files on the internal
flash partition, you can always recover by somehow getting the badge into disk
mode, mounting the internal flash partition and then removing all files and
directories. Then, unmount the badge. After rebooting, the system partition will
be restored to a stock state by the badge firmware.

:ref:`Disk Mode` can be started from the main firmware, either from the menu or
by pressing buttons as indicated in various crash screens.

If the above is not possible, you can also start a limited Disk Mode from the
:ref:`Recovery Mode`. The :ref:`Recovery Mode` can also be used to reflash the badge
firmware `partition`_ in case it got corrupted.

However, if something's really broken, you will have to perform a low-level
flash_ via the ESP32 BootROM - see below.

.. _`recovery mode`:

Recovery Mode
-------------

Recovery Mode is a special mode in which the badge can get started which can
help you perform simple fixes and update your firmware.

To start Recovery Mode:

1. Make sure the badge has a power source: either a charged battery or USB power.
2. Turn off the badge by sliding the power switch to the left.
3. Start holding down the right trigger / shoulder button.
4. Turn on the badge by sliding the power switch to the right.

You should be greeted with a purple screen from which multiple actions can be selected:

1. **Reboot**: reboots the badge back into the current (non-recovery) firmware.
2. **Disk Mode**: mounts the internal SPI flash FAT partition as a USB mass storage. This is effectively a copy of the :ref:`Disk Mode` from the main firmware.
3. **Format FAT partition**: fully wipes the internal SPI flash, which should let you recover from most cases of bricking. On next reboot, the badge will re-populate the FAT partition with :ref:`st3m` files and should start up normally.
4. **Update firmware**: flashes a file from the FAT partition onto the main firmware partition_. This can be used to update your badge to the `newest firmware <https://git.flow3r.garden/flow3r/flow3r-firmware/-/releases>`_ or to an alternative firmware.

.. _flash:

Flashing (low-level)
--------------------

To perform a low-level flash which will reset the entire badge state to a known
state, you have to first put it into bootrom mode by starting it up with the
left shoulder button held. The badge screen will stay off, but when connected
over USB it should show up as an ``Espressif USB/JTAG bridge``.

Compared to recovery modes above, this options requires the use of specialized
software: `esptool.py <https://github.com/espressif/esptool>`_, available from
most Linux distribution package managers.

Instructions on how to run ``esptool.py`` are given with every firmware update release tarball.

Updating Firmware
-----------------

Download a `release <https://git.flow3r.garden/flow3r/flow3r-firmware/-/releases>`_, extract the tarball and follow instructions in the README. There will be notes on how to perform updates through either :ref:`Disk Mode`, `Recovery Mode`_ or through a low-level flash_.

Or, if you're at CCCamp2023, visit our firmware update station, once it is availble.

.. [#WL] Wear leveling, to protect internal flash from death by repeat sector write.

Versioning
----------

Releases follow semantic versioning rules and are names ``v[major].[minor].[patch].``.

Release candidates are suffixed with ``+rc[number]``, with ``v1.2.3+rc23`` being the 23rd release candidate for version 1.2.3 (ie., if the RC is promoted to a build, it will become version 1.2.3).

Development versions (not tagged with a release or release candidate tag) are suffixed with ``-dev[num]``, where ``num`` is the number of commits from the beginning of the main branch history.

Builds from the main branch are currently always at version 0.
