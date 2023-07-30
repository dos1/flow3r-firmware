#!/usr/bin/env bash
set -e -x

if [ ! -f sdkconfig.defaults ] || [ ! -f recovery/sdkconfig.defaults ]; then
    echo >/dev/stderr "Run this script for the root of the repository (ie. tools/clang-tidy.sh)."
    exit 1
fi

export IDF_TOOLCHAIN=clang
idf.py -B clang-build reconfigure
# Minimum mpy build to generate includes.
( cd clang-build ;\
    ninja genhdr/qstrdefs.generated.h ;\
    ninja genhdr/root_pointers.h ;\
    ninja genhdr/moduledefs.h )
idf.py -B clang-build clang-check
