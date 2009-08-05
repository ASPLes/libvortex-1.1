:mod:`vortex` --- PyVortex handlers
===================================

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
    
