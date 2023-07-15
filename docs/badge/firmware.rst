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
	ESP-IDF component) and Micropython code (which lives in ``/flash/sys``).

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
| ``factory``  | 5.9MiB | Main badge firmware (``flow3r.bin``). |
+--------------+--------+---------------------------------------+
| ``vfs``      | 10MiB  | FAT32 filesystem (with [#WL]_ layer). |
+--------------+--------+---------------------------------------+

Accessing files from a PC
------------------------------------

If the badge is running correctly, you can access the filesystem over the micropython REPL:

::

	$ mpremote
	MicroPython c48f94151-dirty on 1980-01-01; badge23 with ESP32S3
	Type "help()" for more information.
	>>> import os
	>>> os.listdir('/')
	['flash']
	>>> os.listdir('/flash/sys')
	['main.py', 'st4m', '.sys-installed']
	>>> 

	$ mpremote ls :flash/sys
	ls :flash/sys
	           0 main.py
	           0 st4m
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



Recovery
--------

If you brick your badge by corrupting or messing up the files on the internal
flash partition, you can always recover by somehow getting the badge into disk
mode, mounting the internal flash partition and then removing all files and
directories. Then, unmount the badge. After rebooting, the system partition will
be restored to a stock state by the badge firmware.

:ref:`Disk Mode` can be started from the main firmware, either from the menu or
by pressing buttons as indicated in various crash screens.

If the above is not possible, you can also start a limited Disk Mode from the
:ref:`Bootloader`. The :ref:`Bootloader` can also be used to reflash the badge
firmware `partition`_ in case it got corrupted.

However, if something's really broken, you will have to perform a low-level
flash via the ESP32 BootROM - see below.

.. _flash:

Flashing (low-level)
--------------------

To perform a low-level flash which will reset the entire badge state to a known
state, you have to first put it into bootrom mode by starting it up with the
left shoulder button held. The badge screen will stay off, but when connected
over USB it should show up as an ``Espressif USB/JTAG bridge``.

Compared to recovery modes above, this options requires the use of specialized
software.

Then use ``esptool.py`` (available from most Linux distribution package
managers), download a release (TODO: link) and run the following:

::

	esptool.py \
		-p /dev/ttyACM0 -b 460800 \
		--before default_reset --after no_reset \
		--chip esp32s3 write_flash -e \
		--flash_mode dio --flash_size 16MB --flash_freq 80m \
		0x0 bootloader.bin \
		0x8000 partition-table.bin \
		0x10000 flow3r.bin

This will erase the entire internal SPI flash, and then program the bootloader,
partition table and main firmware.

.. [#WL] Wear leveling, to protect internal flash from death by repeat sector write.
