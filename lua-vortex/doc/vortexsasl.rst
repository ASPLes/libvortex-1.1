:mod:`vortex.sasl` --- LuaVortex SASL module: SASL authentication functions
===========================================================================

.. module:: vortex.sasl
   :synopsis: Vortex library SASL module
.. moduleauthor:: Advanced Software Production Line, S.L.


This modules includes all functions required to authenticate a BEEP connection.

==========
Module API
==========

.. function:: init (ctx)

   Allows to init SASL module on the provided :mod:`vortex.ctx`
   reference. This is required before any SASL operation is done.

   :param ctx: vortex context where SASL module will be initialized
   :type ctx: vortex.ctx

   :rtype: true it initialization was completed, otherwise false is returned.

.. function:: is_authenticated (conn)

   Allows to check if the provided connection is authenticated using SASL methods.

   :param conn: the connection to check for its authentication status.
   :type conn: :mod:`vortex.connection`

.. function:: start_auth (conn, profile, auth_id/anonymous_token, [password], [authorization_id], [realm], [auth_notify], [auth_notify_data])

   Allows to start a SASL authentication process using the provided SASL mech (profile) on the provided connection.

   The rest of optional arguments are used to either set parameters
   required by the SASL mechanism or to receive async authentication
   termination status (auth_notify). See vortex library manual to know
   which attributes you must provide for each mechanism:

   http://www.aspl.es/fact/files/af-arch/vortex-1.1/html/starting_to_program.html#vortex_manual_sasl_for_client_side
   
   :param conn: The connection where the SASL process will take place.
   :type conn: vortex.Connection

   :param profile: SASL mechanism to use.
   :type profile: string

   :param auth_id/anonymous_token: This is the user identification id or the anonymous token in the case of anonymous profile.
   :type auth_id/anonymous_token: string

   :param authorization_id: This is the user authorization id. 
   :type authorization_id: string

   :param password: This is the user password
   :type password: string

   :param realm: This is the authentication domain.
   :type realm: string

   :param auth_notify: User defined handler that will be used to notify SASL termination status. 
   :type auth_notify: :ref:`auth-notify-handler`

   :param auth_notify_data: User defined data that will notified along with corresponding data at auth notify handler.
   :type  auth_notify_data: object

.. function:: method_used(conn)

   Allows to get the SASL mechanism that was used to authenticate the connection.

.. function:: auth_id(conn)

   Allows to get the SASL auth_id value used during the authentication
   process (only in the case a SASL mechanism requiring it was used).

.. function:: accept_mech(ctx, profile, auth_handler, [auth_handler_data])

   Server side SASL authentication support. This function allows to
   configure a handler that will be called to complete the
   authentication process for the provided SASL mechanism.

   :param ctx: The context where the SASL handling will be configured.
   :type ctx: vortex.Ctx 

   :param profile: The SASL mechanism that will be accepted and managed by the handler provided.
   :type profile: string

   :param auth_handler: This is the SASL auth handler used to complete the operation.
   :type  auth_handler: :ref:`sasl-auth-handler`

   :param auth_handler_data: User defined data to be passed to auth_handler along with corresponding handler parameters.
   :type  auth_handler_data: object

.. attribute:: PLAIN

   (Read only attribute) (String) Full SASL Plain URI.

.. attribute:: ANONYMOUS

   (Read only attribute) (String) Full SASL Anonymous URI.

.. attribute:: EXTERNAL

   (Read only attribute) (String) Full SASL External URI.

.. attribute:: CRAM_MD5

   (Read only attribute) (String) Full SASL Cram-Md5 URI.

.. attribute:: DIGEST_MD5

   (Read only attribute) (String) Full SASL Digest-Md5 URI.



