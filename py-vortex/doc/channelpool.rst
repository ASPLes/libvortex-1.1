:mod:`vortex.ChannelPool` --- PyVortexChannelPool class: channel pools
======================================================================

.. currentmodule:: vortex


API documentation for vortex.ChannelPool object representing a set of
channels that are reused across the session.

==========
Module API
==========

.. class:: ChannelPool

   .. method:: next_ready ([auto_inc], [user_data])

      Allows to get a ready channel from the pool. If auto_inc is set to True, the pool creates a new channel in case there is no ready channel at this time.

      :param auto_inc: Instruct the channel pool to create a new channel in the case no channel is available.
      :type  auto_inc: Boolean (default False if not defined)

      :param user_data: Optional reference to a object that is passed to the create channel handler defined at vortex.Connection.channel_pool_new.
      :type  user_data: Object

      :rtype: a reference to vortex.Channel or None if it fails. None can be returned if auto_inc is not defined or set to False and no ready channel is available.

   .. method:: release (channel)
   
      Allows to release the give channel into the pool, making it available for future next_ready calls.

      :param channel: Channel reference to be released.
      :type  channel: vortex.Channel reference acquired via next_ready() call.

   .. attribute:: id

      (Read only attribute) (Integer) returns the channel pool unique identifier. This identifier is unique in context of the connection. 

   .. attribute:: ctx

      (Read only attribute) (vortex.Ctx) returns the context where the vortex.ChannelPool was created.

   .. attribute:: conn

      (Read only attribute) (vortex.Connection) returns the connection where this channel pool is running.

   .. attribute:: channel_count

      (Read only attribute) (Integer) returns the number of channels that are handled by the channel pool.

   .. attribute:: channel_available

      (Read only attribute) (Integer) returns the number of channels available (that are ready to be returned by calling to next_ready ())



		  
	  
