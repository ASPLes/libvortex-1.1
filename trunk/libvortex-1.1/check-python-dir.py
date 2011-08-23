#!/usr/bin/python

import sys

directory_to_check = sys.argv[1]
for directory in sys.argv:
    if directory_to_check == directory:
        print "ok"
        sys.exit (0)


print "Unable to find directory %s into sys.path, this install directory will fail" % directory_to_check
sys.exit(0)




