#!/usr/bin/env bash
set -e -x

if [ ! -f sdkconfig.defaults ] || [ ! -f recovery/sdkconfig.defaults ]; then
    echo >/dev/stderr "Run this script for the root of the repository (ie. tools/mypy.sh)."
    exit 1
fi

export MYPYPATH=$(pwd)/python_payload/mypystubs
mypy python_payload/main.py --strict
