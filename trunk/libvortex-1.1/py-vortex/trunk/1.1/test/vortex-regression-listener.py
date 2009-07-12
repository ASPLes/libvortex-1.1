#!/usr/bin/python
# -*- coding: utf-8 -*-
#  PyVortex: Vortex Library Python bindings
#  Copyright (C) 2009 Advanced Software Production Line, S.L.
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public License
#  as published by the Free Software Foundation; either version 2.1
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this program; if not, write to the Free
#  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
#  02111-1307 USA
#  
#  You may find a copy of the license under this software is released
#  at COPYING file. This is LGPL software: you are welcome to develop
#  proprietary applications using this library without any royalty or
#  fee but returning back any change, improvement or addition in the
#  form of source code, project image, documentation patches, etc.
#
#  For commercial support on build BEEP enabled solutions contact us:
#          
#      Postal address:
#         Advanced Software Production Line, S.L.
#         C/ Antonio Suarez Nº 10, 
#         Edificio Alius A, Despacho 102
#         Alcalá de Henares 28802 (Madrid)
#         Spain
#
#      Email address:
#         info@aspl.es - http://www.aspl.es/vortex
#
import vortex
import signal
import time
import sys

def info (msg):
    print "[ INFO  ] : " + msg

def error (msg):
    print "[ ERROR ] : " + msg

def ok (msg):
    print "[  OK   ] : " + msg

def default_frame_received (conn, channel, frame, data):
    print ("Received a frame with content: " + frame.payload)
    # reply to the frame received
    if not channel.send_rpy (frame.payload, frame.payload_size, frame.msgno):
        error ("Failed to send reply, some parts of the regression test are not working")
    
    return

# regression test beep uris
REGRESSION_URI      = "http://iana.org/beep/transient/vortex-regression"
REGRESSION_URI_ZERO = "http://iana.org/beep/transient/vortex-regression/zero"
REGRESSION_URI_DENY = "http://iana.org/beep/transient/vortex-regression/deny"
REGRESSION_URI_DENY_SUPPORTED = "http://iana.org/beep/transient/vortex-regression/deny_supported"

def signal_handler (signal, stackframe):
    print ("Received signal: " + str (signal))
    return

if __name__ == '__main__':

    # create a context
    ctx = vortex.Ctx ()

    # init context
    if not ctx.init ():
        error ("Unable to init ctx, failed to start listener")
        sys.exit(-1)

    # configure signal handling
    signal.signal (signal.SIGTERM, signal_handler)
    signal.signal (signal.SIGINT, signal_handler)
    signal.signal (signal.SIGQUIT, signal_handler)

    # register all profiles used 
    vortex.register_profile (ctx, REGRESSION_URI,
                             frame_received=default_frame_received)

    # create a listener
    info ("starting listener at 0.0.0.0:44010")
    listener = vortex.create_listener (ctx, "0.0.0.0", "44010")

    # do a wait operation
    info ("waiting requests..")
    vortex.wait_listeners (ctx)

    sys.exit (0)
        
