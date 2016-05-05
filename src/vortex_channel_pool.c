/* 
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2016 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 *  
 *  You may find a copy of the license under this software is released
 *  at COPYING file. This is LGPL software: you are welcome to develop
 *  proprietary applications using this library without any royalty or
 *  fee but returning back any change, improvement or addition in the
 *  form of source code, project image, documentation patches, etc.
 *
 *  For commercial support on build BEEP enabled solutions contact us:
 *          
 *      Postal address:
 *         Advanced Software Production Line, S.L.
 *         C/ Antonio Suarez Nº 10, 
 *         Edificio Alius A, Despacho 102
 *         Alcalá de Henares 28802 (Madrid)
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.aspl.es/vortex
 */

#include <vortex.h>
#define LOG_DOMAIN "vortex-channel-pool"


/**
 * \defgroup vortex_channel_pool Vortex Channel Pool: Channel Pool management function.
 */

/**
 * \addtogroup vortex_channel_pool
 * @{
 */


/* the channel pool type */
struct _VortexChannelPool {
	/* the unique channel pool identifier */
	int                      id;

	/* the list of channel this pool have */
	axlList                * channels;
	
	/* the connection where the pool was created */
	VortexConnection       * connection;
	
	/* the profile this pool have and all channel to be created on
	 * the future. The following vars will be used to be able to
	 * create new channel setting the same data. */
	char                   * profile;

	/* close handles */
	VortexOnCloseChannel     close;
	axlPointer               close_user_data;
	
	/* frame received functions */
	VortexOnFrameReceived    received;
	axlPointer               received_user_data;

	/* create channel handler */
	VortexChannelPoolCreate  create_channel;
	axlPointer               create_channel_user_data;
};


typedef struct _VortexChannelPoolData {
	axl_bool                    threaded;
	VortexConnection          * connection;
	const char                * profile;
	int                         init_num;
	
	/* on close received */
	VortexOnCloseChannel        close;
	axlPointer                  close_user_data;
	
	/* frame received handler */
	VortexOnFrameReceived       received;
	axlPointer                  received_user_data;

	/* on pool created */
	VortexOnChannelPoolCreated  on_channel_pool_created;
	axlPointer                  user_data;

	/* create channel handler */
	VortexChannelPoolCreate     create_channel;
	axlPointer                  create_channel_user_data;
}VortexChannelPoolData;


/** 
 * @internal
 * Support function for this module which returns if a channel is
 * ready to be used.
 * 
 * @param channel the channel to operate
 * 
 * @return axl_true if ready to be used or axl_false if not
 */
axl_bool      __vortex_channel_pool_is_ready (VortexChannel * channel) 
{

	return (vortex_channel_is_opened (channel)          && 
		(!vortex_channel_is_being_closed (channel)) && 
		vortex_channel_is_ready (channel)      &&
		(vortex_channel_get_data (channel, "status_busy") == NULL));
}

/** 
 * @internal
 * 
 * Support function for __vortex_channel_pool_new and
 * vortex_channel_pool_add. This function actually creates new
 * channels and adds them to the channel pool.
 *
 * @param pool the pool over channels will be created
 *
 * @param init_num the maximum number of channels to create.
 *
 * @param user_data User defined data to be passed to the create
 * channel handler.
 * 
 * @return a reference to the last channel created.
 */
VortexChannel * __vortex_channel_pool_add_channels (VortexChannelPool * pool, int  init_num, axlPointer user_data)
{
	int              iterator = 0;
	VortexChannel  * channel  = NULL;
#if defined(ENABLE_VORTEX_LOG) && ! defined(SHOW_FORMAT_BUGS)
	VortexCtx      * ctx      = vortex_connection_get_ctx (pool->connection);
#endif

	/* start channels */
	while (iterator < init_num) {
		/* check if the channel creation handler is defined */
		if (pool->create_channel != NULL) {
			/* call to create */
			channel = pool->create_channel (pool->connection,
							/* channel num (let vortex to pick one for us) */
							0,
							pool->profile,
							/* close handler stuff */
							pool->close, pool->close_user_data,
							/* received handler stuff */
							pool->received, pool->received_user_data,
							/* the following the user data pointer defined at vortex_channel_pool_new_full */
							pool->create_channel_user_data,
							/* the following is an optional pointer defined at vortex_channel_pool_get_next_ready_full */
							user_data);
							

		} else {
			/* create the channel */
			channel = vortex_channel_new (pool->connection, 
						      /* channel num (let vortex to pick one for us) */
						      0,
						      pool->profile,
						      /* close handler stuff */
						      pool->close, pool->close_user_data,
						      /* received handler stuff */
						      pool->received, pool->received_user_data,
						      /* on created handler not defined */
						      NULL, NULL);
		} /* end if */
		
		/* check if the channel have been created if not
		 * break-the-loop */ 
		if (channel == NULL) {
			vortex_log (VORTEX_LEVEL_CRITICAL, 
				    "unable to create a channel inside the channel pool creation, this pool will have fewer channel than requested");
			break;
		}

		/* increase the reference to the channel */
		/* so the channel have been created  */
		vortex_channel_ref2 (channel, "channel pool");

		/* lock */
		vortex_connection_lock_channel_pool (pool->connection);

		axl_list_append (pool->channels, channel);

		/* unlock */
		vortex_connection_unlock_channel_pool (pool->connection);

		/* set a reference to the pool this channel belongs to */
		vortex_channel_set_pool (channel, pool);

		/* update iterator */
		iterator++;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "channels added %d to the pool id=%d", iterator, pool->id);
	
	return channel;
}

/** 
 * @internal
 *
 * Support function for vortex_channel_pool_new which actually does the work. 
 * 
 * @param data 
 * 
 * @return 
 */
axlPointer __vortex_channel_pool_new (VortexChannelPoolData * data)
{

	/* parameters from main function */
	axl_bool                      threaded                = data->threaded;
	VortexConnection            * connection              = data->connection;

	const char                  * profile                 = data->profile;
	int                           init_num                = data->init_num;

	/* on close channel */
	VortexOnCloseChannel          close                   = data->close;
	axlPointer                    close_user_data         = data->close_user_data;
	
	/* on frame received */
	VortexOnFrameReceived         received                = data->received;
	axlPointer                    received_user_data      = data->received_user_data;
	
	/* on pool created */
	VortexOnChannelPoolCreated    on_channel_pool_created   = data->on_channel_pool_created;
	axlPointer                    user_data                 = data->user_data;

	/* channel creation variables */
	VortexChannelPoolCreate       create_channel            = data->create_channel;
	axlPointer                    create_channel_user_data  = data->create_channel_user_data;

	/* function local parameters */
	VortexChannelPool           * channel_pool;

#if defined(ENABLE_VORTEX_LOG) && ! defined(SHOW_FORMAT_BUGS)
	VortexCtx                   * ctx                     = vortex_connection_get_ctx (connection);
#endif

	/* free data */
	axl_free (data);

	/* init channel pool type */
	channel_pool                           = axl_new (VortexChannelPool, 1);
	channel_pool->id                       = vortex_connection_next_channel_pool_id (connection);
	channel_pool->connection               = connection;
	channel_pool->profile                  = axl_strdup (profile);
	channel_pool->close                    = close;
	channel_pool->close_user_data          = close_user_data;
	channel_pool->received                 = received;
	channel_pool->received_user_data       = received_user_data;
	channel_pool->create_channel           = create_channel;
	channel_pool->create_channel_user_data = create_channel_user_data;
	channel_pool->channels                 = axl_list_new (axl_list_always_return_1, NULL);

	/* init: create channels for the pool */
	__vortex_channel_pool_add_channels (channel_pool, init_num, NULL);

	/* now have have created the channel pool install it inside the connection */
	channel_pool->connection = connection;
	vortex_connection_add_channel_pool (connection, channel_pool);

	/* return the data */
	vortex_log (VORTEX_LEVEL_DEBUG, "channel pool created id=%d over connection id=%d", channel_pool->id,
	       vortex_connection_get_id (connection));
	if (threaded) {
		on_channel_pool_created (channel_pool, user_data);
		return NULL;
	}

	return channel_pool;
}

/** 
 * @brief Creates a new pool of \ref VortexChannel items, that are managed
 * in a way that ensure better throutput relation.
 * 
 * This module was created to allow having a pool of pre-created
 * channels (\ref VortexChannel) to be used over and over again during the application
 * life.
 * 
 * Because channel creation and channel close is an expensive process
 * (especially due to all requirements to be meet during channel
 * close), the channel pool can help to improve greatly your
 * application communication throughput, considering the relation
 * between request/replies received on a selected connection (\ref VortexConnection).
 * 
 * Keep in mind the following facts which may help you to chose using
 * vortex channel pool instead of controlling yourself channels
 * created on a particular connection.
 *
 * <ul>
 * 
 *   <li>If an application needs to not be blocked while sending
 *   messages you have to use a new channel or a channel that is not
 *   waiting for a reply, so new messages (requests) are not blocked
 *   by replies for previous messages (requests).
 * 
 *   BEEP RFC definition states: <i>"any message sent will be
 *   delivered in order at the remote peer and remote peer <b>must</b>
 *   reply the message receive in the same order."</i>
 *
 *   As a consequence, if message a and b are sent over channel 1, the
 *   message reply for b won't be received until message reply for a
 *   is received. 
 *
 *   In other words, if your applications needs to have a and b
 *   message to be managed independently, you'll have to use different
 *   channel, making each interaction to be independent, and those
 *   channels must not be waiting for a previous reply (\ref vortex_channel_is_ready).</li>
 *
 *   <li>To start a channel needs two message to be exchanged between
 *   peers and, of course, the channel creation can be denied by remote
 *   peer. This have a considerable overhead.</li>
 *   
 *   <li>To close a channel needs two message to be exchanged between
 *   peers and remote peers may accept channel to be closed at any
 *   time. This is not only a overhead problem but also great
 *   performance impact problem if your application close channels
 *   frequently.</li>
 * </ul>
 *
 * The \ref VortexChannelPool allows you to create a pool of channels
 * and reuse then on next operations, saving starts/close operations,
 * based on the readyness of channels inside the pool.
 * 
 * This notion of "ready to be used" means that the channel have no
 * pending replies to be received so if you application send a message
 * it will receive the reply as soon as possible (\ref vortex_channel_is_ready).
 * 
 * Actually the reply can be really late due to remote peer processing
 * but it will not be delayed by messages previously sent, awaiting
 * for replies.
 *
 * Once you create a pool you can get its actual size, add or remove
 * channel from the pool or attach new channel that weren't created by
 * this module. In general, we can consider that there are two ways to
 * use the channel pool once created:
 * 
 * <ul>
 * <li>
 *
 *  Allow the channel pool to manage the channel creation (setting
 * all data to be used at this function: start handler, frame
 * received, etc..). In this context you must use the following
 * function to add and remove channels to the pool:
 *
 *   - \ref vortex_channel_pool_add
 *   - \ref vortex_channel_pool_remove 
 *
 * </li>
 * <li>
 * 
 * In the case that the channel creation is too complex or the
 * application level wants to control the channel creation process,
 * you must use the following functions to add channels to the pool:
 * 
 *  - \ref vortex_channel_pool_attach
 *  - \ref vortex_channel_pool_deattach
 *
 * This method have a problem: automatic channel creation is not
 * activated. The channel pool could react by adding more channels if
 * a channel is requested but no channel is available (\ref vortex_channel_pool_get_next_ready). 
 * 
 * </li>
 * </ul>
 *  
 *
 * In the case that channel creation process is managed by the channel
 * pool, all of them will be created over the given session
 * (<i>connection</i>) and will be created under the semantics defined
 * by <i>profile</i>. The function will block you until all channels
 * are created. During the channel creation process one channel may
 * fail to be created so you should check how many channels have been
 * really created.
 *
 * You can create several channel pools over the same
 * connection. Every channel pool is identified by an id. This Id can
 * be used to lookup certain channel pool into a given connection.
 * 
 * Once a channel pool is created, it is associated with the given
 * connection so calls to \ref vortex_connection_get_channel_pool over a
 * connection will return the pool created. You can of course hold a
 * reference to your channel pool and avoid using previous function.
 *
 * The function \ref vortex_channel_pool_get_next_ready will return
 * the next channel ready to be used. But this function may return
 * NULL if no channel is ready to be used. You can optionally set the
 * auto_inc parameter to increase channel pool size to resolve your
 * petition.
 *
 * Once the channel retreived from the channel pool is no longer
 * required, it is "returned to the channel pool", to be usable by
 * other queries, by calling to \ref vortex_channel_pool_release_channel.
 *
 * See this section which contains more information about using the channel pool: \ref vortex_manual_implementing_request_response_pattern.
 * 
 * @param connection              The session were channels will be created.
 * @param profile                 The profile the channels will use.
 * @param init_num                The number of channels this pool will create at the startup.
 * @param close                   Handler to manage channel closing.
 * @param close_user_data         User data to be passed in to close handler.
 * @param received                Handler to manage frame reception on channel.
 * @param received_user_data      Data to be passed in to <i>received</i> handler.
 * @param on_channel_pool_created A callback to be able to make channel process to be async.
 * @param user_data               User data to be passed in to <i>on_channel_created</i>.
 * 
 * @return A newly created \ref VortexChannelPool. The reference
 * returned is already attached to the \ref VortexConnection provided
 * so, it is not required to release it. In the case the
 * <b>on_channel_pool_created</b> handler is provided, the function returns
 * NULL, and the channel pool reference is notified at the
 * handler. Memory deallocation for the reference returned will be
 * produced once the connection associated to it is deallocated.
 */
VortexChannelPool * vortex_channel_pool_new            (VortexConnection          * connection,
							const char                * profile,
							int                         init_num,
							VortexOnCloseChannel        close,
							axlPointer                  close_user_data,
							VortexOnFrameReceived       received,
							axlPointer                  received_user_data,
							VortexOnChannelPoolCreated  on_channel_pool_created,
							axlPointer user_data)
{
	/* call to the full implementation that retains compat with
	 * this function */
	return vortex_channel_pool_new_full (connection, profile, init_num,
					     /* no creation channel handler */
					     NULL, NULL, 
					     /* close handler */
					     close, close_user_data,
					     /* received handler */
					     received, received_user_data,
					     /* on channel pool created */
					     on_channel_pool_created, user_data);
}


/** 
 * @brief Allows to create a new \ref VortexChannelPool providing a
 * function that is called to create channel rather allowing the pool
 * to call \ref vortex_channel_new directly.
 *
 * This function works the same way as \ref vortex_channel_pool_new
 * but, it allows to provide the creation function to be used to
 * initiate channels inside the pool.
 * 
 * Some BEEP profiles are simple and have no especial initial
 * handshake to create the channel properly. In that case you can use
 * safely \ref vortex_channel_pool_new. However, other BEEP
 * profiles includes an initial negotiation that requires more
 * steps to be taken in addition to \ref vortex_channel_new. 
 *
 * This is the case of the XML-RPC profile which once the channel is
 * created, it requires to change the channel into a boot state by
 * exchanging boot resource message and receiving a boot reply.
 *
 * Because the channel pool has a generic channel creation code, it
 * doesn't have the enough knowledge to create the channel properly,
 * so it delegates that task to the \ref VortexChannelPoolCreate
 * "create_channel" handler. This handler must return a properly
 * created and ready to use channel so the channel pool can manage it.
 *
 * 
 * @param connection               The session were channels will be created.
 *
 * @param profile                  The profile the channels will use.
 *
 * @param init_num                 The number of channels this pool will create at the startup.
 *
 * @param close                    Handler to manage channel closing.
 *
 * @param create_channel           Handler to the channel creation function.
 *
 * @param create_channel_user_data User defined data to be passed to
 * the create channel function for the first creation.
 *
 * @param close_user_data          User data to be passed in to close handler.
 *
 * @param received                 Handler to manage frame reception on channel.
 *
 * @param received_user_data       Data to be passed in to <i>received</i> handler.
 *
 * @param on_channel_pool_created  A callback to be able to make channel process to be async.
 *
 * @param user_data                User data to be passed in to <i>on_channel_created</i>.
 * 
 * @return A newly created \ref VortexChannelPool. The reference
 * returned is already attached to the \ref VortexConnection provided
 * so, it is not required to release it. In the case the
 * <b>on_channel_pool_created</b> handler is provided, the function returns
 * NULL, and the channel pool reference is notified at the
 * handler. Memory deallocation for the reference returned will be
 * produced once the connection associated to it is deallocated.
 */
VortexChannelPool * vortex_channel_pool_new_full       (VortexConnection          * connection,
							const char                * profile,
							int                         init_num,
							VortexChannelPoolCreate     create_channel,
							axlPointer                  create_channel_user_data,
							VortexOnCloseChannel        close,
							axlPointer                  close_user_data,
							VortexOnFrameReceived       received,
							axlPointer                  received_user_data,
							VortexOnChannelPoolCreated  on_channel_pool_created,
							axlPointer                  user_data)
{
	VortexChannelPoolData * data;
	VortexCtx             * ctx = vortex_connection_get_ctx (connection);

	/* check input data */
	v_return_val_if_fail (connection,             NULL);
	v_return_val_if_fail (profile && (* profile), NULL);
	v_return_val_if_fail (init_num > 0,           NULL);

	/* prepare data */
	data                           = axl_new (VortexChannelPoolData, 1);
	data->connection               = connection;
	data->profile                  = profile;
	data->init_num                 = init_num;
	data->create_channel           = create_channel;
	data->create_channel_user_data = create_channel_user_data;    
	data->close                    = close;
	data->close_user_data          = close_user_data;
	data->received                 = received;
	data->received_user_data       = received_user_data;
	data->on_channel_pool_created  = on_channel_pool_created;
	data->user_data                = user_data;
	data->threaded                 = (on_channel_pool_created != NULL);

	/* invoke threaded mode if defined on_channel_pool_created */
	if (data->threaded) {
		vortex_thread_pool_new_task (ctx, (VortexThreadFunc) __vortex_channel_pool_new, data);
		return NULL;
	}

	/* invoke channel_pool_new in non-threaded mode due to not
	 * defining on_channel_pool_created */
	return __vortex_channel_pool_new (data);	
}

/** 
 * @brief Returns the number of channel this pool have. 
 * 
 * @param pool the vortex channel pool to operate on.
 * 
 * @return the channel number or -1 if fail
 *
 * See also \ref vortex_channel_pool_get_available_num which returns the total
 * amount of channels that are available (excluding busy channels).
 */
int                 vortex_channel_pool_get_num        (VortexChannelPool * pool)
{
	int  num;
	if (pool == NULL)
		return -1;

	vortex_connection_lock_channel_pool   (pool->connection);

	num = axl_list_length (pool->channels);

	vortex_connection_unlock_channel_pool (pool->connection);

	return num;
}

axl_bool  __count_ready (axlPointer channel, int * _count)
{
	if (__vortex_channel_pool_is_ready (channel)) {
		/* channel ready, increase count */
		(*_count)++;
	}
	return axl_false;
	
} /* end __find_ready */

/** 
 * @brief Allows to get the number of channels that are available to
 * be used on the pool (channels that can be selected by
 * vortex_channel_pool_get_next_ready).
 *
 * @param pool The channel pool to check for available number of channels.
 *
 * @return The number of channel available or -1 if it fails. 
 *
 * See also \ref vortex_channel_pool_get_num which returns the total
 * amount of channels available no matter if they are busy or ready.
 */
int                 vortex_channel_pool_get_available_num (VortexChannelPool * pool)
{
	int  num = 0;
	if (pool == NULL)
		return -1;

	vortex_connection_lock_channel_pool   (pool->connection);

	/* count all ready channels at this moment */
	axl_list_lookup (pool->channels, (axlLookupFunc) __count_ready, &num);

	vortex_connection_unlock_channel_pool (pool->connection);

	return num;
}


/** 
 * @brief Adds more channel to channel pool already created.
 *
 * This function allows to add more channels to the actual pool. The
 * new channels added to the pool will use the same profile
 * configuration, close handler and received handler which was used at
 * \ref vortex_channel_pool_new. 
 *
 * @param pool the vortex channel pool to operate on.
 * @param num the number of channels to add.
 */
void                vortex_channel_pool_add            (VortexChannelPool * pool,
						        int  num)
{
	/* call to the full implementation */
	vortex_channel_pool_add_full (pool, num, NULL);
	return;
}

/** 
 * @brief Adds more channels to the pool provided, allowing to provide
 * a pointer that will be passed to the channel creation function.
 *
 * This function works like \ref vortex_channel_pool_add, but under
 * situations where the channel pool was created providing a \ref
 * VortexChannelPoolCreate handler at \ref
 * vortex_channel_pool_new_full, this function will allow to provide
 * the pointer to be passed to the creation function.
 *
 * This function will have the same effect as \ref
 * vortex_channel_pool_add if no channel creation function is
 * configured on the pool provided.
 *
 * See \ref vortex_channel_pool_add_full for more information. 
 * 
 * @param pool The pool where the channels will be added.
 *
 * @param num The numbers of channels to add.
 *
 * @param user_data User defined data to be passed to the creation function.
 */
void                vortex_channel_pool_add_full          (VortexChannelPool * pool,
							   int  num,
							   axlPointer user_data)
{
	if (pool == NULL || num <= 0)
		return;
	
	/* add channels */
	__vortex_channel_pool_add_channels (pool, num, user_data);

	return;
}

/** 
 * @internal
 *
 * Support function for vortex_channel_pool_remove. This function is
 * also used by vortex_channel_pool_close. Actually close @num channel
 * from the pool.
 * 
 * @param pool the pool were channels will be closed.
 * @param num the number of channels to close.
 */
void __vortex_channel_pool_remove (VortexChannelPool * pool, int  num)
{
	axlListCursor * cursor;
	VortexChannel * channel;
	int             iterator;
	int             init_num;	

	if (pool == NULL || num <= 0) 
		return;

	init_num  = axl_list_length (pool->channels);
	iterator = 0;

	/* do not perform any operation if no channel is available */
	if (init_num == 0) 
		return;

	cursor = axl_list_cursor_new (pool->channels);
	while (axl_list_cursor_has_item (cursor)) {
		
		/* check if number of channel closed have reached max
		 * number of channels to close */
		if (iterator == init_num)
			break;

		/* check if number of channel closed have reached max
		 * number of channels to close */
		if (iterator == num)
			break;
			
		/* get the channel at the current cursor position */
		channel = axl_list_cursor_get (cursor);
		if (__vortex_channel_pool_is_ready (channel)) {
			/* increase number of channels closed */
			iterator++;
			
			/* remove the channel from the channel pool */
			axl_list_cursor_unlink (cursor);

			/* remove the channel from the pool after calling to close */
			vortex_channel_set_pool (channel, NULL);

			/* close the channel */
			vortex_channel_close (channel, NULL);

			/* remove channel reference */
			vortex_channel_unref2 (channel, "channel pool");

			continue;
		} /* end if */

		/* get the next */
		axl_list_cursor_next (cursor);
		
	} /* end if */

	/* free the cursor */
	axl_list_cursor_free (cursor);

	return;
}

/**
 * @brief Removes channels from the given channel pool.
 *
 * 
 * This function allows to remove channels from the pool. This
 * function may block you because the channel to be removed are
 * selected from those which are ready (no message reply waiting). But
 * it may occur all channel from the pool are busy so function will
 * wait until channel gets ready to be removed.
 *
 * If you try to close more channel than the pool have the function
 * will close only those channel the pool already have. No error will
 * be reported on this case.
 *
 * @param pool the vortex channel pool to operate on.
 * @param num the number of channels to remove.
 **/
void                vortex_channel_pool_remove         (VortexChannelPool * pool,
							int  num)
{
	if (pool == NULL || num <= 0)
		return;

	vortex_connection_lock_channel_pool   (pool->connection);

	__vortex_channel_pool_remove (pool, num);

	vortex_connection_unlock_channel_pool (pool->connection);

	return;
}

/** 
 * @internal Common function used to dettach a pool from the
 * connection, allowing to configure if the connection is called back
 * to remove the pool.
 * 
 * @param pool The pool to close (including all its channels)
 * @param deattach_from_connection Flag to dettach the pool or not.
 */
void           __vortex_channel_pool_close_common (VortexChannelPool * pool, 
						   axl_bool            deattach_from_connection)
{
	int                 channels;
	VortexConnection  * connection;
	VortexChannel     * channel;
#if defined(ENABLE_VORTEX_LOG) && ! defined(SHOW_FORMAT_BUGS)
	VortexCtx         * ctx                     = pool ? vortex_connection_get_ctx (pool->connection) : NULL;
#endif
	axlListCursor     * cursor;
	
	if (pool == NULL)
		return;

	/* check if the channel pool is already being called to be
	 * closed */
	connection = pool->connection;
	if (connection == NULL)
		return;

	/* nullify */
	pool->connection = NULL;

	vortex_connection_lock_channel_pool   (connection);  
	vortex_log (VORTEX_LEVEL_DEBUG, "closing channel pool id=%d", pool->id);
	
	/* first close all channels from this pool */
	channels = axl_list_length (pool->channels);
	if (channels > 0) {
		vortex_log (VORTEX_LEVEL_DEBUG, "channel pool id=%d has %d channels, closing them..", 
			    pool->id, channels);

		/* remove all channels from the pool */
		__vortex_channel_pool_remove (pool, channels);

		/* nullify references to pending (not removed) channels */
		cursor = axl_list_cursor_new (pool->channels);
		while (axl_list_cursor_has_item (cursor)) {
			/* get the channel reference */
			channel = axl_list_cursor_get (cursor);

			/* nullify references on channel to removed by
			 * the pool (implicit deattach) */
			vortex_log (VORTEX_LEVEL_DEBUG, "channel pool id=%d doing implicit deattach on channel=%d..", 
				    pool->id, vortex_channel_get_number (channel));
			vortex_channel_set_pool (channel, NULL);

			/* release channel */
			vortex_channel_unref2 (channel, "channel pool");
			/* get the next cursor */
			axl_list_cursor_next (cursor);
		} /* end if */

		/* free the cursor */
		axl_list_cursor_free (cursor);

	}else 
		vortex_log (VORTEX_LEVEL_DEBUG, "channel pool id=%d is empty..",  pool->id);

	vortex_connection_unlock_channel_pool (connection);  

	/* remove the channel pool from the connection */
	if (deattach_from_connection)
		vortex_connection_remove_channel_pool (connection, pool);

	axl_list_free (pool->channels);
	axl_free (pool->profile);
	axl_free (pool);

	return;	
}

/** 
 * @brief Closes a channel pool. 
 * 
 * You should not call this function directly because the channel pool
 * is already registered on a connection which actually will also try
 * to remove the pool registered when the connection is destroyed. So,
 * you can actually leave this function and let the connection
 * destroying process to do the job. 
 *
 * This function will remove the channel pool from the hash the
 * connection have that will activate this function which finally
 * close all channels and deallocated all resources.
 *
 * You may wondering why channel pool destroying is threat this
 * way. This is because a channel pool doesn't exists without the
 * connection concept which means the connection have hard references
 * to channels pools created and must be notified when some pool is
 * not going to be available.
 *
 * This function may block you because a vortex_channel_close will be
 * issued over all channels.
 *
 * This function is also used as a destroy function because it frees
 * all resources allocated by the pool after closing all channels pool.
 * 
 * @param pool the vortex channel pool to operate on.
 **/
void                vortex_channel_pool_close          (VortexChannelPool * pool)
{
	/* call to close the pool dettaching from the connection */
	__vortex_channel_pool_close_common (pool, axl_true);
	return;
}

/** 
 * @internal Function used by the vortex connection module to close
 * channel pools without receiving back a call to unregister the pool
 * (which is already in process to do so).
 * 
 * @param pool The pool to close.
 */
void __vortex_channel_pool_close_internal (VortexChannelPool * pool)
{
	/* remove the pool without removing it from the connection */
	__vortex_channel_pool_close_common (pool, axl_false);
}

/**
 * @internal
 * 
 * Support function which checks if the given pool and the given
 * channel share the same connection.
 * 
 * Return value: axl_false if both elements have different connections and
 * axl_true if share it.
 * 
 * @param pool the pool to check.
 * @param channel the channel to check.
 **/
axl_bool      __vortex_channel_check_same_connections (VortexChannelPool * pool, 
						       VortexChannel     * channel)
{
	VortexConnection * connection;
	VortexConnection * connection2;
	VortexCtx        * ctx;

	if (pool == NULL || channel == NULL)
		return axl_false;
	
	connection  = vortex_channel_pool_get_connection (pool);
	connection2 = vortex_channel_get_connection (channel);
	ctx         = vortex_connection_get_ctx (connection);
	if (ctx == NULL)
		ctx = vortex_connection_get_ctx (connection2);
	
	if (vortex_connection_get_id (connection) !=
	    vortex_connection_get_id (connection2)) {
		vortex_log (VORTEX_LEVEL_CRITICAL, 
			    "trying to add a channel from a different session (pool session: %d != channel session: %d). Channels from different connections can't be mixed",
			    vortex_connection_get_id (connection), vortex_connection_get_id (connection2));
		return axl_false;
	}

	return axl_true;
}

axl_bool  __find_channel (axlPointer channel, axlPointer channel_in_list)
{
	/* return if the channel match */
	return vortex_channel_get_number (channel) == vortex_channel_get_number (channel_in_list);
}

/**
 * @internal
 * 
 * Support channel pool function which returns if the given channel
 * actually belongs to the given vortex channel pool.
 *
 * Keep in mind this function actually doesn't check if the given
 * channel to check actually have the same connection which may be an
 * error even this function returns the channel pool doesn't have a
 * channel with this number.
 * 
 * @param pool the channel pool to check.
 * @param channel the channel to check existence.
 *
 * @return axl_false if channel doesn't exists and axl_true if it does.
 **/
axl_bool      __vortex_channel_pool_channel_exists (VortexChannelPool * pool, 
						    VortexChannel     * channel) 
{
	/* lookup the channel */
	return (axl_list_lookup (pool->channels, __find_channel, channel) != NULL);
}

/** 
 * @brief Adds a channel reference to a channel pool.
 *
 * Allows to add an already created channel to an already create
 * pool. 
 * 
 * @param pool the vortex channel pool to operate on.
 * @param channel the vortex channel to attach
 */
void                vortex_channel_pool_attach         (VortexChannelPool * pool,
							VortexChannel     * channel)
{
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx * ctx;
#endif

	if (pool == NULL || channel == NULL)
		return;

	/* check new channel belongs to the same connection */
	if (!__vortex_channel_check_same_connections (pool, channel))
		return;

	/* lock to operate */
	vortex_connection_lock_channel_pool   (pool->connection);

	/* check if the channel to add doesn't exist on the pool */
	if (__vortex_channel_pool_channel_exists (pool, channel)) {
#if defined(ENABLE_VORTEX_LOG)
		/* get the context */
		ctx = vortex_connection_get_ctx (pool->connection);
#endif
		vortex_log (VORTEX_LEVEL_CRITICAL, "trying to add a channel which already exists on the channel pool");
		vortex_connection_unlock_channel_pool (pool->connection);
		return;
	}
	
	/* it seems the channel wasn't found on the channel pool. Add
	 * the channel. */
	vortex_channel_ref2 (channel, "channel pool");
	axl_list_append (pool->channels, channel);

	vortex_connection_unlock_channel_pool (pool->connection);

	return;
}

/**
 * @brief Detach a channel reference from the channel pool.
 * 
 * Allows to deattach the given channel from the pool. 
 *
 * This function actually doesn't close the channel even the channel
 * is removed from the channel pool. Ensure to close the channel when
 * no longer needed by using vortex_channel_close.
 *
 * @param pool the vortex channel pool to operate on.
 * @param channel the vortex channel to deattach.
 *
 **/
void                vortex_channel_pool_deattach       (VortexChannelPool * pool,
							VortexChannel     * channel)
{
	axlListCursor    * cursor;
	VortexChannel    * channel_aux;
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx        * ctx;
#endif

	if (pool == NULL || channel == NULL)
		return;

	/* check new channel belongs to the same connection */
	if (!__vortex_channel_check_same_connections (pool, channel))
		return;

	/* lock to operate */
	vortex_connection_lock_channel_pool   (pool->connection);

#if defined(ENABLE_VORTEX_LOG)
	/* get the context */
	ctx = vortex_connection_get_ctx (pool->connection);
#endif

	/* check if the channel to add doesn't exist on the pool */
	if (!__vortex_channel_pool_channel_exists (pool, channel)) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "trying to remove a channel which doesn't exists on the channel pool");
		vortex_connection_unlock_channel_pool (pool->connection);
		return;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "channel id=%d deattaching from the pool id=%d (channels=%d)",
		    vortex_channel_get_number (channel), pool->id, axl_list_length (pool->channels));

	/* it seems the channel wasn't found on the channel pool. Add
	 * the channel. */
	cursor = axl_list_cursor_new (pool->channels);
	while (axl_list_cursor_has_item (cursor)) {
		/* get the channel reference */
		channel_aux = axl_list_cursor_get (cursor);

		/* check the channel and remove from the list if found */
		if (vortex_channel_are_equal (channel, channel_aux)) {
			/* decrease reference */
			vortex_channel_unref2 (channel, "channel pool");
			axl_list_cursor_unlink (cursor);
			break;
		} /* end if */

		/* get the next cursor */
		axl_list_cursor_next (cursor);

	} /* end if */

	/* free the cursor */
	axl_list_cursor_free (cursor);

	vortex_log (VORTEX_LEVEL_DEBUG, 
		    "channel id=%d deattached from the pool id=%d (channels=%d)",
		    vortex_channel_get_number (channel), pool->id, axl_list_length (pool->channels));

	vortex_connection_unlock_channel_pool (pool->connection);

	vortex_log (VORTEX_LEVEL_DEBUG, "channel id=%d detached from pool id=%d",
	       vortex_channel_get_number (channel), vortex_channel_pool_get_id (pool));

	return;
}

/**
 * @internal
 *
 * 
 * Support function to print the channel pool status. This function
 * must be used enclosed with lock/unlock pair.
 *
 * @param pool the pool to print out the status.
 **/
void __vortex_channel_pool_print_status (VortexChannelPool * pool, char  * action) 
{
	VortexChannel * channel;
	int             iterator;
	VortexCtx     * ctx = vortex_connection_get_ctx (pool->connection);

	if (vortex_log_is_enabled (ctx)) {

		printf ("[%s] (connection=%d actual pool=%d size: %d channels [", 
			action, vortex_connection_get_id (pool->connection), pool->id,
			axl_list_length (pool->channels));
		
		iterator = 0;
		while (iterator < axl_list_length (pool->channels)) {

			/* get the channel */
			channel = axl_list_get_nth (pool->channels, iterator);

			printf ("%d", vortex_channel_get_number (channel));
			
			__vortex_channel_pool_is_ready (channel) ? printf ("(R)") : printf ("(B)");
			
			/* update the iterator */
			iterator++;

			if (iterator < axl_list_length (pool->channels))
				printf (" ");

		} /* end while */
		
		printf ("]\n");
	} /* end while */

	return;
}


/**
 * @brief Returns the next "ready to use" channel from the given pool.
 * 
 * This function returns the next "ready to use" channel from the
 * given pool. Because a vortex channel pool may have no channel ready
 * to be used the function could return NULL. But you can also make
 * this function to add a new channel to the pool if no channel is
 * ready by using auto_inc as axl_true.
 *
 * The channel returned can be used for any operation, even closing
 * the channel. But, before issuing a close operation over a channel
 * which already belongs to a pool, this channel must be detached
 * first from the pool using \ref vortex_channel_pool_deattach.
 *
 * In general a close operation over channels belonging to a pool is
 * not recommended. In fact, the need to avoid channel closing
 * operations was the main reason to produce the channel pool module
 * because, as we have said, the channel close operation is too
 * expensive.
 * 
 * It's a better approach to let the vortex connection destruction
 * function to close all channels created for a pool.
 *
 * Another recommendation to keep in mind is the startup problem. Due
 * to the initial requirement to create new channels for the pool on
 * the first connection, it may slow down the startup (because it is
 * required to create several channels). A good approach is to create
 * a pool with one channel and use this function with the auto_inc set
 * always to axl_true.
 * 
 * This will enforce to create new channels only when needed reducing
 * the performance impact of creating an arbitrary number of channels
 * (inside the pool) at the startup.
 *
 * Once the channel was used, you should use \ref
 * vortex_channel_pool_release_channel to return the channel to the
 * pool, making it usable by other invocation. The concept is to
 * release the channel as soon as possible.
 *
 * @param pool the pool where the channel will be get.
 *
 * @param auto_inc instruct this function to create a new channel if
 * not channel is ready to be used.k
 *
 * @return the next channel ready to use or NULL if fails. Note NULL may also be returned even setting auto_inc to axl_true.
 **/
VortexChannel     * vortex_channel_pool_get_next_ready (VortexChannelPool * pool,
							axl_bool            auto_inc)
{
	/* call to the full implementation */
	return vortex_channel_pool_get_next_ready_full (pool, auto_inc, NULL);
}

axl_bool  __find_ready (axlPointer channel, axlPointer data)
{
#if defined(ENABLE_VORTEX_LOG) && ! defined(SHOW_FORMAT_BUGS)
	VortexCtx * ctx = vortex_channel_get_ctx (channel);
#endif

	if (__vortex_channel_pool_is_ready (channel)) {
		/* ok!, we have found a channel that is ready
		 * to be use. */
		vortex_log (VORTEX_LEVEL_DEBUG, "channel id=%d is ready to be used, flagged as busy",
			    vortex_channel_get_number (channel));
		return axl_true;
	}
	vortex_log (VORTEX_LEVEL_DEBUG, "channel id=%d is not ready",
		    vortex_channel_get_number (channel));
	return axl_false;
	
} /* end __find_ready */


/** 
 * @brief Allows to get the next channel available on the provided
 * pool, providing a pointer that will be passed to the \ref VortexChannelPoolCreate "create channel" handler.
 *
 * This function works the same way like \ref
 * vortex_channel_pool_get_next_ready but allows to provide a pointer
 * that is passed to the VortexChannelPoolCreate handler configured at
 * \ref vortex_channel_pool_new_full. In the case you didn't configure
 * a creation channel handler, this function is not useful. 
 *
 * Once the channel was used, you should use \ref
 * vortex_channel_pool_release_channel to return the channel to the
 * pool, making it usable by other invocation. The concept is to
 * release the channel as soon as possible.
 *
 * See \ref vortex_channel_pool_get_next_ready function for more information.
 * 
 * @param pool The channel pool where a ready channel is required.
 *
 * @param auto_inc axl_true to signal the function to create a new channel
 * if there is not available.
 *
 * @param user_data User defined data to be passed to the \ref
 * VortexChannelPoolCreate function.
 * 
 * @return A newly allocated channel ready to use, or NULL if it
 * fails.
 */
VortexChannel     * vortex_channel_pool_get_next_ready_full (VortexChannelPool * pool,
							     axl_bool            auto_inc,
							     axlPointer          user_data)
{
	VortexChannel * channel   = NULL;
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx     * ctx;
#endif

	if (pool == NULL)
		return NULL;

#if defined(ENABLE_VORTEX_LOG)
	/* get the context */
	ctx = vortex_connection_get_ctx (pool->connection);
#endif

	vortex_log (VORTEX_LEVEL_DEBUG, "getting new channel before locking..");

	vortex_connection_lock_channel_pool   (pool->connection);

	vortex_log (VORTEX_LEVEL_DEBUG, "getting next channel to use");
	
	/* for each channel inside the pool check is some one is
	 * ready */
	channel  = axl_list_lookup (pool->channels, __find_ready, NULL);

	/* unlock operations */
	vortex_connection_unlock_channel_pool (pool->connection);	

	if (channel == NULL) {
		vortex_log (VORTEX_LEVEL_DEBUG, "it seems there is no channel ready to use, check auto_inc flag");
		/* it seems there is no channel available so check
		 * auto_inc var to create a new channel or simply
		 * return */
		if (auto_inc) {
			vortex_log (VORTEX_LEVEL_DEBUG, "we have auto_inc flag to axl_true, creating a new channel");
			channel = __vortex_channel_pool_add_channels (pool, 1, user_data);
		} /* end if */
	} /* end if */

	if (channel != NULL) {
		/* flag this channel to be busy */
		vortex_channel_set_data (channel, "status_busy", INT_TO_PTR (axl_true));

		vortex_log (VORTEX_LEVEL_DEBUG, "returning channel id=%d for pool id=%d connection id=%d",
			    vortex_channel_get_number (channel), pool->id, 
			    vortex_connection_get_id (pool->connection));
		
		/* __vortex_channel_pool_print_status (pool, "get_next_ready");*/
	} else {
		vortex_log (VORTEX_LEVEL_DEBUG, "unable to return a channel, pool is empty");
	} /* end if */
	
	return channel;
}

/** 
 * @brief Release a channel from the channel pool.
 * 
 * Once a channel from a pool is no longer needed or you may want to
 * flag this channel as ready to use by other thread you must call
 * this function. This function doesn't remove the channel from the
 * pool only release the channel to be used by other thread.
 *
 * @param pool the pool where the channel resides.
 * @param channel the channel to release.
 * 
 **/
void                vortex_channel_pool_release_channel   (VortexChannelPool * pool,
							   VortexChannel     * channel)
{
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx * ctx;
#endif

	if (pool == NULL || channel == NULL)
		return;

#if defined(ENABLE_VORTEX_LOG)
	/* get the context */
	ctx = vortex_connection_get_ctx (pool->connection);
#endif
	
	/* check new channel belongs to the same connection */
	if (!__vortex_channel_check_same_connections (pool, channel))
		return;

	vortex_log (VORTEX_LEVEL_DEBUG, "attempting to release the channel id=%d for pool id=%d connection id=%d", 
	       vortex_channel_get_number (channel), pool->id, vortex_connection_get_id (pool->connection));
	
	/* unlock operations */
	vortex_connection_lock_channel_pool (pool->connection);	

	/* check if the channel to add doesn't exist on the pool */
	if (!__vortex_channel_pool_channel_exists (pool, channel)) {
		vortex_log (VORTEX_LEVEL_WARNING, "trying to release a channel which doesn't exists on the channel pool");
		vortex_connection_unlock_channel_pool (pool->connection);
		return;
	} /* end if */

	/* unflag channel to be choosable */
	vortex_channel_set_data (channel, "status_busy", NULL);

	vortex_log (VORTEX_LEVEL_DEBUG, "channel id=%d for pool id=%d connection id=%d was released", 
	       vortex_channel_get_number (channel), pool->id, vortex_connection_get_id (pool->connection));

	/* __vortex_channel_pool_print_status (pool, "releasing"); */

	/* unlock operations */
	vortex_connection_unlock_channel_pool (pool->connection);	

	return;
}

/**
 * @brief Return the channel pool unique identifier.
 * 
 * Returns the channel pool identification. Every channel pool created
 * have an unique id ranging from 1 to MAX_CHANNEL_NO which is really
 * large. This means you can't create more channel pools than channels
 * allowed over a connection.
 *
 * This unique identifier can be used for several application purposes
 * but from the vortex view it's used to get a given channel pool for
 * a given connection using \ref vortex_connection_get_channel_pool. If you
 * create only one channel pool over a connection the previous
 * function will return this channel pool if 1 is passed in as
 * pool_id.
 *
 * Every channel pool identifier is unique from inside a
 * connection. You can actually create several channel pool which may
 * have as unique id = 1 over different connections.
 * 
 * @param pool the pool to return its id.
 *
 * @return the pool id or -1 if fails
 **/
int                 vortex_channel_pool_get_id         (VortexChannelPool * pool)
{
	if (pool == NULL)
		return -1;

	return pool->id;
}

/**
 * @brief Returns the Vortex Connection where this channel pool is created.
 * 
 * Returns the connection where actually the given pool resides. Every
 * pool is actually a set of channels which are created over the same
 * connection.
 *
 * @param pool the pool to get its connection. 
 *
 * @return The connection is pool have.
 **/
VortexConnection  * vortex_channel_pool_get_connection (VortexChannelPool * pool)
{
	if (pool == NULL)
		return NULL;

	return pool->connection;
}

/* @} */
