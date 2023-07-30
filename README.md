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

Pay attention to install version 5.1 **WITH OUR PATCHES**. To be sure follow the generic instructions below.

### Generic

1. install esp-idf v5.1:
(copied from https://www.wemos.cc/en/latest/tutorials/others/build\_micropython\_esp32.html)
```
$ cd ~
$ git clone https://github.com/espressif/esp-idf.git
$ cd esp-idf
$ git checkout v5.1
$ git submodule update --init --recursive
$ patch -p1 < PathToYourFlow3rFirmwareRepo/third_party/b03c8912c73fa59061d97a2f5fd5acddcc3fa356.patch

$ ./install.sh
$ source export.sh
```
best put something like "alias espidf='source ~/esp-idf/export.sh'" in your .bashrc etc,
you need to run it in every new terminal and adding it to autostart did bother us

Alternatively, we have a fork of esp-idf in this gitlab (TODO: update to new
gitlab url and new org name when that's a thing)

```
$ cd ~
$ git clone git@git.card10.badge.events.ccc.de:badge23/esp-idf.git
$ cd esp-idf
$ git checkout v5.1-flow3r
$ git submodule update --init --recursive
```

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

| Target             | Flashes                             |
|--------------------|-------------------------------------|
| `idf.py flash`     | Bootloader, partition table, C code |
| `idf.py app-flash` | C code                              |

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
components/flow3r_bsp/             - Board Support Package, ie. low-level drivers
components/st3m/                   - Core C software (C-st3m)
python_payload/st3m/               - Core Python software (Py-st3m)
components/micropython/usermodule/ - Bindings between C-st3m and Py-st3m.
components/micropython/vendor/     - Micrpython fork.
```

### General info

Global + micropython entry point: `app_main()` in `micropython/ports/esp32/main.c`, compiled into `main/` component.

C entry point, called by above: `st3m_board_init()` in `components/st3m/st3m_board_init.c`

After C-st3m initializes, it returns and lets Micropython run. Micropython then will stay in a loop which attempts to run main.py, otherwise runs the REPL.

### Debugging

The badge starts with a UART/JTAG bridge, but then after boot switches to a custom USB stack based on TinyUSB. This stack will bring up a serial-based console that will run the Micropython REPL.

The serial console will carry any C printf() you throw in, and any active `ESP_LOGx` call (see: ESP logging levels in sdkconfig). It will also carry anything that micropython prints.

However, if the badge crashes, you will not see any output on the console. You are also not able to run OpenOCD/gdb. If you wish to perform these actions, you will have to modify `st3m_board_init.c` to disable USB console startup. You can also switch the default console to UART0 and use the UART0 peripheral over USB-C sideband pins.

See [tracking issue](https://git.card10.badge.events.ccc.de/badge23/firmware/-/issues/23).


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

(currently broken, see [tracking issue](https://git.card10.badge.events.ccc.de/badge23/firmware/-/issues/23).

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
remove FLOW3R_* options that are usually generated by `idf_ext.py`).

### Documentation

Automatically updated on CI runs of the main branch and lives under https://docs.flow3r.garden

#### Build

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
