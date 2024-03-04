#!/bin/bash

gitlog-to-changelog  | sed  's/libvortex-1.1: *//g' > ChangeLog
