:mod:`vortex.tls` --- LuaVortex TLS module: TLS profile support
===============================================================

.. module:: vortex.tls
   :synopsis: Vortex library TLS module
.. moduleauthor:: Advanced Software Production Line, S.L.


This modules includes all functions required secure BEEP sessions using TLS.

Here is an example how a client can activate TLS on an established connection::

    -- now enable tls support on the connection
    if not vortex.tls.init (ctx) then
        error ("Expected to find proper authentication initialization, but found an error")
        return false
    end

    -- enable TLS on the connection 
    conn, status, status_msg = vortex.tls.start_tls (conn)

    -- check connection after tls activation
    if not conn.is_ok () then
        error ("Expected to find proper connection status after TLS activation..")
        return false
    end

    -- check status 
    if status ~= 2 then
        error ("Expected to find status code : " .. tostring (vortex.status_OK) .. ", but found: " .. tostring (status))
    end

==========
Module API
==========

.. function:: init (ctx)

   Allows to init TLS module on the provided vortex.ctx reference. This is required before any TLS operation is done.

   :param ctx: vortex context where TLS module will be initialized
   :type ctx: vortex.ctx

   :rtype: true it initialization was completed, otherwise false is returned.

.. function:: start_tls (conn, [serverName], [tls_notify], [tls_notify_data])

   Allows to start the TLS process on the given connection. 

   The function creates a new connection object reusing the transport
   of the received connection. This means you have to update
   connection reference to the returned value.

   In the case no tls_notify handler is provided, the function will
   return a tuple with 3 elements (connection, status, status_msg):
   where connection is the connection with TLS activated, status is a
   integer status code that must be checked and status_msg is a
   textual status.

   In the case tls_notify handler is provided the function returns
   nil and the resulting tuple is returned on tls_notify. 

   Providing a tls_notify handler makes this function to not block the
   caller during the TLS process. Calling without tls_notify will
   cause the caller to be blocking until the process finish (no matter
   its result).
   
   :param conn: The connection where the TLS process will take place.
   :type  conn: vortex.connection

   :param serverName: the server name to configure on the TLS channel. This is used to signal server side to use a particular certificate according to the serverName.
   :type  serverName: string or nil.

   :param tls_notify: User defined handler that will be used to notify TLS termination status. 
   :type  tls_notify: :ref:`tls-notify-handler`

   :param tls_notify_data: User defined data that will notified along with corresponding data at tls notify handler.
   :type  tls_notify_data: object


.. function:: accept_tls (ctx, [accept_handler], [accept_handler_data], [cert_handler], [cert_handler_data], [key_handler], [key_handler_data])

   Allows to enable accepting incoming requests to activate TLS profile. 
   
   :param ctx: The context to be configured to accept incoming TLS profile.
   :type  ctx: vortex.ctx

   :param accept_handler: The handler to be called to accept or deny a particular incoming TLS request.
   :type  accept_handler: :ref:`tls-accept-handler`

   :param accept_handler_data: User defined data that will notified along with corresponding data at accept handler.
   :type  accept_handler_data: object

   :param cert_handler: The handler to be called to get the path to the certificate to be used to activate the TLS process.
   :type  cert_handler: :ref:`tls-cert-handler`

   :param cert_handler_data: User defined data that will notified along with corresponding data at cert handler.
   :type  cert_handler_data: object

   :param key_handler: The handler to be called to get the path to the private key to be used to activate the TLS process.
   :type  key_handler: :ref:`tls-key-handler`

   :param key_handler_data: User defined data that will notified along with corresponding data at key handler.
   :type  key_handler_data: object

.. function:: is_enabled (conn)

   Allows to check if the provided connection has successfully activated TLS profile

   :param conn: the connection to check for TLS activation.
   :type conn: vortex.connection
   

