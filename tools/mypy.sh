#!/usr/bin/env bash
set -e

if [ ! -f sdkconfig.defaults ] || [ ! -f recovery/sdkconfig.defaults ]; then
    echo >/dev/stderr "Run this script for the root of the repository (ie. tools/mypy.sh)."
    exit 1
fi

export MYPYPATH=$(pwd)/python_payload/mypystubs:$(pwd)/python_payload
echo "Checking st3m..."

_CI="${CI:-}"

if [ ! -z "${_CI}" ]; then
    : > warnings.txt
fi

failed=""
function _mypy() {
    if [ ! -z "${_CI}" ]; then
        mypy "$1" --strict --no-color-output >> warnings.txt || failed=true
    else
        mypy "$1" --strict || failed=true
    fi
}

_mypy python_payload/main.py
for f in python_payload/apps/*/flow3r.toml; do
    app_name="$(basename $(dirname $f))"
    echo "Checking ${app_name}..."
    _mypy python_payload/apps/${app}
done

if [ $failed ]; then
    echo "Failed"
    exit 1
fi
