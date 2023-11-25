.. include:: <isonum.txt>
.. _updating_firmware:

Updating firmware
=================

Update via webflash
-------------------

If you have a web browser which supports WebSerial, you can connect your badge
to your computer then navigate to:

https://flow3r.garden/flasher/

Update via updat3r
------------------

Since firmware v1.3.0, there is a built-in wifi updater.

Ensure wifi is connected, and then go to Settings |rarr| Check For Updates and
follow the instructions.

Update via USB mass storage
---------------------------

Put your badge into SD Card Disk Mode: either by selecting 'Disk Mode (SD)'
from the badge's System menu, or by rebooting into Recovery Mode (by holding
down the right trigger) and selecting 'Disk Mode (SD)' from there.

Now, with the badge connected to a computer, you should see a USB Mass Storage
('pendrive') appear. Copy flow3r.bin from the release there.

Then, stop disk mode, and from the Recovery Mode (boot the badge with the right
trigger pushed down), select 'Flash Firmware Image', then flow3r.bin.

Update using esptool.py
-----------------------

You can also fully flash the badge from the commandline. To do that, start the
badge in bootloader mode (this is different from recovery mode!) by holding
down the left trigger while powering it up.

Extract the .bin files from a release tarball, and run esptool.py with the
following arguments:

::

    esptool.py -p /dev/ttyACM0 -b 460800 \
        --before default_reset --after no_reset --chip esp32s3 \
        write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m \
        0x0 bootloader.bin \
        0x8000 partition-table.bin \
        0x10000 recovery.bin \
        0x90000 flow3r.bin \
        -e
