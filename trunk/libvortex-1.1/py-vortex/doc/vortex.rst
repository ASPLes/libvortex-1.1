:mod:`vortex` --- PyVortex base module: base functions (create listeners, register profiles)
============================================================================================

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
      vortex.wait_listeners (ctx, unlock_on_signal=True)

   :param ctx: vortex context where the listener will be created
   :type ctx: vortex.Ctx
   
   :param host: the hostname
   :type host: String
  
   :param port: the port to connect to
   :type  port: String

   :rtype: vortex.Connection representing the listener created.

.. function:: wait_listeners (ctx, [unlock_on_signal])

   Allows to perform a wait operation until vortex context is finished
   or due to a signal received. 

   :param ctx: context where a listener or a set of listener were created.
   :type ctx: vortex.Ctx

   :param unlock_on_signal: unlock_on_signal expects to receive True to make wait_listener to unlock on signal received.
   :type unlock_on_signal: Integer: True or False

.. function:: register_profile (ctx = vortex.Ctx, uri, [start], [start_data], [close], [close_data], [frame_received], [frame_received_data])

   Allows to register a supported profile so the current BEEP, inside
   the provided vortex.Ctx will accept incoming requests to open a
   channel running such profile.

   The function requires to mandatory parameters (ctx and uri). The
   rest of parameters are handlers used to handle different situations
   (start channel request, close channel request, and frame received).

   :param ctx: context where the profile will be registered
   :type ctx: vortex.Ctx

   :param uri: Profile unique string identification
   :type uri: string

   :param start: User defined handler that will be used to manage incoming start channel requests. The handler must provide return True to accept the channel to be created or False to deny it.
   :type start: :ref:`channel-start-handler`

   :param start_data: User defined data that will notified along with corresponding data at start handler.
   :type  start_data: object

   :param close: User defined handler that will be used to manage incoming close channel requests. The handler must provide return True to accept the channel to be closed or False to deny it. 
   :type  close: :ref:`channel-close-handler`

   :param close_data: User defined data that will notified along with corresponding data at close channel handler.
   :type  close_data: object

   :param frame_received: User defined handler that will be used to manage frames received under channels running this profile. 
   :type  frame_received: :ref:`frame-received-handler`

   :param frame_received_data: User defined data that will notified along with corresponding data at frame received handler.
   :type  frame_received_data: object


.. function:: queue_reply(conn, channel, frame, o)

   Function used inside the queue reply method. This function is used
   as frame received handler, queuring all frames in the queue
   provided as user data. The, a call to channel.get_reply (queue) is
   required to get all frames received.

   Here is an example::

   	# configure frame received handler 
	queue = vortex.AsyncQueue ()
	channel.set_frame_received (vortex.queue_reply, queue)

	# wait for frames to be received
	frame = channel.get_reply (queue)

