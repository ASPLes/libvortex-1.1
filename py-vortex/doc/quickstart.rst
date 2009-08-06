PyVortex quick start
====================

The following is a quick start, step by step explaining how to create
a python BEEP client and a listener that exchanges some data. Don't
worry if you are not familiar with BEEP. 

===============================
Creating a PyVortex BEEP client
===============================

1. First, we have to create a Vortex context. This is done like
follows::

   # import base vortex module
   import vortex

   # import sys (sys.exit)
   import sys

   # create a vortex.Ctx object 
   ctx = vortex.Ctx ()

   # now, init the context and check status
   if not ctx.init ():
      print ("ERROR: Unable to init ctx, failed to continue")
      sys.exit (-1)

   # ctx created

2. Now, we have to establish a BEEP session with the remote
listener. This is done like follows::

   # create a BEEP session connected to localhost:1602
   conn = vortex.Connection (ctx, "localhost", "1602")

   # check connection status
   if not conn.is_ok ():
      print ("ERROR: Failed to create connection with 'localhost':1602")	
      sys.exit (-1)

3. Once we have a BEEP session opened, we need to open a channel to do
"the useful work". In BEEP, a session may have several channels opened
and each channel may run a particular protocol (message style,
semantic, etc). This is called a profile.

So, to create a channel for our test, we will do as follows::

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
       return False

   # channel created

4. Now we can send and receive messages using the channel
created. Here is where the profile developer design the kind of
messages to exchange, format, etc. To send a message we do as follows::

   if not channel.send_msg ("This is a test", 14):
       print ("ERROR: Failed to send test message, error found: ")
      
       # get first message
       err = conn.pop_channel_error ()
       while error:
            print ("ERROR: Found error message: " + str (err[0]) + ": " + err[1])

            # next message
            err = conn.pop_channel_error ()
       return False

5. Now, to receive replies and other requests, we have to configure a
frame received handler::

   def frame_received (conn, channel, frame, data):
       # messages received asynchronously
       print ("Frame received, type: " + frame.type)
       print ("Frame content: " + frame.payload)
       return
   
   channel.set_frame_received (frame_received)

.. note::

   Full client source code can be found at: https://dolphin.aspl.es/svn/python-vortex/doc/simple-client.py

========================
Creating a BEEP listener
========================

.. note:: 

   If you are creating a BEEP python listener you may find useful to
   use mod-python provided by Turbulence
   (http://www.turbulence.ws). It provides the same python interface
   but all administration support already done (port configuration,
   profile security, SASL users and more..)

1. The process of creating a BEEP listener is pretty
straitforward. You have to follow the same initialization process as
the client. Then, you have to register profiles that will be supported
by your listener. This is done as follows::

   def default_frame_received (conn, channel, frame, data):

       print ("Frame content received: " + frame.payload)

       # reply in the case of MSG received
       if frame.type == 'MSG':
       	  # reply doing an echo
       	  channel.send_rpy (frame.payload, frame.payload_size, frame.msg_no)

       return
       # end default_frame_received 		   		   		       

   # register support for a profile
   vortex.register_profile (ctx, "urn:aspl.es:beep:profiles:my-profile",
   			    frame_received=default_frame_received)

2. After your listener signals its support for a particular profile,
it is required to create a listener instance::

   # start listener and check status
   listener = vortex.create_listener (ctx, "0.0.0.0", "1602")
   
   if not listener.is_ok ():
      print ("ERROR: failed to start listener, error was was: " + listener.error_msg)
      sys.exit (-1)

3. Because we have to wait for frames to be received we need a wait to
block the listener. The following is not strictly necessary it you
have another way to make the main thread to not finish::

   # wait for requests
   vortex.wait_listeners (ctx, unlock_on_signal=True)
   

.. note::

   Full listener source code can be found at: https://dolphin.aspl.es/svn/python-vortex/doc/simple-listener.py 

