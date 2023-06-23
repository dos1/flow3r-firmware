## Demo Payload

See python_payload/README.md for a demo application.

Files can be transferred either by performing an `idf.py flash` or with mpremote, such as:

```
mpremote fs cp python_payload/main.py :main.py
```

Alternatively, adafruit-ampy may work more reliably sometimes:

```
ampy -p /dev/ttyACM0 -d3 put main.py
```

Please transfer all .py files in python_payload/ for using the demo payload.

## How to install dependencies

Pay attention to install version 4.4. Other versions (specifically 5.x) did not work. To be sure follow the generic instructions below.

### Generic

1. install esp-idf v4.4:
(copied from https://www.wemos.cc/en/latest/tutorials/others/build\_micropython\_esp32.html)
```
$ cd ~
$ git clone https://github.com/espressif/esp-idf.git
$ cd esp-idf
$ git checkout v4.4.4
$ git submodule update --init --recursive

$ cd esp-idf
$ ./install.sh
$ source export.sh
```
best put something like "alias espidf='source ~/esp-idf/export.sh'" in your .bashrc etc,
you need to run it in every new terminal and adding it to autostart did bother us

### Nix(OS)

```
$ nix-shell nix/shell.nix
```

## How to build and flash

Standard ESP-IDF project machinery present and working. You can run `idf.py` from the git checkout and things should just work.

### Building

Prepare submodules:

```
git submodule update --init --recursive
```

Build normally with idf.py:

```
$ idf.py build
```

By default, code for the fourth generation prototype will be built. To select a different generation, either set `-g`/`--generation` during an `idf.py build` (which will get cached for subsequent builds) or set the BADGE_GENERATION environment variable to one of the following values:

| `-g` / `BADGE_GENERATION` value | Badge Generation                   | 
|---------------------------------|------------------------------------|
| `p1` or `proto1`                | Prototype 1                        |
| `p3` or `proto3`                | Prototype 3 (B3xx)                 |
| `p4` or `proto4`                | Prototype 4 (B4xx)                 |
| `p6` or `proto6`                | Prototype 6 (B6xx)                 |

**Important**: when switching generations, do a full clean by running `rm -rf sdkconfig build`. Otherwise you will get _weird_ errors and likely will end up building for the wrong architecture.

### Flashing

Put badge into bootloader mode by holding left should button down during boot.

```
$ idf.py -p /dev/ttyACM0 flash
```

The following targets are available for flashing:

| Target             | Flashes                                         |
|--------------------|-------------------------------------------------|
| `idf.py flash`     | Bootloader, C code, Python Code / FAT partition |
| `idf.py app-flash` | C code                                          |
| `idf.py vfs-flash` | Python Code / FAT partition                     |

You can skip `-p /dev/ttyACM0` if you set the environment variable `ESPPORT=/dev/ttyACM0`. This environment variable is also set by default when using Nix.

After flashing, remember to powercycle your badge to get it into the user application.


### Cleaning

For a full clean, do **not** trust `idf.py clean` or `idf.py fullclean`. Instead, do:

```
$ rm -rf build sdkconfig
```

### Accessing MicroPython REPL:

```
$ picocom -b 115200 /dev/ttyACM0
$ # or
$ screen /dev/ttyACM0
$ # or (will eat newlines in REPL, though)
$ idf.py -p /dev/ttyACM0 monitor
$ # or
$ mpremote repl /dev/ttyACM0
```

and press Ctrl-C (potentially twice) to exit the debug output.

### Use CMake

`idf.py` calls cmake under the hood for most operations. If you dislike using wrappers you can do the same work yourself:

```
mkdir build
cd build
cmake .. -G Ninja
ninja
```

There's `flash/monitor` targets, too (but no openocd/gdb...). To pick what port to flash to/monitor, set the ESPPORT environment variable.

## How to modify

### Structure

```
main/               - main module, starts micropython on core1 and continues
                      executing components/badge23/.
usermodule/         - `hardware`, `synth`, ... C modules exposed to micropython.
components/badge23/ - main ESP-IDF `app_main`, runs on core 0 after micropython
                      gets started on core1.
components/gc9a01/  - low-level LCD driver.
```

### General info

Global + micropython entry point: `app_main()` in `micropython/ports/esp32/main.c`, compiled into `main/` component.

C entry point, called by^: `os_app_main()` in components/badge23/espan.c

Register new C files for compilation: add to components/badge23/CMakelists.txt

### Debugging

The badge is currently configured to run in HW USB UART/JTAG mode (vs. using TinyUSB and 'software' CDC/whatever using the generic OTG peripheral).

What this means:

1. You can use the MicroPython REPL over a USB console,
2. The MicroPython REPL will also print ESP-IDF logs, including panics,
3. You can use OpenOCD/GDB.

#### printf() debugging and logging in C-land

Given the above, you can do the following to get a log. This is part of ESP-IDF's standard logging functionality.

```
static const char *TAG = "misery";
// ...
ESP_LOGI(TAG, "i love C");
```

However, this will **only work** if you first set `CONFIG_LOG_DEFAULT_LEVEL_INFO=y` (which will likely break programs interacting with micropython REPL as many components of the ESP-IDF will suddenly become very chatty). But that's fine for troubleshooting some C-land bugs.

If you want to only log errors or just add temporary logs, use `ESP_LOGE` instead, which will always print to the USB console.

`printf()` also just works. But it should only be used for development debugging, for long-term logging statements please use `ESP_LOG*` instead.

#### Running OpenOCD+GDB

First, make sure your badge is running in application mode (not bootloader mode! that will stay in bootloader mode).

Then, start OpenOCD:

```
$ OPENOCD_COMMANDS="-f board/esp32s3-builtin.cfg" idf.py openocd
```

(you can skip setting `OPENOCD_COMMANDS` if you're using Nix)

Then, in another terminal:

```
$ idf.py gdb
```

Good luck. The idf.py gdb/openocd scripts seem somewhat buggy.

### ESP-IDF functionality

#### sdkconfig / menuconfig

We have an sdkconfig.defaults file. It is used to generate a '.generated' file
with flow3r-specific options based on the given -g option. See the build
instructions above to see how to select the generation to build against.

The build system will generate an sdkconfig, but it should not be committed
into version control. Instead, treat it like an ephemeral artifact that you can
also modify for your own needs during development.

To run menuconfig, do the usual::

```
$ idf.py  menuconfig
```

(Specify -g or `BADGE_GENERATION` if you haven't built the firmware yet)

Then, either save into the temporary sdkconfig by using 'S', or save into a
defconfig by using 'D'. The resulting `build/defconfig` file can then be copied
into `sdkconfig` to change the defaults for a given generation (be sure to
remove BADGE23_* options that are usually generated by `idf_ext.py`).

### Badge link

Badge link lets you have UART between badges or other devices using a 3.5mm
audio cable.

Baud rates up to 5mbit are supported in theory, but data corruption is likely
with higher rates.

Use baud rate 31250 for MIDI.

Note that `badge_link.enable()` will refuse to enable line out if the cable is
not connected. Connect it first. Connecting headphones with badge link enabled
is not recommended, especially not when wearing them.

Example usage:

On both badges:

```
import badge_link
from machine import UART
badge_link.enable(badge_link.PIN_MASK_ALL)
```

On badge 1, connect the cable to line out, and configure uart with tx on tip
(as an example)

```
uart = UART(1, baudrate=115200, tx=badge_link.PIN_INDEX_LINE_OUT_TIP, rx=badge_link.PIN_INDEX_LINE_OUT_RING)
```

On badge 2, connect the cable to line in, and configure uart with tx on ring:

```
uart = UART(1, baudrate=115200, tx=badge_link.PIN_INDEX_LINE_IN_RING, rx=badge_link.PIN_INDEX_LINE_IN_TIP)
```

Then write and read from each side:

```
uart.write("hiiii")
uart.read(5)
```

### Documentation

To build sphinx docs:

```
cd docs
make html
firefox _build/html/index.html
```

To continuously build on change:

```
watchexec make html
```

## License
All original source code in this reporitory is Copyright (C) 2023 Flow3r Badge Contributors. This source code is licensed under the GNU General Public License Version 3.0 as described in the file COPYING.

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License Version 3.0 as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License Version 3.0 along with this program.  If not, see <https://www.gnu.org/licenses/>.
