#!/bin/sh

# Argument to this script should be one of the folders
# in this directory

python /usr/src/freebsd-pkg-builder/pkg-builder.py /usr/src/libvortex-1.1 /usr/src/libvortex-1.1/freebsd/arch/ --skip-build --version=`cat /usr/src/libvortex-1.1/VERSION` --description="BEEP core implementation written in C" --outdir=/usr/src/freebsd-packages

