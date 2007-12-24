/*
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2005 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 *  
 *  You may find a copy of the license under this software is released
 *  at COPYING file. This is LGPL software: you are welcome to
 *  develop proprietary applications using this library without any
 *  royalty or fee but returning back any change, improvement or
 *  addition in the form of source code, project image, documentation
 *  patches, etc. 
 *
 *  For commercial support on build BEEP enabled solutions contact us:
 *          
 *      Postal address:
 *         Advanced Software Production Line, S.L.
 *         C/ Dr. Michavila Nº 14
 *         Coslada 28820 Madrid
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://fact.aspl.es
 */

/* global include */
#include <vortex.h>

/* private include */
#include <vortex_ctx_private.h>

#if defined(AXL_OS_UNIX)
# include <netinet/tcp.h>
#endif

#define LOG_DOMAIN "vortex-connection"
#define VORTEX_CONNECTION_BUFFER_SIZE 32768

typedef struct _VortexConnectionNotifyData {
	VortexConnectionNotifyNew handler;
	axlPointer                user_data;
} VortexConnectionNotifyData;


/** 
 * @internal Type used to store error reported by the remote side
 * while creating channels.
 */
typedef struct _VortexChannelError {
	/* error code */
	int    code;
	/* textual diagnostic */
	char * msg;
} VortexChannelError;

typedef struct _VortexChannelStatusUpdate {
	VortexConnectionOnChannelUpdate handler;
	axlPointer                      handler_data;
} VortexChannelStatusUpdate;

/** 
 * @internal Function to free channel error found.
 * 
 * @param error Error to be deallocated.
 */
void __vortex_connection_free_channel_error (VortexChannelError * error)
{
	if (error == NULL)
		return;
	axl_free (error->msg);
	axl_free (error);

	return;
}

/** 
 * @internal
 * @brief Internal VortexConnection representation.
 */
struct _VortexConnection {
	/**
	 * @brief Vortex context where the connection was created.
	 */
	VortexCtx  * ctx;

	/** 
	 * @brief Vortex Connection unique identifier.
	 */
	int          id;

	/** 
	 * @brief Host name this connection is actually connected to.
	 */
	char       * host;

	/** 
	 * @brief Port this connection is actually connected to.
	 */
	char       * port;

	/** 
	 * @brief Allows to hold and report current connection status.
	 */
	bool         is_connected;

	/**
	 * @brief App level message to report current connection
	 * status using a textual message.
	 */
	char       * message;

	/** 
	 * @brief Underlying transport descriptor.
	 * This is actually the network descriptor used to send and
	 * received data.
	 */
	VORTEX_SOCKET  session;

	/** 
	 * @brief Allows to configure if the given session (actually
	 * the socket) associated to the given connection should be
	 * closed once __vortex_connection_set_not_connected function.
	 */
	bool           close_session;

	/**
	 * Profiles supported by the remote peer.
	 */
	axlList      * remote_supported_profiles;

	/**
	 * A list of installed masks.
	 */ 
	axlList      * profile_masks;

	/** 
	 * Channels already created inside the given VortexConnection.
	 */
	VortexHash * channels;
	/** 
	 * Hash to hold miscellaneous data, sometimes used by the
	 * Vortex Library itself, but also exposed to be used by the
	 * Vortex Library programmers through the functions:
	 *   - \ref vortex_connection_get_data 
	 *   - \ref vortex_connection_set_data
	 */
	VortexHash * data;
	/** 
	 * @brief Keeps track for the last channel created on this
	 * session. The connection role modifies the way channel
	 * number are allocated. In the case the role is initiator,
	 * client role, odd channel numbers are generated. In the case
	 * the role is listener, server role, even channel numbers are
	 * generated.
	 */
	int          last_channel;

	/** 
	 * @brief Remote features supported/requested.
	 *
	 * This variable holds possible features content set at the
	 * greetings element received from the remote peer.
	 */
	char       * features;

	/** 
	 * @brief Remote localize supported/requested.
	 * 
	 * This variable holds possible localization values the remote
	 * peer is requesting.
	 */
	char       * localize;

	/**
	 * @brief The channel_mutex
	 *
	 * This mutex is used at vortex_connection_get_next_channel to
	 * avoid race condition of various threads calling in a
	 * reentrant way on this function getting the same value.
	 * 
	 * Although this function use vortex_hash_table, a
	 * thread-safe hash table, this is not enough to ensure only
	 * one thread can execute inside this function.
	 * 
	 * The vortex_connection_add_channel adds a channel into a
	 * existing connection. Inside the implementation, this
	 * function first lookup if channel already exists which means
	 * a program error and the insert the new channel if no
	 * channel was found.
	 *
	 * This mutex is also used on that function to avoid reentrant
	 * conditions on the same function and between function which
	 * access to channel hash table.
	 */
	VortexMutex     channel_mutex;

	/**
	 * @brief The ref_count.
	 *
	 * This enable a vortex connection to keep track of the
	 * reference counting.  The reference counting is controlled
	 * thought the vortex_connection_ref and
	 * vortex_connection_unref.
	 * 
	 */
	int  ref_count;
	
	/**
	 * @brief The ref_mutex
	 * This mutex is used at vortex_connection_ref and
	 * vortex_connection_unref to avoid race conditions especially
	 * at vortex_connection_unref. Because several threads can
	 * execute at the same time this unref function this mutex
	 * ensure only one thread will execute the vortex_connection_free.
	 * 
	 */
	VortexMutex  ref_mutex;

	/**
	 * @brief The op_mutex
	 * This mutex allows to avoid race condition on operating with
	 * the same connection from different threads.
	 */
	VortexMutex op_mutex;
	
	/** 
	 * @brief The handlers mutex
	 *
	 * This mutex is usted to secure code sections destinated to
	 * handle handlers stored associated to the connection.
	 */
	VortexMutex handlers_mutex;

	/**
	 * @brief The channel_pool_mutex
	 * This mutex is used by the vortex_channel_pool while making
	 * operations with channel pool over this connection. This is
	 * necessary to make mutually exclusive the function from that
	 * module while using the same connection.
	 */
	VortexMutex channel_pool_mutex;

	/**
	 * @brief The next_channel_pool_id
	 * This value is used by the channel pool module to hold the
	 * channel pool identifiers that will be used.
	 */
	int  next_channel_pool_id;

	/**
	 * @brief The channel_pools
	 * This hash actually holds every channel pool created over
	 * this connection. Every channel pool have an id that unique
	 * identifies the channel pool from each over inside this
	 * connection and it is used as key.
	 */
	VortexHash * channel_pools;

	/** 
	 * @brief The peer role
	 * 
	 * This is used to know current connection role.	
	 */
	VortexPeerRole role;

	/** 
	 * @brief Writer function used by the Vortex Library to actually send data.
	 */
	VortexSendHandler    send;

	/** 
	 * @brief Writer function used by the Vortex Library to actually received data
	 */
	VortexReceiveHandler receive;

	/** 
	 * @brief On close handler
	 */
	axlList * on_close;

	/** 
	 * @brief On close handler, extended version.
	 */
	axlList * on_close_full;

	/** 
	 * @brief Internal reference to the serverName value.
	 */
	char    * serverName;

	/**
	 * @brief Stack storing pending channel errors found.
	 */ 
	axlStack * pending_errors;

	/**
	 * @brief Mutex used to open the pending errors list.
	 */
	VortexMutex pending_errors_mutex;

	/** 
	 * @brief Two lists that allows keeping reference to handlers
	 * to be called once channels are added removed.
	 */
	axlList  * add_channel_handlers;
	axlList  * remove_channel_handlers;

	/**
	 * @brief Mutex used to protect previous lists.
	 */
	VortexMutex channel_update_mutex;
};



/** 
 * @internal Fucntion that perform the connection socket sanity check.
 * 
 * This prevents from having problems while using socket descriptors
 * which could conflict with reserved file descriptors such as 0,1,2..
 *
 * @param ctx The context where the operation will be performed. 
 *
 * @param connection The connection to check.
 * 
 * @return true if the socket sanity check have passed, otherwise
 * false is returned.
 */
bool     vortex_connection_do_sanity_check (VortexCtx * ctx, VORTEX_SOCKET session)
{
	/* warn the user if it is used a socket descriptor that could
	 * be used */
	if (ctx && ctx->connection_enable_sanity_check) {
		
		/* check for a valid socket descriptor. */
		switch (session) {
		case 0:
		case 1:
		case 2:
			vortex_log (VORTEX_LEVEL_CRITICAL, 
			       "created socket descriptor using a reserved socket descriptor (%d), this is likely to cause troubles",
			       session);
			/* return sanity check have failed. */
			return false;
		}
	}

	/* return the sanity check is ok. */
	return true;
}
/** 
 * @internal
 * @brief Default handler used to send data.
 * 
 * See \ref vortex_connection_set_send_handler.
 */
int  vortex_connection_default_send (VortexConnection * connection,
				     const char       * buffer,
				     int                buffer_len)
{
	/* send the message */
	return send (connection->session, buffer, buffer_len, 0);
}

/** 
 * @internal
 * @brief Default handler to be used while receiving data
 *
 * See \ref vortex_connection_set_receive_handler.
 */
int  vortex_connection_default_receive (VortexConnection * connection,
					char             * buffer,
					int                buffer_len)
{
	/* receive content */
	return recv (connection->session, buffer, buffer_len, 0);
}

/** 
 * @internal Support function for connection identificators.
 *
 * This is used to generate and return the next connection identifier.
 *
 * @param ctx The context where the operation will be performed.
 *
 * @return Next connection identifier available.
 */
int  __vortex_connection_get_next_id (VortexCtx * ctx)
{
	/* get current context */
	int         result;

	/* lock */
	vortex_mutex_lock (&ctx->connection_id_mutex);
	
	/* increase */
	result = ctx->connection_id;
	ctx->connection_id++;

	/* unlock */
	vortex_mutex_unlock (&ctx->connection_id_mutex);

	return result;
}


/** 
 * @internal
 * @brief allows to set a channel as connected
 * 
 * Really bad trick to be able to set connected state to a channel
 * this is needed for channel 0 connections and all channels 
 *
 * @param channel The channel to set the channel status.
 */
extern void __vortex_channel_set_connected (VortexChannel * channel);

typedef struct _VortexConnectionNewData {
	VortexConnection    * connection;
	VortexConnectionNew   on_connected;
	axlPointer            user_data;
	bool                  threaded;
}VortexConnectionNewData;


typedef struct _VortexConnectionOnCloseData {
	VortexConnectionOnCloseFull handler;
	axlPointer                  data;
} VortexConnectionOnCloseData;

bool __vortex_connection_close_list_channels (axlPointer key, axlPointer value, axlPointer user_data)
{
	
	VortexChannel             * channel = value;
	/* save channel */
	axl_list_append ((axlList *) user_data, channel);

	return false;
}

void __close_channel_aux (axlPointer _channel)
{
	VortexChannel * channel = _channel;
	/* get the context */
	VortexCtx     * ctx     = vortex_channel_get_ctx (channel);

	/* check this channel is not the administrative one. */
	if (vortex_channel_get_number (channel) == 0)
		return;
	
	/* close the channel, */
	if (!vortex_channel_close (channel, NULL)) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to close a channel=%d during vortex connection closing",
			    vortex_channel_get_number (channel));
	} /* end if */
	
	return;
}

/** 
 * @brief Closes all channel opened on this connection, including the channel 0.
 *
 * This function allows to close all channels opened under the given
 * connection without setting the connection to be unusable. This
 * function could be used to support those profiles which reset all
 * channels status due to a tuning reset. An example of this type of
 * tuning reset is TLS negotiation.
 * 
 * @param connection The connection where all channels will be closed
 * and released
 *
 * @param also_channel_0 Allows to control if the channel 0 have to be closed.
 *
 * @return true if all channels were close, false if not. Keep in mind
 * that this function returns false if only one channel have rejected
 * to be closed.  If a null reference is received, the function
 * returns false.
 */
bool     vortex_connection_close_all_channels (VortexConnection * connection, bool     also_channel_0)
{
	VortexChannel             * channel0 = NULL;
	VortexCtx                 * ctx;
	axlList                   * result;

	/* define the context */
	if (connection == NULL)
		return false;
	
	/* get the context */
	ctx = connection->ctx;

	/* block connection operations during channel closing for this
	 * session */
	vortex_mutex_lock   (&connection->op_mutex);
	switch (vortex_hash_size (connection->channels)) {
	case 0:
		/* no channels to close, going to close connection */
		goto close_connection;
	case 1:
		/* only the channel 0 is available, close channel
		 * 0. */
		goto close_channel0;
	default:
		/* many channels to close. */
		break;
	}

	/* create a list of opened channels */
	result       = axl_list_new (axl_list_always_return_1, __close_channel_aux);

	/* build the list, with all channels found */
	vortex_hash_foreach (connection->channels, __vortex_connection_close_list_channels, result);

	/* close all channels, using the destroy function associated
	 * to the list, which will close all channels found */
	axl_list_free (result);

	/* check if all channels were closed, without including the
	 * channel 0, that is, at least one channel must be in
	 * place */
	if (vortex_hash_size (connection->channels) > 1) {

		/* release lock and free connection resources */
		vortex_mutex_unlock (&connection->op_mutex);

		vortex_log (VORTEX_LEVEL_WARNING, "unable to close connection, there are channels still working");
		return false;
	}

 close_channel0:
	/* check if it is requested to also close the channel 0 */
	if (also_channel_0) {

		/* now we have closed all channels we have to close
		 * channel 0 */
		channel0 = vortex_connection_get_channel (connection, 0);
		if (channel0 && !vortex_channel_close (channel0, NULL)) {
			/* release lock and free connection
			 * resources */
			vortex_mutex_unlock (&connection->op_mutex);
			
			vortex_log (VORTEX_LEVEL_WARNING, "unable to close the channel 0..");
			return false;
		}
	}
	
 close_connection:
	/* release lock and free connection resources */
	vortex_mutex_unlock (&connection->op_mutex);

	return true;

}

/** 
 * @internal Structure to store cache datat associated to greetings
 * received, storing the connection features and profiles supported.
 */
typedef struct _VortexConnectionGreetingsCache {
	char    * features;
	char    * localize;
	axlList * remote_supported_profiles;
}VortexConnectionGreetingsCache;

void __vortex_connection_free_greetings_cache (VortexConnectionGreetingsCache * cache)
{
	/* do not oper if null reference is received */
	if (cache == NULL)
		return;

	axl_list_free (cache->remote_supported_profiles);
	axl_free (cache->features);
	axl_free (cache->localize);
	axl_free (cache);

	return;
}

VortexConnectionGreetingsCache * __vortex_connection_create_greetings_cache (VortexCtx  * ctx, 
									     const char * index, 
									     axlDoc     * doc)
{
	/* get current context */
	VortexConnectionGreetingsCache * cache;
	axlNode                        * node;
	axlNode                        * child;
	char                           * uri;

	/* Get the root element (greetings element) */
	node = axl_doc_get_root (doc);

	cache           = axl_new (VortexConnectionGreetingsCache, 1);
	cache->features = axl_node_get_attribute_value_copy (node, "features");
	cache->localize = axl_node_get_attribute_value_copy (node, "localize");

	/* create the list */
	cache->remote_supported_profiles = axl_list_new (axl_list_always_return_1, axl_free);

	/* Get the position of the first profile element */
	child    = axl_node_get_first_child (node);
	while (child != NULL) {

		/* get profiles */
		uri   = axl_node_get_attribute_value_copy (child, "uri");
		axl_list_append (cache->remote_supported_profiles, uri);

		/* update the child for the next step */
		child = axl_node_get_next (child);
	} /* end while */

	/* free the document */
	axl_doc_free (doc);

	/* store cache settings */
	axl_hash_insert_full (ctx->connection_xml_cache, 
			      /* store the key and no destroy function */
			      (axlPointer) axl_strdup (index), axl_free, 
			      /* store the doc and its destroy function */
			      cache, (axlDestroyFunc) __vortex_connection_free_greetings_cache);

	/* return the cache settings */
	return cache;
}

VortexConnectionGreetingsCache * __vortex_connection_greetings_cache_get (VortexCtx  * ctx, 
									  const char * index)
{
	/* null received, null returned */
	if (ctx == NULL)
		return NULL;

	/* return the current lookup status */
	return axl_hash_get (ctx->connection_xml_cache, (axlPointer) index);
}

bool     __vortex_connection_parse_greetings (VortexConnection * connection, VortexFrame * frame)
{
	/* local variable */
	axlDoc                         * doc       = NULL;
	bool                             in_cache;
	axlDtd                         * channel_dtd;
	axlError                       * error = NULL;
	VortexConnectionGreetingsCache * cache;
	VortexCtx                      * ctx;

	/* check the reference */
	if (connection == NULL || frame == NULL)
		return false;
	
	/* get a reference to the context */
	ctx = connection->ctx;

	if ((channel_dtd = vortex_dtds_get_channel_dtd (ctx)) == NULL) {
		__vortex_connection_set_not_connected (connection, "Cannot find DTD definition for channel management");
		return false;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "About to parse the following message: (size: %d) '%s'",
	       vortex_frame_get_payload_size (frame), vortex_frame_get_payload (frame));

	/* dtd correct */

	/* now check if the document is in the cache and use it */
	vortex_mutex_lock (&ctx->connection_xml_cache_mutex);
	cache = __vortex_connection_greetings_cache_get (ctx, vortex_frame_get_payload (frame));
	if (cache == NULL) {
		vortex_log (VORTEX_LEVEL_DEBUG, "document not found in cache, parsing as usual");
		/* document not found, flag that it is not in chase
		 * and use it as is. If the validation goes ok, store
		 * in the case */
		in_cache = false;
		doc      = axl_doc_parse (vortex_frame_get_payload (frame), 
					  vortex_frame_get_payload_size (frame), &error);
	} else {
		vortex_log (VORTEX_LEVEL_DEBUG, "document found in cache (hit!), reusing reference");

		/* nice! cache hit, flag as is */
		in_cache = true;
	} /* end if */
	
	/* check here the document reference */
	if (doc == NULL && cache == NULL) {
		/* unlock the cache */
		vortex_mutex_unlock (&ctx->connection_xml_cache_mutex);

		vortex_log (VORTEX_LEVEL_CRITICAL, "found an error while parsing greetings message: %s",
			    error ? axl_error_get (error) : "");
		axl_error_free (error);
		__vortex_connection_set_not_connected (connection, "Cannot parse xml message to validate it");
		return false;
	}

	/* doc correct */
	if (in_cache || axl_dtd_validate (doc, channel_dtd, &error)) {
		/* check if the document is inside the cache, if not,
		 * store it */
		if (! in_cache) {
			vortex_log (VORTEX_LEVEL_DEBUG, "storing xml document for future cache hit");
			/* store the document, indexed by its content,
			 * creating the cache representing the
			 * esential part of the xml document (the
			 * following function already free the axlDoc
			 * reference) */
			cache = __vortex_connection_create_greetings_cache (ctx, vortex_frame_get_payload (frame), doc);
		} /* end if */

		/* unlock the cache */
		vortex_mutex_unlock (&ctx->connection_xml_cache_mutex);

		/* now use cached information to fill remote profiles
		 * settings, features and localize */
		connection->features                  = cache->features;
		connection->localize                  = cache->localize;
		connection->remote_supported_profiles = cache->remote_supported_profiles;
		
		return true;
	}

	/* unlock the cache */
	vortex_mutex_unlock (&ctx->connection_xml_cache_mutex);

	/* Validation failed */
	vortex_log (VORTEX_LEVEL_CRITICAL, "validation failed: %s", axl_error_get (error));
	axl_error_free (error);

	/* release memory used by the xml document */
	axl_doc_free   (doc);

	/* flag the connection to be not connected */
	__vortex_connection_set_not_connected (connection, "Incoming greetings validation failed");
	return false;
}

/**
 * \defgroup vortex_connection Vortex Connection: Related function to create and manage Vortex Connections.
 */

/**
 * \addtogroup vortex_connection
 * @{
 */

/** 
 * @brief Allows to create a new \ref VortexConnection from a socket
 * that is already connected.
 *
 * This function creates the basics structures for a new \ref
 * VortexConnection instance is it were created with \ref
 * vortex_connection_new but without performing any additional BEEP
 * operation such us sending initial greetings. 
 *
 * This function only takes the given underlying transport descriptor
 * (that is, the socket) and gets the remote peer name in order to set
 * which is the remote host peer.
 *
 * This function also sets the default handler for sending and
 * receiving data using the send and recv operations. If you are
 * developing a profile which needs to notify the way data is actually
 * sent or received you should change that behavior after calling
 * this function. This is done by using:
 *
 *  - \ref vortex_connection_set_send_handler
 *  - \ref vortex_connection_set_receive_handler
 *
 * @param ctx     The context where the operation will be performed.
 * @param session An already connected socket.  
 * @param role    The role to be set to the connection being created.
 * 
 * @return a newly allocated \ref VortexConnection. 
 */
VortexConnection * vortex_connection_new_empty            (VortexCtx *    ctx, 
							   int            session, 
							   VortexPeerRole role)
{
	/* creates a new connection */
	return vortex_connection_new_empty_from_connection (ctx, session, NULL, role);
}

/** 
 * @internal
 *
 * Internal function used to create new connection starting from a
 * socket, and optional from internal data stored on the provided
 * connection. It is supposed that the socket provided belongs to the
 * connection provided.
 *
 * The function tries to creates a new connection, using the socket
 * provided. The function also keeps all internal connection that for
 * the new connection creates extracting that data from the connection
 * reference provided. This is useful while performing tuning
 * operations that requires to reset the connection but retain data
 * stored on the connection.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param socket The socket to be used for the new connection.
 *
 * @param connection The connection where the user space data will be
 * extracted. This reference is optional.
 *
 * @param role The connection role to be set to this function.
 * 
 * @return A newly created connection, using the provided data, that
 * must be deallocated using \ref vortex_connection_close.
 */
VortexConnection * vortex_connection_new_empty_from_connection (VortexCtx        * ctx,
								VORTEX_SOCKET      session,
								VortexConnection * __connection,
								VortexPeerRole     role)
{
	VortexConnection   * connection;
	VortexChannel      * channel;
	struct sockaddr_in   sin;
#if defined(AXL_OS_WIN32)
	/* windows flavors */
	int                  sin_size = sizeof (sin);
#else
	/* unix flavors */
	socklen_t            sin_size = sizeof (sin);
#endif

	/* perform connection sanity check */
	if (!vortex_connection_do_sanity_check (ctx, session)) 
		return NULL;
	
	/* get remote peer name */
	if (role == VortexRoleMasterListener) {
		if (getsockname (session, (struct sockaddr *) &sin, &sin_size) < -1) {
			vortex_log (VORTEX_LEVEL_DEBUG, "unable to get remote hostname and port");
			return NULL;
		} /* end if */
	} else {
		if (getpeername (session, (struct sockaddr *) &sin, &sin_size) < -1) {
			vortex_log (VORTEX_LEVEL_DEBUG, "unable to get remote hostname and port");
			return NULL;
		} /* end if */
	} /* end if */

	connection                     = axl_new (VortexConnection, 1);
	connection->ctx                = ctx;
	connection->id                 = __vortex_connection_get_next_id (ctx);
	connection->host               = axl_strdup (inet_ntoa (sin.sin_addr));
	connection->port               = axl_strdup_printf ("%d", ntohs (sin.sin_port));
	connection->message            = axl_strdup ("session established and ready");
	connection->session            = session;
	connection->is_connected       = true;
	connection->ref_count          = 1;
	
	/* remote side profiles, NULL reference filled by the
	 * greetings cache */
	connection->remote_supported_profiles = NULL;
	connection->profile_masks             = axl_list_new (axl_list_always_return_1, axl_free);
	
	/* list to store handlers */
	connection->on_close                  = axl_list_new (axl_list_always_return_1, NULL);
	connection->on_close_full             = axl_list_new (axl_list_always_return_1, axl_free);
	
	/* mutexes */
	vortex_mutex_create (&connection->ref_mutex);
	vortex_mutex_create (&connection->op_mutex);
	vortex_mutex_create (&connection->handlers_mutex);
	vortex_mutex_create (&connection->pending_errors_mutex);
	vortex_mutex_create (&connection->channel_update_mutex);

	/* channel creation errors list */
	connection->pending_errors = axl_stack_new ((axlDestroyFunc) __vortex_connection_free_channel_error);

	/* check connection that is accepting connections */
	if (role != VortexRoleMasterListener) {
		vortex_mutex_create (&connection->channel_pool_mutex);
		connection->channels           = vortex_hash_new_full (axl_hash_int, axl_hash_equal_int, 
								       /* key destroy */
								       NULL,
								       /* channel destroy */
								       (axlDestroyFunc) vortex_channel_unref);
		/* creates the user space data */
		if (__connection != NULL) {
			connection->data       = __connection->data;
			__connection->data     = NULL;
			
			/* remove being closed flag if found */
			vortex_connection_set_data (connection, "being_closed", NULL);
		}else 
			connection->data       = vortex_hash_new_full (axl_hash_string, axl_hash_equal_string,
								       NULL, 
								       NULL);
		
		connection->channel_pools      = vortex_hash_new_full (axl_hash_int, axl_hash_equal_int,
								       NULL, 
								       (axlDestroyFunc) __vortex_channel_pool_close_internal);
		vortex_mutex_create (&connection->channel_mutex);

		/* configure the last channel value */
		connection->last_channel       = (role == VortexRoleListener) ? 2 : 1;

		/* set default send and receive handlers */
		connection->send               = vortex_connection_default_send;
		connection->receive            = vortex_connection_default_receive;

		
		/* create channel 0 (virtually always is created but, is
		 * necessary to have a representation for channel 0, in order
		 * to make channel management function to be consistent). */
		channel = vortex_channel_empty_new (0, "not applicable", connection);
		
		/* associate channel 0 with actual connection */
		vortex_connection_add_channel (connection, channel);

	} else {
		/* create the hash data table for master listener connections */
		connection->data       = vortex_hash_new_full (axl_hash_string, axl_hash_equal_string,
							       NULL, 
							       NULL);
		
	} /* end if */
	
	/* establish the connection role and its initial next channel
	 * number available. */
	connection->role               = role;

	/* set by default to close the underlying connection when the
	 * connection is closed */
	connection->close_session      = true;


	return connection;	
}


/** 
 * \brief Allows to change connection semantic to blocking.
 *
 * This function should not be useful for Vortex Library consumers
 * because the internal Vortex Implementation requires connections to
 * be non-blocking semantic.
 * 
 * @param connection the connection to set as blocking
 * 
 * @return true if blocking state was set or false if not.
 */
bool     vortex_connection_set_blocking_socket (VortexConnection    * connection)
{
	VortexCtx * ctx;
#if defined(AXL_OS_UNIX)
	int  flags;
#endif
	/* check the connection reference */
	if (connection == NULL)
		return false;

	/* get a reference to the ctx */
	ctx = connection->ctx;
	
#if defined(AXL_OS_WIN32)
	if (!vortex_win32_blocking_enable (connection->session)) {
		__vortex_connection_set_not_connected (connection, "unable to set blocking I/O");
		return false;
	}
#else
	if ((flags = fcntl (connection->session, F_GETFL, 0)) < -1) {
		__vortex_connection_set_not_connected (connection, 
						       "unable to get socket flags to set non-blocking I/O");
		return false;
	}
	vortex_log (VORTEX_LEVEL_DEBUG, "actual flags state before setting blocking: %d", flags);
	flags &= ~O_NONBLOCK;
	if (fcntl (connection->session, F_SETFL, flags) < -1) {
		__vortex_connection_set_not_connected (connection, "unable to set non-blocking I/O");
		return false;
	}
	vortex_log (VORTEX_LEVEL_DEBUG, "actual flags state after setting blocking: %d", flags);
#endif
	vortex_log (VORTEX_LEVEL_DEBUG, "setting connection as blocking");
	return true;
}

/** 
 * \brief Allows to change connection semantic to nonblocking.
 *
 * Sets a connection to be non-blocking while sending and receiving
 * data. This function should not be useful for Vortex Library
 * consumers.
 * 
 * @param connection the connection to set as nonblocking.
 * 
 * @return true if nonblocking state was set or false if not.
 */
bool     vortex_connection_set_nonblocking_socket (VortexConnection * connection)
{
	VortexCtx * ctx;

#if defined(AXL_OS_UNIX)
	int  flags;
#endif
	/* check the reference */
	if (connection == NULL)
		return false;

	/* get a reference to context */
	ctx = connection->ctx;

#if defined(AXL_OS_WIN32)
	if (!vortex_win32_nonblocking_enable (connection->session)) {
		__vortex_connection_set_not_connected (connection, "unable to set non-blocking I/O");
		return false;
	}
#else
	if ((flags = fcntl (connection->session, F_GETFL, 0)) < -1) {
		__vortex_connection_set_not_connected (connection, "unable to get socket flags to set non-blocking I/O");
		return false;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "actual flags state before setting nonblocking: %d", flags);
	flags |= O_NONBLOCK;
	if (fcntl (connection->session, F_SETFL, flags) < -1) {
		__vortex_connection_set_not_connected (connection, "unable to set non-blocking I/O");
		return false;
	}
	vortex_log (VORTEX_LEVEL_DEBUG, "actual flags state after setting nonblocking: %d", flags);
#endif
	vortex_log (VORTEX_LEVEL_DEBUG, "setting connection as non-blocking");
	return true;
}

/** 
 * @brief Allows to configure tcp no delay flag (enable/disable Nagle
 * algorithm).
 * 
 * @param socket The socket to be configured.
 *
 * @param enable The value to be configured, true to enable tcp no
 * delay.
 * 
 * @return true if the operation is completed.
 */
bool                vortex_connection_set_sock_tcp_nodelay   (VORTEX_SOCKET socket,
							      bool          enable)
{
	/* local variables */
	int result;

#if defined(AXL_OS_WIN32)
	BOOL   flag = enable ? TRUE : FALSE;
	result      = setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (char  *)&flag, sizeof(BOOL));
#else
	int    flag = enable;
	result      = setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof (flag));
#endif
	if (result < 0) {
		return false;
	}

	/* properly configured */
	return true;
} /* end */

/** 
 * @brief Allows to enable/disable non-blocking/blocking behavior on
 * the provided socket.
 * 
 * @param socket The socket to be configured.
 *
 * @param enable Enable the I/O block, otherwise non blocking I/O will
 * be activated if false is provided.
 * 
 * @return true if the operation was properly done, otherwise false is
 * returned.
 */
bool                vortex_connection_set_sock_block         (VORTEX_SOCKET socket,
							      bool          enable)
{
#if defined(AXL_OS_UNIX)
	int  flags;
#endif

	if (enable) {
		/* enable blocking mode */
#if defined(AXL_OS_WIN32)
		if (!vortex_win32_blocking_enable (socket)) {
			return false;
		}
#else
		if ((flags = fcntl (socket, F_GETFL, 0)) < -1) {
			return false;
		} /* end if */

		flags &= ~O_NONBLOCK;
		if (fcntl (socket, F_SETFL, flags) < -1) {
			return false;
		} /* end if */
#endif
	} else {
		/* enable nonblocking mode */
#if defined(AXL_OS_WIN32)
		/* win32 case */
		if (!vortex_win32_nonblocking_enable (socket)) {
			return false;
		}
#else
		/* unix case */
		if ((flags = fcntl (socket, F_GETFL, 0)) < -1) {
			return false;
		}
		
		flags |= O_NONBLOCK;
		if (fcntl (socket, F_SETFL, flags) < -1) {
			return false;
		}
#endif
	} /* end if */

	return true;
}


/** 
 * @internal wrapper to avoid possible problems caused by the
 * gethostbyname implementation which is not required to be reentrant
 * (thread safe).
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param hostname The host to translate.
 * 
 * @return A reference to the struct hostent or NULL if it fails to
 * resolv the hostname.
 */
struct in_addr * vortex_gethostbyname (VortexCtx  * ctx, 
				       const char * hostname)
{
	/* get current context */
	struct in_addr * result;
	struct hostent * _result;
	
	/* lock and resolv */
	vortex_mutex_lock (&ctx->connection_hostname_mutex);

	/* resolv using the hash */
	result = axl_hash_get (ctx->connection_hostname, (axlPointer) hostname);
	if (result == NULL) {
		_result = gethostbyname (hostname);
		if (_result != NULL) {
			/* alloc and get the address */
			result         = axl_new (struct in_addr, 1);
			result->s_addr = ((struct in_addr *) (_result->h_addr_list)[0])->s_addr;

			/* now store the result */
			axl_hash_insert_full (ctx->connection_hostname, 
					      /* the hostname */
					      axl_strdup (hostname), axl_free,
					      /* the address */
					      result, axl_free);
		} /* end if */
	} /* end if */

	/* unlock and return the result */
	vortex_mutex_unlock (&ctx->connection_hostname_mutex);

	return result;
	
}

/** 
 * @internal
 * @brief Support function to vortex_connection_new. 
 *
 * This function actually does the work for the vortex_connection_new.
 * 
 * @param data To perform vortex connection creation process
 * 
 * @return on thread model NULL on non-thread model the connection
 * created (connected or not connected).
 */
axlPointer __vortex_connection_new (VortexConnectionNewData * data)
{

	/* get current context */
	VortexConnection   * connection   = data->connection;
	VortexCtx          * ctx          = connection->ctx;
	struct in_addr     * haddr;
	struct sockaddr_in   saddr;
	VortexFrame        * frame;
	VortexChannel      * channel;
	int 		     d_timeout    = 0;
	int		     d_start_time = 0;
	int		     err          = 0;
	axlPointer	     on_writing;

	vortex_log (VORTEX_LEVEL_DEBUG, "executing connection new in %s mode to %s:%s id=%d",
	       (data->threaded == true) ? "thread" : "blocking", 
	       connection->host, connection->port,
	       connection->id);

	/*
	 * standard tcp socket voodo connection (I would like to know
	 * who was the great mind designer of this api)
	 */
	haddr = vortex_gethostbyname (ctx, connection->host);
        if (haddr == NULL) {
		vortex_log (VORTEX_LEVEL_WARNING, "unable to get host name by using gethostbyname");
		connection->message      = axl_strdup ("unable to get host name by using gethostbyname");
		goto __vortex_connection_new_finalize;
	}

	/* create the socket and check if it */
        connection->session      = socket(AF_INET, SOCK_STREAM, 0);
	if (connection->session == VORTEX_INVALID_SOCKET) {

		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to create socket");
		connection->message = axl_strdup ("unable to create socket");
		goto __vortex_connection_new_finalize;
	}


	/* do a sanity check on socket created */
	if (!vortex_connection_do_sanity_check (ctx, connection->session)) {
		connection->message = axl_strdup_printf ("created socket descriptor using a reserved socket descriptor (%d), this is likely to cause troubles",
							 connection->session);
		goto __vortex_connection_new_finalize;
	}
	
	/* prepare socket configuration to operate using TCP/IP
	 * socket */
        memset(&saddr, 0, sizeof(saddr));
        memcpy(&saddr.sin_addr, haddr, sizeof(struct in_addr));
        saddr.sin_family    = AF_INET;
        saddr.sin_port      = htons((uint16_t) strtod (connection->port, NULL));
	
	/* get current vortex connection timeout to check if the
	 * application have requested to configure a particular TCP
	 * connect timeout. */
	d_timeout  = vortex_connection_get_connect_timeout (connection->ctx); 
	if (d_timeout) {
		/* make the socket to be nonblocking */
		vortex_connection_set_nonblocking_socket (connection);

		/* get start time */
		d_start_time = time (NULL);
	} /* end if */

	/* do a tcp connect */
        if (connect (connection->session, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
		if(d_timeout == 0 || (errno != VORTEX_EINPROGRESS && errno != VORTEX_EWOULDBLOCK)) { 
			shutdown (connection->session, SHUT_RDWR);
			vortex_log (VORTEX_LEVEL_WARNING, "unable to connect to remote host");
			connection->message = axl_strdup ("unable to connect to remote host");
			goto __vortex_connection_new_finalize;
		}
	}
	
	/* if a connection timeout is defined, wait until connect */
	if(d_timeout) {
		/* create a waiting set using current selected I/O
		 * waiting engine. */
		on_writing = vortex_io_waiting_invoke_create_fd_group (ctx, WRITE_OPERATIONS);
		vortex_io_waiting_invoke_clear_fd_group (ctx, on_writing);

		/* add the socket in connection transit */
		if (vortex_io_waiting_invoke_add_to_fd_group (ctx, connection->session, connection, on_writing)){

			while (d_start_time + d_timeout> time (NULL)) {
				err = vortex_io_waiting_invoke_wait (ctx, on_writing, connection->session, WRITE_OPERATIONS);
				
				if(err == -1 /*EINTR*/ || err == -2 /*SSL*/)
					continue;
				else if (!err) 
					continue; /*select, poll, epoll timeout*/
				else if (err>0) 
					break; /*connect ok*/
				else if (err /*==-3, fatal internal error, other errors*/)
					break;
			} /* end while */
		} /* end if */
		
		if(err <= 0){
			/* timeout reached while waiting for the connection to terminate */
			shutdown (connection->session, SHUT_RDWR);
			vortex_log (VORTEX_LEVEL_WARNING, "unable to connect to remote host (timeout)");
			connection->message = axl_strdup ("unable to connect to remote host (timeout)");
			vortex_io_waiting_invoke_destroy_fd_group (ctx, on_writing);
			goto __vortex_connection_new_finalize;
		}	
		
		/* destroy waiting set */
		vortex_io_waiting_invoke_destroy_fd_group (ctx, on_writing);
		
		/* make the connection to be blocking during the
		 * greetings process */
		vortex_connection_set_blocking_socket (connection);
	}

	/* flag that the connection is ok, and continue with the
	 * process */
	connection->is_connected = true;
	goto __vortex_connection_new_invoke;

 __vortex_connection_new_finalize:
	/* common case when everything goes wrong */
	connection->is_connected = false;
	vortex_close_socket (connection->session);
	connection->session = -1;
		

 __vortex_connection_new_invoke:
	/* according to the connection status (is_connected attribute)
	 * perform the final operations so the connection becomes
	 * usable. Later, the user app level is notified. */
	if (connection->is_connected) {

		/* create channel 0 (virtually always is created but,
		 * is necessary to have a representation for channel
		 * 0, in order to make channel management function to
		 * be consistent). */
		channel = vortex_channel_empty_new (0, "not applicable", connection);
		vortex_connection_add_channel  (connection, channel);

		/* block thread until received remote greetings */
		frame = vortex_greetings_client_process (connection);
		vortex_log (VORTEX_LEVEL_DEBUG, "greetings received, process reply frame");

		/* now we have to send greetings and process them */
		if (frame == NULL || ! vortex_greetings_client_send (connection)) {
			/* greetings have failed, unref the frame */
			vortex_frame_unref (frame);

			vortex_log (VORTEX_LEVEL_DEBUG, vortex_connection_get_message (connection));
			goto __vortex_connection_new_invoke_caller;
		}
		
		vortex_log (VORTEX_LEVEL_DEBUG, "greetings sent, waiting for reply");

		/* process frame response */
		if (!vortex_connection_parse_greetings_and_enable (connection, frame))
			goto __vortex_connection_new_finalize;

		/* check for auto TLS negotiation */
		if (ctx->connection_auto_tls) {
			/* seems automatic TLS profile negotiation is
			 * activated, check for TLS support */
			if  (! vortex_tls_is_enabled (ctx) && !ctx->connection_auto_tls_allow_failures) {
				__vortex_connection_set_not_connected (connection, "Unable to create a new connection, auto TLS activated and current Vortex Library doesn't have support for TLS profile");
				goto __vortex_connection_new_invoke_caller;
			}

			/* seems that current Vortex Library have TLS
			 * profile built-in support and auto TLS is
			 * activated */
			connection = vortex_tls_start_negociation_sync (connection,
									ctx->connection_auto_tls_server_name,
									NULL, NULL);
			if (!vortex_connection_is_ok (connection, false)) {
				goto __vortex_connection_new_invoke_caller;
			}
			
			/* the connection is ok, check if the TLS
			 * profile was activated, but only if auto tls
			 * allow failures is not set */
			if (! ctx->connection_auto_tls_allow_failures) {
				if (! vortex_connection_is_tlsficated (connection)) {
					__vortex_connection_set_not_connected (connection,
									       "Automatic TLS-fication have failed, the connection have been flagged to be closed due to not allowing TLS failure settings");
					goto __vortex_connection_new_invoke_caller;
				} /* end if */
			} /* end if */
			
			/* lets continue calling the user space code */
		}
	}
 __vortex_connection_new_invoke_caller: 
	/* notify on callback or simply return */
	if (data->threaded) {
		data->on_connected (connection, data->user_data);
		axl_free (data);			
		return NULL;
	}
	axl_free (data);
	return connection;
}

/** 
 * @brief Allows to create a new BEEP session (connection) to the given <i>host:port</i>.
 *
 * While working with <b>BEEP</b> frameworks, you need to create a new
 * connection (\ref VortexConnection), and over it, create channels (\ref
 * VortexChannel), which will allow you to send and receive data. Keep
 * in mind connection doesn't send and receive data, this is actually
 * done by channels.
 *
 * Once you have created an connection, using this function, you can
 * use \ref vortex_channel_new function to create new channels, using
 * the profile you have selected. 
 * 
 * <b>Profiles</b> are semantic definitions, representing an agreement
 * between peers for those message to be exchanged inside a channel,
 * and what they mean. You can read the following \ref profile_example
 * "tutorial" to know more \ref profile_example "about profiles" and
 * \ref profile_example "how they interact" with Vortex Library.
 *
 * This function will block you until the connection is created with
 * remote site or until timeout mechanism is reached. You can define
 * vortex timeout for connections creation by using
 * \ref vortex_connection_timeout.
 *
 * Optionally you can define an \ref VortexConnectionNew "on_connected handler" 
 * to process response, avoiding getting blocked on \ref vortex_connection_new call.
 *
 * If the \ref VortexConnectionNew "on_connected handler" is defined
 * the connection creation will be performed in an independent
 * thread. This will allow the caller to keep on doing other tasks
 * while the connection is being created. This means \ref
 * vortex_connection_new function will never block the caller if the
 * \ref VortexConnectionNew "on_connected handler" is defined.
 * 
 * Inside the connection process, a session negotiation will take
 * place. BEEP RFC defines that remote server peer must send its
 * supported profiles. Due to this, \ref VortexConnection will hold
 * all remote supported profiles. You will need this information to
 * know which profiles supports the remote peer, so you can check if
 * your BEEP client application can actually talk to the remote
 * server. Use \ref vortex_connection_get_remote_profiles or \ref vortex_connection_is_profile_supported for this
 * issue.
 *
 * BEEP framework also allows to define special features and localize
 * options to be notified to the remote peer we are going to connect
 * to. This is actually done by using \ref
 * vortex_greetings_set_features and \ref
 * vortex_greetings_set_localize. Values set on that function will be
 * used for all connections created. 
 *
 * To actually get features and localize requested by the peer we have
 * connected to, you have to use \ref vortex_connection_get_features and
 * \ref vortex_connection_get_localize.
 *
 * Once finished with the connection, at the client side, you have to
 * call to \ref vortex_connection_close function close and dispose the
 * BEEP session.
 *
 * If you are interested on getting notifications once your connection
 * is broken, in unix environments know as "broken pipe", you can use
 * the following handler:
 * 
 *   - \ref vortex_connection_set_on_close
 *   - \ref vortex_connection_set_on_close_full
 *
 * At listener side, it could be required to get notifications about
 * new incoming connections accepted, mainly to perform some special
 * initialization task. Then you can use the following:
 * 
 *  - \ref vortex_listener_set_on_connection_accepted 
 *
 * Additionally, you may be interested on securing the connection
 * created using the TLS profile. This is actually done with \ref
 * vortex_tls_start_negociation. Optionally, you can configure your
 * Vortex Library client peer to automatically negotiate the TLS
 * profile for every connection created, allowing to get from this
 * function a connection that already have TLS profile activated. You
 * can configure this behavior using \ref vortex_connection_set_auto_tls. 
 *
 * Check out the \ref vortex_connection_set_auto_tls "documentation"
 * for \ref vortex_connection_set_auto_tls to know more about using
 * automatic TLS profile negotiation.
 *
 * Finally, while considering how to transport user space data that
 * actually applies to the connection being created consider using
 * \ref vortex_connection_set_data and \ref
 * vortex_connection_set_data_full.  This function will allow you to
 * store user space data in a hash-index manner that could be
 * automatically deallocated while using \ref vortex_connection_set_data_full.
 *
 * 
 * @param ctx The context where the operation will be performed.
 *
 * @param host The remote peer to connect to.
 *
 * @param port The peer's port to connect to.
 *
 * @param on_connected Optional handler to process connection new
 * status.
 *
 * @param user_data Optional handler to process connection new status
 * 
 * @return A newly created \ref VortexConnection if called in a
 * blocking manner, that is, without providing the <b>on_connected</b>
 * handler. If you provide the <b>on_connected</b> handler, the
 * function will return NULL, and the connection created will be
 * notified on the handler <b>on_connected</b>. In both cases, you
 * must use \ref vortex_connection_is_ok to check if you are already
 * connected.
 */
VortexConnection * vortex_connection_new (VortexCtx   * ctx,
					  const char  * host, 
					  const char  * port, 
					  VortexConnectionNew on_connected,
					  axlPointer user_data)
{

	VortexConnectionNewData * data;

	/* check context */
	if (ctx == NULL)
		return NULL;

	/* check parameters */
	v_return_val_if_fail (host, NULL);
	v_return_val_if_fail (port, NULL);

	data                                  = axl_new (VortexConnectionNewData, 1);
	data->connection                      = axl_new (VortexConnection, 1);
	data->connection->id                  = __vortex_connection_get_next_id (ctx);
	data->connection->ctx                 = ctx;
	data->connection->host                = axl_strdup (host);
	data->connection->port                = axl_strdup (port);
	data->connection->channels            = vortex_hash_new_full (axl_hash_int, axl_hash_equal_int,
								      NULL,
								      (axlDestroyFunc) vortex_channel_unref);
	vortex_mutex_create (&data->connection->channel_mutex);
	data->connection->ref_count           = 1;
	vortex_mutex_create (&data->connection->ref_mutex);
	vortex_mutex_create (&data->connection->op_mutex);
	vortex_mutex_create (&data->connection->handlers_mutex);
	vortex_mutex_create (&data->connection->channel_pool_mutex);
	vortex_mutex_create (&data->connection->pending_errors_mutex);

	/* channel creation errors list */
	data->connection->pending_errors = axl_stack_new ((axlDestroyFunc) __vortex_connection_free_channel_error);

	data->connection->data                = vortex_hash_new_full (axl_hash_string, axl_hash_equal_string,
								      NULL,
								      NULL);
	data->connection->channel_pools       = vortex_hash_new_full (axl_hash_int, axl_hash_equal_int,
								      NULL, 
								      (axlDestroyFunc) __vortex_channel_pool_close_internal);
	/* remote side profiles, NULL reference filled by the
	 * greetings cache */
	data->connection->remote_supported_profiles = NULL;
	data->connection->profile_masks             = axl_list_new (axl_list_always_return_1, axl_free);

	/* list to store handlers */
	data->connection->on_close                  = axl_list_new (axl_list_always_return_1, NULL);
	data->connection->on_close_full             = axl_list_new (axl_list_always_return_1, axl_free);

	/* establish the connection role and initial next channel
	 * available. */
	data->connection->role                = VortexRoleInitiator;
	data->connection->last_channel        = 1;

	/* set default send and receive handlers */
	data->connection->send                = vortex_connection_default_send;
	data->connection->receive             = vortex_connection_default_receive;

	/* set by default to close the underlying connection when the
	 * connection is closed */
	data->connection->close_session       = true;

	/* establish connection status, connection negotiation method
	 * and user data */
	data->on_connected                    = on_connected;
	data->user_data                       = user_data;
	data->threaded                        = (on_connected != NULL);

	if (data->threaded) {
		vortex_log (VORTEX_LEVEL_DEBUG, "invoking connection_new threaded mode");
		vortex_thread_pool_new_task (ctx, (VortexThreadFunc) __vortex_connection_new, data);
		return NULL;
	}
	vortex_log (VORTEX_LEVEL_DEBUG, "invoking connection_new non-threaded mode");
	return __vortex_connection_new (data);
}

/** 
 * @internal A function which is called to notify if a channel was
 * added or removed from the provided connection.
 * 
 * @param connection The connection that is being notified.
 *
 * @param channel The channel that is being about to be removed/added
 * from the connection.
 *
 * @param is_added true if the channel is being added, otherwise false
 * is received, notifying that is removed.
 */
void __vortex_connection_check_and_notify (VortexConnection * connection, 
					   VortexChannel    * channel, 
					   bool               is_added)
{
	/* get the channel number */
	int                         iterator;
	VortexChannelStatusUpdate * statusUpdate;
	axlList                   * list;

	if (connection == NULL)
		return;

	/* configure the list to operate */
	if (is_added)
		list = connection->add_channel_handlers;
	else
		list = connection->remove_channel_handlers;
	
	/* call to notify channel to be removed */
	if (channel != NULL && axl_list_length (list) > 0) {
		
		/* the channel exists, notify */
		vortex_mutex_lock (&connection->channel_update_mutex);

		/* notify */
		iterator = 0;
		while (iterator < axl_list_length (list)) {

			/* get the reference */
			statusUpdate = axl_list_get_nth (list, iterator);

			/* call */
			if (statusUpdate != NULL) {
				/* notify the channel and the user data */
				statusUpdate->handler (channel, statusUpdate->handler_data);
			} /* end if */

			/* next iterator */
			iterator++;
			
		} /* end while */
		
		/* the channel exists, notify */
		vortex_mutex_unlock (&connection->channel_update_mutex);
		
	} /* end if */

	return;
}

bool __vortex_connection_foreach_check_and_notify (axlPointer key, 
						   axlPointer data,
						   axlPointer user_data, 
						   axlPointer user_data2)
{
	VortexChannel    * channel  = data;
	VortexConnection * conn     = user_data;
	bool               is_added = PTR_TO_INT (user_data2);

	/* call to notify closed handler */
	vortex_channel_invoke_closed (channel);

	/* call to notify */
	__vortex_connection_check_and_notify (conn, channel, is_added);

	/* always return false to make the process to follow to the
	 * end */
	return false;
}

/** 
 * @brief Allows to reconnect the given connection, using actual connection settings.
 * 
 * Given a connection that is actually broken, this function can be
 * used to reconnect it trying to find, on the same host and port, the
 * peer that the connection was linked to.
 *
 * The connection "reconnected" will keep the connection identifier,
 * any data set on this connection using the \ref
 * vortex_connection_set_data and \ref vortex_connection_set_data_full
 * functions, the connection reference count and remote profiles read.
 *
 * If the function detect that the given connection is currently
 * working and connected then the function does nothing.
 *
 * Here is an example:
 * \code
 * // check if the connection is broken, maybe remote site have
 * // failed.
 * if (! vortex_connection_is_ok (connection, false)) {
 *       // connection is not available, try to reconnect.
 *       if (! vortex_connection_reconnect (connection,
 *                                          // no async notify
 *                                          NULL,
 *                                          // no user data
 *                                          NULL)) {
 *            printf ("Link failure detected..");
 *       }else {
 *            printf ("Unable to reconnect to remote site..");
 *       }
 * } // end if
 * \endcode
 * 
 * @param connection    The connection to reconnect to.
 *
 * @param on_connected Optional handler. This function works as \ref
 * vortex_connection_new. This handler can be defined to notify when
 * the connection has been reconnected.
 *
 * @param user_data User defined data to be passed to on_connected.
 * 
 * @return true if the connection was successfully reconnected or false
 * if not. If the <i>on_connected</i> handler is defined, this
 * function will always return true.
 */
bool                vortex_connection_reconnect              (VortexConnection * connection,
							      VortexConnectionNew on_connected,
							      axlPointer user_data)
{
	VortexCtx               * ctx;
	VortexConnectionNewData * data;

	/* we have to check a basic case. A null connection. Null
	 * connection no data to reestablish the connection. */
	if (connection == NULL)
		return false;

	if (vortex_connection_is_ok (connection, false))
		return true;

	/* get a reference to the ctx */
	ctx = connection->ctx;

	vortex_log (VORTEX_LEVEL_DEBUG, "requested a reconnection on a connection which isn't null and is not connected..");

	/* close all pending channels (clear all channels and pools):
	 * the following to lines should be this way and this
	 * order. If not, really bad things will happen. This is
	 * because releasing all channels, due to freeing it from the
	 * hash table, automatically remove it from the pool. Once all
	 * the channels are removed, channels pools are removed with
	 * no problem.  */
	/* call to notify that all channels will be removed */
	vortex_hash_foreach2 (connection->channels, __vortex_connection_foreach_check_and_notify, connection, INT_TO_PTR(false));

	vortex_hash_clear (connection->channels);
	vortex_hash_clear (connection->channel_pools);
	
	/* reset channel pool id counting */
	connection->next_channel_pool_id = 0;

	vortex_log (VORTEX_LEVEL_DEBUG, "channels, channels pools and channel pools indications cleared..");

	/* create data needed to invoke the service */
	data               = axl_new (VortexConnectionNewData, 1);
	data->connection   = connection;
	
	
	data->on_connected = on_connected;
	data->user_data    = user_data;
	data->threaded     = (on_connected != NULL);

	vortex_log (VORTEX_LEVEL_DEBUG, "reconnection request is prepared, doing so..");

	/* clear previous message if defined */
	if (connection->message)
		axl_free (connection->message);
	connection->message = NULL;
	
	if (data->threaded) {
		vortex_log (VORTEX_LEVEL_DEBUG, "reconnecting connection in threaded mode");
		vortex_thread_pool_new_task (ctx, (VortexThreadFunc) __vortex_connection_new, data);
		return true;
	}
	vortex_log (VORTEX_LEVEL_DEBUG, "reconnecting connection in non-threaded mode");
	return vortex_connection_is_ok (__vortex_connection_new (data), false);

}


/** 
 * @brief Tries to close properly a connection and all channels inside it.
 * 
 * This function close a connection and unref it using \ref vortex_connection_unref.
 *
 * Because there can be channels created and still working this
 * function will try to close them by executing vortex_channel_close
 * on them. If vortex_channel_close fails for one of them, the
 * vortex_connection_close will be stopped and the connection will not
 * be closed.
 *
 * Do not call this function twice because a unref operations is
 * performed over the connection if the function returns true. This
 * means after calling this function the connection reference must not
 * be used until a new connection is created. Otherwise a segfault
 * may happen.
 *
 * If the function receives a null connection reference, it will just
 * return a true value without doing nothing.
 * 
 * @param connection the connection to close properly.
 * 
 * @return true if connection was closed and false if not. If there
 * are channels still working, the connection will not be closed.
 */
bool                   vortex_connection_close                  (VortexConnection * connection)
{
	int         refcount = 0;
	VortexCtx * ctx;

	/* if the connection is a null reference, just return true */
	if (connection == NULL)
		return true;

	/* get a reference to the ctx */
	ctx = connection->ctx;

	/* close all channel on this connection */
	if (vortex_connection_is_ok (connection, false)) {
		vortex_log (VORTEX_LEVEL_DEBUG, "closing a connection which is already opened with %d channels opened..",
		       vortex_hash_size (connection->channels));

		/* update the connection reference to avoid race
		 * conditions caused by deallocations */
		vortex_connection_ref (connection, "vortex_connection_close");
		
		/* close all channels because the connection at this
		 * point is running properly */
		if (!vortex_connection_close_all_channels (connection, true)) {
			/* update the connection reference to avoid race
			 * conditions caused by deallocations */
			vortex_connection_unref (connection, "vortex_connection_close");


			vortex_log (VORTEX_LEVEL_CRITICAL, "failed while closing all channels");
			return false;
		}

		/* set the connection to be not connected */
		__vortex_connection_set_not_connected (connection, "close connection called");

		/* update the connection reference to avoid race
		 * conditions caused by deallocations */
		refcount = connection->ref_count;
		vortex_connection_unref (connection, "vortex_connection_close");

		/* check special case where the caller have stoped a
		 * listener reference without taking a reference to it */
		if ((refcount - 1) == 0) {
			return true;
		}
	}else 
		vortex_log (VORTEX_LEVEL_DEBUG, "closing a connection which is not opened, unref resources..");

	/* unref vortex connection resources */
	vortex_log (VORTEX_LEVEL_DEBUG, "connection close finished, now unrefering..");
	vortex_connection_unref (connection, "vortex_connection_close");

	return true;
}

/** 
 * @brief Increase internal vortex connection reference counting.
 * 
 * Because Vortex Library design, several on going threads shares
 * references to the same connection for several purposes. 
 * 
 * Connection reference counting allows to every on going thread to
 * notify the system that connection reference is no longer be used
 * so, if the reference counting reach a zero value, connection
 * resources will be deallocated.
 *
 * While using the Vortex Library is not required to use this function
 * especially for those applications which are built on top of a
 * profile which is layered on Vortex Library. 
 *
 * This is because connection handling is done through functions such
 * \ref vortex_connection_new and \ref vortex_connection_close (which
 * automatically handles connection reference counting for you).
 *
 * However, while implementing new profiles these function becomes a
 * key concept to ensure the profile implementation don't get lost
 * connection references.
 *
 * Keep in mind that using this function implied to use \ref
 * vortex_connection_unref function in all code path implemented. For
 * each call to \ref vortex_connection_ref it should exist a call to
 * \ref vortex_connection_unref. Failing on doing this will cause
 * either memory leak or memory corruption because improper connection
 * deallocations.
 * 
 * The function return true to signal that the connection reference
 * count was increased in one unit. If the function return false, the
 * connection reference count wasn't increased and a call to
 * vortex_connection_unref should not be done. Here is an example:
 * 
 * \code
 * // try to ref the connection
 * if (! vortex_connection_ref (connection, "some known module or file")) {
 *    // unable to ref the connection
 *    return;
 * }
 *
 * // connection referenced, do work 
 *
 * // finally unref the connection
 * vortex_connection_unref (connection, "some known module or file");
 * \endcode
 *
 * @param connection the connection to operate.
 * @param who who have increased the reference.
 *
 * @return true if the connection reference was increased or false if
 * an error was found.
 */
bool                   vortex_connection_ref                    (VortexConnection * connection, 
								 const char       * who)
{
	VortexCtx * ctx;

	v_return_val_if_fail (connection, false);
	v_return_val_if_fail (vortex_connection_is_ok (connection, false), false);

	/* get a reference to the ctx */
	ctx = connection->ctx;
	
	/* lock ref/unref operations over this connection */
	vortex_mutex_lock   (&connection->ref_mutex);

	/* increase and log the connection increased */
	connection->ref_count++;

	vortex_log (VORTEX_LEVEL_DEBUG, "increased connection id=%d reference to %d by %s",
	       connection->id,
	       connection->ref_count, who ? who : "??" );

	/* unlock ref/unref options over this connection */
	vortex_mutex_unlock (&connection->ref_mutex);

	return true;
}

/** 
 * @brief Decrease vortex connection reference counting.
 *
 * Allows to decrease connection reference counting. If this reference
 * counting goes under 0 the connection resources will be deallocated. 
 *
 * See also \ref vortex_connection_ref
 * 
 * @param connection The connection to operate.
 * @param who        Who have decreased the reference. This is a string value used to log which entity have decreased the connection counting.
 */
void               vortex_connection_unref                  (VortexConnection * connection, 
							     char const       * who)
{
	VortexCtx  * ctx;
	int          count;

	/* do not operate if no reference is received */
	if (connection == NULL)
		return;

	/* lock the connection being unrefered */
	vortex_mutex_lock     (&(connection->ref_mutex));

	/* get context */
	ctx = connection->ctx;

	/* decrease reference counting */
	connection->ref_count--;

	vortex_log (VORTEX_LEVEL_DEBUG, "decreased connection id=%d reference count to %d decreased by %s", 
	       connection->id,
	       connection->ref_count, who ? who : "??");

	/* get current count */
	count = connection->ref_count;
	vortex_mutex_unlock (&(connection->ref_mutex));

	/* if counf is 0, free the connection */
	if (count == 0) {
		vortex_connection_free (connection);
	} /* end if */

	return;
}


/** 
 * @brief Allows to get current reference count for the provided connection.
 *
 * See also the following functions:
 *  - \ref vortex_connection_ref
 *  - \ref vortex_connection_unref
 * 
 * @param connection The connection that is requested to return its
 * count.
 *
 * @return The function returns the reference count or -1 if it fails.
 */
int                 vortex_connection_ref_count              (VortexConnection * connection)
{
	/* check reference received */
	if (connection == NULL)
		return -1;

	/* return the reference count */
	return connection->ref_count;
}

/** 
 * @brief Allows to tweak vortex internal timeouts for synchrnous
 * operations.
 * 
 * This function allows to set the timeout to use on new
 * connection creation.  Default timeout is defined to 60 seconds (60
 * x 1000000 = 60000000 microseconds).
 *
 * If you call to create a new connection with \ref vortex_connection_new
 * and remote peer doesn't responds within the period,
 * \ref vortex_connection_new will return with a non-connected vortex
 * connection.
 *
 * @param ctx The context where the operation will be performed.
 *
 * @param period Timeout value to be used.
 */
void               vortex_connection_timeout (VortexCtx * ctx,
					      long int    period)
{
	/* get current context */
	char      * value;
	
	/* check ctx */
	if (ctx == NULL)
		return;
	
	/* clear previous value */
	if (period == 0) {
		vortex_support_unsetenv ("VORTEX_SYNC_TIMEOUT");
		return;
	}

	/* set new value */
	value = axl_strdup_printf ("%ld", period);
	vortex_support_setenv ("VORTEX_SYNC_TIMEOUT", value);
	axl_free (value);

	/* make the next call to vortex_connection_get_timeout to
	 * recheck the value */
	ctx->connection_timeout_checked = false;

	return;
}

/** 
 * @brief Allows to tweak vortex connect timeout.
 * 
 * This function allows to set the timeout to use on a TCP connect.
 * Default timeout is TCP timeout.
 *
 * If you call to create a new connection with \ref vortex_connection_new
 * and connect does not succeed within the period
 * \ref vortex_connection_new will return with a non-connected vortex
 * connection.
 *
 * @param ctx The context where the operation will be performed.
 *
 * @param period Timeout value to be used. The value provided is
 * measured in seconds. 
 */
void               vortex_connection_connect_timeout (VortexCtx * ctx,
						      long int    period)
{
	/* get current context */
	char      * value;

	/* check reference received */
	if (ctx == NULL)
		return;
	
	/* clear previous value */
	if (period == 0) {
		vortex_support_unsetenv ("VORTEX_CONNECT_TIMEOUT");
		return;
	}

	/* set new value */
	value = axl_strdup_printf ("%ld", period);
	vortex_support_setenv ("VORTEX_CONNECT_TIMEOUT", value);
	axl_free (value);

	/* make the next call to vortex_connection_get_connect_timeout
	 * to recheck the value */
	ctx->connection_connect_timeout_checked = false;

	return;
}

/** 
 * @brief Allows to get current timeout set for \ref VortexConnection
 * synchronous operations.
 *
 * See also \ref vortex_connection_timeout.
 *
 * @return Current timeout configured. Returned value is measured in
 * microseconds (1 second = 1000000 microseconds).
 */
long int             vortex_connection_get_timeout (VortexCtx * ctx)
{
	/* get current context */
	long int      d_timeout   = ctx->connection_std_timeout;
	
	/* check reference */
	if (ctx == NULL) {
		/* this value must be synchrnozied by the value
		 * configured at vortex_connection_init. */
		return (60000000);
	} /* end if */

	/* check if we have used the current environment variable */
	if (! ctx->connection_timeout_checked) {
		ctx->connection_timeout_checked = true;
		d_timeout                       = vortex_support_getenv_int ("VORTEX_SYNC_TIMEOUT");
	} /* end if */

	if (d_timeout == 0) {
		/* no timeout definition done using default timeout 10 seconds */
		return ctx->connection_std_timeout;
	}else {
		/* update current std timeout */
		ctx->connection_std_timeout = d_timeout;
		
	} /* end if */

	/* return current value */
	return ctx->connection_std_timeout;
}

/** 
 * @brief Allows to get current timeout set for \ref VortexConnection
 * connect operation.
 *
 * See also \ref vortex_connection_connect_timeout.
 *
 * @return Current timeout configured. Returned value is measured in
 * microseconds (1 second = 1000000 microseconds).
 */
long int             vortex_connection_get_connect_timeout (VortexCtx * ctx)
{
	/* get current context */
	long int     d_timeout;

	/* check context recevied */
	if (ctx == NULL) {
		/* get the the default connect */
		return (60000000);
	} /* end if */
		
	d_timeout   = ctx->connection_connect_std_timeout;

	/* check if we have used the current environment variable */
	if (! ctx->connection_connect_timeout_checked) {
		ctx->connection_connect_timeout_checked = true;
		d_timeout = vortex_support_getenv_int ("VORTEX_CONNECT_TIMEOUT");
	} /* end if */

	if (d_timeout == 0) {
		/* no timeout definition done using default timeout 10 seconds */
		return ctx->connection_connect_std_timeout;
	}else {
		/* update current std timeout */
		ctx->connection_connect_std_timeout = d_timeout;
		
	} /* end if */

	/* return current value */
	return ctx->connection_connect_std_timeout;
}

/** 
 * @brief Allows to get current connection status
 *
 * This function will allow you to check if your vortex connection is
 * actually connected. You must use this function before calling
 * vortex_connection_new to check what have actually happen.
 *
 * You can also use vortex_connection_get_message to check the message
 * returned by the vortex layer. This may be useful on connection
 * errors.  The free_on_fail parameter can be use to free vortex
 * connection resources if this vortex connection is not
 * connected. This operation will be done by using \ref vortex_connection_close.
 *
 * 
 * @param connection the connection to get current status.
 *
 * @param free_on_fail if true the connection will be closed using
 * vortex_connection_close on not connected status.
 * 
 * @return current connection status for the given connection
 */
bool               vortex_connection_is_ok (VortexConnection * connection, bool     free_on_fail)
{
	bool result = false;

	/* check connection null referencing. */
	if  (connection == NULL) 
		return false;

	/* check for the socket this connection has */
	result = (connection->session < 0) || (! connection->is_connected);

	/* implement free_on_fail flag */
	if (free_on_fail && result) {
		vortex_connection_close (connection);
		return false;
	} /* end if */
	
	/* return current connection status. */
	return ! result;
}

/** 
 * @brief Returns actual message status for the given connection
 *
 * Return message associated to this vortex connection. The message
 * associated may be useful on connection errors. On an successful
 * connected vortex connection it will return "already connected".
 * 
 * @param connection the connection used to get message status.
 * 
 * @return a message about vortex connection status. You must not free
 * this value. Use vortex_connection_free.
 */
char             * vortex_connection_get_message (VortexConnection * connection)
{
	if (connection == NULL)
		return NULL;

	return connection->message;
}

/** 
 * @brief Allows to get the next channel error message stored in the
 * provided connection.
 *
 * Every time a channel creation attempt finish without the channel
 * created an error code and a textual diagnostic is returned by the
 * remote side. This function allows to get that messages. 
 *
 * The function returns true if a pending channel creation error
 * message was stored in the connection. You must provide reference to
 * store the content and the caller will own references returned. This
 * means you must deallocate memory returned by this function on
 * success.
 *
 * This function is mainly provided to check errors once a channel
 * creation fails. Here is an example:
 * \code
 * VortexChannel * channel;
 * char          * msg;
 * int             code;
 *
 * // attempt a channel creation (using conn)
 * channel = vortex_channel_new (....);
 *
 * if (channel == NULL) {
 *      // channel have failed 
 *      while (vortex_connection_pop_channel_error (conn, &code, &msg)) {
 *             // drop a error message
 *             printf ("Channel have failed, error was: code=%d, %s\n",
 *                     code, msg);
 *             // dealloc resources 
 *             axl_free (msg);
 *      } 
 * } 
 * \endcode
 * 
 * @param connection The connection where the error is expected to be stored.
 *
 * @param code The error found code. This reference is not optional,
 * you must provide it.
 *
 * @param msg The textual error received. This reference is not
 * optional, you must provide it.
 * 
 * @return true if an error message was pending to be retrieved,
 * otherwise false is returned. The function also returns false if
 * some argument is null. 
 */
bool                vortex_connection_pop_channel_error      (VortexConnection  * connection, 
							      int               * code,
							      char             ** msg)
{
	VortexChannelError * error;

	/* do not operate if null values are received */
	if (! (connection && code && msg))
		return false;

	/* lock the connection during operations */
	vortex_mutex_lock (&connection->pending_errors_mutex);

	/* clear values received */
	*code = 0;
	*msg  = NULL;

	if (axl_stack_is_empty (connection->pending_errors)) {
		/* unlock and return */
		vortex_mutex_unlock (&connection->pending_errors_mutex);

		/* no error to be reported */
		return false;
	} /* end if */

	/* get next error to report */
	error = axl_stack_pop (connection->pending_errors);
	*code = error->code;
	*msg  = error->msg;

	/* make the message reference to be owned by the caller */
	error->msg = NULL;
	__vortex_connection_free_channel_error (error);

	/* unlock */
	vortex_mutex_unlock (&connection->pending_errors_mutex);

	return true;
}

/** 
 * @internal Function used by vortex to store new error message
 * received on channel that were tried to be created at the connection
 * provided.
 * 
 * @param connection The connection where the error will be stored.
 *
 * @param code The code to store.
 *
 * @param msg The message to store, the value provided will be owned
 * by the connection and no copy will be allocated. This variable is
 * not optional.
 */
void                vortex_connection_push_channel_error     (VortexConnection  * connection, 
							      int                 code,
							      char              * msg)
{
	VortexChannelError * error;

	/* check reference received */
	if (connection == NULL || msg == NULL)
		return;
	
	/* lock the connection during operations */
	vortex_mutex_lock (&connection->pending_errors_mutex);

	/* create the value */
	error       = axl_new (VortexChannelError, 1);
	error->code = code;
	error->msg  = msg;

	/* push the data */
	axl_stack_push (connection->pending_errors, error);
	
	/* unlock */
	vortex_mutex_unlock (&connection->pending_errors_mutex);

	return;
}




/** 
 * @brief Frees vortex connection resources
 * 
 * Free all resources allocated by the vortex connection. You have to
 * keep in mind if <i>connection</i> is already connected,
 * vortex_connection_free will close all channels running on this and
 * also close the connection.
 *
 * Generally is not a good a idea to call this function. This is
 * because every connection created using the vortex API is registered
 * at some internal process (the vortex reader, sequencer and writer)
 * so they have references to created connection to do its job. 
 *
 * To close a connection properly call \ref vortex_connection_close.
 * 
 * @param connection the connection to free
 */
void               vortex_connection_free (VortexConnection * connection)
{
	VortexChannelError * error;
	VortexCtx          * ctx;
	
	if (connection == NULL)
		return;

	/* get a reference to the context reference */
	ctx = connection->ctx;

	vortex_log (VORTEX_LEVEL_DEBUG, "freeing connection id=%d", connection->id);

	/*
	 * NOTE: The order in which the channels and the channel pools
	 * are closed must be this way: first channels and the channel
	 * pools. Doing it other way will produce funny dead-locks.
	 */
	vortex_log (VORTEX_LEVEL_DEBUG, "freeing connection channels id=%d", connection->id);

	/* free channel resources */
	if (connection->channels) {
		/* call to notify remaining channels pending to be
		 * closed */
		vortex_hash_foreach2 (connection->channels, __vortex_connection_foreach_check_and_notify, connection, INT_TO_PTR(false));

		/* now remove */
		vortex_hash_destroy (connection->channels);
		connection->channels = NULL;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "freeing connection custom data holder id=%d", connection->id);
	/* free connection data hash */
	if (connection->data) {
		vortex_hash_destroy (connection->data);
		connection->data = NULL;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "freeing connection message id=%d", connection->id);

        /* free all resources */
	if (connection->message != NULL) {
		axl_free (connection->message);
		connection->message = NULL;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "freeing connection host id=%d", connection->id);

	/* free host and port */
	if (connection->host != NULL) {
		axl_free (connection->host);
		connection->host    = NULL;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "freeing connection port id=%d", connection->id);

	if (connection->port != NULL) {
		axl_free (connection->port);
		connection->port    = NULL;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "freeing connection profiles id=%d", connection->id);

	/* do not free remote profile list because they are handled by
	 * the greetings cache */
	connection->remote_supported_profiles = NULL;

	/* profile masks */
	axl_list_free (connection->profile_masks);
	connection->profile_masks = NULL;

	/* do not free features and localize because they are handled
	 * thourgh the cache */
	connection->features = NULL;
	connection->localize = NULL;

	vortex_log (VORTEX_LEVEL_DEBUG, "freeing connection channel pools id=%d", connection->id);
	/* free channel pools */
	if (connection->channel_pools) {
		vortex_hash_destroy (connection->channel_pools);
		connection->channel_pools = NULL;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "freeing connection channel mutex id=%d", connection->id);

	/* free mutex */
	vortex_mutex_destroy (&connection->channel_mutex);

	vortex_log (VORTEX_LEVEL_DEBUG, "freeing connection reference counting mutex id=%d", connection->id);

	/* release ref mutex */
	vortex_mutex_destroy (&connection->ref_mutex);

	vortex_log (VORTEX_LEVEL_DEBUG, "freeing connection operational mutex id=%d", connection->id);

	vortex_mutex_destroy (&connection->op_mutex);

	vortex_mutex_destroy (&connection->handlers_mutex);

	vortex_log (VORTEX_LEVEL_DEBUG, "freeing connection channel pools mutex id=%d", connection->id);
	vortex_mutex_destroy (&connection->channel_pool_mutex);

	/* free items from the stack */
	while (! axl_stack_is_empty (connection->pending_errors)) {
		/* pop error */
		error = axl_stack_pop (connection->pending_errors);

		/* free the error */
		axl_free (error->msg);
		axl_free (error);
	} /* end if */
	vortex_mutex_destroy (&connection->pending_errors_mutex);
	axl_stack_free (connection->pending_errors);

	vortex_log (VORTEX_LEVEL_DEBUG, "freeing/terminating connection id=%d", connection->id);

	/* close connection */
	if (( connection->close_session) && (connection->session != -1)) {
		/* it seems that this connection is  */
		shutdown (connection->session, SHUT_RDWR);
		vortex_close_socket (connection->session);
		connection->session = -1;
		vortex_log (VORTEX_LEVEL_DEBUG, "session socket closed");
	}


	/* release on close notification */
	axl_list_free (connection->on_close);
	connection->on_close = NULL;

	axl_list_free (connection->on_close_full);
	connection->on_close_full = NULL;

	/* free lists and mutex */
	axl_list_free (connection->add_channel_handlers);
	axl_list_free (connection->remove_channel_handlers);
	vortex_mutex_destroy (&connection->channel_update_mutex);

	/* free serverName */
	axl_free (connection->serverName);

	/* free connection */
	axl_free (connection);

	return;
}

/** 
 * @brief Returns the remote peer supported profiles
 * 
 * When a Vortex connection is opened, remote server sends a list of
 * BEEP supported profiles.
 *
 * This is necessary to be able to create new channels. The profile
 * selected for the channel to be created must be supported for both
 * sides.
 *
 * This function allows to get remote peer supported
 * profiles. This can be helpful to avoid connection to a remote BEEP
 * peers that actually doesn't support your profile.  
 *
 * You must free the returned axlList. As a example, you can use this
 * function as follows:
 * 
 * \code
 * axlList * profiles = NULL;
 * int      iterator = 0;
 *
 * // get a list of profiles supported
 * profiles = vortex_connection_get_remote_profiles (connection);
 * printf ("profiles for this peer: %d\n", axl_list_length (profiles));
 *
 * // for each item do
 * while (iterator < axl_list_length (profiles)) {
 *     // show a message 
 *     printf ("  %d) %s\n", iterator, (char *) axl_list_get_nth (profiles, iterator));
 *
 *     // update iterator 
 *     iterator++;
 *
 * } // end while 
 * 
 * // free the list when no longer needed
 * axl_list_free (profiles);
 * \endcode
 * 
 * @param connection the connection to get remote peer profiles.
 * 
 * @return An axlList containing each element a uri identifying a
 * remote peer profile. Free the list returned with axl_list_free.
 */
axlList            * vortex_connection_get_remote_profiles (VortexConnection * connection)
{

	axlList    * result;
	int          iterator;
	const char * uri;

	/* check the connection and its connection status */
	if (connection == NULL || ! connection->is_connected)
		return NULL;

	/* create a list */
	result = axl_list_new (axl_list_always_return_1, axl_free);

	/* for each uri installed */
	iterator = 0;
	while (iterator < axl_list_length (connection->remote_supported_profiles)) {

		/* get the uri */
		uri = axl_list_get_nth (connection->remote_supported_profiles, iterator);
		
		/* check if the profile is filtered providing as
		 * channel num -1, serverName NULL, and profile
		 * content NULL */
		if (! vortex_connection_is_profile_filtered (connection, -1, uri, NULL, NULL)) {
			/* profile is not filtered, add to the result */
			axl_list_append (result, axl_strdup (uri));
		} /* end if */
		
		/* update the iterator */
		iterator++;
		
	} /* end while */
	
	/* return list created */
	return result;
}

/** 
 * @internal Definition to store all mask installed on a connection.
 */
typedef struct _VortexMaskNode {
	int                   mask_id;
	VortexProfileMaskFunc mask;
	axlPointer            user_data;
} VortexMaskNode;

/** 
 * @brief Allows to configure a profile mask, an external handler
 * which is executed to check if a profile must be showed in the
 * greetings process.
 *
 * This function is not thread safe in the following sense: all mask
 * installation must take place from the same thread, ensuring no more
 * operation is taking place at the same time, modifying current mask
 * configuration. Then, using current mask configuration could be done
 * from several threads at the same time.
 *
 * Once you have installed all masks required, the function will use
 * \ref vortex_connection_is_profile_filtered to check if some profile
 * could be used.
 * 
 * @param connection The connection that will be configured with the
 * provided mask.
 *
 * @param mask The mask, a user space defined function which is
 * executed to figure out if a particular profile must be showed. 
 *
 * @param user_data User defined data to be provided to the mask
 * function once executed.
 * 
 * @return A unique identifier to refer to the mask installed. The
 * function returns -1 if the function receives a null reference for
 * the connection and the mask function.
 */
int                 vortex_connection_set_profile_mask       (VortexConnection      * connection,
							      VortexProfileMaskFunc   mask,
							      axlPointer              user_data)
{
	VortexMaskNode * node;

	axl_return_val_if_fail (connection, -1);
	axl_return_val_if_fail (mask, -1);

	/* create and store the mask */
	node            = axl_new (VortexMaskNode, 1);
	node->mask_id   = axl_list_length (connection->profile_masks);
	node->mask      = mask;
	node->user_data = user_data;
	
	/* install the profile mask */
	axl_list_append (connection->profile_masks, node);

	/* mask created and installed. return the unique id */
	return node->mask_id;
}


/** 
 * @brief Checks if a profile could be used, according to the current
 * masks installed on the connection.
 *
 * NOTE: The function return the result of applying the uri to all
 * masks found. It doesn't check if the profile is already used by
 * some channel.
 * 
 * @param connection The connection that is being checked for the
 * particular profile.
 *
 * @param channel_num Optional parameter used to notify the channel
 * num requested provided at the start channel stage. You can safely
 * provide a -1 value if you are only checking if a particular profile
 * is being filtered on the particular connection.
 *
 * @param uri The profile uri to check.
 *
 * @param profile_content Optional parameter used to notify the
 * profile content provided at the start channel stage. You can safely
 * provide a NULL value if you are only checking if a particular
 * profile is being filtered on the particular connection.
 *
 * @param serverName Optional parameter used to notify the serverName
 * provided at the start channel stage. You can safely provide a NULL
 * value if you are only checking if a particular profile is being
 * filtered on the particular connection.
 * 
 * @return true if the if the profile is filtered, otherwise false is
 * returned.
 */
bool                vortex_connection_is_profile_filtered    (VortexConnection      * connection,
							      int                     channel_num,
							      const char            * uri,
							      const char            * profile_content,
							      const char            * serverName)
{
	int              iterator = 0;
	VortexMaskNode * node;

	/* check received data */
	if (connection == NULL || uri == NULL)
		return false;

	/* check all mask installed */
	while (iterator < axl_list_length (connection->profile_masks)) {
		
		/* get the mask reference */
		node = axl_list_get_nth (connection->profile_masks, iterator);

		/* check if the mask filter the provided profile */
		if (node->mask (connection, channel_num, uri, profile_content, serverName, node->user_data)) {
			/* uri filtered, report */
			return true;
		}

		/* update the iterator */
		iterator++;
		
	} /* end while */

	/* no mask have filtered the uri */
	return false;
}

/** 
 * @brief Allows to check if the given profile is supported by the
 * remote peer.
 *
 * This function allows to check if the profile identified by
 * <i>uri</i> is supported by remote peer connected to by <i>connection</i>.
 *
 * You can use this function to check, before creating a channel, if
 * remote peer will support this profile.
 * 
 * @param connection the connection to check.
 * @param uri the profile identified by the string uri to check.
 * 
 * @return true if the profile is supported by remote peer or false if not.
 */
bool               vortex_connection_is_profile_supported (VortexConnection * connection, 
							   const char       * uri)
{
	bool        result;
	VortexCtx * ctx;

	/* check reference received */
	if (connection == NULL || uri == NULL)
		return false;

	/* get a reference to the context */
	ctx = connection->ctx;

	if (!connection->is_connected) {
		vortex_log (VORTEX_LEVEL_CRITICAL, 
		       "trying to get supported profile in a non-connected session");
		return false;
	}

	/* lock the connection while getting the list */
	vortex_mutex_lock (&connection->op_mutex);

	/* check if the profile is found */
	result = (axl_list_lookup (connection->remote_supported_profiles, axl_list_find_string, (axlPointer) uri) != NULL);

	/* unlock and return */
	vortex_mutex_unlock (&connection->op_mutex);
	return result;
}

/** 
 * @brief Check if a channel is already created on the given connection. 
 *
 * This function allows to check if a channel is already created
 * over this connection (or session).  Channel number must be a
 * positive integer starting from 0. Channel 0 is always opened and
 * exists until connection (or session) is closed.
 * 
 * @param connection vortex connection to check channel existence
 * @param channel_num  channel number to check
 * 
 * @return true if channel identified by <i>channel_num</i> exists,
 * otherwise false.
 */
bool               vortex_connection_channel_exists       (VortexConnection * connection, int  channel_num)
{
	bool     result;

	if (channel_num < 0 || connection == NULL || ! connection->is_connected )
		return false;

	/* channel 0 always exists, and cannot be closed. It's closed
	 * when connection (or session) is closed */
	if (channel_num == 0) 
		return true;
	
	result = (vortex_hash_lookup (INT_TO_PTR (connection->channels), 
				      INT_TO_PTR (channel_num)) != NULL);
	return result;
}

/** 
 * @brief Allows to get the number of channels installed on the
 * provided connection.
 * 
 * @param connection The connection that is required to return the
 * number of channels installed.
 * 
 * @return Returns channels installed or -1 if it fails. By default
 * all connections have at least one administrative BEEP channel 0.
 */
int                 vortex_connection_channels_count         (VortexConnection * connection)
{
	if (connection == NULL)
		return -1;
	
	/* return number of channels */
	return vortex_hash_size (connection->channels);
}

/** 
 * @brief Allows to perform an iterator over all channels created
 * inside the given connection.
 *
 * Allows an application to iterate over all channel created over this
 * session. This allows to check or do some operation over the session
 * channels.  
 *
 * The channel iteration is made following the channel number
 * order. First low-numbered channel following high-numbered ones.
 *
 * Example:
 *
 * \code
 * void my_function_foreach (axlPointer key, axlPointer value, axlPointer user_data)
 * {
 *      VortexChannel * channel = value;
 *    
 *      // do some operation with the channel.
 * }
 *
 * void my_function (VortexConnection * connection) 
 * {
 *
 *      // do a channel foreach on this connection
 * 	vortex_connection_foreach_channel (connection, my_function_foreach, NULL);
 *
 *      return;
 * }
 * \endcode
 * 
 * @param connection the connection where channels will be iterated.
 * @param func the function to apply for each channel found
 * @param user_data user data to be passed into the foreach function.
 *
 * @return true if the foreach operation was completed, otherwise
 * false is returned.
 */
bool            vortex_connection_foreach_channel        (VortexConnection * connection,
							  axlHashForeachFunc func,
							  axlPointer         user_data)
{
	VortexCtx * ctx;

	/* do not operate */
	v_return_val_if_fail (connection, false);
	v_return_val_if_fail (func, false);

	/* get context */
	ctx = connection->ctx;

	/* try to ref the connection to avoid loosing the reference
	 * during the process. */
	if (! vortex_connection_ref (connection, "foreach-channel")) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Failed to foreach because ref operation over connection id=%d (%p) have failed",
			    connection->id, connection);
		return false;
	}

	/* perform the foreach operation */
	vortex_hash_foreach (connection->channels, func, user_data);

	/* unref connection increased */
	vortex_connection_unref (connection, "foreach-channel");

	return true;
}


/** 
 * 
 * @brief Returns the next channel number free to be used over this
 * session.
 *
 * This function iterates over the allowed channel number range, that
 * is: 1..2147483647 including both limits, stepping over channel
 * number 0, which always exists during the connection live.
 *
 * Channel numbers assigned automatically depends on the connection
 * role. In the case the connection role is "initiator", next channel
 * number returned will be odd-numbered. In the case the connection
 * role is "listener", next channel number will be even-numbered.
 *
 *
 * @param connection the connection where the channel is going to be
 * created
 * 
 * @return the channel number to use or -1 if fail.
 */
int                vortex_connection_get_next_channel     (VortexConnection * connection)
{
	int         channel_num = -1;

	/* check reference and connection status before oper */
	if (connection == NULL || ! connection->is_connected)
		return -1;

	/* create a critical section until found next channel to
	 * use */
	vortex_mutex_lock (&connection->channel_mutex);
	
	while (true) {
		/* get the next channel number to use */
		connection->last_channel = ((connection->last_channel + 2) % MAX_CHANNELS_NO);

		/* avoid to return 0 as valid channel number to be used */
		if (connection->last_channel == 0) {
			connection->last_channel = (vortex_connection_get_role (connection) == VortexRoleInitiator) ? 1 : 2;
		}

		/* check if channel exists */
		if (!vortex_connection_channel_exists (connection, connection->last_channel)) {
			channel_num = connection->last_channel;
			break;
		}
	}
	/* unblock critical section until found next channel to use */
	vortex_mutex_unlock (&connection->channel_mutex);
	return channel_num;
}


/** 
 * @brief Returns a reference to the channel identified by <i>channel_num</i> on this
 * connection (or vortex session).
 * 
 * @param connection the connection to look for the channel.
 * @param channel_num the channel_num identifier.
 * 
 * @return the VortexChannel reference or NULL if it doesn't exists.
 */
VortexChannel    * vortex_connection_get_channel          (VortexConnection * connection, int  channel_num)
{
	VortexChannel * channel;
	VortexCtx     * ctx;

	/* check values returned */
	if (connection == NULL || ! connection->is_connected || channel_num < 0)
		return NULL;

	/* get a reference to the ctx */
	ctx = connection->ctx;

	/* channel 0 always exists, and cannot be closed. It's closed
	 * when connection (or session) is closed */
	channel = vortex_hash_lookup (INT_TO_PTR (connection->channels), INT_TO_PTR(channel_num));
	
	if (channel == NULL) 
		vortex_log (VORTEX_LEVEL_DEBUG, "failed to get channel=%d", channel_num);
	return channel;
}

/** 
 * @internal Function supporting vortex_connection_get_channel_by_uri.
 */
bool __vortex_connection_get_by_uri_foreach (axlPointer key, axlPointer data, axlPointer user_data, axlPointer user_data2)
{
	VortexChannel  * channel  = data;
	VortexChannel ** result   = user_data;
	const char     * profile  = user_data2;

	/* check if the channel is running the profile requested */
	if (vortex_channel_is_running_profile (channel, profile)) {
		/* channel found, stop the foreach operation and
		 * update the reference */
		*result = channel;
		return true;
	} /* end if */

	/* item not found */
	return false;
}

/** 
 * @brief Allows to get the first channel ocurrence running the
 * profile provided.
 * 
 * @param connection The connection where lookup for channels. 
 *
 * @param profile The profile to use to find the first channel running
 * it.
 * 
 * @return A reference to the channel or NULL if it fails.
 */
VortexChannel     * vortex_connection_get_channel_by_uri     (VortexConnection * connection,
							      const char       * profile)
{
	VortexChannel * result = NULL;

	/* check reference */
	if (connection == NULL || profile    == NULL)
		return NULL;

	/* get the first channel running the profile provided by
	 * foreaching all items */
	vortex_hash_foreach2 (connection->channels, __vortex_connection_get_by_uri_foreach, &result, (axlPointer) profile);

	/* return the channel found */
	return result;
}

/** 
 * @internal Function supporting vortex_connection_get_channel_by_func.
 */
bool __vortex_connection_get_by_func_foreach (axlPointer key, 
					      axlPointer data, 
					      /* user defined data */
					      axlPointer user_data, 
					      axlPointer user_data2,
					      axlPointer user_data3)
{
	VortexChannel           * channel  = data;
	VortexChannel          ** result   = user_data;
	VortexChannelSelector     selector = user_data2;

	/* check if the channel is running the profile requested */
	if (selector (channel, user_data3)) {
		/* channel found, stop the foreach operation and
		 * update the reference */
		*result = channel;
		return true;
	} /* end if */

	/* item not found */
	return false;
}

bool __vortex_connection_count_channel_foreach (axlPointer key, 
						axlPointer data, 
						/* user defined data */
						axlPointer user_data, 
						axlPointer user_data2,
						axlPointer user_data3)
{
	VortexChannel           * channel  = data;
	int                     * result   = user_data;
	const char              * profile  = user_data2;

	/* update the count if channel found */
	if (axl_cmp (vortex_channel_get_profile (channel), profile))
		(*result)++;

	/* item never found */
	return false;
}
						

/** 
 * @brief Allows to select a channel from the set of channels created
 * on the provided connection, using a function that acts as a
 * selector.
 *
 * This function allows to provide a finer control while selecting
 * channels already created on a connection. Similiar to the function
 * provided by \ref vortex_connection_get_channel_by_uri, this
 * function allows implement more elaborated selection patters,
 * especiall if it is found the same profile running several times
 * (for examcple, xml-rpc with different resources).
 *
 * The idea behind this function is that you provide a function which
 * is called for each channel found on the connection until that
 * function returns true. For example:
 *
 * \code
 * bool select_channel (VortexChannel * channel, axlPointer user_data)
 * {
 *       // implement here some selection pattern and return true
 *       // to select
 *       return true;
 * }
 *
 * // call to search using the function
 * channel = vortex_connection_get_channel_by_func (connection,
 *                                                  select_channel,
 *                                                  NULL);
 * \endcode
 *
 * @param connection The connection where the selection will be performed.
 *
 * @param selector The function to be called for each channel found on the
 * connection.
 *
 * @param user_data A reference to user defined pointer to be passed
 * to the function (func parameter).
 *
 * @return A reference to the channel selected or NULL if it fails.
 */
VortexChannel    * vortex_connection_get_channel_by_func (VortexConnection     * connection,
							  VortexChannelSelector  selector,
							  axlPointer             user_data)
{
	VortexChannel * result = NULL;

	/* check reference received */
	if (connection == NULL || selector == NULL)
		return NULL;

	/* get the first channel running the profile provided by
	 * foreaching all items */
	vortex_hash_foreach3 (connection->channels, __vortex_connection_get_by_func_foreach, &result, selector, user_data);

	/* return the channel found */
	return result;
}

/** 
 * @brief Allows to get the number of channels running the profile
 * provided.
 * 
 * @param connection The connection where the operation will be performed.
 *
 * @param profile The profile that is being checked. Channels running
 * this profile will be counted.
 * 
 * @return The number of channels running the profile or 0 if it fails
 * or no channel is running the profile provided.
 */
int                 vortex_connection_get_channel_count      (VortexConnection     * connection,
							      const char           * profile)
{
	int result = 0;

	/* check reference received */
	if (connection == NULL || profile == NULL)
		return 0;

	/* get the first channel running the profile provided by
	 * foreaching all items */
	vortex_hash_foreach3 (connection->channels, __vortex_connection_count_channel_foreach, &result, (axlPointer) profile, NULL);

	/* return the channel found */
	return result;
}

/** 
 * @brief Returns the socket used by this VortexConnection object.
 * 
 * @param connection the connection to get the socket.
 * 
 * @return the socket used or -1 if fail
 */
VORTEX_SOCKET    vortex_connection_get_socket           (VortexConnection * connection)
{
	/* check reference received */
	if (connection == NULL)
		return -1;

	return connection->session;
}

/** 
 * @brief Allows to configure what to do with the underlying socket
 * connection when the \ref VortexConnection is closed.
 *
 * This function could be useful if it is needed to prevent from
 * vortex library to close the underlying socket.
 * 
 * @param connection The connection to configure.
 *
 * @param action true if the given connection have to be close once a
 * Vortex Connection is closed. false to avoid the socket to be
 * closed.
 */
void                vortex_connection_set_close_socket       (VortexConnection * connection, 
							      bool     action)
{
	if (connection == NULL)
		return;
	connection->close_session = action;
	return;
}

/** 
 * @brief Adds a VortexChannel into an existing VortexConnection. 
 *
 * The channel to be added must not exists inside <i>connection</i>,
 * otherwise an error will be produced and channel will not be added.
 *
 * This function only adds the data structure which represents a
 * channel. It doesn't make any work about beep channel starting or
 * something similar.  
 *
 * Mainly, this function is only useful for internal vortex library
 * purposes.
 * 
 * @param connection the connection where channel will be added.
 * @param channel the channel to add.
 */
void               vortex_connection_add_channel          (VortexConnection * connection, 
							   VortexChannel * channel)
{
	VortexChannel * _channel;
	VortexCtx     * ctx;
	
	/* perform some aditional checks */
	if (connection == NULL || channel == NULL || connection->channels == NULL || connection->role == VortexRoleMasterListener)
		return;

	/* get a reference to the context */
	ctx = connection->ctx;
	    
	/* lock channel hash */
	vortex_mutex_lock (&connection->channel_mutex);

	/* check if channel doesn't exists on this connection */
	_channel = vortex_hash_lookup (connection->channels, INT_TO_PTR (vortex_channel_get_number (channel)));
	if (_channel != NULL) {
		vortex_mutex_unlock (&connection->channel_mutex);
		vortex_log (VORTEX_LEVEL_CRITICAL, "trying to add a channel on a connection which already have this channel");
		return;
	}

	/* insert new channel on this connection */
	vortex_hash_replace (connection->channels, 
			     INT_TO_PTR (vortex_channel_get_number (channel)),  
			     channel);

	/* make channel to be on state connected */
	__vortex_channel_set_connected (channel);

	/* unlock channel hash */
	vortex_mutex_unlock (&connection->channel_mutex);

	/* check and notify the channel to be removed. */
	__vortex_connection_check_and_notify (connection, channel, true);

	return;
}

/** 
 * @brief Removes the given channel from this connection. 
 * 
 * @param connection the connection where the channel will be removed.
 * @param channel the channel to remove from the connection.
 */
void               vortex_connection_remove_channel       (VortexConnection * connection, 
							   VortexChannel    * channel)
{
	int         channel_num;
	VortexCtx * ctx;

	/* check reference received */
	if (connection == NULL || channel == NULL || connection->channels == NULL)
		return;

	/* get a reference to the context */
	ctx = connection->ctx;

	/* get channel number */
	channel_num = vortex_channel_get_number (channel);

	vortex_log (VORTEX_LEVEL_DEBUG, "removing channel id=%d", channel_num);

	/* check and notify the channel to be removed. */
	__vortex_connection_check_and_notify (connection, channel, false);

	/* remove the channel */
	vortex_hash_remove (connection->channels, INT_TO_PTR (channel_num));
	
	return;
}

/** 
 * @brief Returns the actual host this connection is connected to.
 *
 * In the case the connection you have provided have the \ref
 * VortexRoleMasterListener role (\ref vortex_connection_get_role),
 * that is, listener connections that are waiting for connections, the
 * function will return the actual host used by the listener.
 *
 * You must not free returned value.  If you do so, you will get
 * unexpected behaviors.
 * 
 * 
 * @param connection the connection to get host value.
 * 
 * @return the host the given connection is connected to or NULL if something fail.
 */
const char         * vortex_connection_get_host             (VortexConnection * connection)
{
	if (connection == NULL)
		return NULL;

	return connection->host;
}

/** 
 * @brief  Returns the connection unique identifier.
 *
 * The connection identifier is a unique integer assigned to all
 * connection created under Vortex Library. This allows Vortex programmer to
 * use this identifier for its own application purposes
 *
 * @param connection the unique integer identifier for the given connection.
 * 
 * @return the unique identifier.
 */
int                vortex_connection_get_id               (VortexConnection * connection)
{
	if (connection == NULL)
		return -1;

	return connection->id;
}

/** 
 * @brief Allows to get the serverName under which the connection is
 * acting.
 *
 * During the BEEP session, the first channel created under a provided
 * serverName attribute is meaningful for the rest of the
 * session. This means that the connection gets flaged with the
 * serverName under which is acting. 
 * 
 * @param connection The connection that is required to return its
 * server name value.
 * 
 * @return The serverName value or NULL if no server name was
 * configured.
 */
const char        * vortex_connection_get_server_name        (VortexConnection * connection)
{
	if (connection == NULL)
		return NULL;
	
	/* return current serverName configured */
	return connection->serverName;
}

/** 
 * @internal Function that allows to configure the serverName for the
 * first caller. Rest of the callers will fail to set the name (doing
 * nothing the function) if the serverName is found to be configured.
 * 
 * @param connection The connection to configure its serverName.
 * @param serverName The server name value to configured.
 */
void                vortex_connection_set_server_name         (VortexConnection * connection,
							       const char       * serverName)
{
	/* check if the connection is null or the serverName is
	 * null */
	if (connection == NULL || serverName == NULL || connection->serverName != NULL)
		return;

	/* configure serverName */
	connection->serverName = axl_strdup (serverName);
	
	return;
}

/** 
 * @brief Returns the actual port this connection is connected to.
 *
 * In the case the connection you have provided have the \ref
 * VortexRoleMasterListener role (\ref vortex_connection_get_role),
 * that is, a listener that is waiting for connections, the
 * function will return the actual port used by the listener.
 *
 * @param connection the connection to get the port value.
 * 
 * @return the port or NULL if something fails.
 */
const char        * vortex_connection_get_port             (VortexConnection * connection)
{
	
	/* check reference received */
	if (connection == NULL)
		return NULL;

	return connection->port;
}

/** 
 * @internal 
 *
 * Function used to perform on close invocation
 * 
 * @param conneciton The connection where all on close handlers will
 * be notified.
 */
void __vortex_connection_invoke_on_close (VortexConnection * connection, bool     is_full)
{
	int                            iterator = 0;
	VortexConnectionOnClose        on_close_handler;
	VortexConnectionOnCloseData  * handler;

	/* invoke full */
	if (is_full) {
		/* iterate over all full handlers and invoke them */
		while (iterator < axl_list_length (connection->on_close_full)) {
			/* get a reference to the handler */
			handler = axl_list_get_nth (connection->on_close_full, iterator);

			/* invoke */
			handler->handler (connection, handler->data);
			
			/* get next iterator */
			iterator++;
		} /* end if */

	} else {

		/* iterate over all full handlers and invoke them */
		while (iterator < axl_list_length (connection->on_close)) {

			/* get a reference to the handler */
			on_close_handler = axl_list_get_nth (connection->on_close, iterator);

			/* invoke */
			on_close_handler (connection);

			/* get next iterator */
			iterator++;
		} /* end if */

	} /* if */

	return;
} 

/** 
 * @brief Makes its underlaying connection to be closed, flagging the
 * connection as non connected (without deallocating resources
 * associated to the connection).
 *
 * This function is callable over and over again on the same
 * connection. The first time the function is called sets to false
 * connection state and its error message. It also close the BEEP
 * session and sets its socket to -1 in order to make it easily to
 * recognize for other functions.
 *
 * You can use this function to perform a forced connection close
 * (without meeting BEEP requirements while closing sessions). Once
 * the connection have been closed by using the function, vortex
 * library will detect its status, removing all its references to the
 * connection (\ref vortex_connection_unref). 
 *
 * The function do not updates the reference counting. The caller will
 * still have to do all required calls to \ref vortex_connection_unref
 * (as much as times called to \ref vortex_connection_ref).
 * 
 * @param connection The connection to be shutted down.
 */
void    vortex_connection_shutdown           (VortexConnection * connection)
{
	/* call to internal set not connected */
	__vortex_connection_set_not_connected (connection, "(vortex connection shutdown)");
	return;
	
} /* end vortex_connection_shutdown */

/** 
 * @brief Allows to configure a handler which is executed once a
 * channel is added to the provided connection. This could be used to
 * implement some init policy once the channel is added to the
 * connection. The function support adding several handlers.
 * 
 * @param connection The connection where the handler is going to be
 * installed.
 *
 * @param added_handler The handler to be called once the even
 * happens.
 *
 * @param user_data A reference to the user data to be passed to the
 * function.
 */
void                vortex_connection_set_channel_added_handler   (VortexConnection                * connection,
								   VortexConnectionOnChannelUpdate   added_handler,
								   axlPointer                        user_data)
{
	VortexChannelStatusUpdate * update;

	/* check parameters received */
	if (connection == NULL || added_handler == NULL)
		return;

	/* lock the connection */
	vortex_mutex_lock (&connection->channel_update_mutex);

	/* add the handler */
	update               = axl_new (VortexChannelStatusUpdate, 1);
	update->handler      = added_handler;
	update->handler_data = user_data;

	/* check the list and add the item */
	if (connection->add_channel_handlers == NULL)
		connection->add_channel_handlers = axl_list_new (axl_list_always_return_1, axl_free);
	axl_list_add (connection->add_channel_handlers, update);

	/* unlock the connection */
	vortex_mutex_unlock (&connection->channel_update_mutex);

	return;
}

/** 
 * @brief Allows to configure a handler which is executed once a
 * channel is removed from the provided connection. This could be used
 * to implement some init policy once the channel is removed from the
 * connection. The function support adding several handlers to be
 * called.
 * 
 * @param connection The connection where the handler is going to be
 * installed.
 *
 * @param removed_handler The handler to be called once the even
 * happens.
 *
 * @param user_data A reference to the user data to be passed to the
 * function.
 */
void                vortex_connection_set_channel_removed_handler  (VortexConnection                * connection,
								    VortexConnectionOnChannelUpdate   removed_handler,
								    axlPointer                        user_data)
{
	VortexChannelStatusUpdate * update;
	VortexCtx                 * ctx;

	/* check parameters received */
	if (connection == NULL || removed_handler == NULL)
		return;

	/* get a reference to the context */
	ctx = connection->ctx;

	/* lock the connection */
	vortex_mutex_lock (&connection->channel_update_mutex);

	/* add the handler */
	update               = axl_new (VortexChannelStatusUpdate, 1);
	update->handler      = removed_handler;
	update->handler_data = user_data;
	vortex_log (VORTEX_LEVEL_DEBUG, "Configure on channel removed handler at: 0x%x",
		    removed_handler);

	/* check the list and add the item */
	if (connection->remove_channel_handlers == NULL)
		connection->remove_channel_handlers = axl_list_new (axl_list_always_return_1, axl_free);
	axl_list_add (connection->remove_channel_handlers, update);

	/* unlock the connection */
	vortex_mutex_unlock (&connection->channel_update_mutex);

	return;
}

/** 
 * @brief Allows to configure a handler which is called for each
 * connection created. The function supports configuring several handlers.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param notify_new The function to be called to produce the
 * notification.
 *
 * @param user_data User defined data which is provided to the handler
 * once executed.
 */
void                vortex_connection_notify_new_connections       (VortexCtx                       * ctx,
								    VortexConnectionNotifyNew         notify_new,
								    axlPointer                        user_data)
{
	/* get current context */
	VortexConnectionNotifyData * data;

	v_return_if_fail (notify_new);

	/* get a reference to the data */
	data            = axl_new (VortexConnectionNotifyData, 1);
	data->handler   = notify_new;
	data->user_data = user_data;

	/* lock mutex */
	vortex_mutex_lock (&ctx->connection_new_notify_mutex);

	/* create the list if it is not defined */
	if (ctx->connection_new_notify_list == NULL)
		ctx->connection_new_notify_list = axl_list_new (axl_list_always_return_1, axl_free);

	/* store data */
	axl_list_add (ctx->connection_new_notify_list, data);

	/* unlock mutex */
	vortex_mutex_unlock (&ctx->connection_new_notify_mutex);

	return;
}

/** 
 * @internal Function used to perform notifications for a connection
 * created.
 */
void                vortex_connection_notify_created               (VortexConnection                * conn)
{
	/* get current context */
	VortexCtx                  * ctx;
	int                          iterator;
	VortexConnectionNotifyData * data;

	/* do not notify if the connection is not running */
	if (! vortex_connection_is_ok (conn, false))
		return;

	/* get a reference to the context */
	ctx = conn->ctx;
	
	/* lock mutex */
	vortex_mutex_lock (&ctx->connection_new_notify_mutex);
	
	/* for each chandler stored */
	iterator = 0;
	while (iterator < axl_list_length (ctx->connection_new_notify_list)) {

		/* get data associated */
		data = axl_list_get_nth (ctx->connection_new_notify_list, iterator);

		/* call to notify */
		data->handler (conn, data->user_data);

		/* next iterator */
		iterator++;
	} /* end while */

	/* unlock mutex */
	vortex_mutex_unlock (&ctx->connection_new_notify_mutex);	

	return;
}

/** 
 * @internal
 * @brief Sets the given connection the not connected status.
 *
 * This internal vortex function allows library to set connection
 * status to false for a given connection. This function should not be
 * used by vortex library consumers.
 *
 * This function is callable over and over again on the same
 * connection. The first time the function is called sets to false
 * connection state and its error message. It also close the BEEP
 * session and sets its socket to -1 in order to make it easily to
 * recognize for other functions.
 *
 * It doesn't make a connection unref. This actually done by the
 * Vortex Library internal process once they detect that the given
 * connection is not connected. 
 *
 * NOTE: next calls to function will lose error message.
 * 
 * @param connection the connection to set as.
 * @param message the new message to set.
 */
void           __vortex_connection_set_not_connected (VortexConnection * connection, 
						      const char       * message)
{
	VortexCtx * ctx;

	/* check reference received */
	if (connection == NULL || message == NULL)
		return;

	/* get a reference to the context */
	ctx = connection->ctx;

	/* set connection status to false if weren't */
	vortex_mutex_lock (&connection->op_mutex);

	if (connection->is_connected) {
		vortex_log (VORTEX_LEVEL_DEBUG, "flagging the connection as non-connected");
		connection->is_connected = false;

		/* renew the message */
		if (connection->message)
			axl_free (connection->message);
		connection->message = axl_strdup (message);

		/* unlock now the op mutex is not blocked */
		vortex_mutex_unlock (&connection->op_mutex);

		/* check to invoke on close handler */
		if (connection->on_close != NULL) {
			/* invoking on close handler */
			__vortex_connection_invoke_on_close (connection, false);
		}

		/* check for the close handler full definition */
		if (connection->on_close_full != NULL) {
			/* invokin on close handler full */ 
			__vortex_connection_invoke_on_close (connection, true);
		}

		/* close socket connection if weren't  */
		if (( connection->close_session) && (connection->session != -1)) {
			vortex_log (VORTEX_LEVEL_DEBUG, "closing connection id=%d to %s:%s", 
			       connection->id,
			       connection->host, connection->port);
			shutdown (connection->session, SHUT_RDWR); 
			vortex_close_socket (connection->session);  
			connection->session      = -1;
			vortex_log (VORTEX_LEVEL_DEBUG, "closing session id=%d and set to be not connected",
			       connection->id);
 	        } /* end if */

		return;
	}

	vortex_mutex_unlock (&connection->op_mutex);

	return;
}


/** 
 * @internal
 *
 * @brief The function which actually send the data
 *
 * This function helps the vortex writer to do a sending round over
 * this connection. Because there are several channels over the same
 * connection sending frames, this function ensure a package for every
 * channel is sent on every round. This allows to avoid channel or
 * session starvation. 
 *
 * This function is executed from \ref
 * vortex_connection_do_a_sending_round as a foreach channel for a
 * given connection.
 * 
 * @param key   
 * @param value a channel to check if packets to be sent are waiting
 * @param user_data 
 */
bool  __vortex_connection_one_sending_round (axlPointer key,
					     axlPointer value,
					     axlPointer user_data)
{
	/* deprecated function */
	return false;
}

/**
 * @internal
 * @brief Helper function to Vortex Writer process
 * 
 * This function must be not used by vortex library consumer. This
 * function is mainly used by the vortex writer process to signal a
 * connection to do a sending round. 
 *
 * A sending round means to check every channel queue to see if have
 * pending frames to send. If have pending frames to be sent, the next
 * one to is sent. This tries to avoid a channel consuming all the
 * vortex connection bandwidth.
 * 
 * @param connection a connection where a sending round robin will
 * performed.
 * 
 * @return  it returns how message frame have been sent or 0 if
 * no message was sent.
 */
int                vortex_connection_do_a_sending_round (VortexConnection * connection)
{
	VortexCtx * ctx;
	int         messages = 0;

	/* check connection status */
	if (! vortex_connection_is_ok (connection, false))
		return 0;
	
	/* get a reference to the ctx */
	ctx = connection->ctx;

	/* foreach channel, check if we have something to send */
	vortex_hash_foreach (connection->channels, 
			     __vortex_connection_one_sending_round, &messages);

	vortex_log (VORTEX_LEVEL_DEBUG, "message sent for this connection id=%d: %d", 
		    vortex_connection_get_id (connection),
		    messages);

	return messages;
}

/** 
 * @brief Sets user defined data associated with the given connection.
 *
 * This function allows to store arbitrary data associated to the
 * given connection. Data stored in insided by the provided key,
 * allowing to retrieve the information using: \ref
 * vortex_connection_get_data.
 *
 * If the value provided is NULL, this will be considered as a
 * removing request for the given key and its associated data.
 * 
 * See also \ref vortex_connection_set_data_full function. It is an
 * alternative API provided to allow providing destroy handlers for
 * key and data stored.
 *
 *
 * @param connection The connection to set data.
 * @param key The string key indexing the data.
 * @param value The value to be stored.
 */
void               vortex_connection_set_data               (VortexConnection * connection,
							     const char       * key,
							     axlPointer         value)
{
	/* use the full version so all source code is supported in one
	 * function. */
	vortex_connection_set_data_full (connection, (axlPointer) key, value, NULL, NULL);
	return;
}

/** 
 * @brief Allows to store user space data into the connection like
 * \ref vortex_connection_set_data does but configuring functions to
 * be called once required to deallocate data stored.
 *
 * While storing user defined data into the connection it could be
 * necessary to also define destroy functions for the value stored and
 * the key stored. This allows to not worry about to free those data
 * (including the key) once the connection is dropped.
 *
 * This function allows to store data into the given connection
 * defining destroy functions for the key and the value stored, per item.
 * 
 * \code
 * [...]
 * void destroy_data_1 (axlPointer data) 
 * {
 *     // perform a memory deallocation for data1
 * }
 * 
 * void destroy_data_2 (axlPointer data) 
 * {
 *     // perform a memory deallocation for data2
 * }
 * [...]
 * // store data 1 providing a destroy value function
 * vortex_connection_set_data_full (connection, "some:data:1", 
 *                                  data_1, NULL, destroy_data_1);
 *
 * // store data 2 providing a destroy value function
 * vortex_connection_set_data_full (connection, "some:data:2",
 *                                  data_2, NULL, destroy_data_2);
 * [...]
 * \endcode
 * 
 *
 * @param connection    The connection where the data will be stored.
 * @param key           The unique string key value.
 * @param value         The value to be stored associated to the given key.
 * @param key_destroy   An optional key destroy function used to destroy (deallocate) memory used by the key.
 * @param value_destroy An optional value destroy function used to destroy (deallocate) memory used by the value.
 */
void                vortex_connection_set_data_full          (VortexConnection * connection,
							      char             * key,
							      axlPointer         value,
							      axlDestroyFunc     key_destroy,
							      axlDestroyFunc     value_destroy)
{

	/* check reference */
	if (connection == NULL || key == NULL)
		return;

	/* check if the value is not null. It it is null, remove the
	 * value. */
	if (value == NULL) {
		vortex_hash_remove (connection->data, key);
		return;
	}

	/* store the data selected replacing previous one */
	vortex_hash_replace_full (connection->data, 
				  key, key_destroy, 
				  value, value_destroy);
	
	/* return from setting the value */
	return;
}


/** 
 * @brief Allows to activate TLS profile automatic negotiation for every connection created.
 * 
 * Once a user application is developed using Vortex Library it could
 * be interesting to instruct Vortex Library to automatically
 * negotiate the TLS profile for every connection created. This will
 * make that every call to \ref vortex_connection_new will return not
 * only an instance already connected but also with the TLS profile
 * already activated.
 * 
 * This allows to take advantage about source code developed to create
 * and wait for a \ref VortexConnection to be created rather than
 * having two steps at the user space: first create the connection and
 * the TLS-ficate it with \ref vortex_tls_start_negociation.
 *
 * The function allows to specify the optional serverName value to be
 * used when \ref vortex_tls_start_negociation is called. The values
 * set on this function will make effect to all connections created.
 * 
 * Once a \ref VortexConnection "connection" is created, the TLS
 * profile negotiation could fail. This is because the remote peer could be not
 * accepting TLS request, or the serverName request is not accepted or
 * any other issue. 
 * 
 * This could be a security problem because there is no difference from
 * using a \ref VortexConnection with TLS profile activated from other
 * one without it. This could cause user application to start using a
 * connection that is successfully connected but the TLS profile have
 * actually failed, sending and receiving text in plain mode.
 * 
 * The parameter <b>allow_tls_failures</b> allows to configure what is
 * the default action is to be taken on TLS failures. By default, if
 * TLS profile negotiation fails, the connection is closed, returning that the TLS
 * profile have failed. 
 * 
 * Using a true value allows to still keep on working even if the TLS
 * profile negotiation have failed.
 *
 * By default, Vortex Library have auto TLS feature disabled.
 * 
 * <i><b>NOTE:</b> If current Vortex Library doesn't have built-in
 * support for TLS profile, automatic TLS profile negotiation will
 * always fails. This means that setting <b>allow_tls_failures </b>
 * to false will cause Vortex Library client peer to always fail to
 * create new connections.</i>
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param enabled true to activate the automatic TLS profile
 * negotiation for every connection created, false to disable it.
 *
 * @param allow_tls_failures Configure how to handle errors produced
 * while activating automatic TLS negotiation.
 *
 * @param serverName The server name value to be passed in to \ref
 * vortex_tls_start_negociation. If the received is not NULL the
 * function will perform a local copy
 */
void                vortex_connection_set_auto_tls           (VortexCtx  * ctx,
							      bool         enabled,
							      bool         allow_tls_failures,
							      const char * serverName)
{

	/* check reference received */
	if (ctx == NULL)
		return;

	/* save boolean values */
	ctx->connection_auto_tls                = enabled;
	ctx->connection_auto_tls_allow_failures = allow_tls_failures;

	/* unref previous values */
	if (ctx->connection_auto_tls_server_name != NULL)
		axl_free (ctx->connection_auto_tls_server_name);

	/* store new value */
	ctx->connection_auto_tls_server_name    = (serverName != NULL) ? axl_strdup (serverName) : NULL;
	
	return;
}

/** 
 * @brief Gets stored value indexed by the given key inside the given connection.
 *
 * Returns value indexed by key inside connection. This function does
 * not retrieve data from remote peer, it just returns stored data
 * using \ref vortex_connection_set_data or \ref vortex_connection_set_data_full.
 * 
 * @param connection the connection where the value will be looked up.
 * @param key the key to look up.
 * 
 * @return the value or NULL if fails.
 */
axlPointer         vortex_connection_get_data               (VortexConnection * connection,
							     const char       * key)
{
	if (connection == NULL || key == NULL)
		return NULL;

	return vortex_hash_lookup (connection->data, (axlPointer) key);
}

/** 
 * @brief Returns the channel pool identified by <i>pool_id</i>.
 *
 * If the connection has only one channel pool created, you can
 * provided as pool_id = 1.
 * 
 * @param connection the connection where the channel pool is found.
 *
 * @param pool_id the channel pool id to look up. Remember that the
 * first pool is always have the pool id 1.
 * 
 * @return the channel pool or NULL if fails.
 */
VortexChannelPool * vortex_connection_get_channel_pool       (VortexConnection * connection,
							      int                pool_id)
{
	VortexChannelPool * pool = NULL;

	/* check the reference and the connection id */
	if (connection == NULL || pool_id <= 0)
		return NULL;

	pool = vortex_hash_lookup (connection->channel_pools, INT_TO_PTR (pool_id));

	return pool;
}


/** 
 * @internal
 * @brief Locks the channel pool mutex for the given connection.
 *
 * This function is for internal Vortex purposes. This function is
 * actually used by the vortex_channel_pool module to lock the
 * channel_pool_mutex this connection have while operating with
 * channel pool over this connection.
 * 
 * @param connection the connection to lock.
 */
void                vortex_connection_lock_channel_pool      (VortexConnection * connection)
{
	VortexCtx * ctx;
	if (connection == NULL)
		return;

	/* get the ctx */
	ctx = connection->ctx;

	vortex_log (VORTEX_LEVEL_DEBUG, "locking pool channel..");
	vortex_mutex_lock (&connection->channel_pool_mutex);
	
	return;
}

/** 
 * @internal
 * @brief Unlocks the channel pool mutex for the given connection.
 *
 * This function is for internal vortex purposes. This function is
 * actually used by the vortex_channel_pool module to unlock the
 * channel_pool_mutex this connection have while operating with
 * channel pool over this connection.
 * 
 * @param connection the connection to unlock.
 */
void                vortex_connection_unlock_channel_pool    (VortexConnection * connection)
{
	VortexCtx * ctx;

	if (connection == NULL)
		return;
	
	/* get the ctx */
	ctx = connection->ctx;

	vortex_log (VORTEX_LEVEL_DEBUG, "unlocking pool channel..");

	vortex_mutex_unlock (&connection->channel_pool_mutex);

	return;
}

/** 
 * @internal
 * @brief Return the next channel pool identifier.
 *
 * This function is for internal vortex purposes. This function is
 * actually used by the vortex_channel_pool module to get the next
 * channel pool id to be used.
 * 
 * @param connection the connection where the channel pool will be created.
 * 
 * @return the next id to use or -1 if fails
 */
int                 vortex_connection_next_channel_pool_id   (VortexConnection * connection)
{
	if (connection == NULL)
		return -1;

	connection->next_channel_pool_id++;
	return connection->next_channel_pool_id;
}

/** 
 * @internal
 * @brief Adds a new channel pool on the given connection.
 * 
 * This function is for internal vortex purposes. This function allows
 * vortex_channel_pool module to add new channel pool created over the
 * given connection.  
 *
 * @param connection the connection to operate.
 * @param pool the channel pool to add.
 */
void                vortex_connection_add_channel_pool       (VortexConnection  * connection,
							      VortexChannelPool * pool)
{
	if (connection == NULL || pool == NULL)
		return;

	vortex_hash_replace (connection->channel_pools, 
			     INT_TO_PTR (vortex_channel_pool_get_id (pool)),
			     pool);

	return;
}

/** 
 * @brief
 * @internal Removes the given channel pool on the given connection.
 *
 * This function is for internal vortex purposes. This function allows
 * vortex_channel_pool module to remove channel pools from the given
 * connection. 
 *
 * @param connection the connection where the pool will be removed.
 * @param pool the pool to remove.
 */
void                vortex_connection_remove_channel_pool    (VortexConnection  * connection,
							      VortexChannelPool * pool)
{
	/* check references received */
	if (connection == NULL || pool == NULL)
		return;

	vortex_hash_remove (connection->channel_pools, 
			    INT_TO_PTR (vortex_channel_pool_get_id (pool)));

	return;
}


/** 
 * @internal
 * @brief Support function for \ref vortex_connection_get_pending_msgs
 *
 * This function adds the message pending to be send over a given
 * channel.
 * 
 * @param key 
 * @param value the channel to retrieve how many messages are waiting
 * @param user_data 
 */
bool __vortex_connection_get_pending_msgs (axlPointer key, axlPointer value, axlPointer user_data)
{
	VortexChannel * channel  = value;
	int           * messages = user_data;
	int             msgs     = vortex_channel_queue_length (channel);
	
	(* messages ) = (* messages) + msgs;

	return false;
}

/** 
 * @brief Allows to get current frames waiting to be sent on the given connection.
 *
 * This function will iterate over all channels the connection have
 * checking if there are frames pending to be sent. The cumulative
 * result for this iteration will be returned as the pending message
 * to be sent over this connection or session.
 *
 * This function is actually used by the vortex writer to be able to
 * normalize its pending task to be processed situation while
 * unreferring connections.
 * 
 * @param connection a connection to know how many message are pending
 * to be sent.
 * 
 * @return the number or message pending to be sent.
 */
int                 vortex_connection_get_pending_msgs       (VortexConnection * connection)
{
	int  messages = 0;

	vortex_hash_foreach (connection->channels, 
			     __vortex_connection_get_pending_msgs, &messages);	
	return messages;
}

/** 
 * @brief Allows to get current connection role.
 * 
 * @param connection The VortexConnection to get the current role from.
 * 
 * @return Current role represented by \ref VortexPeerRole. If the
 * given connection is not connected, using \ref
 * vortex_connection_is_ok, the function will return \ref VortexRoleUnknown.
 */
VortexPeerRole      vortex_connection_get_role               (VortexConnection * connection)
{
	if (!vortex_connection_is_ok (connection, false))
		return VortexRoleUnknown;

	return connection->role;
}

/** 
 * @brief Returns current features requested by the remote peer this
 * connection is linked to.
 *
 * Features are a way for the remote peer to advertise some requested
 * feature. This is used/defined for each profile. Not all profiles
 * makes use of the feature optional attribute. Value get from this
 * function is the feature set by the remote peer. In case the remote
 * peer is a Vortex Library one, this value is set using \ref
 * vortex_greetings_set_features.
 * 
 * Features are sent at greetings time, which is the initial handshake
 * done between BEEP peers and it is previous to any channel
 * created. Features could be considered globally to the connection.
 *
 * You should not confuse this function with \ref
 * vortex_greetings_get_features. That function returns current status
 * of features that will be sent to the remote peers while current
 * host creates a new session.
 *
 * @param connection The connection where the greetings were received,
 * signaling the features request.
 * 
 * @return Features requested by the remote peer or NULL if none
 * features was defined. This function also return NULL if the
 * connection is not connected (verified with \ref
 * vortex_connection_is_ok).
 */
const char        * vortex_connection_get_features           (VortexConnection * connection)
{
	if (!vortex_connection_is_ok (connection, false))
		return NULL;
	return connection->features;
}

/** 
 * @brief Returns current localize requested by the remote peer this
 * connection is linked to.
 *
 * Localize value is a way for the remote peer to advertise preferred
 * language while generating textual diagnostics. If no localize is
 * defined by the remote peer, that is, the value returned by this
 * function is NULL, then you can generate the default language
 * diagnostic.
 *
 * You should not confuse this function with \ref
 * vortex_greetings_get_localize. That function is used to get
 * preferred localize to be sent while connecting to a remote peer.
 * 
 * @param connection The connection where the greetings were received,
 * signaling the localize requested.
 * 
 * @return Localize requested by the remote peer or NULL if no
 * localize was defined. This function also return NULL if the
 * connection is not connected (verified with \ref
 * vortex_connection_is_ok).
 */
const char       * vortex_connection_get_localize           (VortexConnection * connection)
{
	if (!vortex_connection_is_ok (connection, false))
		return NULL;
	return connection->localize;
}

/** 
 * @brief Allows to get current opened channels for the provided
 * connection.
 *
 * @param connection The connection that is requested to return the
 * number of channels opened.
 * 
 * @return The number of channels opened or zero if none. The function
 * returns 0 in the case the connection reference is null or not ok
 * (\ref vortex_connection_is_ok (connection, false)).
 */
int                 vortex_connection_get_opened_channels    (VortexConnection * connection)
{
	/* checke incoming data */
	if (connection == NULL || 
	    ! vortex_connection_is_ok (connection, false))
		return 0;

	/* return number of channels */
	return vortex_hash_size (connection->channels);
}

/** 
 * @brief In the case the connection was automatically created at the
 * listener BEEP side, the connection was accepted under an especific
 * listener started with \ref vortex_listener_new (and its associated
 * functions).
 *
 * This function allows to get a reference to the listener that
 * accepted and created the current connection. In the case this
 * function is called over a client connection NULL, will be returned.
 * 
 * @param connection The connection that is requried to return the
 * master connection associated (the master connection will have the
 * role: \ref VortexRoleMasterListener).
 * 
 * @return The reference or NULL if it fails. 
 */
VortexConnection  * vortex_connection_get_listener           (VortexConnection * connection)
{
	/* check reference received */
	if (connection == NULL)
		return NULL;
	
	/* returns current master connection associated */
	return vortex_connection_get_data (connection, "_vo:li:master");
}

/** 
 * @brief Allows to get the context under which the connection was
 * created. 
 * 
 * @param connection The connection that is requried to return the context under
 * which it was created.
 * 
 * @return A reference to the context associated to the connection or
 * NULL if it fails.
 */
VortexCtx         * vortex_connection_get_ctx                (VortexConnection * connection)
{
	/* check value received */
	if (connection == NULL)
		return NULL;

	/* reference to the connection */
	return connection->ctx;
}

/* @internal Function used by the macro CONN_CTX that allows to return
 * the context associated to the connection, and at the same time,
 * some checks are done.
 */
VortexCtx         * vortex_connection_get_ctx_aux            (const char * file,
							             int  line, 
							             VortexConnection * connection)
{
	/* fake ctx declaration for the following vortex_log calls */
	VortexCtx * ctx = NULL;

	if (connection == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "failed to return context because a null connection was received (%s:%d)",
			    file, line);
		return NULL;
	} else if (connection->ctx == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "failed to return context because it was received a connection without context configured (%s:%d)",
			    file, line);
		return NULL;
	} /* end if */

	/* return context configured */
	return connection->ctx;
}

/** 
 * @brief Allows to configure the send handler used to actually
 * perform sending operations over the underlying connection.
 *
 * 
 * @param connection The connection where the send handler will be set.
 * @param send_handler The send handler to be set.
 * 
 * @return Returns the previous send handler defined or NULL if fails.
 */
VortexSendHandler      vortex_connection_set_send_handler    (VortexConnection * connection,
							      VortexSendHandler  send_handler)
{
	VortexSendHandler previous_handler;

	/* check parameters received */
	if (connection == NULL || send_handler == NULL)
		return NULL;

	/* save previous handler defined */
	previous_handler = connection->send;

	/* set the new send handler to be used. */
	connection->send = send_handler;

	/* returns previous handler */
	return previous_handler;
 
	
}

/** 
 * @brief Allows to configure receive handler use to actually receive
 * data from remote peer. 
 * 
 * @param connection The connection where the receive handler will be set.
 * @param receive_handler The receive handler to be set.
 * 
 * @return Returns current receive handler set or NULL if it fails.
 */
VortexReceiveHandler   vortex_connection_set_receive_handler (VortexConnection     * connection,
							      VortexReceiveHandler   receive_handler)
{
	VortexReceiveHandler previous_handler;

	/* check parameters received */
	if (connection == NULL || receive_handler == NULL)
		return NULL;

	/* save previous handler defined */
	previous_handler    = connection->receive;

	/* set the new send handler to be used. */
	connection->receive = receive_handler;

	/* returns previous handler */
	return previous_handler;
}

/** 
 * @brief Set default IO handlers to be used while sending and
 * receiving data for the given connection.
 * 
 * Vortex Library allows to configure which are the function to be
 * used while sending and receiving data from the underlying
 * transport. This functions are usually not required API consumers.
 *
 * However, while implementing tuning profiles that needs to modify
 * how data is received and sent, the following function are used:
 * 
 *  - \ref vortex_connection_set_receive_handler
 *  - \ref vortex_connection_set_send_handler 
 *
 * This function allows to returns to the default handlers used by the
 * Vortex Library to perform these operations. This useful to
 * implement handler restoring when the tuning profile have failed.
 * 
 * @param connection The connection where the handler restoring will
 * take place.
 *
 */
void                   vortex_connection_set_default_io_handler (VortexConnection * connection)
{
	VortexCtx * ctx;

	/* check reference */
	if (connection == NULL)
		return;

	/* get a reference */
	ctx = connection->ctx;

	/* set default send and receive handlers */
	connection->send       = vortex_connection_default_send;
	connection->receive    = vortex_connection_default_receive;
	vortex_log (VORTEX_LEVEL_DEBUG, "restoring default IO handlers for connection id=%d", 
		    connection->id);

	return;
}

/** 
 * @brief Allows to set a new on close handler to be executed only
 * once the connection is being closed.
 *
 * The handler provided on this function is called once the connection
 * provided is closed. This is useful to detect connection broken or "broken pipe".
 *
 * If you require to set a user pointer to be received by the handler,
 * you can use \ref vortex_connection_set_on_close_full.
 * 
 * @param connection The connection being closed
 * @param on_close_handler The handler that will be executed.
 * 
 * @return In the past, the function was returning the previous
 * handler configured. However, because the function now support
 * receiving several on close handlers to be invoked, this have
 * changed. The function always return NULL.
 */
VortexConnectionOnClose vortex_connection_set_on_close       (VortexConnection * connection,
							      VortexConnectionOnClose on_close_handler)
{

	/* check reference received */
	if (connection == NULL || on_close_handler == NULL)
		return NULL;

	/* lock until done */
	vortex_mutex_lock (&connection->handlers_mutex);

	/* save previous handler defined */
	axl_list_append (connection->on_close, on_close_handler);

	/* unlock now the item is removed */
	vortex_mutex_unlock (&connection->handlers_mutex);

	/* returns previous handler */
	return NULL;
}

/** 
 * @brief Extended version for \ref vortex_connection_set_on_close
 * handler which also support receiving a user data pointer.
 *
 * See \ref vortex_connection_set_on_close for more details. This
 * function could be called several times to install several handlers.
 *
 * Once a handler is installed, you can use \ref
 * vortex_connection_remove_on_close_full to uninstall the handler if
 * it is required to avoid getting more notifications.
 * 
 * @param connection The connection that is required to get close
 * notifications.
 *
 * @param on_close_handler The handler to be executed once the event
 * is produced.
 *
 * @param data User defined data to be passed to the handler.
 * 
 * @return In the past, the function was implemented to support only
 * one handler. This was changed to support several handlers to be
 * notified. Now, the function doesn't return the previous
 * function. Always returns NULL.
 */
VortexConnectionOnCloseFull vortex_connection_set_on_close_full  (VortexConnection * connection,
								  VortexConnectionOnCloseFull on_close_handler,
								  axlPointer data)
{
	VortexConnectionOnCloseData * handler;

	/* check reference received */
	if (connection == NULL || on_close_handler == NULL)
		return NULL;

	/* lock during the operation */
	vortex_mutex_lock (&connection->handlers_mutex);

	/* save handler defined */
	handler          = axl_new (VortexConnectionOnCloseData, 1);
	handler->handler = on_close_handler;
	handler->data    = data;

	/* store the handler */
	axl_list_append (connection->on_close_full, handler);

	/* unlock now it is done */
	vortex_mutex_unlock (&connection->handlers_mutex);

	/* returns previous handler */
	return NULL;
}

/** 
 * @brief Allows to uninstall a particular handler installed to get
 * notifications about the connection close.
 *
 * If the handler is found, it will be uninstalled and the data
 * associated to the handler will be returned.
 * 
 * @param connection The connection where the uninstall operation will
 * be performed.
 *
 * @param on_close_handler The handler to uninstall.
 * 
 * @return The user data associated to the handler, configured at \ref
 * vortex_connection_set_on_close_full
 */
axlPointer vortex_connection_remove_on_close_full (VortexConnection              * connection, 
						   VortexConnectionOnCloseFull     on_close_handler)
{
	int                           iterator;
	VortexConnectionOnCloseData * handler;
	axlPointer                    data;

	/* check reference received */
	if (connection == NULL || on_close_handler == NULL)
		return NULL;

	/* look during the operation */
	vortex_mutex_lock (&connection->handlers_mutex);

	/* remove by pointer */
	iterator = 0;
	while (iterator < axl_list_length (connection->on_close_full)) {

		/* get a reference to the handler */
		handler = axl_list_get_nth (connection->on_close_full, iterator);
		
		if (on_close_handler == handler->handler) {
			/* handler found */
			data = handler->data;

			/* remove by pointer */
			axl_list_remove_ptr (connection->on_close_full, handler);

			/* unlock */
			vortex_mutex_unlock (&connection->handlers_mutex);

			return data;
			
		} /* end if */

		/* update the iterator */
		iterator++;
		
	} /* end while */
	
	/* unlock */
	vortex_mutex_unlock (&connection->handlers_mutex);

	return NULL;
}

/** 
 * @internal
 * @brief Allows to invoke current receive handler defined by \ref VortexReceiveHandler.
 * 
 * This function is actually no useful for Vortex Library consumer. It
 * is used by the library to actually perform the invocation in a
 * transparent way.
 *
 * @param connection The connection where the receive handler will be invoked.
 * @param buffer     The buffer to be received.
 * @param buffer_len The buffe size to be received.
 * 
 * @return How many data was actually received or -1 if it fails. The
 * function returns -2 if the connection isn't still prepared to read
 * data.
 */
int                 vortex_connection_invoke_receive         (VortexConnection * connection,
							      char             * buffer,
							      int                buffer_len)
{
	/* return -1 */
	if (connection == NULL || buffer == NULL || connection->receive == NULL)
		return -1;

	return connection->receive (connection, buffer, buffer_len);
}

/** 
 * @internal
 * @brief Allows to invoke current send handler defined by \ref VortexSendHandler.
 * 
 * This function is actually not useful for Vortex Library
 * consumers. It is used by the library to perform the invocation of the send handler.
 * 
 * @param connection The connection where the invocation of the send handler will be performed.
 * @param buffer     The buffer data to be sent.
 * @param buffer_len The buffer data size.
 * 
 * @return How many data was actually sent or -1 if it fails. -2 is
 * returned if the connection isn't still prepared to write or send
 * the data.
 */
int                 vortex_connection_invoke_send            (VortexConnection * connection,
							      const char       * buffer,
							      int                buffer_len)
{
	if (connection == NULL || ! connection->is_connected ||
	    buffer     == NULL || ! connection->send)
		return -1;
	    
	return connection->send (connection, buffer, buffer_len);
}

/** 
 * @brief Allows to disable sanity socket check, by default enabled.
 *
 * While allocating underlying socket descriptors, at the connection
 * creation using (<b>socket</b> system call) \ref
 * vortex_connection_new, it could happen that the OS assign a socket
 * descriptor used for standard task like: stdout(0), stdin(1),
 * stderr(2).
 * 
 * This happens when the user app issue a <b>close() </b> call over
 * the previous standard descriptors (<b>0,1,2</b>) causing vortex to
 * allocate a reserved descriptor with some funny behaviors.
 * 
 * Starting from previous context, any user app call issuing a console
 * print will cause to automatically send to the remote site the
 * message printed, bypassing all vortex mechanisms.
 * 
 * Obviously, this is an odd situation and it is not desirable. By
 * default, Vortex Library includes a sanity check to just reject
 * creating a Vortex Connection with an underlying socket descriptor
 * which could cause applications problems.
 *
 * You can disable this sanity check using this function.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param enable According to the value, the santicy check will be
 * enabled or disabled.
 * 
 */
void                vortex_connection_sanity_socket_check (VortexCtx * ctx, bool     enable)
{
	/* do not perform any operation if a null context is
	 * received */
	if (ctx == NULL)
		return;

	ctx->connection_enable_sanity_check  = enable;
	return;
}


/** 
 * @brief Allows to parse greetings reply received and prepare the
 * connection to be become usable.
 * 
 * This function allows to parse a greetings reply received and set
 * values received on that reply into the given connection. Values
 * received are: profiles supported and localize and features
 * attributes.
 * 
 * If the previous process is finished correctly, the connection is
 * registered into the vortex reader so the connection could start to
 * receive data from remote peer. 
 * 
 * If the function fails while parsing greetings reply, the connection
 * received will be set to be closed (that is, not usable). You should
 * call to \ref vortex_connection_unref if such situation
 * happens. Because this function is to be used while implementing a
 * profile that requires a tuning reset the \ref
 * vortex_connection_close function should not be used for close the
 * connection. 
 * 
 * This function also deallocate the frame received. 
 * 
 * @param connection The connection where the greetings status
 * checking will be performed.
 *
 * @param frame      The frame containing a greeting reply
 * 
 * @return true if the process have finished properly, otherwise false
 * is returned.
 */
bool     vortex_connection_parse_greetings_and_enable (VortexConnection * connection, 
						       VortexFrame      * frame)
{
	VortexCtx * ctx;

	/* check parametes received */
	if (connection == NULL || frame == NULL)
		return false;

	/* get the connection context */
	ctx = connection->ctx;

	/* process frame response */
	if (frame != NULL) {
		/* now, we have to read remote site supported
		 * profiles */
		if (!__vortex_connection_parse_greetings (connection, frame)) {
			/* parse ok, free frame and establish new
			 * message */
			vortex_frame_unref (frame);

			/* something wrong have happened while parsing
			 * XML greetings */
			return false;
		}
		vortex_log (VORTEX_LEVEL_DEBUG, "greetings parsed..");
		
		/* parse ok, free frame and establish new message */
		vortex_frame_unref (frame);

		/* free previous message and stablish the new one */
		if (connection->message)
			axl_free (connection->message);
		connection->message = axl_strdup ("session established and ready");
		
		vortex_log (VORTEX_LEVEL_DEBUG, "new connection created to %s:%s", connection->host, connection->port);

		/* call to notify connection created */
		vortex_connection_notify_created (connection);
		
		/* now, every initial message have been sent, we need
		 * to send this to the reader manager */
		vortex_reader_watch_connection (ctx, connection);
		return true;
	}
	vortex_log (VORTEX_LEVEL_CRITICAL, "received a null frame (null reply) from remote side when expected a greetings reply, closing session");
	__vortex_connection_set_not_connected (connection, "received a null frame (null reply) from remote side when expected a greetings reply, closing session");
	return false;
}


/** 
 * @brief Allows to configure a handler to be executed before any
 * operations is applied inside the Vortex Reader process.
 * 
 * @param connection The connection where the pre read will be executed.
 *
 * @param pre_accept_handler The handler to be executed. If NULL is
 * used, the handler is no longer executed.
 */
void                vortex_connection_set_preread_handler        (VortexConnection * connection, 
								  VortexConnectionOnPreRead pre_accept_handler)
{
	/* no reference, no operation */
	if (connection == NULL)
		return;

	vortex_connection_set_data (connection, "vo:co:pr", pre_accept_handler);
	
	return;
}

/** 
 * @internal
 * @brief Allows to set current TLS status for the given connection.
 * 
 * This function is not for Vortex Library API consumers.
 * 
 * @param connection The connection to set TLS-fication status.
 * @param status The status to set.
 */
void                vortex_connection_set_tlsfication_status     (VortexConnection * connection,
								  bool               status)
{
	/* flag this connection to be already TLS-ficated */
	vortex_connection_set_data (connection, "tls-fication:status", INT_TO_PTR (status));
	return;
}

/** 
 * @brief Allows to get current status for TLS activation on the given connection.
 *
 * Every \ref VortexConnection instance used inside the Vortex Library
 * could be already running under the TLS profile. This function
 * allows to get current TLS activation status. 
 * 
 * @param connection The connection to check for TLS status.
 * 
 * @return true if TLS status is already activated, otherwise false is returned.
 */
bool                vortex_connection_is_tlsficated              (VortexConnection * connection)
{
	return (PTR_TO_INT (vortex_connection_get_data (connection, "tls-fication:status")));
}


/** 
 * @brief Allows to check if there are an pre read handler defined on the given connection.
 * 
 * @param connection The connection to check.
 * 
 * @return true if the pre-read handler is defined, otherwise, false is returned
 */
bool                vortex_connection_is_defined_preread_handler (VortexConnection * connection)
{
	return (vortex_connection_get_data (connection, "vo:co:pr") != NULL);
}

/** 
 * @brief Invokes the prer-read handler defined on the given
 * connection.
 * 
 * @param connection The connection where the preread handler will be
 * invoked
 * 
 * @return 
 */
void            vortex_connection_invoke_preread_handler     (VortexConnection * connection)
{
	VortexConnectionOnPreRead pre_accept_handler = vortex_connection_get_data (connection, "vo:co:pr");

	if (pre_accept_handler != NULL)
		pre_accept_handler (connection);
	return;
}

/** 
 * @internal Allows to init the connection module using the provided
 * context (\ref VortexCtx).
 * 
 * @param ctx The vortex context that is going to be initialized (only the connection module)
 */
void                vortex_connection_init                   (VortexCtx        * ctx)
{

	v_return_if_fail (ctx);

	/**** vortex_connection.c: init connection module */
	ctx->connection_id                        = 1;
	ctx->connection_enable_sanity_check       = true;
	ctx->connection_std_timeout               = 60000000;

	vortex_mutex_create (&ctx->connection_xml_cache_mutex);
	vortex_mutex_create (&ctx->connection_hostname_mutex);
	vortex_mutex_create (&ctx->connection_new_notify_mutex);

	/* init hashes */
	ctx->connection_xml_cache = axl_hash_new (axl_hash_string, axl_hash_equal_string);
	ctx->connection_hostname  = axl_hash_new (axl_hash_string, axl_hash_equal_string);

	return;
}

/** 
 * @internal Allows to terminate the vortex connection module context
 * on the provided vortex context.
 * 
 * @param ctx The \ref VortexCtx to cleanup
 */
void                vortex_connection_cleanup                (VortexCtx        * ctx)
{
	v_return_if_fail (ctx);


	/**** vortex_connection.c: cleanup ****/
	vortex_mutex_destroy (&ctx->connection_xml_cache_mutex);
	vortex_mutex_destroy (&ctx->connection_hostname_mutex);
	vortex_mutex_create  (&ctx->connection_new_notify_mutex);

	/* drop hashes */
	axl_hash_free (ctx->connection_xml_cache);
	ctx->connection_xml_cache = NULL;
	axl_hash_free (ctx->connection_hostname);
	ctx->connection_hostname = NULL;

	/* free list */
	if (ctx->connection_new_notify_list != NULL)
		axl_list_free (ctx->connection_new_notify_list);
	ctx->connection_new_notify_list = NULL;

	return;
}

/* @} */
