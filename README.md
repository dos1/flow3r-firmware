## Current functionality

Micropython repl hooked up, hw functionality partly broken>

Some fun commands to try:

```
import hardware
#turn on sound
hardware.set_global_volume_dB(-10)

from synth import tinysynth
a=tinysynth(440,1); # enters decay phase without stop signal
a.start();
b=tinysynth(660,0); # sustains until stop signal
b.start();
b.stop();

#tiny issue with garbage collect:
hardware.count_sources();
a.__del__();
hardware.count_sources();
import gc
del b
gc.collect()
hardware.count_sources();
#...don't know how to hook up gc to __del__, maybe wrong approach
```

Files can be transferred with mpremote, such as:

```
mpremote fs cp python_payload/boot.py :boot.py
mpremote fs cp python_payload/cap_touch_demo.py :cap_touch_demo.py
```

## How to install dependencies

### Generic

1. install esp-idf v4.4:
(copied from https://www.wemos.cc/en/latest/tutorials/others/build\_micropython\_esp32.html)
```
$ cd ~
$ git clone https://github.com/espressif/esp-idf.git
$ cd esp-idf
$ git checkout v4.4
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

Prepare build:

```
$ cd micropython/
$ make -C mpy-cross
$ cd ports/esp32
$ make submodules
$ cd ../../../
```

Build normally with idf.py:

```
$ idf.py build
```

### Flashing

Put badge into bootloader mode by holding left should button down during boot.

```
$ idf.py -p /dev/ttyACM0 flash
```

You can skip `-p /dev/ttyACM0` if you set the environment variable `ESPPORT=/dev/ttyACM0`. This environment variable is also set by default when using Nix.

After flashing, remember to powercycle your badge to get it into the user application.

### Accessing MicroPython REPL:

```
$ picocom -b 115200 /dev/ttyACM0
$ # or
$ screen /dev/ttyACM0
$ # or (will eat newlines in REPL, though)
$ idf.py -p /dev/ttyACM0 monitor
```

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

Change output volume in the `set_global_vol_dB(int8_t)` call; -90 for mute

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

Currently we have one large sdkconfig file. To modify it, run:

```
$ idf.py menuconfig
```

TODO(q3k): split into defaults
