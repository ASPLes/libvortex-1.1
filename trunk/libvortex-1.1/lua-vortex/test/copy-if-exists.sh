#!/bin/sh

if [ -e $1 ]; then
    cp $1 $2
fi

exit 0