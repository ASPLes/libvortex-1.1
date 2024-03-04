#!/usr/bin/python

from core_admin_common import command, support
import sys

import time
start = time.time ()
print "INFO: wait to avoid overwhelming github.."
# implement a random wait to avoid Too many requests error from github.com
from random import randint
from time import sleep
sleep (randint(1,20))
print "INFO: wait done (%d seconds waited).." % (time.time () - start)

(osname, oslongname, osversion) = support.get_os ()
release_name = osversion.split (" ")[1]
no_github_com_access = ["lenny", "squeeze", "wheezy", "centos6", "precise"]

if release_name in no_github_com_access:
    command.run ("cp -f LATEST-VERSION VERSION")
    sys.exit (0)
# end if

tries = 0
while True:
    # atttempt to download
    (status, info) = command.run ("""LANG=C git log | grep "^commit " | wc -l""")
    if status:
        if "Too Many Requests" in info and tries < 10:
            # increase attempts 
            tries += 1
            print "WARNING: tries=%d, found too many connections report: %s" % (tries, info)
            print "WARNING: waiting a bit to retry..."
            start = time.time ()
            sleep (randint(1,20))
            print "INFO: wait done (%d seconds waited).." % (time.time () - start)
            continue
        # end if
    # end if
    
    if status:
        print "ERROR: unable to get git version: %s" % info
        sys.exit (-1)
    # end if

    print "INFO: git finished without error.."
    for line in info.split ("\n"):
        print "  | %s" % line
    break
# end while

# get versision
# revision = info.split (" ")[2].replace (".", "").strip ()
revision = int (info.strip ()) + 1
print "INFO: Revision found: %s" % revision

version = open ("VERSION").read ().split (".b")[0].strip ()
version = "%s.b%s" % (version, revision)
print "INFO: Updated vesion to: %s" % version

open ("VERSION", "w").write ("%s\n" % version)
open ("LATEST-VERSION", "w").write ("%s\n" % version)

# also update Changelog
command.run ("./update-changelog.sh")




