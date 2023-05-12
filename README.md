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
```

## how to modify

### general info
global + micropython entry point: app_main() in micropython/ports/esp32/main.c (includes badge23/espan.h)
c entry point, called by^: os_app_main() in badge23/espan.c
register new c files for compilation: add to set(BADGE23_LIB) in micropython/ports/esp32/main/CMakelists.txt
change output volume in the set_global_vol_dB(int8_t) call; -90 for mute

to debug c files: printf broken atm, instead use:
#include "../../py/mphal.h"
mp_hal_stdout_tx_str("debug output: string literal here\n\r");

don't expect esp component registry or idf.py menuconfig to work.
tried adding config items manually to micropython/ports/esp32/boards/sdkconfig.badge23, doesn't work :/
just put them into header files manually ig;;
