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
import time

# import python vortex binding
import vortex

# import vortex sasl support
import vortex.sasl

# import vortex tls support
import vortex.tls

# import alive support
import vortex.alive

# import common items for reg test
from regtest_common import *

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
    # del queue # queue.unref ()

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

    ##### now add different types of data
    queue = vortex.AsyncQueue ()

    queue.push ((2, 3))
    queue.push ("This is an string")
    queue.push ([1, 2, 3, 4])

    # get a tuple
    if not test_00_a_check (queue):
        return False

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

# test connection shutdown before close.
def test_03_a ():
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

    # set some data
    conn.set_data ('value', 1)
    conn.set_data ('value2', 2)
    conn.set_data ('boolean', True)

    # now set a connection to also check it is released
    conn2 = vortex.Connection (ctx, host, port)

    # check connection status after if 
    if not conn2.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    conn.set_data ('conn', conn2)

    # recover data
    if conn.get_data ('value') != 1:
        error ("Expected to find value == 1 but found: " + str (conn.get_data ('value')))
        return False
    if conn.get_data ('value2') != 2:
        error ("Expected to find value2 == 2 but found: " + str (conn.get_data ('value2')))
        return False
    if not conn.get_data ('boolean'):
        error ("Expected to find boolean == True but found: " + str (conn.get_data ('boolean')))
        return False

    conn3 = conn.get_data ('conn')

    # check conn references
    if conn2.id != conn3.id:
        error ("Expected to find same connection references but found they differs: " + str (conn2.id) + " != " + str (conn3.id))

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

    # check find by uri method
    channels = conn.find_by_uri (REGRESSION_URI)
    if len (channels) != 0:
        error ("Expected to find 0 channels opened with " + REGRESSION_URI + ", but found: " + str (len (channels)))
        return False

    # now create a channel
    channel     = conn.open_channel (0, REGRESSION_URI)

    if not channel:
        error ("Expected to find proper channel creation, but error found:")
        # get first message
        err = conn.pop_channel_error ()
        while err:
            error ("Found error message: " + str (err[0]) + ": " + err[1])

            # next message
            err = conn.pop_channel_error ()
        return False

    # check ready flag 
    if not channel.is_ready:
        error ("Expected to find channel flagged as ready..")
        return False

    # check find by uri method
    channels = conn.find_by_uri (REGRESSION_URI)
    if len (channels) != 1:
        error ("Expected to find 1 channels opened with " + REGRESSION_URI + ", but found: " + str (len (channels)))
        return False

    if channels[0].number != channel.number:
        error ("Expected to find equal channel number, but found: " + str (channels[0].number))
        return False

    if channels[0].profile != channel.profile:
        error ("Expected to find equal channel number, but found: " + str (channels[0].number))
        return False

    # check channel installed
    if conn.num_channels != 2:
        error ("Expected to find only two channels installed (administrative BEEP channel 0 and test channel) but found: " + conn.num_channels ())
        return False

    # now close the channel
    if not channel.close ():
        error ("Expected to find proper channel close operation, but error found: ")
        # get first message
        err = conn.pop_channel_error ()
        while err:
            error ("Found error message: " + str (err[0]) + ": " + err[1])

            # next message
            err = conn.pop_channel_error ()
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
    channel = conn.open_channel (0, REGRESSION_URI)

    if not channel:
        error ("Expected to find proper channel creation, but error found:")
        # get first message
        err = conn.pop_channel_error ()
        while err:
            error ("Found error message: " + str (err[0]) + ": " + err[1])

            # next message
            err = conn.pop_channel_error ()
        return False

    # configure frame received handler 
    queue = vortex.AsyncQueue ()
    channel.set_frame_received (vortex.queue_reply, queue)

    # send a message to test */
    channel.send_msg ("This is a test", 14)

    # wait for the reply
    frame = channel.get_reply (queue)

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

    # now test to remove frame received
    channel.set_frame_received ()

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
    test_09_max_channels = 24
    
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
        channels.append (conn.open_channel (0, REGRESSION_URI, 
                                            # configure frame received
                                            frame_received=vortex.queue_reply, frame_received_data=queue))

        # next iterator
        iterator += 1

    # send content over all channels
    for channel in channels:
        # check message send status
        if channel.send_msg ("This is a test..", 16) < 0:
            print ("Failed to send message..")

    # pop all messages replies
    for channel in channels:

        # get frame
        frame = channel.get_reply (queue)

        # check content
        if frame.payload != "This is a test..":
            error ("Expected to find 'This is a test' but found: " + frame.payload)
            return False

    # check no pending items are in the queue
    if queue.items != 0:
        error ("Expected to find 0 items on the queue, but found: " + queue.items)
        return False

    # now close all channels
    for channel in channels:
        # close the channels
        if not channel.close ():
            error ("Expected to close channel opened previously, but found an error..")
            return False

    # check channels opened on the connection
    if conn.num_channels != 1:
        error ("Expected to find only two channels installed (administrative BEEP channel 0 and test channel) but found: " + str (conn.num_channels))
        return False

    # close connection
    conn.close ()

    # finish context
    ctx.exit ()

    return True

def test_10 ():
    
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

    # open a channel
    channel = conn.open_channel (0, REGRESSION_URI_DENY)
    if channel: 
        error ("Expected to find channel error but found a proper channel reference (1)")
        return False

    # check errors here 
    err = conn.pop_channel_error ()
    if err[0] != 554:
        error ("Expected to find error code 554 but found: " + str (err[0]))
        return False

    # check for no more pending errors
    err = conn.pop_channel_error ()
    if err:
        error ("Expected to find None (no error) but found: " + err)
        return False

    # open a channel (DENY with a supported profile) 
    channel = conn.open_channel (0, REGRESSION_URI_DENY_SUPPORTED)
    if channel: 
        error ("Expected to find channel error but found a proper channel reference (2)")
        return False

    # check errors here 
    err = conn.pop_channel_error ()
    if err[0] != 421:
        error ("Expected to find error code 421 but found: " + str (err[0]))
        return False

    # check for no more pending errors
    err = conn.pop_channel_error ()
    if err:
        error ("Expected to find None (no error) but found: " + err)
        return False

    # close connection
    conn.close ()

    # finish context
    ctx.exit ()

    return True

def test_10_a ():
    
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

    # open a channel
    queue   = vortex.AsyncQueue ()
    channel = conn.open_channel (0, REGRESSION_URI_ANS, 
                                 frame_received=vortex.queue_reply, frame_received_data=queue)
    if not channel: 
        error ("Expected to find channel error but found a proper channel reference")
        return False

    # enable serialization
    channel.set_serialize = True

    # send a message to receive all the content
    channel.send_msg ("give da content", 15)

    # wait for all replies 
    iterator = 0
    while iterator < 10:
        # get frame
        frame = channel.get_reply (queue)
        if not frame:
            print ("ERROR: expected to not receive None")
            
            # check connection
            if not conn.is_ok ():
                print ("ERROR: found connection closed (not working)")
        
            return False
            
        # check frame type
        if frame.type != "ANS":
            error ("Expected to receive frame type ANS but received: " + frame.type)
            return False

        # check frame size
        if frame.payload_size != len (TEST_REGRESSION_URI_4_MESSAGE):
            error ("Expected to receive " + str (frame.payload_size) + " bytes but received: " + str (len (TEST_REGRESSION_URI_4_MESSAGE)))
            return False

        # check frame content
        if frame.payload != TEST_REGRESSION_URI_4_MESSAGE:
            error ("Expected to receive content: " + frame.payload + " but received: " + TEST_REGRESSION_URI_4_MESSAGE)
            return False

        # next message
        iterator += 1
        
    # now check for last ans 
    frame = channel.get_reply (queue)

    # check frame type
    if frame.type != "NUL":
        error ("Expected to receive frame type NUL but received: " + frame.type)
        return False

    # check frame size
    if frame.payload_size != 0:
        error ("Expected to receive 0 bytes but received: " + str (frame.payload_size))
        return False

    return True

def test_10_b_received (conn, channel, frame, data):
    info ("Test 10-b: Notification received..")
    
    # queue connection and frame
    data.push (conn)
    data.push (frame)
    data.push (channel)
    return

def test_10_b_create_connection_and_send_content (ctx, queue):

    conn = vortex.Connection (ctx, host, port)

    # check connection status after if 
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    channel = conn.open_channel (0, REGRESSION_URI)
    if not channel: 
        error ("Expected to find channel error but found a proper channel reference")
        return False

    # now setup received handler
    channel.set_frame_received (test_10_b_received, queue)

    # send content
    channel.send_msg ("This is a test", 14)

    # channel.incref ()

    info ("Content sent, now wait for replies..")
    return conn

def test_10_b ():
    # create a context
    ctx = vortex.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    info ("Creating queue, connection, channel and sending content..")
    queue = vortex.AsyncQueue ()

    # PART 1: check channel.incref
    info ("PART 1: check channel.incref ()")
    conn2 = test_10_b_create_connection_and_send_content (ctx, queue)
    if not conn2:
        error ("Failed to initialize connection, channel or content to be sent")
        return False

    # now get reply
    info ("Waiting for replies....")
    conn    = queue.pop ()
    frame   = queue.pop ()
    channel = queue.pop ()

    info ("Received content.....")

    # check connection status
    if not conn.is_ok ():
        error ("Expected to find connection status ok, but found a failure: " + conn.status_msg)
        return False

    # check frame type and content
    if not frame.type == "RPY":
        error ("Expected to find frame type RPY but found: " + frame.type)
        return False

    if not frame.payload == "This is a test":
        error ("Expected to find frame content 'This is a test' but found: " + frame.payload)

    # decrement reference counting
    # channel.decref ()
    conn.close ()

    return True

def test_10_c_on_channel (number, channel, conn,  queue):

    info ("Received async channel notification, number: " + str (number) )
    queue.push (channel)
    return

def test_10_c ():
    # create a context
    ctx = vortex.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    queue = vortex.AsyncQueue ()

    # create connection
    conn = vortex.Connection (ctx, host, port)

    # check connection status
    if not conn.is_ok ():
        error ("Expected to find connection status ok, but found a failure: " + conn.status_msg)
        return False

    # ok now create channel without waiting
    conn.open_channel (0, REGRESSION_URI, on_channel=test_10_c_on_channel, on_channel_data=queue)

    # wait for response
    channel = queue.pop ()

    # check channel value here and send some content
    info ("Channel received in main thread: " + str (channel.number))

    # send the message
    message = "This is a test message after async channel notification"
    iterator = 0
    channel.set_frame_received (vortex.queue_reply, queue)
    while iterator < 10:
        channel.send_msg (message, len (message))

        # now get a frame
        frame = channel.get_reply (queue)
        if not frame:
            error ("Expected to find frame reply but found None reference")
            return False
        
        if frame.payload != message:
            error ("Expected to receive different message but found: " + frame.payload + ", rather: " + message)
            return False

        # next position
        iterator += 1

    # NOTE: the test do not close conn or channe (this is intentional)

    return True

def test_10_d ():
    # create a context
    ctx = vortex.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    queue = vortex.AsyncQueue ()

    # create connection
    conn = vortex.Connection (ctx, host, port)

    # check connection status
    if not conn.is_ok ():
        error ("Expected to find connection status ok, but found a failure: " + conn.status_msg)
        return False

    # ok now create channel without waiting
    conn.open_channel (0, REGRESSION_URI_DENY_SUPPORTED, on_channel=test_10_c_on_channel, on_channel_data=queue)

    # wait for response
    channel = queue.pop ()

    # check channel value here and send some content
    if channel:
        error ("Expected to find None value at channel reference..")
        return False

    # ok check again connection and create a channel
    if not conn.is_ok ():
        error ("Expected to find connection properly created..")
        return False

    channel = conn.open_channel (0, REGRESSION_URI)
    if not channel:
        error ("Expected to find proper channel..")
        return False

    # send some data
    iterator = 0
    channel.set_frame_received (vortex.queue_reply, queue)
    message = "This is a test at channel error expected.."
    while iterator < 10:
        channel.send_msg (message, len (message))

        # now get a frame
        frame = channel.get_reply (queue)
        if not frame:
            error ("Expected to find frame reply but found None reference")
            return False
        
        if frame.payload != message:
            error ("Expected to receive different message but found: " + frame.payload + ", rather: " + message)
            return False

        # next position
        iterator += 1

    # NOTE: the test do not close conn or channe (this is intentional)

    return True

def queue_reply (conn, channel, frame, data):
    data.push (frame)
    return

def create_channel_and_send (conn, queue):

    # ok now create channel without waiting
    channel = conn.open_channel (0, REGRESSION_URI)

    # set frame received
    channel.set_frame_received (queue_reply, queue)

    channel.send_msg ("This is a test", -1)

    return

def test_10_e ():
    # create a context
    ctx = vortex.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    queue = vortex.AsyncQueue ()

    # create connection
    conn = vortex.Connection (ctx, host, port)

    # check connection status
    if not conn.is_ok ():
        error ("Expected to find connection status ok, but found a failure: " + conn.status_msg)
        return False

    # create channel and send content
    create_channel_and_send (conn, queue)

    # wait for response
    frame = queue.pop ()

    if frame.payload != "This is a test":
        error ("Expected to find '%s' but found '%s'" % ("This is a test", frame.payload))
        return False

    info ("Found expected content!")

    # NOTE: the test do not close conn or channe (this is intentional)
    return True

def test_10_f ():

    # create a context
    ctx = vortex.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    queue = vortex.AsyncQueue ()

    # create connection
    iterator = 0
    while iterator < 5:
        conn = vortex.Connection (ctx, host, port)

        # check connection status
        if not conn.is_ok ():
            error ("Expected to find connection status ok, but found a failure: " + conn.status_msg)
            return False

        # ok now create channel without waiting
        channel = conn.open_channel (0, REGRESSION_URI)
        channel.set_frame_received (queue_reply, queue)
        channel.send_msg ("<close-connection>", -1)

        iterator += 1
 
    info ("Found expected content!")

    # NOTE: the test do not close conn or channe (this is intentional)
    return True

def test_11 ():
    # create a context
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

    # now check for services not available for a simple connection
    if conn.role != "initiator":
        error ("Expected to find 'initiator' as connection role, but found: " + conn.role)
        return False

    conn.close ()

    # now open a listener and check its function
    listener = vortex.create_listener (ctx, "0.0.0.0", "0")

    # check listener status
    if not listener.is_ok ():
        error ("Expected to find proper listener creation, but a failure found: " + listener.error_msg)
        return False

    # now check for 
    if listener.pop_channel_error ():
        error ("Expected to find None value returned from a method not available for listeners")
        return False

    # try to open a channel with the listener
    channel = listener.open_channel (0, REGRESSION_URI)
    if channel: 
        error ("Expected to find channel error but found a proper channel reference")
        return False

    # now try to connect 
    conn = vortex.Connection (ctx, listener.host, listener.port)
    
    # check connection
    if not conn.is_ok ():
        error ("Expected to find proper connection to local listener")
        return False

    # call to shutdown 
    listener.shutdown ()


    return True

def test_12_on_close_a (conn, queue2):
    queue = queue2.pop ()
    queue.push (1)

def test_12_on_close_b (conn, queue2):
    queue = queue2.pop ()
    queue.push (2)

def test_12_on_close_c (conn, queue2):
    queue = queue2.pop ()
    queue.push (3)

def test_12():
       # create a context
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

    # create a queue
    queue  = vortex.AsyncQueue ()
    queue2 = vortex.AsyncQueue ()

    # wait for replies
    queue2.push (queue)

    # configure on close
    queue_list = []
    conn.set_on_close (test_12_on_close_a, queue2)
    conn.set_on_close (test_12_on_close_b, queue2)
    conn.set_on_close (test_12_on_close_c, queue2)

    # now shutdown 
    conn.shutdown ()

    value = queue.pop ()
    if value != 1:
        error ("Test 12: Expected to find 1 but found (0001): " + str (value))
        return False

    # wait for replies
    queue2.push (queue)
    value = queue.pop ()
    if value != 2:
        error ("Expected to find 2 but found (0002): " + str (value))
        return False

    # wait for replies
    queue2.push (queue)
    value = queue.pop ()
    if value != 3:
        error ("Expected to find 3 but found (0003): " + str (value))
        return False

    # re-connect 
    conn = vortex.Connection (ctx, host, port)

    # check connection status after if 
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    # wait for replies
    queue2.push (queue)

    # configure on close
    conn.set_on_close (test_12_on_close_a, queue2)
    conn.set_on_close (test_12_on_close_a, queue2)
    conn.set_on_close (test_12_on_close_a, queue2)

    # now shutdown 
    conn.shutdown ()

    value = queue.pop ()
    if value != 1:
        error ("Expected to find 1 but found (0004): " + str (value))
        return False

    # wait for replies
    queue2.push (queue)
    value = queue.pop ()
    if value != 1:
        error ("Expected to find 1 but found (0005): " + str (value))
        return False

    # wait for replies
    queue2.push (queue)
    value = queue.pop ()
    if value != 1:
        error ("Expected to find 1 but found (0006): " + str (value))
        return False

    return True

def test_12_a_closed (conn, queue):
    queue.push (3)

def test_12_a ():
    # create a context
    ctx = vortex.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    iterator = 10
    while iterator > 0:
        # call to create a connection
        conn = vortex.Connection (ctx, host, port)

        # check connection status after if 
        if not conn.is_ok ():
            error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
            return False

        # create a queue
        queue = vortex.AsyncQueue ()

        # configure on close 
        conn.set_on_close (test_12_a_closed, queue)

        # start a channel that will be closed by listener
        channel = conn.open_channel (0, REGRESSION_URI_START_CLOSE)
        if channel:
            error ("Expected to find channel error creation, but found proper reference")
            return False

        # check value from queue 
        value = queue.pop ()
        if value != 3:
            error ("Expected to find 3 but found" + str (value))
            return False

        # reduce iterator
        iterator -= 1
    return True

def test_12_b_closed (conn, queue):
    queue.push (3)

def test_12_b ():
    # create a context
    ctx = vortex.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    iterator = 3
    while iterator > 0:
        # call to create a connection
        info ("registering connection to be closed...")
        conn = vortex.Connection (ctx, host, port)

        # check connection status after if 
        if not conn.is_ok ():
            error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
            return False

        # create a queue
        queue = vortex.AsyncQueue ()

        # configure on close 
        conn.set_on_close (test_12_b_closed, queue)

        # start a channel to notify the connection to shutdown on next start
        channel = conn.open_channel (0, REGRESSION_URI_RECORD_CONN)
        if not channel:
            error ("(1) Expected proper channel creation..")
            return False

        # ok, now create a second different content and start a
        # channel that will fail and will also close previous
        # connection
        info ("creating second connection...")
        conn2 = vortex.Connection (ctx, host, port)
        if not conn2.is_ok ():
            error ("Expected proper second connection creation..")
            return False

        # start a channel that will be closed by listener
        info ("opening second channel......")
        channel = conn2.open_channel (0, REGRESSION_URI_CLOSE_RECORDED_CONN)
        if channel:
            error ("Expected to find channel error creation, but found proper reference")
            return False

        # check value from queue
        info ("test 12-b: checking value from the queue..")
        value = queue.pop ()
        if value != 3:
            error ("Expected to find 3 but found" + str (value))
            return False

        # reduce iterator
        iterator -= 1

    return True

def test_12_c_conn_closed (conn, queue):
    info ("Received connection close, pushing reference to main thread")
    queue.push (conn)
    return

def test_12_c_on_channel (number, channel, conn, data):
    info ("Received expected channel start failure..")
    return

def test_12_c_create_conn (ctx, queue):
    conn = vortex.Connection (ctx, host, port)
    if not conn.is_ok ():
        error ("Expected proper connection created..")
        return False

    # set connection close
    conn.set_on_close (test_12_c_conn_closed, queue)

    # now create a channel
    conn.open_channel (0, REGRESSION_URI_START_CLOSE, on_channel=test_12_c_on_channel)
    info ("Finished connection and channel start requests..")
    return

def test_12_c ():
    # create a context
    ctx = vortex.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    queue = vortex.AsyncQueue ()
    
    # now create a connection inside a function that finishes
    test_12_c_create_conn (ctx, queue)

    info ("receiving connection from connection close..")
    conn = queue.pop ()

    info ("Waiting two seconds..")
    time.sleep (2)

    # check internal references
    if conn.id == -1:
        error ("Error, expected to find valid connection id identifier")
        return False

    info ("Ok, received connection reference with id: " + str (conn.id))
    return True

def test_12_d_on_close (conn, queue):
    info ("Received connection close with id: " + str (conn.id))
    queue.push (conn)
    return
    

def test_12_d_frame_received (conn, channel, frame, queue):
    # ok, set on close handler
    info ("Received frame received, setting on close notification..")
    conn.set_on_close (test_12_d_on_close, queue)
    return

def test_12_d ():
    # create a context
    ctx = vortex.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    # create a listener
    listener = vortex.create_listener (ctx, "127.0.0.1", "0")
    if not listener.is_ok ():
        error ("Expected to find proper listener creation but found a failure..")
        return False

    queue = vortex.AsyncQueue ()
    vortex.register_profile (ctx, "urn:beep:aspl.es:profiles:test_12_d",
                             frame_received=test_12_d_frame_received,
                             frame_received_data=queue)

    # ok, now create a connection to this listener
    conn = vortex.Connection (ctx, "127.0.0.1", listener.port)
    if not conn.is_ok ():
        error ("Expected proper connection create but failure found..")
        return False

    # ok, now create a channel and send a message
    channel = conn.open_channel (0, "urn:beep:aspl.es:profiles:test_12_d")
    if not channel:
        error ("Expected proper channel creation..")
        return False

    # send a message to record the channel
    channel.send_msg ("this is a test", 14)

    info ("Waiting 2 seconds to close connection..")
    time.sleep (1);
    conn.shutdown ()

    info ("Waiting connection from queue")
    conn2 = queue.pop ()

    if conn2.id == -1 or conn.id == -1:
        error ("Expected to connection id values different from -1 but found: " + str (conn.id) + " != " + str (conn2.id))
        return False

    info ("connection matches..")
    return True

def test_12_failing (queue, data):
    import sys
    error ("ERROR: connection close handler should not be received")
    sys.exit (-1)

def test_12_e ():
    # create a context
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

    # set on close connection
    close_id = conn.set_on_close (test_12_failing)

    # now rmeove connection close
    if not conn.remove_on_close (close_id):
        error ("Expected proper status (True) after removing on close handler..")
        return False

    info ("removed on close handler..")

    # close connection
    conn.shutdown ()

    info ("connection shutted down, waiting to close connection..")

    # waiting to trigger failure..
    queue = vortex.AsyncQueue ()
    queue.timedpop (200000)

    return True
    

def test_13():
    # create a context
    ctx = vortex.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    iterator = 0
    while iterator < 4:
        # create a listener
        info ("Test 13: creating listener iterator=%d" % iterator)
        listener = vortex.create_listener (ctx, "0.0.0.0", "0")

        # check listener status
        if not listener.is_ok ():
            error ("Expected to find proper listener creation, but found error: " + listener.error_msg)
            return False

        # create another listener reusing the port
        listener2 = vortex.create_listener (ctx, "0.0.0.0", listener.port)

        if listener2.is_ok ():
            error ("Expected to find failure while creating a second listener reusing a port: " + listener2.error_msg)
            return False

        # check the role even knowning it is not working
        info ("Test 13: checking role for listener, iterator=%d" % iterator)
        if listener.role != "master-listener":
            error ("Expected to find master-listener role but found: " + listener2.role)
            return False

        # close listener2
        listener2.close ()

        # check listener status
        if not listener.is_ok ():
            error ("Expected to find proper listener creation, but found error: " + listener.error_msg)
            return False

        # close the listener
        listener.close ()

        iterator += 1

    return True

def test_14():
    # create a context
    ctx = vortex.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    # connect
    conn = vortex.Connection (ctx, host, port)

    # check connection status after if 
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    # now authenticate connection
    if not vortex.sasl.init (ctx):
        error ("Expected to find proper authentication initialization, but found an error")
        return False

    # do an auth opeation using plain profile
    (status, message) = vortex.sasl.start_auth (conn=conn, profile="plain", auth_id="bob", password="secret")

    # check for VortexOk status 
    if status != 2: 
        error ("Expected to find VortexOk status code, but found: " + str (status) + ", error message was: " + message)
        return False
        
    # check authentication status
    if not vortex.sasl.is_authenticated (conn):
        error ("Expected to find is authenticated status but found un-authenticated connection")
        return False

    if "http://iana.org/beep/SASL/PLAIN" != vortex.sasl.method_used (conn):
        error ("Expected to find method used: http://iana.org/beep/SASL/PLAIN, but found: " + vortex.sasl.method_used (conn))
        return False

    # check auth id 
    if vortex.sasl.auth_id (conn) != "bob":
        error ("Expected to find auth id bob but found: " + vortex.sasl.auth_id (conn))
        return False

    # close connection
    conn.close ()
    
    # do a SASL PLAIN try with wrong crendetials
    conn = vortex.Connection (ctx, host, port)

    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    # do an auth opeation using plain profile
    (status, message) = vortex.sasl.start_auth (conn=conn, profile="plain", auth_id="bob", password="secret1")

    if status != 1:
        error ("Expected to find status 1 but found: " + str (status))
        
    # check authentication status
    if vortex.sasl.is_authenticated (conn):
        error ("Expected to not find is authenticated status but found un-authenticated connection")
        return False

    if vortex.sasl.method_used (conn):
        error ("Expected to find none method used but found: " + vortex.sasl.method_used (conn))
        return False

    # check auth id 
    if vortex.sasl.auth_id (conn):
        error ("Expected to find none auth id but found something defined: " + vortex.sasl.auth_id (conn))
        return False

    return True

def test_15():
    # create a context
    ctx = vortex.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    # connect
    conn = vortex.Connection (ctx, host, port)

    # check connection status after if 
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    # now authenticate connection
    if not vortex.sasl.init (ctx):
        error ("Expected to find proper authentication initialization, but found an error")
        return False

    # do an auth opeation using anonymous profile
    (status, message) = vortex.sasl.start_auth (conn=conn, profile="anonymous", anonymous_token="test@aspl.es")

    # check for VortexOk status 
    if status != 2: 
        error ("Expected to find VortexOk status code, but found: " + str (status) + ", error message was: " + message)
        return False
        
    # check authentication status
    if not vortex.sasl.is_authenticated (conn):
        error ("Expected to find is authenticated status but found un-authenticated connection")
        return False

    if vortex.sasl.ANONYMOUS != vortex.sasl.method_used (conn):
        error ("Expected to find method used: http://iana.org/beep/SASL/ANONYMOUS, but found: " + vortex.sasl.method_used (conn))
        return False

    # close connection
    conn.close ()
    
    # do a SASL ANONYMOUS try with wrong crendetials
    conn = vortex.Connection (ctx, host, port)

    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    # do an auth opeation using anonymous profile
    (status, message) = vortex.sasl.start_auth (conn=conn, profile="anonymous", anonymous_token="wrong@aspl.es")

    if status != 1:
        error ("Expected to find status 1 but found: " + str (status))
        
    # check authentication status
    if vortex.sasl.is_authenticated (conn):
        error ("Expected to not find is authenticated status but found un-authenticated connection")
        return False

    if vortex.sasl.method_used (conn):
        error ("Expected to find none method used but found: " + vortex.sasl.method_used (conn))
        return False
    

    return True

def test_16():
    # create a context
    ctx = vortex.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    # connect
    conn = vortex.Connection (ctx, host, port)

    # check connection status after if 
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    # now authenticate connection
    if not vortex.sasl.init (ctx):
        error ("Expected to find proper authentication initialization, but found an error")
        return False

    # do an auth opeation using DIGEST-MD5 profile
    (status, message) = vortex.sasl.start_auth (conn=conn, profile="digest-md5", auth_id="bob", password="secret", realm="aspl.es")

    # check for VortexOk status 
    if status != 2: 
        error ("Expected to find VortexOk status code, but found: " + str (status) + ", error message was: " + message)
        return False
        
    # check authentication status
    if not vortex.sasl.is_authenticated (conn):
        error ("Expected to find is authenticated status but found un-authenticated connection")
        return False

    if vortex.sasl.DIGEST_MD5 != vortex.sasl.method_used (conn):
        error ("Expected to find method used: " + vortex.sasl.DIGEST_MD5 + ", but found: " + vortex.sasl.method_used (conn))
        return False

    # check auth id 
    if vortex.sasl.auth_id (conn) != "bob":
        error ("Expected to find auth id bob but found: " + vortex.sasl.auth_id (conn))
        return False

    # close connection
    conn.close ()
    
    # do a SASL DIGEST-MD5 try with wrong crendetials
    conn = vortex.Connection (ctx, host, port)

    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    # do an auth opeation using DIGEST-MD5 profile
    (status, message) = vortex.sasl.start_auth (conn=conn, profile="digest-md5", auth_id="bob", password="secret1")

    if status != 1:
        error ("Expected to find status 1 but found: " + str (status))
        
    # check authentication status
    if vortex.sasl.is_authenticated (conn):
        error ("Expected to not find is authenticated status but found un-authenticated connection")
        return False

    if vortex.sasl.method_used (conn):
        error ("Expected to find none method used but found: " + vortex.sasl.method_used (conn))
        return False

    # check auth id 
    if vortex.sasl.auth_id (conn):
        error ("Expected to find none auth id but found something defined: " + vortex.sasl.auth_id (conn))
        return False
    

    return True

def test_17():
    # create a context
    ctx = vortex.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    # connect
    conn = vortex.Connection (ctx, host, port)

    # check connection status after if 
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    # now authenticate connection
    if not vortex.sasl.init (ctx):
        error ("Expected to find proper authentication initialization, but found an error")
        return False

    # do an auth opeation using CRAM-MD5 profile
    (status, message) = vortex.sasl.start_auth (conn=conn, profile="cram-md5", auth_id="bob", password="secret")

    # check for VortexOk status 
    if status != 2: 
        error ("Expected to find VortexOk status code, but found: " + str (status) + ", error message was: " + message)
        return False
        
    # check authentication status
    if not vortex.sasl.is_authenticated (conn):
        error ("Expected to find is authenticated status but found un-authenticated connection")
        return False

    if vortex.sasl.CRAM_MD5 != vortex.sasl.method_used (conn):
        error ("Expected to find method used: " + vortex.sasl.CRAM_MD5 + ", but found: " + vortex.sasl.method_used (conn))
        return False

    # check auth id 
    if vortex.sasl.auth_id (conn) != "bob":
        error ("Expected to find auth id bob but found: " + vortex.sasl.auth_id (conn))
        return False

    # close connection
    conn.close ()
    
    # do a SASL CRAM-MD5 try with wrong crendetials
    conn = vortex.Connection (ctx, host, port)

    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    # do an auth opeation using CRAM-MD5 profile
    (status, message) = vortex.sasl.start_auth (conn=conn, profile="cram-md5", auth_id="bob", password="secret1")

    if status != 1:
        error ("Expected to find status 1 but found: " + str (status))
        
    # check authentication status
    if vortex.sasl.is_authenticated (conn):
        error ("Expected to not find is authenticated status but found un-authenticated connection")
        return False

    if vortex.sasl.method_used (conn):
        error ("Expected to find none method used but found: " + vortex.sasl.method_used (conn))
        return False

    # check auth id 
    if vortex.sasl.auth_id (conn):
        error ("Expected to find none auth id but found something defined: " + vortex.sasl.auth_id (conn))
        return False

    return True

def test_18_common (conn):
    # now create a channel and send content 
    channel  = conn.open_channel (0, REGRESSION_URI)

    # flag the channel to do deliveries in a serial form
    channel.set_serialize = True

    # configure frame received
    queue    = vortex.AsyncQueue ()
    channel.set_frame_received (vortex.queue_reply, queue)

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

    info ("receiving replies..")

    # now receive and process all messages
    iterator = 0
    while iterator < 100:
        # build message to check
        message = ";; This buffer is for notes you don't want to save, and for Lisp evaluation.\n\
;; If you want to create a file, visit that file with C-x C-f,\n\
;; then enter the text in that file's own buffer: message num: " + str (iterator)

        # now get a frame
        frame = channel.get_reply (queue)

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

    return True

def test_18():
    # create a context
    ctx = vortex.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    # now enable tls support on the connection
    if not vortex.tls.init (ctx):
        error ("Expected to find proper authentication initialization, but found an error")
        return False

    # connect
    conn = vortex.Connection (ctx, host, port)

    # check connection status after if 
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    # enable TLS on the connection 
    (conn, status, status_msg) = vortex.tls.start_tls (conn)

    # check connection after tls activation
    if not conn.is_ok ():
        error ("Expected to find proper connection status after TLS activation..")
        return False

    # check status 
    if status != vortex.status_OK:
        error ("Expected to find status code : " + str (vortex.status_OK) + ", but found: " + str (status))

    info ("TLS session activated, sending content..")
    if not test_18_common (conn):
        return False

    return True

def test_19_notify (conn, status, status_msg, queue):
    # push a tuple
    queue.push ((conn, status, status_msg))
    return

def test_19():
    # create a context
    ctx = vortex.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    # now enable tls support on the connection
    if not vortex.tls.init (ctx):
        error ("Expected to find proper authentication initialization, but found an error")
        return False

    # connect
    conn = vortex.Connection (ctx, host, port)

    # check connection status after if 
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    # enable TLS on the connection using async notification
    queue = vortex.AsyncQueue ()
    if vortex.tls.start_tls (conn, tls_notify=test_19_notify, tls_notify_data=queue):
        error ("Expected to receive None after async tls activation, but something different was found")
        return False

    # wait for the connection
    (conn, status, statu_msg) = queue.pop ()

    # check connection after tls activation
    if not conn.is_ok ():
        error ("Expected to find proper connection status after TLS activation..")
        return False

    # check status 
    if status != vortex.status_OK:
        error ("Expected to find status code : " + str (vortex.status_OK) + ", but found: " + str (status))

    info ("TLS session activated, sending content..")
    if not test_18_common (conn):
        return False

    return True

def test_20_notify(conn, status, status_msg, queue):
    # push status
    queue.push ((status, status_msg))

def test_20():
    # create a context
    ctx = vortex.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    # connect
    conn = vortex.Connection (ctx, host, port)

    # check connection status after if 
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    # now authenticate connection
    if not vortex.sasl.init (ctx):
        error ("Expected to find proper authentication initialization, but found an error")
        return False

    # do an auth opeation using plain profile
    queue = vortex.AsyncQueue ()
    if vortex.sasl.start_auth (conn=conn, profile="plain", auth_id="bob", password="secret", auth_notify=test_20_notify, auth_notify_data=queue):
        error ("Expected to find none result but found something different..")

    # wait for reply
    (status, status_msg) = queue.pop ()

    # check for VortexOk status 
    if status != 2: 
        error ("Expected to find VortexOk status code, but found: " + str (status) + ", error message was: " + message)
        return False
        
    # check authentication status
    if not vortex.sasl.is_authenticated (conn):
        error ("Expected to find is authenticated status but found un-authenticated connection")
        return False

    if "http://iana.org/beep/SASL/PLAIN" != vortex.sasl.method_used (conn):
        error ("Expected to find method used: http://iana.org/beep/SASL/PLAIN, but found: " + vortex.sasl.method_used (conn))
        return False

    # check auth id 
    if vortex.sasl.auth_id (conn) != "bob":
        error ("Expected to find auth id bob but found: " + vortex.sasl.auth_id (conn))
        return False

    # close connection
    conn.close ()
    
    return True

def test_21_create_channel (conn, channel_num, profile, received, received_data, close, close_data, user_data, next_data):
    info ("Called to create channel with profile: " + profile)
    channel = conn.open_channel (channel_num, profile)
    return channel

def test_21():
    # create a context
    ctx = vortex.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    # connect
    conn = vortex.Connection (ctx, host, port)

    # create channel pool
    pool = conn.channel_pool_new (REGRESSION_URI, 1)

    # check number of channels in the pool
    if pool.channel_count != 1:
        error ("Expected to find channel count equal to 1 but found: " + str (pool.channel_count))
        return False

    # check channel pool id
    if pool.id != 1:
        error ("Expected to find channel pool id equal to 1 but found: " + str (pool.id))
        return False

    info ("Checking to acquire and release channel..")
    iterator = 0
    while iterator < 10:
        # get a channel from the pool
        channel = pool.next_ready ()

        if not channel:
            error ("Expected to find a channel reference available in the pool..but not found")
            return False

        if channel.number != 3:
            error ("Expected to find channel number 3 but found: " + str (channel.number))
            return False

        # check number of channels that are available at this moment
        if pool.channel_available != 0:
            error ("Expected to not find any channel available but found: " + str (pool.channel_available))
            return False

        # ok, now release channel
        pool.release (channel)

        # check number of channels that are available at this moment
        if pool.channel_available != 1:
            error ("Expected to find 1 channel available but found: " + str (pool.channel_available))
            return False

        # next position
        iterator += 1

    info ("Checking to acquire and release channel through conn.pool() method")

    # get a channel from the default pool
    channel = conn.pool().next_ready ()

    if not channel:
        error ("Expected to find a channel reference available in the pool..but not found")
        return False

    if channel.number != 3:
        error ("Expected to find channel number 3 but found: " + str (channel.number))
        return False

    # check number of channels that are available at this moment
    if conn.pool().channel_available != 0:
        error ("Expected to not find any channel available but found: " + str (conn.pool().channel_available))
        return False

    # ok, now release channel
    conn.pool().release (channel)

    # check number of channels that are available at this moment
    if conn.pool().channel_available != 1:
        error ("Expected to find 1 channel available but found: " + str (conn.pool().channel_available))
        return False

    info ("Checking to acquire and release channel through conn.pool(1) method")

    # get a channel from a particular pool
    channel = conn.pool(1).next_ready ()

    if not channel:
        error ("Expected to find a channel reference available in the pool..but not found")
        return False

    if channel.number != 3:
        error ("Expected to find channel number 3 but found: " + str (channel.number))
        return False

    # check number of channels that are available at this moment
    if conn.pool(1).channel_available != 0:
        error ("Expected to not find any channel available but found: " + str (conn.pool(1).channel_available))
        return False

    # ok, now release channel
    conn.pool(1).release (channel)

    # check number of channels that are available at this moment
    if conn.pool(1).channel_available != 1:
        error ("Expected to find 1 channel available but found: " + str (conn.pool(1).channel_available))
        return False

    info ("Creating a new pool (using same variables)")

    # create channel pool
    pool = conn.channel_pool_new (REGRESSION_URI, 1,
                                  create_channel=test_21_create_channel, create_channel_data=17)
    
    # check number of channels in the pool
    if pool.channel_count != 1:
        error ("Expected to find channel count equal to 1 but found: " + str (pool.channel_count))
        return False

    # check channel pool id
    if pool.id != 2:
        error ("Expected to find channel pool id equal to 2 but found: " + str (pool.id))
        return False

    # get a channel from a particular pool
    channel = conn.pool(2).next_ready ()

    if not channel:
        error ("Expected to find a channel reference available in the pool..but not found")
        return False

    if channel.number != 5:
        error ("Expected to find channel number 5 but found: " + str (channel.number))
        return False

    # release channel
    conn.pool(2).release (channel)

    info ("Now checking to access to channels from first pool..");

    # get a channel from a particular pool
    channel = conn.pool(1).next_ready ()

    if not channel:
        error ("Expected to find a channel reference available in the pool..but not found")
        return False

    if channel.number != 3:
        error ("Expected to find channel number 3 but found: " + str (channel.number))
        return False

    # release channel
    conn.pool(1).release (channel)

    info ("Finished release channel from first pool")

    return True

def test_22_create_channel(conn, channel_num, profile, received, received_data, close, close_data, user_data, next_data):
    info ("Called to create channel with profile: " + profile + ", and channel num: " + str (channel_num))
    info ("User data received: " + str (user_data))
    info ("Next data received: " + str (next_data))

    # check beacon
    if user_data[0] != 20:
        error ("Expected to find create beacon equal to 20, but found: " + str (user_data[0]))
        return None

    # update beacon
    user_data[0] = 21
    
    return conn.open_channel (channel_num, profile)

def test_22_pool_created (pool, data):

    info ("Called pool on created: " + str (pool) + ", with id: " + str (pool.id))
    if pool.id != 1:
        error ("ON HANDLER: Expected to find pool id equal to 1 but found: " + str (pool.id))

    # now push the pool
    data.push (pool)
    info ("Pushed pool created")
    return

def test_22_received (conn, channel, frame, queue):
    # push frame received
    queue.push (frame)
    return

def test_22 ():
    # create a context
    ctx = vortex.Ctx ()

    # call to init ctx 
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    # connect
    conn = vortex.Connection (ctx, host, port)
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    # create channel pool
    info ("Creating channel pool..")
    close_beacon  = 10
    create_beacon = [20]
    queue         = vortex.AsyncQueue ()
    conn.channel_pool_new (REGRESSION_URI, 1,
                           create_channel=test_22_create_channel, create_channel_data=create_beacon,
                           received=test_22_received, received_data=queue,
                           on_created=test_22_pool_created, user_data=queue)
    info ("Getting channel pool reference..")
    pool = queue.pop ()
    info ("Received pool reference..")

    # check channel pool
    value = pool.id
    if value != 1:
        error ("Expected to find channel pool id equal to 1 but found: " + str (value))
        print pool
        print ("Id found: " + str (pool.id))
        return False

    # now check connection
    if pool.conn.id != conn.id:
        error ("Expected to find connection id: " + str (conn.id) + ", but found: " + str (pool.conn.id))
        return False

    info ("Checking rest of the API..")
    if create_beacon[0] != 21:
        error ("Expected to find value 21 but found: " + str (create_beacon[0]))
        return False

    # now check frame received
    channel = conn.pool().next_ready ()
    if not channel:
        error ("Expected to find channel reference but found None..");
        return False

    # send message
    channel.send_msg ("This is a test..", 16)
    info ("Getting reply result..")
    frame = queue.pop ()

    if not frame:
        error ("Expected to find frame reference but found None..")
        return False

    if frame.payload != "This is a test..":
        error ("Expected to find frame payload content: 'This is a test..' but found: " + frame.payload)
        return False

    return True

def test_23_execute (ctx, queue, count):

    count[0] += 1
    info ("Count updated (1): " + str (count[0]))

    if count[0] == 10:
        # unlock caller
        queue.push (1)

        # request to finish event
        return True
    
    return False

def test_23_execute_2 (ctx, queue, count):

    count[0] += 1
    info ("Count updated (2): " + str (count[0]))

    if count[0] == 4:
        # unlock caller
        queue.push (1)

        # request to finish event
        return True
    
    return False

def test_23_execute_3 (ctx, queue, count):

    count[0] += 1
    info ("Count updated (3): " + str (count[0]))

    if count[0] == 7:
        # unlock caller
        queue.push (1)

        # request to finish event
        return True
    
    return False

def test_23 ():

    # all to register events
    ctx = vortex.Ctx ()
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    # register event
    info ("Installing event..")
    queue = vortex.AsyncQueue ()
    count = [0]
    ctx.new_event (30000, test_23_execute, queue, count)

    # get value
    info ("Waiting for event to finish")
    queue.pop ()

    # register event
    info ("Installing event 6..")
    count = [0]
    ctx.new_event (30000, test_23_execute, queue, count)
    count2 = [0]
    ctx.new_event (230000, test_23_execute_2, queue, count2)
    count3 = [0]
    ctx.new_event (830000, test_23_execute_3, queue, count3)

    count4 = [0]
    ctx.new_event (1130000, test_23_execute, queue,   count4)
    count5 = [0]
    ctx.new_event (1230000, test_23_execute_2, queue, count5)
    count6 = [0]
    ctx.new_event (1830000, test_23_execute_3, queue, count6)

    # get value
    info ("Waiting for events to finish")
    iterator = 0
    while iterator < 6:
        info (" ...one finished")
        queue.pop ()

        # next iterator
        iterator += 1
    
    
    
    return True

def test_24_failure_handler (conn, check_period, unreply_count):
    # push connection id that failed
    conn.get_data ("test_24_queue").push (conn.id)
    return

def test_24 ():

    # all to register events
    ctx = vortex.Ctx ()
    if not ctx.init ():
        error ("Failed to init Vortex context")
        return False

    # call to create a connection
    conn = vortex.Connection (ctx, host, port)
    info ("Created connection id: " + str (conn.id))

    # check connection status after if 
    if not conn.is_ok ():
        error ("Expected to find proper connection result, but found error. Error code was: " + str(conn.status) + ", message: " + conn.error_msg)
        return False

    # print ref count
    info ("Connection ref count: %d" % sys.getrefcount(conn))

    # configure queue
    queue = vortex.AsyncQueue ()
    conn.set_data ("test_24_queue", queue)

    # ok, now enable alive 
    if not vortex.alive.enable_check (conn, 20000, 10, test_24_failure_handler):
        error ("Expect to find proper alive.enable_check but found failure..")
        return False

    # print ref count
    info ("Connection ref count: %d" % sys.getrefcount(conn))

    # block connection for a period
    conn.block ()

    # print ref count
    info ("Connection ref count: %d" % sys.getrefcount(conn))

    # check that the connection is blocked
    if not conn.is_blocked ():
        error ("Expected to find blocked connection but different status..")
        return False

    # print ref count
    info ("Connection ref count: %d" % sys.getrefcount(conn))

    # wait until failure happens
    result = queue.pop ()
    info ("Received connection closed id: " + str (result))
    if result != conn.id:
        error ("Expected to find connection id: " + str (conn.id) + ", but found: " + str (result))

    info ("received connection close..")
    queue.timedpop (200000)

    # print ref count
    info ("Connection ref count: %d" % sys.getrefcount(conn))

    info ("Finshed test..")

    # alive check ok
    return True

def test_25 ():

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
    channel = conn.open_channel (0, REGRESSION_URI)

    if not channel:
        error ("Expected to find proper channel creation, but error found:")
        # get first message
        err = conn.pop_channel_error ()
        while err:
            error ("Found error message: " + str (err[0]) + ": " + err[1])

            # next message
            err = conn.pop_channel_error ()
        return False

    # configure frame received handler 
    queue = vortex.AsyncQueue ()
    channel.set_frame_received (vortex.queue_reply, queue)

    # send a message to test */
    channel.send_msg ("Camión", -1)

    # wait for the reply
    frame = channel.get_reply (queue)

    # check result
    if frame.payload != "Camión":
        error ("Expected to find content: Camión but found: " + frame.payload)
        return False

    # send utf-8 content ok
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
        info ("TEST-" + str(test_count) + ": Running " + test[1])
        
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
   (test_00_a, "Check PyVortex async queue wrapper"),
   (test_01,   "Check PyVortex context initialization"),
   (test_02,   "Check PyVortex basic BEEP connection"),
#  (test_02a,   "Check PyVortex log handler configuration"),
   (test_03,   "Check PyVortex basic BEEP connection (shutdown)"),
   (test_03_a, "Check PyVortex connection set data"),
   (test_04,   "Check PyVortex basic BEEP channel creation"),
   (test_05,   "Check BEEP basic data exchange"),
   (test_06,   "Check BEEP check several send operations (serialize)"),
   (test_07,   "Check BEEP check several send operations (one send, one receive)"),
   (test_08,   "Check BEEP transfer zeroed binaries frames"),
   (test_09,   "Check BEEP channel support"),
   (test_10,   "Check BEEP channel creation deny"),
   (test_10_a, "Check BEEP channel creation deny (a)"),
   (test_10_b, "Check reference counting on async notifications"),
   (test_10_c, "Check async channel start notification"),
   (test_10_d, "Check async channel start notification (failure expected)"),
   (test_10_e, "Check channel creation inside a function with frame received"),
   (test_10_f, "Check connection close after sending message"),
   (test_11,   "Check BEEP listener support"),
   (test_12,   "Check connection on close notification"),
   (test_12_a, "Check connection on close notification (during channel start)"),
   (test_12_b, "Check channel start during connection close notify"),
   (test_12_c, "Check close notification for conn refs not owned by caller"),
   (test_12_d, "Check close notification for conn refs at listener"),
   (test_12_e, "Check removing close notification"),
   (test_13,   "Check wrong listener allocation"),
   (test_14,   "Check SASL PLAIN support"),
   (test_15,   "Check SASL ANONYMOUS support"),
   (test_16,   "Check SASL DIGEST-MD5 support"),
   (test_17,   "Check SASL CRAM-MD5 support"),
   (test_18,   "Check TLS support"),
   (test_19,   "Check TLS support (async notification)"),
   (test_20,   "Check SASL PLAIN support (async notification)"),
   (test_21,   "Check channel pool support"),
   (test_22,   "Check channel pool support (handlers)"),
   (test_23,   "Check event tasks"),
   (test_24,   "Check alive implementation"),
   (test_25,   "Check sending utf-8 content")
]

# declare default host and port
host     = "localhost"
port     = "44010"

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




