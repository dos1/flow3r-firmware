## current functionality
micropython repl hooked up, hw functionality partly broken

some fun commands to try:
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

files can be transferred with mpremote, such as:
```
mpremote fs cp python_payload/boot.py :boot.py
mpremote fs cp python_payload/cap_touch_demo.py :cap_touch_demo.py
```

## how to install dependencies

### Generic

1. install esp-idf v4.4:
(copied from https://www.wemos.cc/en/latest/tutorials/others/build_micropython_esp32.html)
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

## how to build

1. prepare build
```
$ cd micropython/
$ make -C mpy-cross
$ cd ports/esp32
$ make submodules
```
2. build/flash
make sure esp-idf is sourced as in step 1 and that you are in micropython/ports/esp32
build:
```
$ make
```
flash + build: (put badge into bootloader mode*)
```
$ make deploy PORT=/dev/ttyACM0
```
_*press right shoulder button down during boot (on modified "last gen" prototypes)_

empty build cache (useful when moving files around):
```
$ make clean
```

3. access micropython repl:

```
$ picocom -b 115200 /dev/ttyACM0
$ # OR
# screen /dev/ttyACM0
```

## how to modify

### general info

global + micropython entry point: app_main() in micropython/ports/esp32/main.c (includes badge23/espan.h)
c entry point, called by^: os_app_main() in badge23/espan.c
register new c files for compilation: add to set(BADGE23_LIB) in micropython/ports/esp32/main/CMakelists.txt
change output volume in the set_global_vol_dB(int8_t) call; -90 for mute

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

However, this will **only work** if you modify `micropython/ports/esp32/boards/sdkconfig.base` to set `CONFIG_LOG_DEFAULT_LEVEL_INFO=y` (which will likely break programs interacting with micropython REPL as many components of the ESP-IDF will suddenly become very chatty). But that's fine for troubleshooting some C-land bugs.

If you want to only log errors or just add temporary logs, use `ESP_LOGE` instead, which will always print to the USB console.

`printf()` also just works. But it should only be used for development debugging, for long-term logging statements please use `ESP_LOG*` instead.

#### Running OpenOCD+GDB

First, make sure your badge is running in application mode (not bootloader mode! that will stay in bootloader mode).

```
$ make -C micropython/ports/esp32 openocd
```

Then, in another terminal:

```
$ make -C micropython/ports/esp32 gdb
```

Good luck.

### ESP-IDF functionality

Micropython splits up sdkconfig into a bunch of small files. Unfortunately, that doesn't work well with ESP-IDFs menuconfig.

If you want to permanently toggle options, you'll have to do that by modifying `micropython/ports/esp32/boards/sdkconfig.badge23`, or another file referenced by `micropython/ports/esp32/GENERIC_S3_BADGE/mpconfigboard.cmake`. Keep in mind the usual Kconfig shenanigants: disabling an options requires removing its line (not setting it to 'n'!), and that some options might be enabled automatically by other options. After modifying files, you must `make clean` in micropython/ports/esp32. Bummer.

To verify whether your configuration is what you expect it to be, you can still run menuconfig on the effective/calculated sdkconfig that micropython assembles. You can even modify the settings which will affect the build, but will be lost on `make clean` (and on git push, of course). To do so, run:

```
$ make -C micropython/ports/esp32 menuconfig
```