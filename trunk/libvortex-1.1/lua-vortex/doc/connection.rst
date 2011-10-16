:mod:`vortex.connection` --- LuaVortexConnection class: BEEP session creation and management
============================================================================================

.. currentmodule:: vortex

API documentation for vortex.connection object representing a BEEP
session. vortex.connection represents a single BEEP session which
holds a set of channels that are in charge of sending and receiving
useful user content, implement user authentication, and so on.

To create a connection, you need a context where to create it. See
vortex.Ctx documenation to know about it: :class:`vortex.Ctx`

A connection is created as follows::

   conn = vortex.connection.new (ctx, "localhost", "44010")
   if not conn:is_ok () then
      -- error handling here
      return false
   end

Note that after creating a connection, you must check if it is ok
using :meth:`vortex.connection.is_ok` method.

Once a connection is created, you have to create channels to actually
send and receive data (and do any other useful work). This is done
using :class:`vortex.channel`.


==========
Module API
==========

.. class:: connection 

   .. method:: new (ctx, host, port [, serverName])

      Creates a new connection to the provided host and port. uninitialized vortex.ctx object. 

      :param ctx: Vortex context where the connection will be created.
      :param type: vortex.ctx
   
      :param host: Host to connect to.
      :type host: String

      :param port: Port to connect to.
      :type port: String

      :param serverName: Optional serverName value to ask for this channel. This value will change automatic behaviour notifying the provided serverName on next channel start request. 
      :type serverName: String

   .. method:: is_ok (connection)

      Allows to check if the current instance is properly connected
      and available for operations.  This method is used to check
      connection status after a failure.

      :param connection: The connection to check.
      :type  connection: vortex.connection

      :rtype: true if the connection is ready, otherwise false is returned.

   .. method:: open_channel (connection, number, profile, [frame_received], [frame_received_data], [on_channel], [on_channel_data], [on_close], [on_close_data], [encoding], [profile_content])
   
      Allows to create a new BEEP channel under the provided
      connection. The method receives two mandatory arguments channel
      number and profile.

      :param connection: The connection.
      :type  connection: vortex.connection

      :param number: Channel number requested to created. Channel number can be 0 (which makes PyVortex to select the next available for you).
      :type  number: Integer (> 0)

      :param profile: The channel profile to run.
      :type  profile: String

      :param frame_received: The handler to be configured on this channel to handle frame reception. 
      :type  frame_received: :ref:`frame-received-handler`.

      The following is an code example of a frame_received handler::

          function received_content (conn, channel, frame, data) 
	      print ("Received frame, content: " .. frame.payload)
	      return
	  end

      :param frame_received_data: User defined data to be passed to frame_received handler along with corresponding handler parameters.
      :type  frame_received_data: Object

      :param on_channel: Optional handler that activates async channel create. If this handler is defined the method will return inmediately and channel creation will continue in background. Once finished the process (no matter the result) the channel will be notified on this handler (on_channel).
      :type  on_channel: :ref:`on-channel-handler`.

      :param on_channel_data: User defined data to be passed to on_channel handler along with corresponding handler parameters.
      :type  on_channel_data: Object

      :param encoding: The type of encoding used for profile_content. This is optional and if not provided, encoding_NONE will be assumed. 
      :type  encoding: Integer. Valid values are 0 for nil encoding or 1 for base64 encoding.

      :param profile_content: The profile channel start connect to be sent.
      :type  profile_content: String. Content codified as signaled by encoding param.

      :rtype: Returns a new reference to vortex.Channel or nil if the method fails. 

      In the case of function failure you can use the following code to check for channel failure::

          if channel == nil then
	      print ("ERROR: failed to start channel, error found:")
	      err = conn:pop_channel_error ()
	      while (err) do
	          print ("ERROR: error found: code=" .. err[1] .. ", msg=" .. err[2])

		  -- next error found
		  err = conn:pop_channel_error ()
	      end
          end

   .. method:: channel_pool_new (connection, profile, init_num, [create_channel],[create_channel_data],[received],[received_data],[close],[close_data],[on_created],[user_data])
   
      Allows to create a new channel pool on the provided connection. 

      :param connection: The connection.
      :type  connection: vortex.connection

      :param profile: The profile that will run channel on the pool created.
      :type  profile: String

      :param init_num: Initial number of channels to be created. At least 1 must be provided.
      :type  init_num: Integer (> 0)

      :param create_channel: Optional handler used to create a channel to be added to the pool. 
      :type  create_channel: :ref:`create-channel-handler`.

      :param create_channel_data: User defined data to be passed to create_channel handler along with corresponding handler parameters.
      :type  create_channel_data: Object

      :param received: Optional frame received handler to be configured on all channels created or handled by the pool.
      :type  received: :ref:`frame-received-handler`.

      :param received_data: User defined data to be passed to received handler along with corresponding handler parameters.
      :type  received_data: Object

      :param close: Optional channel on close handler to be configured on all channels handled by the channel pool.
      :type  close: Still not implemented.

      :param close_data: User defined data to be passed to close handler along with corresponding handler parameters.
      :type  close_data: Object

      :param on_created: Optional handler that is used to notify the pool created in an async manner. If this handler is provided, the method returns nil and the pool reference is notified at this handler. 
      :type  on_created: :ref:`on-channel-pool-created`.

      :param user_data: User defined data to be passed to on_created handler along with corresponding handler parameters.
      :type  user_data: Object

      :return: Returns a new reference to vortex.ChannelPool or nil if the on_created handler is defined.

   .. method:: pop_channel_error (connection)

      :param connection: The connection.
      :type  connection: vortex.connection
   
      Allows to get the set of errors that happend on the connection
      instance. Each call to the method pops (and removes) the next
      error message data. The function returns a list where the first
      position includes the error code and the second position inclues
      the error string.

      :rtype: Returns a list with two position where the first its the error code and the second position is the textual error. The function returns nil if no error was found.

   .. method:: set_on_close (connection, on_close, [on_close_data])
   
      Allows to configure a handler which will be called in the case
      the connection is closed. This is useful to detect broken pipe.

      :param connection: The connection.
      :type  connection: vortex.connection

      :param on_close: The handler to be executed when the connection close is detected on the instance provided.
      :type  on_close: :ref:`on-close-handler`.

      :param on_close_data: The user defined data to be passed to the on_close handler along with handler corresponding parameters.
      :type  on_close_data: Object

      :return: Returns a new reference to a vortex.Handle that can be used to remove the on close handler configured using :meth:`remove_on_close`

   .. method:: remove_on_close (connection, handle_ref)
   
      Allows to remove an on close handler configured using
      :meth:`set_on_close`. The close handler to remove is identified
      by the handle_ref parameter which is the value returned by the
      :meth:`set_on_close` handler.

      :param connection: The connection.
      :type  connection: vortex.connection

      :param handle_ref: Reference to the on close handler to remove (value returned by :meth:`set_on_close`).
      :type  handl_ref: :class:`vortex.Handle`

      :return: true in the case the handler was found and removed, otherwise false is returned.

   .. method:: find_by_uri (connection, profile)
   
      Allows to get the list of channels available (opened) on the
      connection which are running the provided profile.

      :param connection: The connection.
      :type  connection: vortex.connection

      :param profile: Profile unique id used to select channels
      :type  profile: String

      :rtype: Returns a list of channels running the provided profile.

   .. method:: set_data (connection, key, object)
   
      Allows to store arbitrary references associated to the
      connection. See also get_data.

      :param connection: The connection.
      :type  connection: vortex.connection

      :param key: The key index to which the data gets associated.
      :type  key: String

      :param object: The object to store associated on the connection index by the given key.
      :type  object: Object

   .. method:: get_data (connection, key)
   
      Allows to retreive a previously stored object using set_data

      :param connection: The connection.
      :type  connection: vortex.connection

      :param key: The index for the object looked up
      :type  key: String

      :rtype: Returns the object stored or nil if fails. 

   .. method:: pool (connection, [pool_id])
   
      Allows to return a reference to the vortex.ChannelPool already
      created on the connection with the provided pool id.

      :param pool_id: Optional vortex.ChannelPool id. If no value is provided, channel pool id takes 1, which returns the default/first channel pool created.
      :type  pool_id: Integer (> 0)

      :rtype: Returns the vortex.ChannelPool associated or nil if no channel pool exists with the provided id.

   .. method:: block (connection, [block=true])
   
      Allows to block all incoming content on the provided connection
      by skiping connection available data state. This method binds
      vortex_connection_block C API.

      :param block: Optional boolean value that configure if the connection must be blocked (true) or unblocked (false). If not configured the connection is blocked (true).
      :type  block: Boolean (true if not configured)

   .. method:: check (connection)

      :param connection: The connection.
      :type  connection: vortex.connection

      Allows to check if the provided :mod:`vortex.connection` is indeed a :mod:`vortex.connection` object.
   
      :rtype: Returns true in the case it is a :mod:`vortex.connection` object. Otherwise, false is returned.

   .. method:: is_blocked (connection)
   
      :rtype: Returns if the connection is blocked (due to :meth:`block`) or not.

   .. method:: close (connection)

      :param connection: The connection.
      :type  connection: vortex.connection
   
      Allows to close the connection using full BEEP close negotation procotol. Keep in mind that using full BEEP close procedure may suffer from BNRA (see http://www.aspl.es/vortex/draft-brosnan-beep-limit-close.txt). It is recommended to use shutdown method.

      :rtype: Return true in the case the connection was close, otherwise false is returned (because some channel was denied to be closed by remote side).

   .. method:: shutdown (connection)

      :param connection: The connection.
      :type  connection: vortex.connection
   
      Allows to close the connection by shutting down the transport layer supporting it. This causes the connection to be closed without taking place BEEP close negotiation. 

   .. method:: skip_conn_close ([skip])
   
      Allows to configure this connection reference to not call to shutdown its associated reference when the python connection is collected. This method is really useful when it is required to keep working a connection that was created inside a function but that has finished.

      By default, any :class:`vortex.connection` object created will be finished when its environment finishes. This means that when the function that created the connection finished, then the connection will be finished automatically.

      In many situations this is a desirable behaviour because your python application finishes automatically all the stuff opened. However, when the connection is created inside a handler or some method that implements connection startup but do not waits for the reply (asynchronous replies), then the connection must be still running until reply arrives. For these scenarios you have to use :meth:`skip_conn_close`.
      
   .. attribute:: id

      (Read only attribute) (Integer) returns the connection unique identifier.

   .. attribute:: ctx

      (Read only attribute) (vortex.ctx) returns the context where the connection was created.

   .. attribute:: error_msg

      (Read only attribute) (String) returns the last error message found while using the connection.

   .. attribute:: status

      (Read only attribute) (Integer) returns an integer status code representing the textual error string returned by attribute error_msg.

   .. attribute:: host

      (Read only attribute) (String) returns the host string the connection is connected to.

   .. attribute:: host_ip

      (Read only attribute) (String) returns the host IP string the connection is connected to.

   .. attribute::  port

      (Read only attribute) (String) returns the port string the connection is using to connected to.

   .. attribute:: server_name

      (Read only attribute) (String) returns connection's configured serverName (BEEP serverName value set by the first channel accepted).

   .. attribute:: local_addr

      (Read only attribute) (String) returns the local address used by the connection.

   .. attribute::  local_port

      (Read only attribute) (String) returns the local port string used by the connection.

   .. attribute:: num_channels

      (Read only attribute) (Integer) returns the number of channels opened on the connection.

   .. attribute:: role

      (Read only attribute) (String) returns a string representing the connection role. Allowed connection role strings are: initiator (client connection), listener (connection that was accepted due to a listener installed), master-listener (the master listener that is waiting for new incoming requests).  




		  
	  
