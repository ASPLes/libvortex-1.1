:mod:`vortex.channel` --- LuaVortexchannel class: BEEP channel creation and management
======================================================================================

.. currentmodule:: vortex


=====
Intro
=====

API documentation for vortex.channel object representing a BEEP
channel.

==========
Module API
==========

.. class:: channel

   .. method:: send_msg (content, [size])
   
      Allows to send a message (content) with the size provided.

      :param content: The content to send.
      :type  content: String (may contain binary data like \0)

      :param size: Optional content size indication or -1 to let the function to deduce string size. 
      :type  size: Integer 

      :rtype: Returns the msg_no used for the send operation or nil if the send operation fails. 

      Note the proper manner to detect if the send operation was done
      is to use::

      	   msg_no = channel:send_msg (content, size)
	   if msg_no == nil then
      	      print ("ERROR: failed to send message")
	      -- return or appropriate error recover
           end if

   .. method:: send_rpy (content, size, msg_no)
   
      Allows to send a reply message (frame type RPY) to a message received (frame type MSG) with the provided msg_no.

      :param content: The content to send.
      :type  content: String (may contain binary data like \0)

      :param size: The content size or -1 to let the function to deduce string size. 
      :type  size: Integer

      :param msg_no: The frame msgno that identifies the frame  MSG we are replying
      :type  msg_no: Integer > 0

      :rtype: Returns true if the reply operation was done, otherwise false is returned. 

   .. method:: send_err (content, size, msg_no)
   
      Allows to send an error reply message (frame type ERR) to a message received (frame type MSG) with the provided msg_no.

      :param content: The content to send.
      :type  content: String (may contain binary data like \0)

      :param size: The content size or -1 to let the function to deduce string size. 
      :type  size: Integer 

      :param msg_no: The frame msgno that identifies the frame  MSG we are replying
      :type  msg_no: Integer > 0

      :rtype: Returns true if the reply operation was done, otherwise false is returned. 

   .. method:: send_ans (content, size, msg_no)
   
      Allows to send a reply message (frame type ANS) to a message received (frame type MSG) with the provided msg_no.

      :param content: The content to send.
      :type  content: String (may contain binary data like \0)

      :param size: The content size or -1 to let the function to deduce string size. 
      :type  size: Integer 

      :param msg_no: The frame msgno that identifies the frame  MSG we are replying
      :type  msg_no: Integer > 0

      :rtype: Returns true if the reply operation was done, otherwise false is returned. 

   .. method:: finalize_ans (msg_no)
   
      Finish an ANS exchange with the last NUL frame (created by this method). 

      :param msg_no: The frame msgno that identifies the frame  MSG we are replying with the last NUL.
      :type  msg_no: Integer > 0

      :rtype: Returns true if the reply operation was done, otherwise false is returned. 

   .. method:: set_frame_received (handler, data)
   
      Allows to configure the frame received handler (the method or function that will be called for each frame received over this channel). The frame handler must have the following signature::
      
          function frame_received (conn, channel, frame, data)
              -- handle frame received
	      return
          end

      :param handler: The handler to configure
      :type  handler:  :ref:`frame-received-handler`.

      :param data: User defined data passed to the frame received handler.
      :type  data: Object

   .. method:: close ()
   
      Allows to request close the channel. The function will issue a close request that must be accepted by remote BEEP peer. 

      :rtype: Returns true if the channel was closed, otherwise false is returned. In the case false is returned you can use :meth:`connection.pop_channel_error`.

   .. method:: check (channel)

      :param channel: The channel.
      :type  channel: vortex.channel

      Allows to check if the provided :mod:`vortex.channel` is indeed a :mod:`vortex.channel` object.
   
      :rtype: Returns true in the case it is a :mod:`vortex.channel` object. Otherwise, false is returned.

   .. attribute:: number

      (Read only attribute) (Number) returns the channel number.

   .. attribute:: profile

      (Read only attribute) (String) returns the channel profile.

   .. attribute:: is_ready

      (Read only attribute) (true/false) returns is ready status. See vortex_channel_is_ready for more information.

   .. attribute:: conn

      (Read only attribute) (vortex.connection) returns a reference to the connection where the channel is working.

   .. attribute:: set_serialize

      (Write only attribute) (true/false) Allows to configure channel delivery serialization. See also vortex_channel_set_serialize.


 
      
    
