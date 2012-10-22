#!/usr/bin/python

# import base vortex module
import vortex

# import sys (sys.exit)
import sys

def frame_received (conn, channel, frame, queue):
    # messages received asynchronously
    print ("Frame received, type: " + frame.type)
    print ("Frame content: " + frame.payload)

    # push que message into the queue so the waiter can get it
    queue.push (frame)
    return

# create a vortex.Ctx object
ctx = vortex.Ctx ()

# now, init the context and check status
if not ctx.init ():
   print ("ERROR: Unable to init ctx, failed to continue")
   sys.exit (-1)

# ctx created

# create a BEEP session connected to localhost:1602
conn = vortex.Connection (ctx, "localhost", "1602")

# check connection status
if not conn.is_ok ():
   print ("ERROR: Failed to create connection with 'localhost':1602")
   sys.exit (-1)

# create a channel running a user defined profile
channel = conn.open_channel (0, "urn:aspl.es:beep:profiles:my-profile")

if not channel:
    print ("ERROR: Expected to find proper channel creation, but error found:")
    # get first message
    (err_code, err_msg) = conn.pop_channel_error ()
    while err_code:
         print ("ERROR: Found error message: " + str (error_code) + ": " + error_msg)

         # next message
         (err_code, err_msg) = conn.pop_channel_error ()
    sys.exit (-1)

# channel created  
if channel.send_msg ("This is a test", 14) is None:
    print ("ERROR: Failed to send test message, error found: ")

    # get first message
    err = conn.pop_channel_error ()
    while err:
         print ("ERROR: Found error message: " + str (err[0]) + ": " + err[1])

         # next message
         err = conn.pop_channel_error ()
    sys.exit (-1)

# create a queue (just for the purpose to be blocked waiting for the
# reply). Note that this is not a recommended way to do a wait (only
# some cases).
queue = vortex.AsyncQueue ()

# set a frame received to receive frames there
channel.set_frame_received (frame_received, queue)

# wait for 3 seconds for a reply
print "Waiting reply.."
frame = queue.timedpop (3000000)
if not frame:
    print "ERROR: timeout found while waiting for reply"
    sys.exit (-1)

# frame received
print "INFO: frame received (%d bytes).." % frame.payload_size
sys.exit (0)
    
