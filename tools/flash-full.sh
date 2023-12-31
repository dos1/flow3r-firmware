#!/usr/bin/env bash
set -e -x

# This script performs a full erase & flash of all the firmware components,
# including the recovery mode.

if [ ! -f sdkconfig.defaults ] || [ ! -f recovery/sdkconfig.defaults ]; then
    echo >/dev/stderr "Run this script for the root of the repository (ie. tools/flash-full.sh)."
    exit 1
fi

# Uncomment the following and adapt the path to make this script also
# initialize esp-idf:
#
#source ~/esp-idf/export.sh

# and the following to ensure a clean build
#
#rm -rf build sdkconfig

idf.py build erase_flash
( cd recovery ; idf.py build flash )
idf.py app-flash
