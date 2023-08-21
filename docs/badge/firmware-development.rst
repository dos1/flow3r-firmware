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

	$ git clone --recursive https://git.flow3r.garden/flow3r/flow3r-firmware

Don't forget the ``--recursive``, otherwise you'll get weird errors from missing submodules, like:

::

    CMake Error at esp-idf/tools/cmake/component.cmake:313 (message):
     Include directory
     '.../flow3r-firmware/components/micropython/vendor/lib/berkeley-db-1.xx/PORT/include'
     is not a directory.

If you've already cloned without ``--recursive`` you can update your submodules the following way:

::

    $ git submodule update --init

Dependencies
------------

If you're using Nix(OS), just run ``nix-shell nix/shell.nix``.

On other Linux-based distributions, you will have to manually install ESP-IDF alongside our custom patches (note that install.sh installs stuff to your $HOME so that you may want to use a container or nix):

::

	$ git clone --recursive https://git.flow3r.garden/flow3r/esp-idf
	$ cd esp-idf
    $ git checkout 5.1-flow3r
	$ ./install.sh
	$ source export.sh

To compile, see `Working on C st3m code`_.

For running the simulator, you'll need Python 3 with pygame and wasmer:

::

	$ python3 -m venv venv
	$ venv/bin/pip install pygame requests
        $ venv/bin/pip install wasmer wasmer-compiler-cranelift

.. warning::

    The wasmer python module from PyPI `doesn't work with Python versions 3.10 or 3.11
    <https://github.com/wasmerio/wasmer-python/issues/539>`_.  You will get
    ``ImportError: Wasmer is not available on this system`` when trying to run
    the simulator.

    Instead, install our `rebuilt wasmer wheels <https://flow3r.garden/tmp/wasmer-py311/>`_ using

    ::

        venv/bin/pip install https://flow3r.garden/tmp/wasmer-py311/wasmer_compiler_cranelift-1.2.0-cp311-cp311-manylinux_2_34_x86_64.whl
        venv/bin/pip install https://flow3r.garden/tmp/wasmer-py311/wasmer-1.2.0-cp311-cp311-manylinux_2_34_x86_64.whl

For Python development, you're also encouraged to use mypy for typechecks. It should be available in your distribution repositories.

On macOS: the above might work.

On Windows: good luck.

Working on Python st3m code
---------------------------

You can use `mpremote` and similar to copy edited files from ``python_payload/``:

::

	$ mpremote cp python_payload/main.py :/flash/sys/main.py

*TODO: document mpremote mount, it's currently broken*

As with application development, you can first check your changes using the simulator:

::

	$ python3 sim/run.py

You should also run typechecks:

::

	$ MYPYPATH=python_payload/mypystubs mypy python_payload/main.py

Working on C st3m code
----------------------

Make sure you have ``ninja`` installed - CMake will happily generate code for Make if Ninja is missing, but it won't necessarily work.

To compile:

::
	
	$ idf.py build

To flash the main firmware only (without overwriting the FAT32 partition or recovery image), put the badge in :ref:`flash` mode and run:

::
	
	$ idf.py app-flash

Note: do not run ``idf.py flash`` as that will prevent you from going into recovery mode. If you're flashing a factory-new badge, you also need to flash the recovery partition/bootloader/firmware first. See `flashing recovery`_.

To clean, do not trust ``idf.py clean``. Instead, kill everything with fire:

::
	
	$ rm -rf sdkconfig build

To edit the sdkconfig temporarily:

::
	
	$ idf.py menuconfig

To commit your sdkconfig changes to git, run menuconfig, press *d*, accept the default path. Then, copy over ``build/defconfig`` onto ``sdkconfig.defaults``.

.. _`flashing recovery`:

Flashing Recovery
-----------------

Tl;DR use the following script to flash *everything*:

::

	$ tools/flash-full.sh

The long story is that the main firmware codebase has a slightly different
partition layout (as seen by the flashing tooling) than the recovery tooling.
The one used in the recovery project (``recovery/partitions.csv``) is the
correct one. However, we can't use it as the main ``partitions.csv`` file as
ESP-IDF performs magical detection from that file on where the build artifact
should be located, and it always defaults to flashing to the ``factory`` image.
Thus, in the real/recovery partition table the recovery firmware is the
``factory`` image, while the main firmware is in the ``ota_0`` partition. But to
make ``idf.py app-flash`` work in the main firmware repository, there the main
firmware is marked as ``factory``. But if you flash the main firmware's
partition table to the device, the recovery partition will stop working.

In addition to Different-Partition-Table shenanigans, the second-stage
bootloader is also a problem. As with the partition teable, the correct one is
the recovery one. Using this bootloader allows you to pick the recovery image on
startup by holding the right trigger.

So, in order to have a functioning badge you shoud:

 1. Flash the partition table from recovery
 2. Flash the bootloader from recovery
 3. Flash the factory image from recovery
 4. Flash the ota_0 image from main

Or, in code:

::

	$ (cd recovery && idf.py erase-flash flash)
	$ idf.py app-flash

Thich is what ``tools/flash-full`` does.

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

Rewrite it in Rust
^^^^^^^^^^^^^^^^^^

If you fancy playing with Rust on the flow3r, check out the `flow3-rs <https://git.flow3r.garden/flow3r/flow3-rs>`_ project.

Hardware Generations
--------------------

If you've received your badge at CCCamp2023, you have a Production Badge and thus you don't need to worry about this section. Congratulations!

For those who have a prototype badge, there's an ``idf.py -g pX`` flag which you can use to get the firmware running on your hardware:

+------------------+----------+-----------------------------------+
| Badge Generation | Markings | Flag                              |
+==================+==========+===================================+
| Prototype 4      | B4xx     | *dead*                            |
+------------------+----------+-----------------------------------+
| Prototype 3      | B3xx     | ``-g p3``                         |
+------------------+----------+-----------------------------------+
| Prototype 4      | B4xx     | ``-g p4``                         |
+------------------+----------+-----------------------------------+
| Prototype 5      | B5xx     | *port me*                         |
+------------------+----------+-----------------------------------+
| Prototype 6      | B6xx     | ``-g p6`` (default, same as prod) |
+------------------+----------+-----------------------------------+

*NOTE: Anything older than p6 is not (yet?) supported by the recovery firmware.*

Writing Docs
------------

Automatically updated on CI runs of the main branch and lives under https://docs.flow3r.garden.

You will need ``sphinx`` and ``sphinx_rtd_theme`` installed. If you're not usinx Nix, install these via venv:

::

    $ python3 -m venv venv
    $ venv/bin/pip install sphinx sphinx_rtd_theme
    $ . venv/bin/activate

To build the docs locally:

::

    $ cd docs
    $ make html
    $ firefox _build/html/index.html

To continuously build on change:

::
    
    $ watchexec make html

Releasing
---------

1. Check out a version of main that you'd like to cut a release from.
2. Create a new branch named ``release/[major].[minor].[patch]``, eg. ``git checkout -b release/1.2.3``.
3. Tag a the first release candidate: ``git tag v1.2.3+rc1``.
4. Build and perform QA (*TODO: document*).
5. If the release canidate needs more work, cherry-pick fixes from main, tag a subsequent RC (eg. ``git tag v1.2.3+rc2``) and go back to step 4.
6. If the release candidate is ready to be released, tag a full release (``git tag v1.2.3``) and push branch/tags to gitlab. (*TODO: build CI pipeline for release tags*)
