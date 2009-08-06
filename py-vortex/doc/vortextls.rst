:mod:`vortex.tls` --- PyVortex TLS module: TLS profile support
==============================================================

.. module:: vortex.tls
   :synopsis: Vortex library TLS module
.. moduleauthor:: Advanced Software Production Line, S.L.


This modules includes all functions required secure BEEP sessions using TLS.

==========
Module API
==========

.. function:: init (ctx)

   Allows to init TLS module on the provided vortex.Ctx reference. This is required before any TLS operation is done.

   :param ctx: vortex context where TLS module will be initialized
   :type ctx: vortex.Ctx

   :rtype: True it initialization was completed, otherwise False is returned.

.. function:: start_tls (conn, serverName, [tls_notify], [tls_notify_data])

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
   None and the resulting tuple is returned on tls_notify. 

   Providing a tls_notify handler makes this function to not block the
   caller during the TLS process. Calling without tls_notify will
   cause the caller to be blocking until the process finish (no matter
   its result).
   
   :param conn: The connection where the TLS process will take place.
   :type  conn: vortex.Connection

   :param serverName: the server name to configure on the TLS channel. This is used to signal server side to use a particular certificate according to the serverName.
   :type  serverName: string

   :param tls_notify: User defined handler that will be used to notify TLS termination status. 
   :type  tls_notify: :ref:`tls-notify-handler`

   :param tls_notify_data: User defined data that will notified along with corresponding data at tls notify handler.
   :type  tls_notify_data: object

   

.. function:: accept_tls (ctx, [accept_handler], [accept_handler_data], [cert_handler], [cert_handler_data], [key_handler], [key_handler_data])

   Allows to enable accepting incoming requests to activate TLS profile. 
   
   :param ctx: The context to be configured to accept incoming TLS profile.
   :type  ctx: vortex.Ctx

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

   

   

