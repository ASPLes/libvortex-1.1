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

# import sys for command line parsing
import sys

# import python vortex binding
import vortex

####################
# regression tests #
####################

def test_00_a_check (queue):

    a_tuple = queue.pop ()
    if not a_tuple:
        error ("Found not defined expected tuple, but found: " + a_tuple)
        return False
    if a_tuple[0] != 2 or a_tuple[1] != 3:
        error ("Expected to find differente values but found: " + str (a_tuple[0]) + ", and: " + str (a_tuple[1]))
        return False

    # get a string
    a_string = queue.pop ()
    if a_string != "This is an string":
        error ("Expected to receive string: 'This is an string', but received: " + a_string)
        return False

    # get a list
    a_list = queue.pop ()
    if len (a_list) != 4:
        error ("Expected to find list length: " + len (a_list))
        return False

    return True

def test_00_a():
    ##########
    # create a queue
    queue = vortex.AsyncQueue ()

    # call to terminate queue 
    del queue

    #########

    # create a queue
    queue = vortex.AsyncQueue ()

    # call to unref
    iterator = 0
    while iterator < 100:
        # unref 
        queue.unref ()
        
        # next operation
        iterator += 1

    # and now finish 
    del queue

    ######### now check data storage
    queue = vortex.AsyncQueue ()
    
    # push items
    queue.push (1)
    queue.push (2)
    queue.push (3)
    
    # get items
    value = queue.pop ()
    if value != 1:
        error ("Expected to find 1 but found: " + str(value))
        return False

    value = queue.pop ()
    if value != 2:
        error ("Expected to find 2 but found: " + str(value))
        return False

    value = queue.pop ()
    if value != 3:
        error ("Expected to find 3 but found: " + str(value))
        return False

    # call to unref 
    queue.unref ()

    ###### now masive add operations
    queue = vortex.AsyncQueue ()

    # add items
    iterator = 0
    while iterator < 1000:
        queue.push (iterator)
        iterator += 1

    # restore items
    iterator = 0
    while iterator < 1000:
        value = queue.pop ()
        if value != iterator:
            error ("Expected to find: " + str(value) + ", but found: " + str(iterator))
            return False
        iterator += 1

    # finish queue 
    queue.unref ()

    ##### now add different types of data
    queue = vortex.AsyncQueue ()

    queue.push ((2, 3))
    queue.push ("This is an string")
    queue.push ([1, 2, 3, 4])

    # get a tuple
    if not test_00_a_check (queue):
        return False

    # unref the queue
    queue.unref ()

    #### now add several different item
    queue    = vortex.AsyncQueue ()
    iterator = 0
    while iterator < 1000:
        
        queue.push ((2, 3))
        queue.push ("This is an string")
        queue.push ([1, 2, 3, 4])

        # next iterator
        iterator += 1

    # now retreive all items
    iterator = 0
    while iterator < 1000:
        # check queue items
        if not test_00_a_check (queue):
            return False
        
        # next iterator
        iterator += 1

    # finish the queue
    queue.unref ()

    return True

def test_01():
    # call to initilize a context and to finish it 
    ctx = vortex.Ctx ()

    # init context and finish it */
    info ("init context..")
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    # ok, now finish context
    info ("finishing context..")
    ctx.exit ()

    # finish ctx 
    del ctx

    return True

def test_02():
    # call to initialize a context 
    ctx = vortex.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    # call to create a connection
    conn = vortex.Connection (ctx, host, port)

    # check connection status after if 
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    info ("BEEP connection created to: " + conn.host + ":" + conn.port) 
    
    # now close the connection
    info ("Now closing the BEEP session..")
    conn.close ()

    ctx.exit ()

    # finish ctx 
    del ctx

    return True

# test connection shutdown before close.
def test_03 ():
    # call to initialize a context 
    ctx = vortex.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    # call to create a connection
    conn = vortex.Connection (ctx, host, port)

    # check connection status after if 
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    # now shutdown
    conn.shutdown ()
    
    # now close the connection (already shutted down)
    conn.close ()

    ctx.exit ()

    # finish ctx 
    del ctx

    return True

# create a channel
def test_04 ():
    # call to initialize a context 
    ctx = vortex.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    # call to create a connection
    conn = vortex.Connection (ctx, host, port)

    # check connection status after if 
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    # now create a channel
    channel     = conn.open_channel (0, REGRESSION_URI)

    if not channel:
        error ("Expected to find proper channel creation, but error found:")
        # get first message
        error = conn.pop_channel_error ()
        while error:
            error ("Found error message: " + str (error.code) + ": " + error.msg)

            # next message
            error = conn.pop_channel_error ()
        return False

    # check channel installed
    if conn.num_channels != 2:
        error ("Expected to find only two channels installed (administrative BEEP channel 0 and test channel) but found: " + conn.num_channels ())
        return False

    # now close the channel
    if not channel.close ():
        error ("Expected to find proper channel close operation, but error found: ")
        # get first message
        error = conn.pop_channel_error ()
        while error:
            error ("Found error message: " + str (error.code) + ": " + error.msg)

            # next message
            error = conn.pop_channel_error ()
        return False

    # check channel installed
    if conn.num_channels != 1:
        error ("Expected to find only one channel installed (administrative BEEP channel 0) but found: " + conn.num_channels ())
        return False
    
    # now close the connection (already shutted down)
    conn.close ()

    ctx.exit ()

    # finish ctx 
    del ctx

    return True

def test_05_received (conn, channel, frame, data):

    # push data received
    data.push (frame)
    
    return

# create a channel
def test_05 ():
    # call to initialize a context 
    ctx = vortex.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    # call to create a connection
    conn = vortex.Connection (ctx, host, port)

    # check connection status after if 
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    # now create a channel
    channel  = conn.open_channel (0, REGRESSION_URI)

    if not channel:
        error ("Expected to find proper channel creation, but error found:")
        # get first message
        error_data = conn.pop_channel_error ()
        while error_data:
            error ("Found error message: " + str (error_data.code) + ": " + error_data.msg)

            # next message
            error_data = conn.pop_channel_error ()
        return False

    # configure frame received handler 
    queue = vortex.AsyncQueue ()
    channel.set_frame_received (test_05_received, queue)

    # send a message to test */
    channel.send_msg ("This is a test", 14)

    # wait for the reply
    frame = queue.pop ()

    # finish the queue (not required)
    queue.unref ()

    # check frame content here 
    if frame.payload != "This is a test":
        error ("Received frame content '" + frame.payload + "', but expected: 'This is a test'")
        return False

    # check frame type
    if frame.type != "RPY":
        error ("Expected to receive frame type RPY but found: " + frame.type)
        return False

    # check frame sizes
    if frame.content_size != 16:
        error ("Expected to find content size equal to 16 but found: " + frame.content_size)
        
    # check frame sizes
    if frame.payload_size != 14:
        error ("Expected to find payload size equal to 14 but found: " + frame.payload_size)


    # now close the connection (already shutted down)
    conn.close ()

    ctx.exit ()

    return True

def test_06_received (conn, channel, frame, data):
    # push frame received
    data.push (frame)

def test_06 ():
    # call to initialize a context 
    ctx = vortex.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    # call to create a connection
    conn = vortex.Connection (ctx, host, port)

    # check connection status after if 
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    # now create a channel
    channel  = conn.open_channel (0, REGRESSION_URI)

    # flag the channel to do deliveries in a serial form
    channel.set_serialize = True

    # configure frame received
    queue    = vortex.AsyncQueue ()
    channel.set_frame_received (test_06_received, queue)

    # send 100 frames and receive its replies
    iterator = 0
    while iterator < 100:
        # build message
        message = ";; This buffer is for notes you don't want to save, and for Lisp evaluation.\n\
;; If you want to create a file, visit that file with C-x C-f,\n\
;; then enter the text in that file's own buffer: message num: " + str (iterator)

        # send the message
        channel.send_msg (message, len (message))

        # update iterator
        iterator += 1

    # now receive and process all messages
    iterator = 0
    while iterator < 100:
        # build message to check
        message = ";; This buffer is for notes you don't want to save, and for Lisp evaluation.\n\
;; If you want to create a file, visit that file with C-x C-f,\n\
;; then enter the text in that file's own buffer: message num: " + str (iterator)

        # now get a frame
        frame = queue.pop ()

        # check content
        if frame.payload != message:
            error ("Expected to find message '" + message + "' but found: '" + frame.payload + "'")
            return False

        # next iterator
        iterator += 1

    # now check there are no pending message in the queue
    if queue.items != 0:
        error ("Expected to find 0 items in the queue but found: " + queue.items)
        return False

    # close connection
    conn.close ()

    # finish context
    ctx.exit ()

    return True

def test_07 ():
    # call to initialize a context 
    ctx = vortex.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    # call to create a connection
    conn = vortex.Connection (ctx, host, port)

    # check connection status after if 
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    # now create a channel
    channel  = conn.open_channel (0, REGRESSION_URI)

    # configure frame received
    queue    = vortex.AsyncQueue ()
    channel.set_frame_received (test_06_received, queue)

    # send 100 frames and receive its replies
    iterator = 0
    while iterator < 100:
        # build message
        message = ";; This buffer is for notes you don't want to save, and for Lisp evaluation.\n\
;; If you want to create a file, visit that file with C-x C-f,\n\
;; then enter the text in that file's own buffer: message num: " + str (iterator)

        # send the message
        channel.send_msg (message, len (message))

        # now get a frame
        frame = queue.pop ()

        # check content
        if frame.payload != message:
            error ("Expected to find message '" + message + "' but found: '" + frame.payload + "'")
            return False

        # next iterator
        iterator += 1

    # now check there are no pending message in the queue
    if queue.items != 0:
        error ("Expected to find 0 items in the queue but found: " + queue.items)
        return False

    # close connection
    conn.close ()

    # finish context
    ctx.exit ()

    return True

def test_08 ():
    # call to initialize a context 
    ctx = vortex.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    # call to create a connection
    conn = vortex.Connection (ctx, host, port)

    # check connection status after if 
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    # now create a channel
    channel  = conn.open_channel (0, REGRESSION_URI_ZERO)

    # configure frame received
    queue    = vortex.AsyncQueue ()

    # configure frame received
    channel.set_frame_received (test_06_received, queue)

    # build the content to transfer (add r to avoid python to handle it)
    message = r"\0\0\0\0\0\0\0\0" * 8192

    iterator = 0
    while iterator < 10:
        # send the message
        channel.send_msg (message, len (message))

        # next iterator
        iterator += 1

    # now receive content and check
    iterator = 0
    while iterator < 10:
        # receive 
        frame = queue.pop ()

        # check content
        if frame.payload != message:
            error ("Expected to find binary zerored string but found string mismatch")
            return False

        # check content length
        if frame.payload_size != len (message):
            error ("String size mismatch, expected to find: " + str (len (message)) + ", but found: " + frame.payload_size)
            return False

        # next iterator
        iterator += 1

    # close connection
    conn.close ()

    # finish context
    ctx.exit ()

    return True

def test_09 ():
    # max channels
    test_09_max_channels = 24;
    
    # call to initialize a context 
    ctx = vortex.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    # call to create a connection
    conn = vortex.Connection (ctx, host, port)

    # check connection status after if 
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    # now create a channel
    queue    = vortex.AsyncQueue ()

    iterator = 0
    channels = []
    while iterator < test_09_max_channels:
        # create the channel
        channels.append (conn.open_channel (0, REGRESSION_URI))

        # set frame received
        channels[iterator].set_frame_received (test_06_received, queue)

        # next iterator
        iterator += 1

    # send content over all channels
    for channel in channels:
        channel.send_msg ("This is a test..", 16)

    print ("test_09: Messages sent..");

    # pop all messages replies
    iterator = 0
    while iterator < test_09_max_channels:

        print ("test_09: about to receive a frame..");
        
        # get frame
        frame = queue.pop ()

        # check content
        if frame.payload != "This is a test..":
            error ("Expected to find 'This is a test' but found: " + frame.payload)
            return False

        # next iterator
        iterator += 1

    print ("test_09: After all frames received..");

    # check no pending items are in the queue
    if queue.items != 0:
        error ("Expected to find 0 items on the queue, but found: " + queue.items)
        return False

    # now close all channels
    for channel in channels:
        # close the channels
        channel.close ()

    # check channels opened on the connection
    if conn.num_channels != 1:
        error ("Expected to find only two channels installed (administrative BEEP channel 0 and test channel) but found: " + conn.num_channels ())
        return False
    
    # close connection
    conn.close ()

    # finish context
    ctx.exit ()

    return True

###########################
# intraestructure support #
###########################

def info (msg):
    print "[ INFO  ] : " + msg

def error (msg):
    print "[ ERROR ] : " + msg

def ok (msg):
    print "[  OK   ] : " + msg

def run_all_tests ():
    test_count = 0
    for test in tests:

        # print log
        info ("Test-" + str(test_count) + ": Running " + test[1])
        
        # call test
        if not test[0]():
            error ("detected test failure at: " + test[1])
            return False
        
        # next test
        test_count += 1
    
    ok ("All tests ok!")
    return True

# declare list of tests available
tests = [
#    (test_00_a, "Check PyVortex async queue wrapper"),
#    (test_01,   "Check PyVortex context initialization"),
#    (test_02,   "Check PyVortex basic BEEP connection"),
#    (test_03,   "Check PyVortex basic BEEP connection (shutdown)"),
#    (test_04,   "Check PyVortex basic BEEP channel creation"),
#    (test_05,   "Check BEEP basic data exchange"),
#    (test_06,   "Check BEEP check several send operations (serialize)"),
#    (test_07,   "Check BEEP check several send operations (one send, one receive)"),
#    (test_08,   "Check BEEP transfer zeroed binaries frames"),
    (test_09,   "Check BEEP channel support")
]

# declare default host and port
host     = "localhost"
port     = "44010"

# regression test beep uris
REGRESSION_URI      = "http://iana.org/beep/transient/vortex-regression"
REGRESSION_URI_ZERO = "http://iana.org/beep/transient/vortex-regression/zero"

if __name__ == '__main__':
    iterator = 0
    for arg in sys.argv:
        # according to the argument position, take the value 
        if iterator == 1:
            host = arg
        elif iterator == 2:
            port = arg
            
        # next iterator
        iterator += 1

    # drop a log
    info ("Running tests against " + host + ":" + port)

    # call to run all tests
    run_all_tests ()




