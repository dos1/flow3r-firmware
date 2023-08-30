#!/usr/bin/env bash
set -e -x
if (( $# != 2 )); then
    >&2 echo "Usage: encode-mpg <video-file> <output-file>"
    exit
fi

ffmpeg -i "$1" -vf scale=128:96 -c:v mpeg1video -b:v 96k -ac 1 -c:a mp2 -format mpeg -b:a 64k  -ar 48000  -r 24 "$2"


