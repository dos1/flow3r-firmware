#!/usr/bin/env bash
set -e -x

# Python: black
black \
    python_payload \
    sim \
    tools \
    components/micropython/frozen

# C: clang-format
find . \
    -type d \
    \( \
        -path './build' -o \
        -path './recovery/build' -o \
        -path './components/micropython/vendor' -o \
        -path './components/tinyusb' -o \
        -path './components/ctx' -o \
        -path './components/bl00mbox' -o \
        -path './components/bmi270' -o \
        -path './components/bmp581' -o \
        -path './sim' \
    \) -prune \
    -o \
    -name '*.[ch]' \
    -exec clang-format -i \{\} \;
