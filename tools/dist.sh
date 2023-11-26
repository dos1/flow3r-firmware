#!/usr/bin/env bash

# Builds an official distribution tarball for the firmware.
#
# Contains main firmware, recovery firmware and recovery bootloader.

set -e -x

if [ ! -f sdkconfig.defaults ] || [ ! -f recovery/sdkconfig.defaults ]; then
    echo >/dev/stderr "Run this script for the root of the repository (ie. tools/dist.sh)."
    exit 1
fi

# Always fetch full history on Gitlab when not building a tag, otherwise we get bogus results.
if [ ! -z "${CI}" && -z "${CI_COMMIT_TAG}" ]; then
    git fetch --unshallow origin "$CI_COMMIT_REF_NAME"
    git checkout "$CI_COMMIT_REF_NAME"
fi
version="$(python3 components/st3m/host-tools/version.py)"

rm -rf sdkconfig build
rm -rf recovery/sdkconfig recovery/build
idf.py build
( cd recovery ; idf.py build )

tmpdir="$(mktemp -d -t flow3r-dist-XXXX)"
function cleanup {
  rm -rf "$tmpdir"
}
trap cleanup EXIT

name="flow3r-${version}"
distdir="${tmpdir}/${name}"
mkdir -p "${distdir}"

cp recovery/build/flow3r-recovery.bin "${distdir}/recovery.bin"
cp recovery/build/partition_table/partition-table.bin "${distdir}/partition-table.bin"
cp recovery/build/bootloader/bootloader.bin "${distdir}/bootloader.bin"
cp build/flow3r.bin "${distdir}/flow3r.bin"

cat > "${distdir}/README.txt" <<EOF
This is release ${version} of the flow3r firmware.

There are three ways to install this firmware to your badge:

// Option 1: Update via webflash

If you have a web browser which supports WebSerial, you can connect your badge
to your computer then navigate to:

  https://flow3r.garden/flasher/

// Option 2: Update via updat3r

Since firmware v1.3.0, there is a built-in wifi updater.

Ensure wifi is connected, and then go to Settings -> Check For Updates and
follow the instructions.

// Option 3: Update via the badge

Put your badge into SD Card Disk Mode: either by selecting 'Disk Mode (SD)'
from the badge's System menu, or by rebooting into Recovery Mode (by holding
down the right trigger) and selecting 'Disk Mode (SD)' from there.

Now, with the badge connected to a computer, you should see a USB Mass Storage
('pendrive') appear. Copy flow3r.bin from this archive there.

Then, stop disk mode, and from the Recovery Mode (boot the badge with the right
trigger pushed down), select 'Flash Firmware Image', then flow3r.bin.

// Option 4: Update using esptool.py

You can also fully flash the badge from the commandline. To do that, start the
badge in bootloader mode (this is different from recovery mode!) by holding
down the left trigger while powering it up.

Extract the .bin files from a release tarball, and run esptool.py with the
following arguments:

    esptool.py -p /dev/ttyACM0 -b 460800 \\
        --before default_reset --after no_reset --chip esp32s3 \\
        write_flash --flash_mode dio --flash_size 16MB --flash_freq 80m \\
        0x0 bootloader.bin \\
        0x8000 partition-table.bin \\
        0x10000 recovery.bin \\
        0x90000 flow3r.bin \\
        -e
EOF

mkdir -p dist
tar="dist/${name}.tar.bz2"
tar -cjf "${tar}" -C "${tmpdir}" "${name}"

function upload() {
    from=$1
    to=$2
    if [ ! -z "$CI_JOB_TOKEN" ]; then
        curl --header "JOB-TOKEN: $CI_JOB_TOKEN" \
             --upload-file "${from}" \
             "${CI_API_V4_URL}/projects/${CI_PROJECT_ID}/packages/generic/flow3r-firmware/${version}/${to}"
    else
        echo Would upload $from to $to
    fi
}

upload "${tar}" "${name}.tar.bz2"
upload "${distdir}/recovery.bin" "recovery.bin"
upload "${distdir}/partition-table.bin" "partition-table.bin"
upload "${distdir}/bootloader.bin" "bootloader.bin"
upload "${distdir}/flow3r.bin" "flow3r.bin"
