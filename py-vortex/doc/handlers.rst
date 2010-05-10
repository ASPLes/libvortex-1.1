:mod:`vortex` --- PyVortex handlers: List of handlers used by PyVortex API
==========================================================================

.. currentmodule:: vortex


The following are a list of handlers that are used by PyVortex to
notify events. They include a description and an signature example to
ease development:

.. _channel-start-handler:

=====================
Channel start handler
=====================

This handler is executed when a channel start request is received. The
handler returns True to accept channel creation or False to deny
it. Its signature is the following::

    def channel_start_received (channel_num, conn, data):
	      
	# accept the channel to be created
	return True

.. _channel-close-handler:

=====================
Channel close handler
=====================

This handler is executed when a channel close request is received. The
handler returns True to accept channel to be closed, otherwise False
to cancel close operation. Its signature is the following::

    def channel_close_request (channel_num, conn, data):

	# accept the channel to be closed
	return True 

.. _on-channel-handler:

==================
On channel handler
==================

This handler is executed when an async channel creation was requested
(at vortex.Connection.open_channel). The handler receives the channel
created or None reference if a failure was found. Here is an example::

    def on_channel (number, channel, conn, data):
    	# number: of the channel number that was created. In case of failure -1 is received.
	# channel: is the vortex.Channel reference created or None if it failed.
	# conn: the connection where the channel was created
	# data: user defined data (set at open_channel)
	return

.. _frame-received-handler:

======================
Frame received handler
======================

This handler is executed when PyVortex needs to notify a frame
received. Its signature is the following::

    def frame_received (conn, channel, frame, data):
        # handle the frame here
        return

.. _on-close-handler:

===========================
On connection close handler
===========================

This handler is executed when a connection is suddently closed (broken
pipe). Its signature is the following::

    def connection_closed (conn, data):
        # handle connection close
        return

.. _auth-notify-handler:

======================
SASL auth notification
======================

This handler is used to notify the termination status of a SASL
authentication process started on a connection. Its signature is the
following::

    def sasl_status (conn, status, status_msg, queue):
        # check status received
	if status != vortex.status_OK:
	     # SASL authentication failed
	
.. _sasl-auth-handler:

========================
SASL common auth handler
========================

This handler is used to complete the SASL authentication process. The
handler must either return True or False to accept or deny the
connection and, for some SASL profiles, the handler must return the
password associated to the user being authenticated or None if it is
required to deny auth operation.

Here is an example of the handler signature and some code example to
that access to the proper auth variable to finish the auth process::

    def sasl_auth_handler (conn, auth_props, user_data):
    
        print ("Received request to complete auth process using profile: " + auth_props["mech"])
        # check plain
        if auth_props["mech"] == vortex.sasl.PLAIN:
            if auth_props["auth_id"] == "bob" and auth_props["password"] == "secret":
                return True

        # check anonymous
        if auth_props["mech"] == vortex.sasl.ANONYMOUS:
            if auth_props["anonymous_token"] == "test@aspl.es":
                return True

        # check digest-md5
        if auth_props["mech"] == vortex.sasl.DIGEST_MD5:
            if auth_props["auth_id"] == "bob":
                # set password notification
                auth_props["return_password"] = True
                return "secret"

        # check cram-md5
        if auth_props["mech"] == vortex.sasl.CRAM_MD5:
            if auth_props["auth_id"] == "bob":
                 # set password notification
                 auth_props["return_password"] = True
                 return "secret"
    
        # deny if not accepted
        return False
    
.. _tls-notify-handler:

===============================
TLS status notification handler
===============================

This handler is used to notify TLS activation on a connection. The
handler signature is::

    def tls_notify (conn, status, status_msg, data):
        # handle TLS request
        return

.. _tls-accept-handler:

==========================
TLS accept request handler
==========================

This handler is used accept or deny an incoming TLS request. The
handler must returnd True to accept the request to continue or False
to cancel it. The handler signature is::

    def tls_accept_handler(conn, server_name, data):
        # accept TLS request
        return True

.. _tls-cert-handler:

================================
TLS certificate location handler
================================

This handler is used to get the location of the certificate to be used
during the activation of the TLS profile. The handler signature is::

    def tls_cert_handler (conn, server_name, data):
        return "test.crt"

.. _tls-key-handler:

========================
TLS key location handler
========================

This handler is used to get the location of the private key to be used
during the activation of the TLS profile. The handler signature is::

    def tls_key_handler (conn, server_name, data):
        return "test.key"


.. _create-channel-handler:

===============================================
Channel create handler (for vortex.ChannelPool)
===============================================

This handler is executed when a vortex.ChannelPool requires to add a
new channel into the pool. Its signature is the following::

    def create_channel (conn, channel_num, profile, received, received_data, close, close_data, user_data, next_data):
    	# create a channel
        return conn.open_channel (channel_num, profile)

.. _on-channel-pool-created:

=========================================================
Channel pool create notification (for vortex.ChannelPool)
=========================================================

This handler is executed when a vortex.ChannelPool was created and the
on_created handler was configured at
vortex.Connection.channel_pool_new. Its signature is the following::

    def on_pool_created (pool, data):
    	print ("Pool created: " + str (pool))
        return 