:mod:`vortex.Channel` --- PyVortexChannel class: BEEP channel creation and management
=====================================================================================

.. currentmodule:: vortex


=====
Intro
=====

API documentation for vortex.Channel object representing a BEEP
channel.

==========
Module API
==========

.. class:: Channel

   .. method:: send_msg (content, size)
   
      Allows to send a message (content) with the size provided.

      :param content: The content to send.
      :type  content: String (may contain binary data like \0)

      :param size: The content size
      :type  size: Integer > 0

      :rtype: Returns the msg_no used for the send operation or None if the send operation fails. 

      Note the proper manner to detect if the send operation was done
      is to use::

      	   msg_no = channel.send_msg (content, size)
	   if msg_no is None:
      	      print ("ERROR: failed to send message")
	      # return or appropriate error recover

   .. method:: send_rpy (content, size, msg_no)
   
      Allows to send a reply message (frame type RPY) to a message received (frame type MSG) with the provided msg_no.

      :param content: The content to send.
      :type  content: String (may contain binary data like \0)

      :param size: The content size
      :type  size: Integer > 0

      :param msg_no: The frame msgno that identifies the frame  MSG we are replying
      :type  msg_no: Integer > 0

      :rtype: Returns True if the reply operation was done, otherwise False is returned. 

   .. method:: send_err (content, size, msg_no)
   
      Allows to send an error reply message (frame type ERR) to a message received (frame type MSG) with the provided msg_no.

      :param content: The content to send.
      :type  content: String (may contain binary data like \0)

      :param size: The content size
      :type  size: Integer > 0

      :param msg_no: The frame msgno that identifies the frame  MSG we are replying
      :type  msg_no: Integer > 0

      :rtype: Returns True if the reply operation was done, otherwise False is returned. 

   .. method:: send_ans (content, size, msg_no)
   
      Allows to send a reply message (frame type ANS) to a message received (frame type MSG) with the provided msg_no.

      :param content: The content to send.
      :type  content: String (may contain binary data like \0)

      :param size: The content size
      :type  size: Integer > 0

      :param msg_no: The frame msgno that identifies the frame  MSG we are replying
      :type  msg_no: Integer > 0

      :rtype: Returns True if the reply operation was done, otherwise False is returned. 

   .. method:: finalize_ans (msg_no)
   
      Finish an ANS exchange with the last NUL frame (created by this method). 

      :param msg_no: The frame msgno that identifies the frame  MSG we are replying with the last NUL.
      :type  msg_no: Integer > 0

      :rtype: Returns True if the reply operation was done, otherwise False is returned. 

   .. method:: set_frame_received (handler, data)
   
      Allows to configure the frame received handler (the method or function that will be called for each frame received over this channel). The frame handler must have the following signature::
      
          def frame_received (conn, channel, frame, data):
              # handle frame received
	      return

      :param handler: The handler to configure
      :type  handler:  :ref:`frame-received-handler`.

      :param size: The content size
      :type  size: Integer > 0

      :param msg_no: The frame msgno that identifies the frame  MSG we are replying
      :type  msg_no: Integer > 0

      :rtype: Returns True if the reply operation was done, otherwise False is returned. 
 
      
   .. method:: get_reply (queue)
   
      This method is used as part of the queue reply method. It receives the queue configured along with vortex.queue_reply method as frame received. Calling to this method will block the caller until a frame is received.

      The following is an example of queue reply method::
      
           # configure frame received handler 
           queue = vortex.AsyncQueue ()
           channel.set_frame_received (vortex.queue_reply, queue)

           # wait for the reply
           while True:
               frame = channel.get_reply (queue)
               
               # frame content received
               print ("Received frame type: " + frame.type + ", content: " + frame.payload)

      :param handler: The queue that was activated with vortex.queue_reply (as :ref:`frame-received-handler`)
      :type  handler: vortex.AsyncQueue

      :rtype: Returns next frame received (vortex.Frame) or None if it fails

   .. method:: close ()
   
      Allows to request close the channel. The function will issue a close request that must be accepted by remote BEEP peer. 

      :rtype: Returns True if the channel was closed, otherwise False is returned. In the case false is returned you can use :meth:`Connection.pop_channel_error`.

   .. method:: incref ()
   
      Allows to increment python reference count.  This is used in cases where the channel reference is automatically collected by python GC but the VortexChannel reference that it was representing is still working (and may receive notifications, like frame received). Note that a call to this method, requires a call to :meth:`decref`.

   .. method:: decref ()
   
      Allows to decrement python reference count.  See :meth:`incref` for more information.

   .. attribute:: number

      (Read only attribute) (Number) returns the channel number.

   .. attribute:: profile

      (Read only attribute) (String) returns the channel profile.

   .. attribute:: is_ready

      (Read only attribute) (True/False) returns is ready status. See vortex_channel_is_ready for more information.

   .. attribute:: conn

      (Read only attribute) (vortex.Connection) returns a reference to the connection where the channel is working.

   .. attribute:: set_serialize

      (Write only attribute) (True/False) Allows to configure channel delivery serialization. See also vortex_channel_set_serialize.


 
      
    
