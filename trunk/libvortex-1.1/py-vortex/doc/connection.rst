:mod:`vortex` --- PyVortexConnection class: BEEP session creation and management
================================================================================

.. currentmodule:: vortex


API documentation for vortex.Connection object representing a BEEP
session. 

==========
Module API
==========

.. class:: Connection

   .. method:: is_ok ()

      Allows to check if the current instance is properly connected  and available for operations.  This method is used to check  connection status after a failure.

      :rtype: True if the connection is ready, otherwise False is returned.

   .. method:: open_channel (number, profile, [frame_received], [frame_received_data])
   
      Allows to create a new BEEP channel under the provided connection. The method receives two mandatory arguments channel number and profile.       

      :param number: Channel number requested to created. Channel number can be 0 (which makes PyVortex to select the next available for you).
      :type  number: Integer (> 0)

      :param profile: The channel profile to run.
      :type  profile: String

      :param frame_received: The handler to be configured on this channel to handle frame reception. 
      :type  frame_received: :ref:`frame-received-handler`.

      The following is an code example of a frame_received handler::

          def received_content (conn, channel, frame, data):
	      print ("Received frame, content: " + frame.payload)
	      return

      :param frame_received_data: User defined data to be passed to frame_received handler along with corresponding handler parameters.
      :type  frame_received_data: Object

      :rtype: Returns a new reference to vortex.Channel or None if the method fails. 

      In the case of function failure you can use the following code to check for channel failure::

          if not channel:
	      print ("ERROR: failed to start channel, error found:")
	      (err_code, err_msg) = conn.pop_channel_error ()
	      while (err_code):
	          print ("ERROR: error found: " + err_msg)

		  # next error found
		  (err_code, err_msg) = conn.pop_channel_error ()

   .. method:: pop_channel_error ()
   
      Allows to get the set of errors that happend on the connection instance. Each call to the method pops (and removes) the next error message tuple. Each tuple (Integer, String) contains an error code and an error string.

      :rtype: Returns the next tuple (Integer, String) or None if no error was found.

   .. method:: set_on_close (on_close, [on_close_data])
   
      Allows to configure a handler which will be called in the case the connection is closed. This is useful to detect broken pipe.

      :param on_close: The handler to be executed when the connection close is detected on the instance provided.
      :type  on_close: :ref:`on-close-handler`.

      :param on_close_data: The user defined data to be passed to the on_close handler along with handler corresponding parameters.
      :type  on_close_data: Object

   .. method:: find_by_uri (profile)
   
      Allows to get the list of channels available (opened) on the connection which are running the provided profile.

      :param profile: Profile unique id used to select channels
      :type  profile: String

      :rtype: Returns a list of channels running the provided profile.

   .. method:: close ()
   
      Allows to close the connection using full BEEP close negotation procotol. Keep in mind that using full BEEP close procedure may suffer from BNRA (see http://www.aspl.es/vortex/draft-brosnan-beep-limit-close.txt). It is recommended to use shutdown method.

      :rtype: Return True in the case the connection was close, otherwise False is returned (because some channel was denied to be closed by remote side).

   .. method:: shutdown ()
   
      Allows to close the connection by shutting down the transport layer supporting it. This causes the connection to be closed without taking place BEEP close negotiation. 

   .. attribute:: error_msg

      (Read only attribute) (String) returns the last error message found while using the connection.

   .. attribute:: status

      (Read only attribute) (Integer) returns an integer status code representing the textual error string returned by attribute error_msg.

   .. attribute:: host

      (Read only attribute) (String) returns the host string the connection is connected to.

   .. attribute::  port

      (Read only attribute) (String) returns the port string the connection is using to connected to.

   .. attribute:: local_addr

      (Read only attribute) (String) returns the local address used by the connection.

   .. attribute::  local_port

      (Read only attribute) (String) returns the local port string used by the connection.

   .. attribute:: num_channels

      (Read only attribute) (Integer) returns the number of channels opened on the connection.

   .. attribute:: role

      (Read only attribute) (String) returns a string representing the connection role. Allowed connection role strings are: initiator (client connection), listener (connection that was accepted due to a listener installed), master-listener (the master listener that is waiting for new incoming requests).  




		  
	  
