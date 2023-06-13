SD Card
-------

There's a microSD card slot on the inner side of the badge.

There's no first-class firmware support for it yet, but you can mount it in the
micropython VFS with:

.. code-block:: python

   import machine, os
   sdcard = machine.SDCard(clk=47, cmd=48, d0=21)
   os.mount(sdcard, "/sd")

Filesystem support is provided by `FatFs
<http://elm-chan.org/fsw/ff/00index_e.html>` which claims to support
FAT16/32/exFAT. TODO: test

