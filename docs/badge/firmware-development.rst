Firmware Development
====================

*Note: if you just want to make apps/instruments, see the Programming section.*

*Note: we assume you've read the Firmware section before, as that contains general information about the firmware structure.*

Source Code
-----------

The core of the flow3rbadge codebade is :ref:`st3m`, with part of it implemented in
Python, part in C. To work on both, you will need to clone the flow3rbadge
firmware repository.

::

	$ git clone TODO

Dependencies
------------

If you're using Nix(OS), just run ``nix-shell nix/shell.nix``.

On other Linux-based distributions, you will have to manually install ESP-IDF alongside our custom patches:

::

	$ git clone https://github.com/espressif/esp-idf.git
	$ cd esp-idf
	$ git checkout v5.1
	$ git submodule update --init --recursive
	$ cd esp-idf
	$ patch -p1 < ../firmware/third_party/b03c8912c73fa59061d97a2f5fd5acddcc3fa356.patch
	$ ./install.sh
	$ source export.sh

For running the simulator, you'll need Python 3 with pygame and wasmer:

::

	$ python3 -m venv venv
	$ venv/bin/pip install pygame
	$ venv/bin/pip install wasmer
	$ venv/bin/pip install wasmer-compiler-cranelift
	$ . venv/bin/activate

For Python development, you're also encouraged to use mypy for typechecks. It should be available in your distribution repositories.

On macOS: the above might work.

On Windows: good luck.

Working on Python st3m code
---------------------------

You can use `mpremote` and similar to copy edited files from ``python_payload/``:

::

	$ mpremote cp python_payload/main.py :/flash/sys/main.py

*TODO: document mpremote mount*

As with application development, you can first check your changes using the simulator:

::

	$ python3 sim/run.py

You should also run typechecks:

::

	$ MYPYPATH=python_payload/mypystubs mypy python_payload/main.py

Working on C st3m code
----------------------

To compile:

::
	
	$ idf.py build

To flash, put the badge in :ref:`flash` mode and run:

::
	
	$ idf.py flash

Or, to just write the main firmware partition:

::
	
	$ idf.py app-flash

To clean, do not trust ``idf.py clean``. Instead, kill everything with fire:

::
	
	$ rm -rf sdkconfig build

To edit the sdkconfig temporarily:

::
	
	$ idf.py menuconfig

To commit your sdkconfig changes to git, run menuconfig, press *d*, accept the default path. Then, copy over ``build/defconfig`` onto ``sdkconfig.defaults``.

printf-Debugging
----------------

All printf() (and other stdio) calls will be piped to the default Micropython REPL console. For logging, please use ``ESP_LOGx`` calls.

If you're debugging the USB stack, or want to see Guru Meditation crashes, connect to UART0 over the USB-C connector's sideband pins (**TODO**: link to flow3rpot).

You can also disable the USB stack and make the badge stay in UART/JTAG mode: **TODO: issue 23**. Then, you can use openocd/gdb:

::
	
	$ OPENOCD_COMMANDS="-f board/esp32s3-builtin.cfg" idf.py opencod

*TODO: document how to start gdb*

Porting Doom (or other alternate firmware)
------------------------------------------

You should be able to use the ``flow3r_bsp`` component from any ESP-IDF 5 project. Either vendor the files, use a submodule and a symlink...

You should stay compatible with our :ref:`partition` layout. The easiest way to do that is to copy ``partitions.csv`` and refer to it from your own project. Your firmware should fit the ``factory`` slot.

Then, you can run your firmware by distributing the resulting ``.bin`` file and letting people flash to it via :ref:`Recovery Mode`.

For an example, see our doom port at **TODO**.