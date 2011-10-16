:mod:`vortex` --- LuaVortex base module: base functions (create listeners, register profiles)
=============================================================================================

.. module:: vortex
   :synopsis: Vortex library base module
.. moduleauthor:: Advanced Software Production Line, S.L.


This modules includes all functions required to establish a BEEP
session, create channels and develop BEEP profiles.

This module also includes type definition for the following classes:

.. toctree::
   :maxdepth: 1

   ctx
   connection
   channel
   frame
   asyncqueue
   handlers
   vortexsasl

==========
Module API
==========

.. function:: create_listener (ctx, host, port)

   Allows to create a BEEP listener to receiving incoming
   connections. Here is an example:: 

      # create a listener
      listener = vortex.create_listener (ctx, "0.0.0.0", "44010")

      # check listener started
      if not listener.is_ok ():
          # do some error handling
          sys.exit (-1)

      # do a wait operation
      vortex.wait_listeners (ctx, true)

   :param ctx: vortex context where the listener will be created
   :type ctx: vortex.Ctx
   
   :param host: the hostname
   :type host: String
  
   :param port: the port to connect to
   :type  port: String

   :rtype: vortex.Connection representing the listener created.

.. function:: wait_listeners (ctx, [unlock_on_signal])

   Allows to perform a wait operation until vortex context is finished
   or due to a signal received (when unlock_on_signal is set to true). 

   Note this call is not necessary in the case your application or host program has a wait to ensure the program do not finish. This method, like C function vortex_listener_wait, is just one method to ensure the calling thread (usually the main()) do not ends until we wait for requests. 

   This function is often use at the server side. To unlock manually call to :mod:`vortex.unlock_listeners`

   :param ctx: context where a listener or a set of listener were created.
   :type ctx: vortex.Ctx

   :param unlock_on_signal: unlock_on_signal expects to receive true to make wait_listener to unlock on signal received.
   :type unlock_on_signal: Integer: true or false

.. function:: unlock_listeners (ctx)

   Unlocks the :mod:`vortex.wait_listeners` caller (if any).

   :param ctx: context where a listener or a set of listener were created.
   :type ctx: vortex.Ctx

.. function:: register_profile (ctx = vortex.Ctx, uri, frame_received, [frame_received_data], [start], [start_data], [close], [close_data])

   Allows to register a supported profile so the current BEEP, inside
   the provided vortex.Ctx will accept incoming requests to open a
   channel running such profile.

   The function requires to mandatory parameters (ctx and uri). The
   rest of parameters are handlers used to handle different situations
   (start channel request, close channel request, and frame received).

   A usual register operation will look like this::

       vortex.register_profile (ctx, your_uri, frame_received_handler)

   Where frame_received_handler will look like::

       function frame_received_handler (conn, channel, frame, data) 
           -- frame recevied, look into frame.payload
	   return
       end

   In the case you want to provide a start handler, you need to also define the frame received handler. Here is how to setup start handler::

       vortex.register_profile (ctx, your_profile, 
			 -- frame received 
			 frame_received_handler, nil, 
			 -- start handler
			 start_handler, nil)

   :param ctx: context where the profile will be registered
   :type ctx: vortex.Ctx

   :param uri: Profile unique string identification
   :type uri: string

   :param start: User defined handler that will be used to manage incoming start channel requests. The handler must provide return true to accept the channel to be created or false to deny it.
   :type start: :ref:`channel-start-handler`

   :param start_data: User defined data that will notified along with corresponding data at start handler.
   :type  start_data: object

   :param close: User defined handler that will be used to manage incoming close channel requests. The handler must provide return true to accept the channel to be closed or false to deny it. 
   :type  close: :ref:`channel-close-handler`

   :param close_data: User defined data that will notified along with corresponding data at close channel handler.
   :type  close_data: object

   :param frame_received: User defined handler that will be used to manage frames received under channels running this profile. 
   :type  frame_received: :ref:`frame-received-handler`

   :param frame_received_data: User defined data that will notified along with corresponding data at frame received handler.
   :type  frame_received_data: object

.. function:: yield ([timeout])

   Because lua has no GIL (global interpreter lock) concept like python, the executing thread
   must yield the execution to let other threads to enter into the lua
   space. In python this is done automatically. In lua, the user must
   take care of this detail, especially for client side code (for
   server side programming, this is mostly not required).

   You can yield forever until other thread enters into lua space or
   just yield during a limited period (timeout is measured in
   microseconds: 1 second = 1000000 microseconds).

   Nothing happens if you call to yield several times a now event or
   thread is attended.

   Here is an example::

       -- send message 
       if channel:send_msg ("This is a test...", 17) == nil then
          print ("ERROR: expected to find proper send operation..")
          return false
       end

       -- now yield
       print ("Let other threads to enter..")
       vortex.yield (1000000)

       -- if we are here, we own the execution

   See also :mod:`vortex.wait_events` and :mod:`vortex.event_fd` to know more
   about methods that provides a notification about when it is
   required to yield.

   :param timeout: Optional timeout that control how long to wait
   :type  timeout: Number

.. function:: event_fd ()

   This function allows to get a file descriptor that can be checked
   for changes that indicates a call to :mod:`vortex.yield` is
   required (another thread is asking to acquire permission to
   execute). This descriptor can be used integrated into a polling
   method (like select() or similar). In the case changes are
   detected, the running thread must call to :mod:`vortex.yield` to
   yield execution.

   :rtype: File descriptor to watch for changes. Do not read or close descriptor returned.

.. function:: wait_events ([timeout])

   Implements a select () operation over the internal descriptor (that
   can be acquired by calling to :mod:`vortex.event_fd`), unlocking
   the caller when a change is detected. In such case, a call to
   :mod:`vortex.yield` is required to yield execution.

   This function exists mainly to provide a simple method to implement
   a "wait for events" method especially because socket.select method
   provided by lua socket module only accepts descriptors created by
   the socket module. 

   A typical usage example is::

      -- wait for events
      while vortex.wait_events () do 
           print ("Event detected, yielding execution....")
	   vortex.yield ()
      end

   NOTE: if you are integrating LuaVortex with another product like a
   gui or similar, this method is not recommended. Try to see if that
   toolkit can watch for changes on a particular descriptor, and put
   under supervision the descriptor returned by
   :mod:`vortex.event_fd`. Then you call to `vortex.yield`, every time
   a change is detected on that descriptor. 

   :param timeout: Optional timeout that control how long to wait
   :type  timeout: Number

