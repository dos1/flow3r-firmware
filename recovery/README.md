Recovery
========

This is the source for the recovery app, which is flashed to the 'factory' slot on the ESP32.

This is a separate ESP-IDF project because it's not trivial to create more than
one application binary per ESP-IDF project.

This project has the 'real' partition table that is on production devices, containing both the factory slot (for the recovery image) and the ota_0 slot (for the current app). This is in contrast to partitions.csv in the main firmware project, which has a simplified (but compatible) layout where there is no partition for the recovery image, and the app is the factory image instead.

This project also contains a custom bootloader that chooses whether to boot recovery or the badge app depending on the right shoulder button state.

The current way to flash both recovery+app is to:

  1. Do `idf.py erase_flash flash` in the recovery project
  2. Do `idf.py app-flash` in the app project.

In the future we'll have some easier way to flash both, but this will do for now.
