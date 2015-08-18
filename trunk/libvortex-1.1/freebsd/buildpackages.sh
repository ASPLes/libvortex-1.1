#!/bin/sh

python /usr/src/freebsd-pkg-builder/pkg-builder.py /usr/src/libvortex-1.1 /usr/src/libvortex-1.1/freebsd/freebsd\:9\:x86\:64/ --skip-build --version=`cat /usr/src/libvortex-1.1/VERSION` --description="BEEP core implementation written in C" --outdir=/usr/src/freebsd-packages

