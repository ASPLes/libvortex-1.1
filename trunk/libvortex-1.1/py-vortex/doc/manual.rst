PyVortex manual
===============

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


========================================
Enabling server side SASL authentication
========================================

To enable server side SASL authentication, we activate the set of
mechanisms that will be used to implement auth operations and a handler
(or a set of handlers) that will be called to complete auth
operation. Some handlers must return True/False to accept/deny the
auth operation. Other SASL mechanisms must return the password
associated to a user. See documentation associated to each mechanish.

In all cases, vortex.sasl it is at the end a binding on top of Vortex
Library SASL implementation. See also its documentation.

1. First, you have to include vortex.sasl 
component::

   import vortex
   import vortex.sasl

2. Then, you have to enable which SASL mechanism to be used to
authenticate remote peer. For example, we can use "plain" mechanism as
follows. It is possible to have several mechanism available at the
same time, allowing remote peer to choose one::

   # activate support for SASL plain mechanism
   vortex.sasl.accept_mech (ctx, "plain", auth_handler)

3. After that, each time a request to activate an incoming connection
is handle using auth_handler provided. An example handling SASL plain
mechanism is the following::

   def auth_handler (conn, auth_props, user_data):

       if auth_props["mech"] == vortex.sasl.PLAIN:
       	  # only authenticate users with user bob and password secret
       	  if auth_props["auth_id"] == "bob" and auth_props["password"] == "secret":
	      return True

       # fail to authentcate connection
       return False

Previous auth handler example it's authenticating
statically. Obviously that could be replaced with appropriate database
access check to implement dynamic SASL auth.

