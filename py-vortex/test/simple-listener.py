#!/usr/bin/python

import sys
import vortex

def default_frame_received (conn, channel, frame, data):

    print ("Frame content received: " + frame.payload)

    # reply in the case of MSG received
    if frame.type == 'MSG':
       # reply doing an echo
       channel.send_rpy (frame.payload, frame.payload_size, frame.msg_no)

    return
    # end default_frame_received

# create a vortex.Ctx object
ctx = vortex.Ctx ()

# now, init the context and check status
if not ctx.init ():
   print ("ERROR: Unable to init ctx, failed to continue")
   sys.exit (-1)

# ctx created

# register support for a profile
vortex.register_profile (ctx, "urn:aspl.es:beep:profiles:my-profile",
                         frame_received=default_frame_received)

# start listener and check status
listener = vortex.create_listener (ctx, "0.0.0.0", "1602")

if not listener.is_ok ():
   print ("ERROR: failed to start listener, error was was: " + listener.error_msg)
   sys.exit (-1)

# wait for requests
vortex.wait_listeners (ctx, unlock_on_signal=True)
