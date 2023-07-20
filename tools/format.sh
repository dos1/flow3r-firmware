#!/usr/bin/env bash
set -e -x

# Python: black
black \
    python_payload \
    sim \
    tools

# C: clang-format
find . \
    -type d \
    \( \
        -path './build' -o \
        -path './recovery/build' -o \
        -path './components/micropython/vendor' -o \
        -path './components/tinyusb' -o \
        -path './components/ctx' -o \
        -path './sim' \
    \) -prune \
    -o \
    -name '*.[ch]' \
    -exec clang-format -i \{\} \;
