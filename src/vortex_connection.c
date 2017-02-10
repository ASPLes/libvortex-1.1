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

/* global include */
#include <vortex.h>

/* private include */
#include <vortex_ctx_private.h>
#include <vortex_hash_private.h>

/* include connection internal definition */
#include <vortex_connection_private.h>

#if defined(AXL_OS_UNIX)
# include <netinet/tcp.h>
#endif

#define LOG_DOMAIN "vortex-connection"
#define VORTEX_CONNECTION_BUFFER_SIZE 32768

/** 
 * \defgroup vortex_connection_opts Vortex Connection Options: connection create options
 */

/** 
 * \addtogroup vortex_connection_opts
 * @{
 */

struct _VortexConnectionOpts {
	/** 
	 * @internal serverName to be signed on connection greetings.
	 */
	char       * serverName;

	/** 
	 * @internal Signal to acquire the serverName value as defined
	 * by the host parameter provided at
	 * vortex_connection_new_full.
	 */
	axl_bool     serverName_acquire;

	/** 
	 * @internal Signals if the connection option must be released
	 * on connection creation.
	 */
	axl_bool     release_opts;
};

/** 
 * @internal Process all options found in args, assuming opt_item is
 * the first option to process.
 */
VortexConnectionOpts * __vortex_connection_opts_process (VortexConnectionOptItem opt_item, va_list args)
{
	VortexConnectionOpts * opts;

	/* create connection options */
	opts = axl_new (VortexConnectionOpts, 1);
	VORTEX_CHECK_REF (opts, NULL);

	/* set default values */
	opts->serverName_acquire = axl_true;

	/* according to each opt_item, do: */
	while (opt_item) {
		/* check to finish option list */
		if (opt_item == VORTEX_OPTS_END)
			break;

		/* for each option list do */
		switch (opt_item) {
		case VORTEX_OPTS_END:
			/* nothing to do here */
			break;
		case VORTEX_SERVERNAME_FEATURE:
			/* get serverName value */
			opts->serverName   = (char *) va_arg (args, const char *);
			opts->serverName   = axl_strdup (opts->serverName);
			break;
		case VORTEX_SERVERNAME_ACQUIRE:
			/* get serverName acquire status */
			opts->serverName_acquire = va_arg (args, axl_bool);
			break;
		case VORTEX_OPTS_RELEASE:
			/* check release status */
			opts->release_opts = va_arg (args, axl_bool);
			break;
		} /* end switch */

		/* get next option */
		opt_item = va_arg (args, VortexConnectionOptItem);

	} /* end while */	

	return opts;
}

VortexNetTransport __vortex_connection_detect_transport (VortexCtx * ctx, const char * host)
{
	/* if something os not configured, report IPv4 support */
	if (ctx == NULL || host == NULL) {
		return VORTEX_IPv4;
	} /* end if */

	if (strstr (host, ":")) {
		/* IPv6 resolution */
		return VORTEX_IPv6;
	} /* end if */

	/* IPv4 resolution (default with for IPv4 address too) */
	return VORTEX_IPv4;
}

/** 
 * @brief Allows to create a connection options object.
 *
 * The object returned is used to signal a list of options and values
 * to be used by \ref vortex_connection_new_full. See \ref
 * VortexConnectionOptItem for more information.
 *
 * @param opt_item First option item to be configured.
 *
 * @return A newly created \ref VortexConnectionOpts object.
 */
VortexConnectionOpts * vortex_connection_opts_new (VortexConnectionOptItem opt_item, ...)
{
	va_list                args;
	VortexConnectionOpts * opts;

	va_start (args, opt_item);

	/* process options */
	opts = __vortex_connection_opts_process (opt_item, args);

	va_end (args);

	/* return created option */
	return opts;
}

/** 
 * @brief Allows to create a connection options object with default
 * settings.
 *
 * Current default options enable \ref VORTEX_OPTS_RELEASE.
 *
 * See \ref vortex_connection_opts_new for more info. 
 *
 * @return A newly created \ref VortexConnectionOpts object.
 */
VortexConnectionOpts * vortex_connection_opts_default (VortexConnectionOptItem opt_item, ...)
{
	va_list                args;
	VortexConnectionOpts * opts;

	va_start (args, opt_item);

	/* process options */
	opts = __vortex_connection_opts_process (opt_item, args);

	va_end (args);

	/* set up automatic release */
	opts->release_opts = axl_true;

	/* return created option */
	return opts;
}

#define CONN_OPTS_SERVERNAME         "vo:opts:serverName"
#define CONN_OPTS_SERVERNAME_ACQUIRE "vo:opts:serverNameAcquire"

/** 
 * @internal Function used to get the serverName to be used, if any,
 * according to connection options and current VortexCtx
 * configuration.
 *
 * @param conn The connection to get the serverName name from.
 *
 */
const char * vortex_connection_opts_get_serverName (VortexConnection     * conn)
{
	VortexCtx * ctx;

	/* return no serverName in case some value received is NULL */
	if (conn == NULL)
		return NULL;

	/* check if context allows acquiring serverName from
	 * connection host */
	ctx = vortex_connection_get_ctx (conn);
	if (! ctx->serverName_acquire)
		return NULL; /* do not acquire serverName */

	/* return serverName configured */
	if (vortex_connection_get_data (conn, CONN_OPTS_SERVERNAME))
		return vortex_connection_get_data (conn, CONN_OPTS_SERVERNAME);

	/* check for acquire serverName from current connection host */
	if (vortex_hash_exists (conn->data, CONN_OPTS_SERVERNAME_ACQUIRE)) {
		if (vortex_connection_get_data (conn, CONN_OPTS_SERVERNAME_ACQUIRE))
			return vortex_connection_get_host (conn);
	} else  {
		return vortex_connection_get_host (conn);
	} /* end if */

	return NULL;
}
						    

/** 
 * @internal Function used to check and release connection option
 * reference provided.
 */
void vortex_connection_opts_check_and_release (VortexConnectionOpts * conn_opts)
{
	/* check for null reference */
	if (conn_opts == NULL)
		return;
	/* check if connection option was signeld to be released after
	   connection new call */
	if (conn_opts->release_opts)
		vortex_connection_opts_free (conn_opts);
	return;
}

/** 
 * @brief Finishes the provided connection option.
 * @param conn_opts The reference to the connection option to finish.
 */
void vortex_connection_opts_free (VortexConnectionOpts * conn_opts)
{
	if (conn_opts == NULL)
		return;
	axl_free (conn_opts->serverName);
	axl_free (conn_opts);
	return;
}


/* @} */



/**
 * @internal Type definition to hold all actions to be executed during
 * the connection creation.
 */
typedef struct _VortexConnectionActionData {
	VortexConnectionStage  stage;
	VortexConnectionAction action;
	axlPointer             action_data;
} VortexConnectionActionData;


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
 * @internal Function to init all mutex associated to this particular
 * connection 
 */
void __vortex_connection_init_mutex (VortexConnection * connection)
{
	/* inits all mutex associated to the connection provided. */
	vortex_mutex_create (&connection->channel_mutex);
	vortex_mutex_create (&connection->ref_mutex);
	vortex_mutex_create (&connection->op_mutex);
	vortex_mutex_create (&connection->handlers_mutex);
	vortex_mutex_create (&connection->channel_pool_mutex);
	vortex_mutex_create (&connection->pending_errors_mutex);
	vortex_mutex_create (&connection->channel_update_mutex);
	vortex_mutex_create (&connection->profile_masks_mutex);

	return;
}


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
 * @return axl_true if the socket sanity check have passed, otherwise
 * axl_false is returned.
 */
axl_bool      vortex_connection_do_sanity_check (VortexCtx * ctx, VORTEX_SOCKET session)
{
	/* warn the user if it is used a socket descriptor that could
	 * be used */
	if (ctx && ctx->connection_enable_sanity_check) {
		
		if (session < 0) {
			vortex_log (VORTEX_LEVEL_CRITICAL,
				    "Socket receive is not working, invalid socket descriptor=%d", session);
			return axl_false;
		} /* end if */

		/* check for a valid socket descriptor. */
		switch (session) {
		case 0:
		case 1:
		case 2:
			vortex_log (VORTEX_LEVEL_CRITICAL, 
			       "created socket descriptor using a reserved socket descriptor (%d), this is likely to cause troubles",
			       session);
			/* return sanity check have failed. */
			return axl_false;
		}
	}

	/* return the sanity check is ok. */
	return axl_true;
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
	VortexConnection      * connection;
	VortexConnectionOpts  * options;
	VortexConnectionNew     on_connected;
	axlPointer              user_data;
	axl_bool                threaded;
	VortexNetTransport      transport;
}VortexConnectionNewData;


typedef struct _VortexConnectionOnCloseData {
	VortexConnectionOnCloseFull handler;
	axlPointer                  data;
} VortexConnectionOnCloseData;

axl_bool  __vortex_connection_close_list_channels (axlPointer key, axlPointer value, axlPointer user_data)
{
	
	VortexChannel             * channel = value;

	/* save channel */
	axl_list_append ((axlList *) user_data, channel);

	/* get a reference and will be finalized by __close_channel_aux to avoid races */
	if (vortex_channel_get_number (channel) != 0)
		vortex_channel_ref2 (channel, "close-channels");

	return axl_false;
}

void __close_channel_aux (axlPointer _channel)
{
	VortexChannel * channel = _channel;
	/* get the context */
#if defined(ENABLE_VORTEX_LOG) && ! defined(SHOW_FORMAT_BUGS)
	VortexCtx     * ctx     = vortex_channel_get_ctx (channel);
#endif

	/* check this channel is not the administrative one. */
	if (vortex_channel_get_number (channel) == 0)
		return;
	
	/* close the channel, */
	if (!vortex_channel_close (channel, NULL)) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to close a channel=%d during vortex connection closing",
			    vortex_channel_get_number (channel));
	} /* end if */

	/* release reference acquired during close */
	vortex_channel_unref2 (channel, "close-channels");
	
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
 * @return axl_true if all channels were close, axl_false if not. Keep in mind
 * that this function returns axl_false if only one channel have rejected
 * to be closed.  If a null reference is received, the function
 * returns axl_false.
 */
axl_bool      vortex_connection_close_all_channels (VortexConnection * connection, int      also_channel_0)
{
	VortexChannel             * channel0 = NULL;
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx                 * ctx;
#endif
	axlList                   * result;

	/* define the context */
	if (connection == NULL)
		return axl_false;

#if defined(ENABLE_VORTEX_LOG)	
	/* get the context */
	ctx = connection->ctx;
#endif

	/* block connection operations during channel closing for this
	 * session */
	switch (axl_hash_items (connection->channels->table)) {
	case 0:
		/* no channels to close, going to close connection */
		return axl_true;
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
	if (axl_hash_items (connection->channels->table) > 1) {

		/* return */
		vortex_log (VORTEX_LEVEL_WARNING, "unable to close connection, there are channels still working");
		return axl_false;
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
			vortex_log (VORTEX_LEVEL_WARNING, "unable to close the channel 0..");
			return axl_false;
		}
	}
	
	/* release lock and free connection resources */

	return axl_true;

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
	VORTEX_CHECK_REF (cache, NULL);

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

axl_bool      __vortex_connection_parse_greetings (VortexConnection * connection, VortexFrame * frame)
{
	/* local variable */
	axlDoc                         * doc       = NULL;
	axl_bool                         in_cache;
	axlDtd                         * channel_dtd;
	axlError                       * error = NULL;
	VortexConnectionGreetingsCache * cache;
	VortexCtx                      * ctx;

	/* check the reference */
	if (connection == NULL || frame == NULL)
		return axl_false;
	
	/* get a reference to the context */
	ctx = connection->ctx;

	if ((channel_dtd = vortex_dtds_get_channel_dtd (ctx)) == NULL) {
		__vortex_connection_shutdown_and_record_error (
			connection, VortexError,
			"Cannot find DTD definition for channel management");
		return axl_false;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "About to parse the following message: (size: %d) '%s'",
		    vortex_frame_get_payload_size (frame), (const char *) vortex_frame_get_payload (frame));

	/* dtd correct */

	/* now check if the document is in the cache and use it */
	vortex_mutex_lock (&ctx->connection_xml_cache_mutex);
	cache = __vortex_connection_greetings_cache_get (ctx, vortex_frame_get_payload (frame));
	if (cache == NULL) {
		vortex_log (VORTEX_LEVEL_DEBUG, "document not found in cache, parsing as usual");
		/* document not found, flag that it is not in chase
		 * and use it as is. If the validation goes ok, store
		 * in the case */
		in_cache = axl_false;
		doc      = axl_doc_parse (vortex_frame_get_payload (frame), 
					  vortex_frame_get_payload_size (frame), &error);
	} else {
		vortex_log (VORTEX_LEVEL_DEBUG, "document found in cache (hit!), reusing reference");

		/* nice! cache hit, flag as is */
		in_cache = axl_true;
	} /* end if */
	
	/* check here the document reference */
	if (doc == NULL && cache == NULL) {
		/* unlock the cache */
		vortex_mutex_unlock (&ctx->connection_xml_cache_mutex);

		__vortex_connection_shutdown_and_record_error (
			connection, VortexXmlValidationError, "found an error while parsing greetings message: %s",
			error ? axl_error_get (error) : "");
		axl_error_free (error);

		return axl_false;
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
		
		return axl_true;
	}

	/* unlock the cache */
	vortex_mutex_unlock (&ctx->connection_xml_cache_mutex);

	/* Validation failed */
	vortex_log (VORTEX_LEVEL_CRITICAL, "validation failed: %s", axl_error_get (error));
	axl_error_free (error);

	/* release memory used by the xml document */
	axl_doc_free   (doc);

	/* flag the connection to be not connected */
	__vortex_connection_shutdown_and_record_error (
		connection, VortexProtocolError, "Incoming greetings validation failed");
	return axl_false;
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
 * VortexConnection instance as if it were created with \ref
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
 * @param socket  An already connected socket.  
 * @param role    The role to be set to the connection being created.
 * 
 * @return a newly allocated \ref VortexConnection. 
 */
VortexConnection * vortex_connection_new_empty            (VortexCtx *    ctx, 
							   VORTEX_SOCKET  socket, 
							   VortexPeerRole role)
{
	/* creates a new connection */
	return vortex_connection_new_empty_from_connection (ctx, socket, NULL, role);
}

void __vortex_connection_channel_unref (axlPointer channel)
{
	vortex_channel_unref2 (channel, "vortex connection");
	return;
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
								 VORTEX_SOCKET      _socket,
								 VortexConnection * __connection,
								 VortexPeerRole     role)
{
	/* do not skip naming */
	return vortex_connection_new_empty_from_connection2 (ctx, _socket, __connection, role, axl_false);
}

VortexConnection * vortex_connection_new_empty_from_connection2 (VortexCtx        * ctx,
								 VORTEX_SOCKET      _socket,
								 VortexConnection * __connection,
								 VortexPeerRole     role,
								 axl_bool           skip_naming)
{
	VortexConnection   * connection;
	VortexChannel      * channel;

	/* create connection object without setting socket (this is
	 * done by vortex_connection_set_sock) */
	connection                     = axl_new (VortexConnection, 1);
	VORTEX_CHECK_REF (connection, NULL);

	connection->ctx                = ctx;
	vortex_ctx_ref2 (ctx, "new connection"); /* acquire a reference to context */
	connection->id                 = __vortex_connection_get_next_id (ctx);

	connection->message            = axl_strdup ("session established and ready");
	VORTEX_CHECK_REF2 (connection->message, NULL, connection, axl_free);

	connection->status             = VortexOk;
	connection->is_connected       = axl_true;
	connection->ref_count          = 1;

	/* remote side profiles, NULL reference filled by the
	 * greetings cache */
	connection->remote_supported_profiles = NULL;

	/* call to init all mutex associated to this particular connection */
	__vortex_connection_init_mutex (connection);

	/* check connection that is accepting connections */
	if (role != VortexRoleMasterListener) {
		connection->channels           = vortex_hash_new_full (axl_hash_int, axl_hash_equal_int, 
								       /* key destroy */
								       NULL,
								       /* channel destroy */
								       (axlDestroyFunc) __vortex_connection_channel_unref);

		/* creates the user space data */
		if (__connection != NULL) {
			/* set current serverName if defined */
			connection->serverName        = __connection->serverName;
			__connection->serverName      = NULL;

			/* transfer hash used by previous connection into the new one */
			connection->data       = __connection->data;
			/* creates a new hash to keep the connection internal state consistent */
			__connection->data     = vortex_hash_new_full (axl_hash_string, axl_hash_equal_string,
								       NULL,
								       NULL);

			/* remove being closed flag if found */
			vortex_connection_set_data (connection, "being_closed", NULL);
		} else 
			connection->data       = vortex_hash_new_full (axl_hash_string, axl_hash_equal_string,
								       NULL, 
								       NULL);
		
		connection->channel_pools      = vortex_hash_new_full (axl_hash_int, axl_hash_equal_int,
								       NULL, 
								       (axlDestroyFunc) __vortex_channel_pool_close_internal);
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
		if (! vortex_connection_add_channel (connection, channel)) {
			vortex_log (VORTEX_LEVEL_CRITICAL, "failed to add channel 0 to new connection id=%d created, unable to maintain this connection, killing",
				    connection->id);

			/* release all references */
			vortex_channel_unref2 (channel, "new connection");
			vortex_connection_unref (connection, "vortex_connection_new_empty_from_connection");
			return NULL;
		}

	} else {
		/* create the hash data table for master listener connections */
		connection->data       = vortex_hash_new_full (axl_hash_string, axl_hash_equal_string,
							       NULL, 
							       NULL);
		
	} /* end if */
	
	/* set by default to close the underlying connection when the
	 * connection is closed */
	connection->close_session      = axl_true;

	/* establish the connection role and its initial next channel
	 * number available. */
	connection->role               = role;

	/* set socket provided (do not allow stdin(0), stdout(1), stderr(2) */
	if (skip_naming) {
		connection->session = _socket;
	} else {
		if (_socket > 2) {
			if (! vortex_connection_set_socket (connection, _socket, NULL, NULL)) {
				vortex_log (VORTEX_LEVEL_CRITICAL, "failed to configure socket associated to connection");
				vortex_connection_unref (connection, "vortex_connection_new_empty_from_connection");
				return NULL;
			} /* end if */
		} else {
			/* set a wrong socket connection in the case a not
			   proper value is received */
			vortex_log (VORTEX_LEVEL_WARNING, "received wrong socket fd, setting invalid fd beacon: -1");
			connection->session = -1;
		} /* end if */
	}

	return connection;	
}

/** 
 * @brief Allows to configure the socket to be used by the provided
 * connection. This function is usually used in conjunction with \ref
 * vortex_connection_new_empty.
 *
 * @param conn The connection to be configured with the socket
 * provided.
 *
 * @param _socket The socket connection to configure.
 *
 * @param real_host Optional reference that can configure the host
 * value associated to the socket provided. This is useful on
 * environments were a proxy or TUNNEL is used. In such environments
 * the host and port value get from system calls return the middle hop
 * but not the destination host. You can safely pass NULL, causing the
 * function to figure out which is the right value using the socket
 * provided.
 *
 * @param real_port Optional reference that can configure the port
 * value associated to the socket provided. See <b>real_host</b> param
 * for more information. In real_host is defined, it is required to
 * define this parameter.
 *
 * @return axl_true in the case the function has configured the
 * provided socket, otherwise axl_false is returned.
 */
axl_bool            vortex_connection_set_socket                (VortexConnection * conn,
								 VORTEX_SOCKET      _socket,
								 const char       * real_host,
								 const char       * real_port)
{

	struct sockaddr_storage   sin;
#if defined(AXL_OS_WIN32)
	/* windows flavors */
	int                  sin_size = sizeof (sin);
#else
	/* unix flavors */
	socklen_t            sin_size = sizeof (sin);
#endif
	VortexCtx          * ctx;
	char                 host_name[NI_MAXHOST];
	char                 srv_name[NI_MAXSERV]; 

	/* check conn reference */
	if (conn == NULL)
		return axl_false;

	ctx  = CONN_CTX(conn);

	/* perform connection sanity check */
	if (! vortex_connection_do_sanity_check (ctx, _socket)) 
		return axl_false;

	/* disable nagle */
	vortex_connection_set_sock_tcp_nodelay (_socket, axl_true);

	/* set socket */
	conn->session = _socket;
	
	/* get remote peer name */
	if (real_host && real_port) {
		/* set host and port from user values */
		conn->host = axl_strdup (real_host);
		conn->port = axl_strdup (real_port);
	} else {
		/* clear structures */
		memset (host_name, 0, NI_MAXHOST);
		memset (srv_name, 0, NI_MAXSERV);

		if (conn->role == VortexRoleMasterListener) {
			if (getsockname (_socket, (struct sockaddr *) &sin, &sin_size) < 0) {
				vortex_log (VORTEX_LEVEL_CRITICAL, "unable to get local hostname and port from socket=%d, errno=%d (%s)", 
					    _socket, errno, vortex_errno_get_error (errno));
				return axl_false;
			} /* end if */
		} else {
			if (getpeername (_socket, (struct sockaddr *) &sin, &sin_size) < 0) {
				vortex_log (VORTEX_LEVEL_CRITICAL, "unable to get remote hostname and port from socket=%d (errno=%d)", 
					    _socket, errno);
				return axl_false;
			} /* end if */
		} /* end if */

		/* set host and port from socket recevied */
		if (getnameinfo ((struct sockaddr *) &sin, sin_size, host_name, NI_MAXHOST, srv_name, NI_MAXSERV, NI_NUMERICSERV | NI_NUMERICHOST) != 0) {
			vortex_log (VORTEX_LEVEL_CRITICAL, "getnameinfo () call failed, error was errno=%d", errno);
			return axl_false;
		} /* end if */

		/* copy values */
		conn->host = axl_strdup (host_name);
		conn->port = axl_strdup (srv_name);
	} /* end if */

	/* now set local address */
	if (getsockname (_socket, (struct sockaddr *) &sin, &sin_size) < 0) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to get local hostname and port to resolve local address from socket=%d", _socket);
		return axl_false;
	} /* end if */

	/* set host and port from socket recevied */
	if (getnameinfo ((struct sockaddr *) &sin, sin_size, host_name, NI_MAXHOST, srv_name, NI_MAXSERV, NI_NUMERICSERV | NI_NUMERICHOST) != 0) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "getnameinfo () call failed, error was errno=%d", errno);
		return axl_false;
	}

	/* set local addr and local port */
	conn->local_addr = axl_strdup (host_name);
	conn->local_port = axl_strdup (srv_name);

	return axl_true;
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
 * @return axl_true if blocking state was set or axl_false if not.
 */
axl_bool      vortex_connection_set_blocking_socket (VortexConnection    * connection)
{
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx * ctx;
#endif
#if defined(AXL_OS_UNIX)
	int  flags;
#endif
	/* check the connection reference */
	if (connection == NULL)
		return axl_false;

#if defined(ENABLE_VORTEX_LOG)
	/* get a reference to the ctx */
	ctx = connection->ctx;
#endif
	
#if defined(AXL_OS_WIN32)
	if (!vortex_win32_blocking_enable (connection->session)) {
		__vortex_connection_shutdown_and_record_error (
			connection, VortexError, "unable to set blocking I/O");
		return axl_false;
	}
#else
	if ((flags = fcntl (connection->session, F_GETFL, 0)) < 0) {
		__vortex_connection_shutdown_and_record_error (
			connection, VortexError,
			"unable to get socket flags to set non-blocking I/O");
		return axl_false;
	}
	vortex_log (VORTEX_LEVEL_DEBUG, "actual flags state before setting blocking: %d", flags);
	flags &= ~O_NONBLOCK;
	if (fcntl (connection->session, F_SETFL, flags) < 0) {
		__vortex_connection_shutdown_and_record_error (
			connection, VortexError, "unable to set non-blocking I/O");
		return axl_false;
	}
	vortex_log (VORTEX_LEVEL_DEBUG, "actual flags state after setting blocking: %d", flags);
#endif
	vortex_log (VORTEX_LEVEL_DEBUG, "setting connection as blocking");
	return axl_true;
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
 * @return axl_true if nonblocking state was set or axl_false if not.
 */
axl_bool      vortex_connection_set_nonblocking_socket (VortexConnection * connection)
{
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx * ctx;
#endif

#if defined(AXL_OS_UNIX)
	int  flags;
#endif
	/* check the reference */
	if (connection == NULL)
		return axl_false;

#if defined(ENABLE_VORTEX_LOG)
	/* get a reference to context */
	ctx = connection->ctx;
#endif
	
#if defined(AXL_OS_WIN32)
	if (!vortex_win32_nonblocking_enable (connection->session)) {
		__vortex_connection_shutdown_and_record_error (
			connection, VortexError, "unable to set non-blocking I/O");
		return axl_false;
	}
#else
	if ((flags = fcntl (connection->session, F_GETFL, 0)) < 0) {
		__vortex_connection_shutdown_and_record_error (
			connection, VortexError,
			"unable to get socket flags to set non-blocking I/O");
		return axl_false;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "actual flags state before setting nonblocking: %d", flags);
	flags |= O_NONBLOCK;
	if (fcntl (connection->session, F_SETFL, flags) < 0) {
		__vortex_connection_shutdown_and_record_error (
			connection, VortexError, "unable to set non-blocking I/O");
		return axl_false;
	}
	vortex_log (VORTEX_LEVEL_DEBUG, "actual flags state after setting nonblocking: %d", flags);
#endif
	vortex_log (VORTEX_LEVEL_DEBUG, "setting connection as non-blocking");
	return axl_true;
}

/** 
 * @brief Allows to configure tcp no delay flag (enable/disable Nagle
 * algorithm).
 * 
 * @param socket The socket to be configured.
 *
 * @param enable The value to be configured, axl_true to enable tcp no
 * delay.
 * 
 * @return axl_true if the operation is completed.
 */
axl_bool                 vortex_connection_set_sock_tcp_nodelay   (VORTEX_SOCKET socket,
								   axl_bool      enable)
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
		return axl_false;
	}

	/* properly configured */
	return axl_true;
} /* end */

/** 
 * @brief Allows to enable/disable non-blocking/blocking behavior on
 * the provided socket.
 * 
 * @param socket The socket to be configured.
 *
 * @param enable axl_true to enable blocking I/O, otherwise use
 * axl_false to enable non blocking I/O.
 * 
 * @return axl_true if the operation was properly done, otherwise axl_false is
 * returned.
 */
axl_bool                 vortex_connection_set_sock_block         (VORTEX_SOCKET socket,
								   axl_bool      enable)
{
#if defined(AXL_OS_UNIX)
	int  flags;
#endif

	if (enable) {
		/* enable blocking mode */
#if defined(AXL_OS_WIN32)
		if (!vortex_win32_blocking_enable (socket)) {
			return axl_false;
		}
#else
		if ((flags = fcntl (socket, F_GETFL, 0)) < 0) {
			return axl_false;
		} /* end if */

		flags &= ~O_NONBLOCK;
		if (fcntl (socket, F_SETFL, flags) < 0) {
			return axl_false;
		} /* end if */
#endif
	} else {
		/* enable nonblocking mode */
#if defined(AXL_OS_WIN32)
		/* win32 case */
		if (!vortex_win32_nonblocking_enable (socket)) {
			return axl_false;
		}
#else
		/* unix case */
		if ((flags = fcntl (socket, F_GETFL, 0)) < 0) {
			return axl_false;
		}
		
		flags |= O_NONBLOCK;
		if (fcntl (socket, F_SETFL, flags) < 0) {
			return axl_false;
		}
#endif
	} /* end if */

	return axl_true;
}

void __free_addr_info (axlPointer ptr)
{
	freeaddrinfo (ptr);
	return;
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
struct addrinfo * vortex_gethostbyname (VortexCtx           * ctx, 
					const char          * hostname, 
					const char          * port,
					VortexNetTransport    transport)
{
	/* get current context */
	struct addrinfo    hints, *res = NULL;
	char             * key;

	/* check that context and hostname are valid */
	if (ctx == NULL || hostname == NULL)
		return NULL;
	
	/* lock and resolv */
	vortex_mutex_lock (&ctx->connection_hostname_mutex);

	/* resolv using the hash */
	key = axl_strdup_printf ("%s:%s", hostname, port);
	res = axl_hash_get (ctx->connection_hostname, (axlPointer) key);
	if (res) {
		/* release key */
		axl_free (key);

		/* unlock and return the result */
		vortex_mutex_unlock (&ctx->connection_hostname_mutex);

		return res;

	}

	/* reached this point, key wasn't found, now try to DNS-resolve */

	/* clear hints structure */
	memset (&hints, 0, sizeof(struct addrinfo));

	switch (transport) {
	case VORTEX_IPv4:
		hints.ai_family = AF_INET;
		break;
	case VORTEX_IPv6:
		hints.ai_family = AF_INET6;
		break;
	} /* end switch */
	hints.ai_socktype = SOCK_STREAM;

	/* resolve hostname with hints */
	vortex_log (VORTEX_LEVEL_DEBUG, "Calling getaddrinfo (%s:%s), transport=%s", hostname, port, transport == VORTEX_IPv6 ? "IPv6" : "IPv4");
	if (getaddrinfo (hostname, port, &hints, &res) != 0) {
		axl_free (key);
		
		vortex_mutex_unlock (&ctx->connection_hostname_mutex);
		vortex_log (VORTEX_LEVEL_CRITICAL, "getaddrinfo (%s:%s) call failed, found errno=%d", hostname, port, errno);
		return NULL;
	}

	/* now store the result */
	axl_hash_insert_full (ctx->connection_hostname, 
			      /* the hostname */
			      key, axl_free,
			      /* the address */
			      res, (axlDestroyFunc) __free_addr_info);

	/* unlock and return the result */
	vortex_mutex_unlock (&ctx->connection_hostname_mutex);

	/* no need to release key here: this will be done once
	 * ctx->connection_hostname hash is fihished */

	return res;
	
}

/** 
 * @internal Function to perform a wait operation on the provided
 * socket, assuming the wait operation will be performed on a
 * nonblocking socket. The function configure the socket to be
 * non-blocking.
 * 
 * @param wait_for The kind of operation to wait for to be available.
 *
 * @param session The socket associated to the BEEP session.
 *
 * @param wait_period How many seconds to wait for the connection.
 * 
 * @return The error code to return:
 *    -4: Add operation into the file set failed.
 *     1: Wait operation finished.
 *    -2: Timeout
 *    -3: Fatal error found.
 */
int __vortex_connection_wait_on (VortexCtx           * ctx,
				     VortexIoWaitingFor    wait_for, 
				     VORTEX_SOCKET         session,
				     int                 * wait_period)
{
	int           err         = -2;
	axlPointer    wait_set;
	int           start_time;
#if defined(AXL_OS_UNIX)
 	int           sock_err = 0;       
 	unsigned int  sock_err_len;
#endif

	/* do not perform a wait operation if the wait period is zero
	 * or less */
	if (*wait_period <= 0) {
		vortex_log (VORTEX_LEVEL_WARNING, 
			    "requested to perform a wait operation but the wait period configured is 0 or less: %d",
			    *wait_period);
		return -2;
	} /* end if */

	/* make the socket to be nonblocking */
	vortex_connection_set_sock_block (session, axl_false);

	/* create a waiting set using current selected I/O
	 * waiting engine. */
	wait_set     = vortex_io_waiting_invoke_create_fd_group (ctx, wait_for);

	/* flag the starting time */
	start_time   = time (NULL);

	vortex_log (VORTEX_LEVEL_DEBUG, 
		    "starting connect timeout during %d seconds (starting from: %d)", 
		    *wait_period, start_time);

	/* add the socket in connection transit */
	while ((start_time + (*wait_period)) > time (NULL)) {
		/* clear file set */
		vortex_io_waiting_invoke_clear_fd_group (ctx, wait_set);

		/* add the socket into the file set (we can pass the
		 * socket and a NULL reference to the VortexConnection
		 * because we won't use dispatch API: we are only
		 * checking for changes) */
		if (! vortex_io_waiting_invoke_add_to_fd_group (ctx, session, NULL, wait_set)) {
			vortex_log (VORTEX_LEVEL_WARNING, "failed to add session to the waiting socket");
			err = -4;
			break;
		} /* end if */
				
		/* perform wait operation */
		err = vortex_io_waiting_invoke_wait (ctx, wait_set, session, wait_for);
		vortex_log (VORTEX_LEVEL_DEBUG, "__vortex_connection_wait_on (sock=%d) operation finished, err=%d, errno=%d (%s), wait_for=%d (ellapsed: %d)",
			    session, err, errno, vortex_errno_get_error (errno) ? vortex_errno_get_error (errno) : "", wait_for, (int) (time (NULL) - start_time));

		/* check for bad file description error */
		if (errno == EBADF) {
			vortex_log (VORTEX_LEVEL_CRITICAL, "Found bad file descriptor while waiting");
			/* destroy waiting set */
			vortex_io_waiting_invoke_destroy_fd_group (ctx, wait_set);
			return -3;
		} /* end if */

		if(err == -1 /* EINTR */ || err == -2 /* SSL */)
			continue;
		else if (!err) 
			continue; /*select, poll, epoll timeout*/
		else if (err > 0) {
#if defined(AXL_OS_UNIX)
			/* check estrange case on older linux 2.4 glib
			 * versions that returns err > 0 but it is not
			 * really connected */
			sock_err_len = sizeof(sock_err);
			if (getsockopt (session, SOL_SOCKET, SO_ERROR, (char*)&sock_err, &sock_err_len) < 0){
				vortex_log (VORTEX_LEVEL_WARNING, "failed to get error level on waiting socket");
				err = -5;
			} else if (sock_err) {
				vortex_log (VORTEX_LEVEL_WARNING, "error level set on waiting socket");
				err = -6;
			}
#endif
			/* connect ok */
			break; 
		} else if (err /*==-3, fatal internal error, other errors*/)
			return -1;

	} /* end while */
	
	vortex_log (VORTEX_LEVEL_DEBUG, "timeout operation finished, with err=%d, errno=%d, ellapsed time=%d (seconds)", 
		    err, errno, (int) (time (NULL) - start_time));

	/* destroy waiting set */
	vortex_io_waiting_invoke_destroy_fd_group (ctx, wait_set);

	/* update the return timeout code to signal that the timeout
	 * period was reached */
	if ((start_time + (*wait_period)) <= time (NULL))
		err = -2;

	/* update wait period */
	(*wait_period) -= (time (NULL) - start_time);

	/* return error code */
	return err;
}

/** 
 * @brief Allows to create a plain socket connection against the host
 * and port provided. 
 *
 * @param ctx The context where the connection happens.
 *
 * @param host The host server to connect to.
 *
 * @param port The port server to connect to.
 *
 * @param timeout Parameter where optionally is returned the timeout
 * defined by the library (\ref vortex_connection_get_connect_timeout)
 * that remains after only doing a socket connected. The value is only
 * returned if the caller provide a reference.
 *
 * @param error Optional axlError reference to report an error code
 * and a textual diagnostic.
 *
 * @return A connected socket or -1 if it fails. The particular error
 * is reported at axlError optional reference.
 */
VORTEX_SOCKET vortex_connection_sock_connect (VortexCtx   * ctx,
					      const char  * host,
					      const char  * port,
					      int         * timeout,
					      axlError   ** error)
{
	return vortex_connection_sock_connect_common (ctx, host, port, timeout, VORTEX_IPv4, error);
}

/** 
 * @brief Allows to create a plain socket connection against the host
 * and port provided allowing to configure the transport. 
 *
 * This function differs from \ref vortex_connection_sock_connect in
 * the sense it allows to configure the transport (\ref
 * VortexNetTransport) so you can create TCP/IPv4 (\ref VORTEX_IPv4)
 * and TCP/IPv6 (\ref VORTEX_IPv6) connections.
 *
 * @param ctx The context where the connection happens.
 *
 * @param host The host server to connect to.
 *
 * @param port The port server to connect to.
 *
 * @param timeout Parameter where optionally is returned the timeout
 * defined by the library (\ref vortex_connection_get_connect_timeout)
 * that remains after only doing a socket connected. The value is only
 * returned if the caller provide a reference.
 *
 * @param transport The network transport to use for this connect operation.
 *
 * @param error Optional axlError reference to report an error code
 * and a textual diagnostic.
 *
 * @return A connected socket or -1 if it fails. The particular error
 * is reported at axlError optional reference.
 */
VORTEX_SOCKET vortex_connection_sock_connect_common (VortexCtx            * ctx,
						     const char           * host,
						     const char           * port,
						     int                  * timeout,
						     VortexNetTransport     transport,
						     axlError            ** error)
{

	struct addrinfo    * res;
	int		     err          = 0;
	VORTEX_SOCKET        session      = -1;

	/* do resolution according to the transport */
	res = vortex_gethostbyname (ctx, host, port, transport);
        if (res == NULL) {
		vortex_log (VORTEX_LEVEL_WARNING, "unable to get host name by using vortex_gethostbyname () host=%s",
			    host);
		axl_error_report (error, VortexNameResolvFailure, "unable to get host name by using vortex_gethostbyname ()");
		return -1;
	} /* end if */

	/* create the socket and check if it */
	switch (transport) {
	case VORTEX_IPv4:
		session      = socket (AF_INET, SOCK_STREAM, 0);
		break;
	case VORTEX_IPv6:
		session      = socket (AF_INET6, SOCK_STREAM, 0);
		break;
	default:
		vortex_log (VORTEX_LEVEL_CRITICAL, "Transport value is not supported (%d), unable to create socket", transport);
		axl_error_report (error, VortexNameResolvFailure, "Transport value is not supported, unable to create socket");
		return -1;
	}
	if (session == VORTEX_INVALID_SOCKET) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to create socket");
		axl_error_report (error, VortexNameResolvFailure, "unable to create socket (socket call have failed)");
		return -1;
	} /* end if */

	/* check socket limit */
	if (! vortex_connection_check_socket_limit (ctx, session)) {
		axl_error_report (error, VortexSocketSanityError, "Unable to create more connections, socket limit reached");
		return -1;
	}

	/* do a sanity check on socket created */
	if (!vortex_connection_do_sanity_check (ctx, session)) {
		/* close the socket */
		vortex_close_socket (session);

		/* report error */
		axl_error_report (error, VortexSocketSanityError, 
				  "created socket descriptor using a reserved socket descriptor (%d), this is likely to cause troubles");
		return -1;
	} /* end if */
	
	/* disable nagle */
	vortex_connection_set_sock_tcp_nodelay (session, axl_true);

	/* get current vortex connection timeout to check if the
	 * application have requested to configure a particular TCP
	 * connect timeout. */
	if (timeout) {
		(*timeout)  = vortex_connection_get_connect_timeout (ctx); 
		if ((*timeout) > 0) {
			/* translate hold value for timeout into seconds  */
			(*timeout) = (int) (*timeout) / (int) 1000000;
			
			/* set non blocking connection */
			vortex_connection_set_sock_block (session, axl_false);
		} /* end if */
	} /* end if */

	/* do a tcp connect */
        if (connect (session, res->ai_addr, res->ai_addrlen) < 0) {
		if(timeout == 0 || (errno != VORTEX_EINPROGRESS && errno != VORTEX_EWOULDBLOCK)) { 
			shutdown (session, SHUT_RDWR);
			vortex_close_socket (session);
			vortex_log (VORTEX_LEVEL_WARNING, "unable to connect(), session=%d to remote host errno=%d (%s), timeout reached", session, errno, strerror (errno));
			axl_error_report (error, VortexConnectionError, "unable to connect to remote host");
			return -1;
		} /* end if */
	} /* end if */

	/* if a connection timeout is defined, wait until connect */
	if (timeout && ((*timeout) > 0)) {
		/* wait for write operation, signaling that the
		 * connection is available */
		err = __vortex_connection_wait_on (ctx, WRITE_OPERATIONS, session, timeout);

#if defined(AXL_OS_WIN32)
		/* under windows we have to also we to be readable */

		/* NOTE: the following code was commented because
		 * starting from 1.1.3 BEEP listener do send content
		 * inmmediately (greetings) but waits for client
		 * greetings to reply with the proper values. The
		 * following waiting code causes select(2) call to not
		 * report TCP proper connection until some data is
		 * received, which is obviously a windows winsock
		 * bug. */
		if (err > 0) {  
/*			vortex_log (VORTEX_LEVEL_DEBUG, "connect ok, but need to check readable state for socket %d..", session); */
/*			err = __vortex_connection_wait_on (ctx, READ_OPERATIONS, session, timeout); */
		} /* end if */
#endif
		
		if(err <= 0){
			/* timeout reached while waiting for the connection to terminate */
			shutdown (session, SHUT_RDWR);
			vortex_close_socket (session);
			vortex_log (VORTEX_LEVEL_WARNING, "unable to connect to remote host (timeout)");
			axl_error_report (error, VortexNameResolvFailure, "unable to connect to remote host (timeout)");
			return -1;
		} /* end if */
	} /* end if */

	/* return socket created */


	return session;
}
			

/** 
 * @brief Do greetings exchange (BEEP session initialization) on the
 * provided connection.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param connection The connection where the greetings exchange will
 * take place.
 *
 * @param options The set of options to be applied on the connection
 * that can be useful for greetings exchange (for example: greetings
 * features).
 *
 * @param timeout A timeout defined by the caller under which the
 * operation should finish.
 *
 * @return axl_true in the case greetings exchange finished properly,
 * without errors. Otherwise axl_false is returned and the connection
 * is flaged as unconnected with the appropiate status (\ref
 * vortex_connection_get_status) and error message (\ref
 * vortex_connection_get_message).
 */
axl_bool vortex_connection_do_greetings_exchange (VortexCtx             * ctx, 
						  VortexConnection      * connection, 
						  VortexConnectionOpts  * options,
						  int                     timeout)
{
	VortexFrame          * frame;
	int                    err;

	v_return_val_if_fail (vortex_connection_is_ok (connection, axl_false), axl_false);

	/* now we have to send greetings and process them */
	if (! vortex_greetings_client_send (connection, options)) {
		vortex_log (VORTEX_LEVEL_DEBUG, vortex_connection_get_message (connection));
		return axl_false;
	}
	
	vortex_log (VORTEX_LEVEL_DEBUG, "greetings sent, waiting for reply");


	/* while we did not finish */
	while (axl_true) {

		/* block thread until received remote greetings */
		vortex_log (VORTEX_LEVEL_DEBUG, "getting initial greetings frame..");
		frame = vortex_greetings_client_process (connection, options);
		vortex_log (VORTEX_LEVEL_DEBUG, "finished wait for initial greetings frame=%p timeout=%d, conn-id=%d, socket=%d", 
			    frame, timeout, connection->id, connection->session);

		/* check frame received */
		if (frame != NULL) {

			vortex_log (VORTEX_LEVEL_DEBUG, "greetings received, process reply frame");
			break;

		} else if (timeout > 0 && vortex_connection_is_ok (connection, axl_false)) {

			vortex_log (VORTEX_LEVEL_WARNING, 
				    "found NULL frame referecence connection=%d, checking to wait timeout=%d seconds for read operation..",
				    connection->id, timeout);

			/* try to perform a wait operation */
			err = __vortex_connection_wait_on (ctx, READ_OPERATIONS, connection->session, &timeout);
			if (err <= 0 || timeout <= 0) {
				/* timeout reached while waiting for the connection to terminate */
				vortex_log (VORTEX_LEVEL_WARNING, 
					    "reached timeout=%d or general operation failure=%d while waiting for initial greetings frame for connection id=%d",
					    err, timeout, connection->id);
				
				/* close the connection */
				shutdown (connection->session, SHUT_RDWR);
				vortex_close_socket (connection->session);
				connection->session      = -1;

				/* free previous message */
				if (connection->message)
					axl_free (connection->message);
				connection->message      = axl_strdup_printf (
					"reached timeout while waiting for initial greetings frame, err=%d, timeout=%d",
					err, timeout);
				connection->status       = VortexGreetingsFailure;
				connection->is_connected = axl_false;

				/* error found, stop greetings process */
				return axl_false;
			} /* end if */
				
			vortex_log (VORTEX_LEVEL_DEBUG,
				    "found the connection is ready to provide data, checking..");
			continue;
		} else {
			/* check if a pending frame was found */
			if (vortex_connection_get_data (connection, VORTEX_GREETINGS_PENDING_FRAME) ||
			    vortex_connection_get_data (connection, "frame")) {
				vortex_log (VORTEX_LEVEL_DEBUG, "found partial greetings frame, reading rest..");
				continue;
			} /* end if */

			/* null frame received */
			vortex_log (VORTEX_LEVEL_CRITICAL,
				    "Connection refused. Received null frame were it was expected initial greetings, finish connection id=%d", connection->id);
			
			/* timeout reached while waiting for the connection to terminate */
			shutdown (connection->session, SHUT_RDWR);
			vortex_close_socket (connection->session);
			connection->session      = -1;

			/* free previous message */
			if (connection->message == NULL) {
				connection->message      = 
					axl_strdup_printf ("Connection refused. Received null frame were it was expected initial greetings, finish connection id=%d", 
							   connection->id);
				connection->status       = VortexConnectionError;
			} /* end if */
			connection->is_connected = axl_false;
			return axl_false;
		} /* end if */
		
	} /* end while */

	/* make the connection to be blocking during the
	 * greetings process (if it were not) */
	vortex_connection_set_blocking_socket (connection);
	
	/* process frame response */
	if (!vortex_connection_parse_greetings_and_enable (connection, frame))
		return axl_false;

	vortex_log (VORTEX_LEVEL_DEBUG, "greetings exchange ok");

	/* check here connection options like CONN_OPTS_SERVERNAME OR
	   CONN_OPTS_SERVERNAME_ACQUIRE */
	if (options) {
		if (options->serverName_acquire) 
			vortex_connection_set_data (connection, CONN_OPTS_SERVERNAME_ACQUIRE, INT_TO_PTR (axl_true));
		if (options->serverName) 
			vortex_connection_set_data_full (connection, CONN_OPTS_SERVERNAME, axl_strdup (options->serverName), NULL, axl_free);
	} /* end if */
	return axl_true;
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
	struct sockaddr_storage   sin;
#if defined(AXL_OS_WIN32)
	/* windows flavors */
	int                    sin_size     = sizeof (sin);
#else
	/* unix flavors */
	socklen_t              sin_size     = sizeof (sin);
#endif
	/* get current context */
	VortexConnection     * connection   = data->connection;
	VortexConnectionOpts * options      = data->options;
	VortexCtx            * ctx          = connection->ctx;
	VortexChannel        * channel;
	axlError             * error        = NULL;
	int                    d_timeout    = 0;
	axl_bool               threaded     = data->threaded;
	VortexConnectionNew    on_connected = data->on_connected;
	axlPointer             user_data    = data->user_data;
	VortexNetTransport     transport    = data->transport;
	char                   host_name[NI_MAXHOST];
	char                   srv_name[NI_MAXSERV]; 

	vortex_log (VORTEX_LEVEL_DEBUG, "executing connection new in %s mode to %s:%s id=%d",
	       (data->threaded == axl_true) ? "thread" : "blocking", 
	       connection->host, connection->port,
	       connection->id);

	/* release data */
	axl_free (data);

	/* create channel 0 (virtually always is created but, is
	 * necessary to have a representation for channel 0, in order
	 * to make channel management function to be consistent). */
	channel = vortex_channel_empty_new (0, "not applicable", connection);
	vortex_connection_add_channel  (connection, channel);

	/* notify connection created before running TCP connect */
	if (ctx->conn_created)
		ctx->conn_created (ctx, connection, ctx->conn_created_data);

	/* configure the socket created */
	connection->session = vortex_connection_sock_connect_common (ctx, connection->host, connection->port, &d_timeout, transport, &error);
	if (connection->session == -1) {
		/* free previous message */
		if (connection->message)
			axl_free (connection->message);

		/* get error message and error status */
		connection->message = axl_strdup (axl_error_get (error));
		connection->status  = axl_error_get_code (error);
		axl_error_free (error);

		/* flag as not connected */
		connection->is_connected = axl_false;
	} else {
		/* flag as connected */
		connection->is_connected = axl_true;
	} /* end if */
	
	/* according to the connection status (is_connected attribute)
	 * perform the final operations so the connection becomes
	 * usable. Later, the user app level is notified. */
	if (connection->is_connected) {

		/* configure local address used by this connection */
		/* now set local address */
		if (getsockname (connection->session, (struct sockaddr *) &sin, &sin_size) < 0) {
			vortex_log (VORTEX_LEVEL_DEBUG, "unable to get local hostname and port to resolve local address");

			/* check to release options if defined */
			vortex_connection_opts_check_and_release (options);

			return NULL;
		} /* end if */

		/* set host and port from socket recevied */
		memset (host_name, 0, NI_MAXHOST);
		memset (srv_name, 0, NI_MAXSERV);
		if (getnameinfo ((struct sockaddr *) &sin, sin_size, host_name, NI_MAXHOST, srv_name, NI_MAXSERV, NI_NUMERICSERV | NI_NUMERICHOST) != 0) {
			vortex_log (VORTEX_LEVEL_CRITICAL, "getnameinfo () call failed, error was errno=%d", errno);
			return NULL;
		}
	
		/* set local addr and local port */
		connection->local_addr = axl_strdup (host_name);
		connection->local_port = axl_strdup (srv_name);

		/* block thread until received remote greetings */
		if (vortex_connection_do_greetings_exchange (ctx, connection, options, d_timeout)) {

			/* call to notify CONECTION_STAGE_POST_CREATED */
			vortex_log (VORTEX_LEVEL_DEBUG, "doing post creation notification for connection id=%d", connection->id);
			vortex_connection_actions_notify (ctx, &connection, CONNECTION_STAGE_POST_CREATED);
		} /* end if */
	} /* end if */

	/* notify on callback or simply return */
	if (threaded) {
		/* notify connection */
		on_connected (connection, user_data);

		/* check to release options if defined */
		vortex_connection_opts_check_and_release (options);
	
		return NULL;
	}

	/* check to release options if defined */
	vortex_connection_opts_check_and_release (options);

	return connection;
}

VortexConnection  * vortex_connection_new_full_common        (VortexCtx            * ctx,
							      const char           * host, 
							      const char           * port,
							      VortexConnectionOpts * options,
							      VortexConnectionNew    on_connected, 
							      VortexNetTransport     transport,
							      axlPointer             user_data)
{
	VortexConnectionNewData * data;

	/* check context is initialized */
	if ((! vortex_init_check (ctx)) || ctx->vortex_exit) {
		/* check to release options if defined */
		vortex_connection_opts_check_and_release (options);
		return NULL;
	}

	/* check parameters */
	if (host == NULL || port == NULL) {
		/* check to release options if defined */
		vortex_connection_opts_check_and_release (options);
		return NULL;
	} /* end if */

	data                                  = axl_new (VortexConnectionNewData, 1);
	if (data == NULL) {
		/* check to release options if defined */
		vortex_connection_opts_check_and_release (options);
		return NULL;
	} /* end if */
	data->options                         = options;
	data->connection                      = axl_new (VortexConnection, 1);
	/* check allocated connection */
	if (data->connection == NULL) {
		/* check to release options if defined */
		vortex_connection_opts_check_and_release (options);
		axl_free (data);
		return NULL;
	} /* end if */
	data->connection->id                  = __vortex_connection_get_next_id (ctx);
	data->connection->ctx                 = ctx;
	vortex_ctx_ref2 (ctx, "new connection"); /* acquire a reference to context */
	data->connection->host                = axl_strdup (host);
	data->connection->port                = axl_strdup (port);
	data->connection->channels            = vortex_hash_new_full (axl_hash_int, axl_hash_equal_int,
								      NULL,
								      (axlDestroyFunc) __vortex_connection_channel_unref);
	data->connection->ref_count           = 1;

	/* call to init all mutex associated to this particular connection */
	__vortex_connection_init_mutex (data->connection);

	data->connection->data                = vortex_hash_new_full (axl_hash_string, axl_hash_equal_string,
								      NULL,
								      NULL);
	data->connection->channel_pools       = vortex_hash_new_full (axl_hash_int, axl_hash_equal_int,
								      NULL, 
								      (axlDestroyFunc) __vortex_channel_pool_close_internal);
	/* remote side profiles, NULL reference filled by the
	 * greetings cache */
	data->connection->remote_supported_profiles = NULL;

	/* establish the connection role and initial next channel
	 * available. */
	data->connection->role                = VortexRoleInitiator;
	data->connection->last_channel        = 1;

	/* set default send and receive handlers */
	data->connection->send                = vortex_connection_default_send;
	data->connection->receive             = vortex_connection_default_receive;

	/* set by default to close the underlying connection when the
	 * connection is closed */
	data->connection->close_session       = axl_true;

	/* establish connection status, connection negotiation method
	 * and user data */
	data->on_connected                    = on_connected;
	data->user_data                       = user_data;
	data->threaded                        = (on_connected != NULL);

	/* check allocated values */
	if (data->connection->host          == NULL ||
	    data->connection->port          == NULL ||
	    data->connection->channels      == NULL ||
	    data->connection->data          == NULL ||
	    data->connection->channel_pools == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Connection memory allocation failed.."); 
		/* connection allocation failed */
		vortex_connection_free (data->connection);
		axl_free (data);
		return NULL;
	}

	/* detect transport we have to configure */
	data->transport = transport;
	/* set same transport on the connection */
	data->connection->transport = data->transport;

	if (data->threaded) {
		vortex_log (VORTEX_LEVEL_DEBUG, "invoking connection_new threaded mode");
		vortex_thread_pool_new_task (ctx, (VortexThreadFunc) __vortex_connection_new, data);
		return NULL;
	}
	vortex_log (VORTEX_LEVEL_DEBUG, "invoking connection_new non-threaded mode");
	return __vortex_connection_new (data);
}


/** 
 * @brief Allows to create a new BEEP session (connection) to the
 * given <i>host:port</i> using provided options.
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
 * remote site or until timeout mechanism is reached (if
 * configured). You can define the connect timeout by using \ref
 * vortex_connection_connect_timeout. Note that if nothing is
 * configured, no timeout is implemented. There is also a separate
 * timeout that is used for general receive operations: \ref
 * vortex_connection_timeout (though this timeout is not used for this
 * connect operation).
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
 * vortex_tls_start_negotiation. Optionally, you can configure your
 * Vortex Library client peer to automatically negotiate the TLS
 * profile for every connection created, allowing to get from this
 * function a connection that already have TLS profile activated. You
 * can configure this behavior using \ref vortex_tls_set_auto_tls. 
 *
 * Check out the \ref vortex_tls_set_auto_tls "documentation"
 * for \ref vortex_tls_set_auto_tls to know more about using
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
 * @param options Additional connection options to be used for this
 * particular operation. This is mainly used to provide feature
 * notification to remote BEEP listener. See \ref
 * vortex_connection_opts_new and \ref VortexConnectionOptItem for more details.
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
 *
 * <b>Note about reference returned by this function</b>
 *
 * \ref VortexConnection and \ref VortexChannel objects implements a reference
 * counting that tries to be as much automatic as possible.
 *
 * In this context, when you actively create a connection, via
 * \ref vortex_connection_new (or this one), you are acquiring a reference that must be
 * terminated either with \ref vortex_connection_close, or if you know that
 * you are doing, with \ref vortex_connection_unref.
 *
 * At the same time, the connection you create is also used by vortex
 * internals (specially vortex reader) which also acquires a reference
 * to the connection, to ensure he can access to those connections
 * even if the user has closed it.
 *
 * Having said that, let's take a look at the listener side.
 *
 * What happens when you receive a server side notification, that
 * involves a connection you didn't create and your code has no
 * reference to it? How do you release that memory when server stops,
 * or a connection is closed?
 *
 * As you are guessing, those connections has references that are only
 * owned by your vortex engine at the listener side, and temporally,
 * those references are incremented to implement handler
 * notifications, but, again, those references will be removed when
 * those listener side handler finishes.
 * 
 * Now, assuming this context, you now know that a \ref
 * vortex_connection_close is not required at the listener side,
 * because you didn't create that connection, and that those
 * connections will be eventually collected, and that you must do a
 * \ref vortex_connection_close at client side because you did create
 * the connection, not only to initiate the BEEP close protocol, but
 * to also release the reference you acquired previously.
 *
 * <b>Calling to create connections when vortex is being finished or uninitialized</b>
 *
 * If a call to vortex_exit_ctx was done before calling to create a
 * new connection, then the function will return NULL. 
 */
VortexConnection  * vortex_connection_new_full               (VortexCtx            * ctx,
							      const char           * host, 
							      const char           * port,
							      VortexConnectionOpts * options,
							      VortexConnectionNew    on_connected, 
							      axlPointer             user_data)
{
	VortexNetTransport transport;
	
	/* get transport we have to use */
	transport = __vortex_connection_detect_transport (ctx, host);

	/* call to create the connection */
	return vortex_connection_new_full_common (ctx, host, port, options, on_connected, transport, user_data);
}

/** 
 * @brief Allows to create a new BEEP session (connection) to the
 * given <i>host:port</i> using provided options and using TCP/IPv6.
 *
 * See \ref vortex_connection_new_full for extended information. This function is the same as \ref vortex_connection_new_full but using IPv6 transport and resolution.
 * 
 * @param ctx The context where the operation will be performed.
 *
 * @param host The remote peer to connect to.
 *
 * @param port The peer's port to connect to.
 *
 * @param options Additional connection options to be used for this
 * particular operation. This is mainly used to provide feature
 * notification to remote BEEP listener. See \ref
 * vortex_connection_opts_new and \ref VortexConnectionOptItem for more details.
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
 *
 * Remember to check documentation at \ref vortex_connection_new_full
 * to get information that also applies to this function.
 */
VortexConnection  * vortex_connection_new_full6              (VortexCtx            * ctx,
							      const char           * host, 
							      const char           * port,
							      VortexConnectionOpts * options,
							      VortexConnectionNew    on_connected, 
							      axlPointer             user_data)
{
	/* call to create the connection */
	return vortex_connection_new_full_common (ctx, host, port, options, on_connected, VORTEX_IPv6, user_data);
}

/** 
 * @brief Allows to create a new BEEP session (connection) to the given <i>host:port</i> using TCP/IPv4 resolution and transport.
 *
 * This is the IPv4 version support for BEEP session. If you need IPv6 support, see \ref vortex_connection_new6.
 *
 * See \ref vortex_connection_new_full for complete
 * documentation. This function implements the same behaviour than
 * \ref vortex_connection_new_full but without providing additional
 * options (\ref VortexConnectionOpts).
 *
 * This function is implemented on top of \ref vortex_connection_new_full.
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
	return vortex_connection_new_full (ctx, host, port, NULL, on_connected, user_data);
}

/** 
 * @brief Allows to create a new BEEP session (connection) to the given <i>host:port</i> using TCP/IPv6 resolution and transport.
 *
 * This is the IPv6 version support for BEEP session. If you need IPv4 support, see \ref vortex_connection_new6.
 *
 * See \ref vortex_connection_new_full for complete
 * documentation. This function implements the same behaviour than
 * \ref vortex_connection_new_full but without providing additional
 * options (\ref VortexConnectionOpts).
 *
 * This function is implemented on top of \ref vortex_connection_new_full.
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
VortexConnection * vortex_connection_new6 (VortexCtx   * ctx,
					  const char  * host, 
					  const char  * port, 
					  VortexConnectionNew on_connected,
					  axlPointer user_data)
{
	return vortex_connection_new_full6 (ctx, host, port, NULL, on_connected, user_data);
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
 * @param is_added axl_true if the channel is being added, otherwise axl_false
 * is received, notifying that is removed.
 */
void __vortex_connection_check_and_notify (VortexConnection * connection, 
					   VortexChannel    * channel, 
					   axl_bool           is_added)
{
	/* get the channel number */
	int                              iterator;
	VortexChannelStatusUpdate       * statusUpdate;
	VortexConnectionOnChannelUpdate    handler;
	axlPointer                         handler_data;
	axlList                         * list;
	VortexCtx                       * ctx = CONN_CTX (connection);

	if (connection == NULL)
		return;

	/* check for global handlers */
	if (vortex_channel_get_number (channel) != 0) {
		if (is_added && ctx->global_channel_added) {
			vortex_log (VORTEX_LEVEL_DEBUG, "notifying channel=%d added on global channel added handler",
				    vortex_channel_get_number (channel));
			/* call to notify */
			ctx->global_channel_added (channel, ctx->global_channel_added_data);
		} else if (! is_added && ctx->global_channel_removed) {
			vortex_log (VORTEX_LEVEL_DEBUG, "notifying channel=%d removed on global channel removed handler",
				    vortex_channel_get_number (channel));
			/* call to notify */
			ctx->global_channel_removed (channel, ctx->global_channel_removed_data);
		} /* end if */
	} /* end if */

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
				vortex_log (VORTEX_LEVEL_DEBUG, 
					    "notifying channel num=%d (ref count:%d) %s on connection id=%d (ref count:%d)", 
					    vortex_channel_get_number (channel),
					    vortex_channel_ref_count (channel),
					    is_added ? "addition" : "removal",
					    connection->id, vortex_connection_ref_count (connection));

				/* get references to the handler and data before releasing the mutex */
				handler      = statusUpdate->handler;
				handler_data = statusUpdate->handler_data;

				/* the channel exists, notify */
				vortex_mutex_unlock (&connection->channel_update_mutex);

				/* notify the channel and the user data */
				handler (channel, handler_data);

				/* the channel exists, notify */
				vortex_mutex_lock (&connection->channel_update_mutex);
			} /* end if */

			/* next iterator */
			iterator++;
			
		} /* end while */
		
		/* the channel exists, notify */
		vortex_mutex_unlock (&connection->channel_update_mutex);
		
	} /* end if */

	return;
}

axl_bool  __vortex_connection_foreach_check_and_notify (axlPointer key, 
							axlPointer data,
							axlPointer user_data, 
							axlPointer user_data2)
{
	VortexChannel    * channel  = data;
	VortexConnection * conn     = user_data;
	axl_bool           is_added = PTR_TO_INT (user_data2);

	/* call to notify closed handler */
	vortex_channel_invoke_closed (channel);

	/* call to notify */
	__vortex_connection_check_and_notify (conn, channel, is_added);

	if (! is_added) { /* only if the channel is being removed from the connection */
		
		/* nullify channel reference to the connection to avoid
		 * issues. This is done at this point because the connection
		 * reference counting reached 0 so, those API consumers
		 * updating reference counting to the connection will avoid
		 * this. */
		__vortex_channel_nullify_conn (channel);
		
	} /* end if */

	/* always return axl_false to make the process to follow to the
	 * end */
	return axl_false;
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
 * if (! vortex_connection_is_ok (connection, axl_false)) {
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
 * @return axl_true if the connection was successfully reconnected or axl_false
 * if not. If the <i>on_connected</i> handler is defined, this
 * function will always return axl_true.
 */
axl_bool            vortex_connection_reconnect              (VortexConnection * connection,
							      VortexConnectionNew on_connected,
							      axlPointer user_data)
{
	VortexCtx               * ctx;
	VortexConnectionNewData * data;

	/* we have to check a basic case. A null connection. Null
	 * connection no data to reestablish the connection. */
	if (connection == NULL)
		return axl_false;

	if (vortex_connection_is_ok (connection, axl_false))
		return axl_true;

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
	vortex_hash_foreach2 (connection->channels, __vortex_connection_foreach_check_and_notify, connection, INT_TO_PTR(axl_false));

	vortex_hash_clear (connection->channels);
	vortex_hash_clear (connection->channel_pools);
	
	/* reset channel pool id counting */
	connection->next_channel_pool_id = 0;

	vortex_log (VORTEX_LEVEL_DEBUG, "channels, channels pools and channel pools indications cleared..");

	/* create data needed to invoke the service */
	data               = axl_new (VortexConnectionNewData, 1);
	/* check allocated value */
	if (data == NULL) 
		return axl_false;
	data->connection   = connection;
	
	data->on_connected = on_connected;
	data->user_data    = user_data;
	data->threaded     = (on_connected != NULL);

	vortex_log (VORTEX_LEVEL_DEBUG, "reconnection request is prepared, doing so..");

	/* clear previous message if defined */
	if (connection->message)
		axl_free (connection->message);
	connection->message = NULL;

	/* detect transport we have to configure */
	data->transport = connection->transport;
	
	if (data->threaded) {
		vortex_log (VORTEX_LEVEL_DEBUG, "reconnecting connection in threaded mode");
		vortex_thread_pool_new_task (ctx, (VortexThreadFunc) __vortex_connection_new, data);
		return axl_true;
	}
	vortex_log (VORTEX_LEVEL_DEBUG, "reconnecting connection in non-threaded mode");
	return vortex_connection_is_ok (__vortex_connection_new (data), axl_false);

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
 * performed over the connection if the function returns axl_true. This
 * means after calling this function the connection reference must not
 * be used until a new connection is created. Otherwise a segfault
 * may happen.
 *
 * If the function receives a null connection reference, it will just
 * return a axl_true value without doing nothing.
 * 
 * @param connection the connection to close properly.
 * 
 * @return axl_true if connection was closed and axl_false if not. If there
 * are channels still working, the connection will not be closed.
 *
 * <b>NOTE:</b> Using full close procedure (the one implemented by this
 * function) may make your connection to suffer BNRA (see
 * http://www.aspl.es/vortex/draft-brosnan-beep-limit-close.txt). In
 * general, is a good practise to close the connection using
 * \ref vortex_connection_shutdown.
 */
axl_bool                    vortex_connection_close                  (VortexConnection * connection)
{
	int         refcount = 0;
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx * ctx;
#endif

	/* if the connection is a null reference, just return axl_true */
	if (connection == NULL)
		return axl_true;

#if defined(ENABLE_VORTEX_LOG)
	/* get a reference to the ctx */
	ctx = connection->ctx;
#endif

	/* ensure only one call to vortex_connection_close will
	   progress */
	vortex_mutex_lock (&connection->op_mutex);
	if (connection->close_called) {
		vortex_mutex_unlock (&connection->op_mutex);
		return axl_true;
	}
	/* flag as connection close called */
	connection->close_called = axl_true;
	vortex_mutex_unlock (&connection->op_mutex);

	/* close all channel on this connection */
	if (vortex_connection_is_ok (connection, axl_false)) {
		vortex_log (VORTEX_LEVEL_DEBUG, "closing a connection id=%d which is already opened with %d channels opened..",
			    connection->id, axl_hash_items (connection->channels->table));

		/* update the connection reference to avoid race
		 * conditions caused by deallocations */
		if (! vortex_connection_ref (connection, "vortex_connection_close")) {

			__vortex_connection_shutdown_and_record_error (
				connection, VortexError,
				"failed to update reference counting on the connection during close operation, skiping clean close operation..");
			vortex_connection_unref (connection, "vortex_connection_close");
			return axl_true;
		}
		
		/* close all channels because the connection at this
		 * point is running properly */
		if (!vortex_connection_close_all_channels (connection, axl_true)) {
			/* update the connection reference to avoid race
			 * conditions caused by deallocations */
			vortex_connection_unref (connection, "vortex_connection_close");


			vortex_log (VORTEX_LEVEL_WARNING, "failed while closing all channels");

			/* undo connection close */
			connection->close_called = axl_false;
			return axl_false;
		}

		/* set the connection to be not connected */
		__vortex_connection_shutdown_and_record_error (
			connection, VortexConnectionCloseCalled, "close connection called");

		/* update the connection reference to avoid race
		 * conditions caused by deallocations */
		refcount = connection->ref_count;
		vortex_connection_unref (connection, "vortex_connection_close");

		/* check special case where the caller have stoped a
		 * listener reference without taking a reference to it */
		if ((refcount - 1) == 0) {
			return axl_true;
		}
	} else {
		vortex_log (VORTEX_LEVEL_DEBUG, "closing a connection which is not opened, unref resources..");
		/* check here for the case when a close operation is
		   requested for a listener that was started but not
		   created (port binding problem, port bind
		   permission, etc). This is allowed because under
		   such cases, vortex reader do not own a reference to
		   the listener and it is required to remove it,
		   allowing to do so by the user. */
		if (connection->role == VortexRoleMasterListener)  {
			vortex_connection_unref (connection, "vortex_connection_close");
			return axl_true;
		}
	}

 	/* unref vortex connection resources, but check first this is
 	 * not a listener connection, which is already owned by the
 	 * vortex engine */
 	if (connection->role == VortexRoleInitiator) {
 		vortex_log (VORTEX_LEVEL_DEBUG, "connection close finished, now unrefering (refcount=%d)..", 
 			    connection->ref_count);
 		vortex_connection_unref (connection, "vortex_connection_close");
 	} /* end if */

	return axl_true;
}

/**
 * @internal Reference counting update implementation.
 */
axl_bool               vortex_connection_ref_internal                    (VortexConnection * connection, 
									  const char       * who,
									  axl_bool           check_ref)
{
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx * ctx;
#endif

	v_return_val_if_fail (connection, axl_false);
	if (check_ref)
		v_return_val_if_fail (vortex_connection_is_ok (connection, axl_false), axl_false);

#if defined(ENABLE_VORTEX_LOG)
	/* get a reference to the ctx */
	ctx = connection->ctx;
#endif
	
	/* lock ref/unref operations over this connection */
	vortex_mutex_lock   (&connection->ref_mutex);

	/* increase and log the connection increased */
	connection->ref_count++;

	vortex_log (VORTEX_LEVEL_DEBUG, "%d increased connection id=%d (%p) reference to %d by %s\n",
		    vortex_getpid (),
		    connection->id, connection,
		    connection->ref_count, who ? who : "??" ); 

	/* unlock ref/unref options over this connection */
	vortex_mutex_unlock (&connection->ref_mutex);

	return connection->ref_count > 1;
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
 * The function return axl_true to signal that the connection reference
 * count was increased in one unit. If the function return axl_false, the
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
 * @return axl_true if the connection reference was increased or axl_false if
 * an error was found.
 */
axl_bool               vortex_connection_ref                    (VortexConnection * connection, 
								 const char       * who)
{
	/* checked ref */
	return vortex_connection_ref_internal (connection, who, axl_true);
}

/**
 * @brief Allows to perform a ref count operation on the connection
 * provided without checking if the connection is working (no call to
 * \ref vortex_connection_is_ok).
 *
 * @param connection The connection to update.

 * @return axl_true if the reference update operation is completed,
 * otherwise axl_false is returned.
 */
axl_bool               vortex_connection_uncheck_ref           (VortexConnection * connection)
{
	/* unchecked ref */
	return vortex_connection_ref_internal (connection, "unchecked", axl_false);
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

#if defined(ENABLE_VORTEX_LOG)
	VortexCtx  * ctx;
#endif
	int          count;

	/* do not operate if no reference is received */
	if (connection == NULL)
		return;

	/* lock the connection being unrefered */
	vortex_mutex_lock     (&(connection->ref_mutex));

#if defined(ENABLE_VORTEX_LOG)
	/* get context */
	ctx = connection->ctx;
#endif

	/* decrease reference counting */
	connection->ref_count--;

	vortex_log (VORTEX_LEVEL_DEBUG, "%d decreased connection id=%d (%p) reference count to %d decreased by %s\n", 
		    vortex_getpid (),
		    connection->id, connection,
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
	int result;

	/* check reference received */
	if (connection == NULL)
		return -1;

	/* return the reference count */
	/* vortex_mutex_lock     (&(connection->ref_mutex)); */
	result = connection->ref_count;
	/* vortex_mutex_unlock     (&(connection->ref_mutex)); */
	return result;
}

/** 
 * @brief Allows to configure vortex internal timeouts for synchrnous
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
 * @param microseconds_to_wait Timeout value to be used. Providing a
 * value of 0, will reset the timeout to the default value.
 */
void               vortex_connection_timeout (VortexCtx * ctx,
					      long        microseconds_to_wait)
{
	/* get current context */
	char      * value;
	
	/* check ctx */
	if (ctx == NULL)
		return;
	
	/* clear previous value */
	if (microseconds_to_wait == 0) {
		/* reset value */
		vortex_support_unsetenv ("VORTEX_SYNC_TIMEOUT");
	} else {
		/* set new value */
		value = axl_strdup_printf ("%ld", microseconds_to_wait);
		vortex_support_setenv ("VORTEX_SYNC_TIMEOUT", value);
		axl_free (value);
	} /* end if */

	/* make the next call to vortex_connection_get_timeout to
	 * recheck the value */
	ctx->connection_timeout_checked = axl_false;

	return;
}

/** 
 * @brief Allows to configure vortex connect timeout.
 * 
 * This function allows to set the TCP connect timeout used by \ref
 * vortex_connection_new_full and \ref vortex_connection_new. 
 *
 * If you call to create a new connection with \ref vortex_connection_new
 * and connect does not succeed within the period
 * \ref vortex_connection_new will return with a non-connected vortex
 * connection.
 *
 * Value configured on this function, will be returned by \ref
 * vortex_connection_get_connect_timeout. By default, if not
 * configured, no timeout is implemented.
 *
 * See also \ref vortex_connection_timeout which implements the
 * general timeout used by vortex engine for I/O operations.
 *
 * @param ctx The context where the operation will be performed.
 *
 * @param microseconds_to_wait Timeout value to be used. The value
 * provided is measured in microseconds (1 seg = 1000000 us). Use 0 to restore the connect
 * timeout to the default value.
 */
void               vortex_connection_connect_timeout (VortexCtx * ctx,
						      long        microseconds_to_wait)
{
	/* get current context */
	char      * value;

	/* check reference received */
	if (ctx == NULL)
		return;
	
	/* clear previous value */
	if (microseconds_to_wait == 0) {
		/* unset value */
		vortex_support_unsetenv ("VORTEX_CONNECT_TIMEOUT");
	} else {
		/* set new value */
		value = axl_strdup_printf ("%ld", microseconds_to_wait);
		vortex_support_setenv ("VORTEX_CONNECT_TIMEOUT", value);
		axl_free (value);
	} /* end if */

	/* make the next call to vortex_connection_get_connect_timeout
	 * to recheck the value */
	ctx->connection_connect_timeout_checked = axl_false;

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
long                vortex_connection_get_timeout (VortexCtx * ctx)
{
        
        long       d_timeout;
	
	/* check reference */
	if (ctx == NULL) {
		/* this value must be synchrnozied by the value
		 * configured at vortex_connection_init. */
		return (60000000);
	} /* end if */

	/* get current context */
	d_timeout   = ctx->connection_std_timeout;

	/* check if we have used the current environment variable */
	vortex_mutex_lock  (&(ctx->ref_mutex));
	if (! ctx->connection_timeout_checked) {
		ctx->connection_timeout_checked = axl_true;
		d_timeout                       = vortex_support_getenv_int ("VORTEX_SYNC_TIMEOUT");
	} /* end if */

	if (d_timeout == 0) {
		/* no timeout definition done using default timeout 10 seconds */
		d_timeout = ctx->connection_std_timeout;
	} else if (d_timeout != ctx->connection_std_timeout) {
		/* update current std timeout */
		ctx->connection_std_timeout = d_timeout;
	} /* end if */

	vortex_mutex_unlock  (&(ctx->ref_mutex));

	/* return current value */
	return d_timeout;
}

/** 
 * @brief Allows to get current timeout set for \ref VortexConnection
 * connect operation.
 *
 * See also \ref vortex_connection_connect_timeout.
 *
 * @return Current timeout configured. Returned value is measured in
 * microseconds (1 second = 1000000 microseconds). If a null value is
 * received, 0 is return and no timeout is implemented.
 */
long              vortex_connection_get_connect_timeout (VortexCtx * ctx)
{
	/* check context recevied */
	if (ctx == NULL) {
		/* get the the default connect */
		return (60000000);
	} /* end if */
		
	/* check if we have used the current environment variable */
	if (! ctx->connection_connect_timeout_checked) {
		ctx->connection_connect_timeout_checked = axl_true;
		ctx->connection_connect_std_timeout     = vortex_support_getenv_int ("VORTEX_CONNECT_TIMEOUT");
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
 * @param free_on_fail if axl_true the connection will be closed using
 * vortex_connection_close on not connected status.
 * 
 * @return current connection status for the given connection
 */
axl_bool                vortex_connection_is_ok (VortexConnection * connection, 
						 axl_bool           free_on_fail)
{
	axl_bool  result = axl_false;

	/* check connection null referencing. */
	if  (connection == NULL) 
		return axl_false;

	/* check for the socket this connection has */
	vortex_mutex_lock  (&(connection->ref_mutex));
	result = (connection->session < 0) || (! connection->is_connected);
	vortex_mutex_unlock  (&(connection->ref_mutex));

	/* implement free_on_fail flag */
	if (free_on_fail && result) {
		vortex_connection_close (connection);
		return axl_false;
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
const char   * vortex_connection_get_message (VortexConnection * connection)
{
	if (connection == NULL)
		return NULL;

	return connection->message;
}

/** 
 * @brief Returns the current status for the provided connection. This
 * function can be used to check the connection status (error codes)
 * just before an operation have failed. In the case the connection is
 * properly running, the function will return \ref VortexOk.
 *
 * This function complements and completes the function provided by
 * \ref vortex_connection_get_message which returns an textual error
 * (but not an error code).
 *
 * @param connection The connection that is required to return its
 * status.
 *
 * @return Current status error 
 */
VortexStatus        vortex_connection_get_status             (VortexConnection * connection)
{
	v_return_val_if_fail (connection, VortexWrongReference);
	/* return current status */
	return connection->status;
}

/** 
 * @brief Allows to get the next channel error message stored in the
 * provided connection.
 *
 * Every time a channel creation attempt finish without the channel
 * created an error code and a textual diagnostic is returned by the
 * remote side. This function allows to get that messages. 
 *
 * The function returns axl_true if a pending channel creation error
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
 * @return axl_true if an error message was pending to be retrieved,
 * otherwise axl_false is returned. The function also returns axl_false if
 * some argument is null. 
 */
axl_bool            vortex_connection_pop_channel_error      (VortexConnection  * connection, 
							      int               * code,
							      char             ** msg)
{
	VortexChannelError * error;

	/* do not operate if null values are received */
	if (! (connection && code && msg))
		return axl_false;

	/* lock the connection during operations */
	vortex_mutex_lock (&connection->pending_errors_mutex);

	/* clear values received */
	*code = 0;
	*msg  = NULL;

	if (connection->pending_errors == NULL || axl_stack_is_empty (connection->pending_errors)) {
		/* unlock and return */
		vortex_mutex_unlock (&connection->pending_errors_mutex);

		/* no error to be reported */
		return axl_false;
	} /* end if */

	/* get next error to report */
	error = axl_stack_pop (connection->pending_errors);
	if (error == NULL) {
		/* unlock and return */
		vortex_mutex_unlock (&connection->pending_errors_mutex);

		/* no error to be reported */
		return axl_false;
	}
	*code = error->code;
	*msg  = error->msg;

	/* make the message reference to be owned by the caller */
	error->msg = NULL;
	__vortex_connection_free_channel_error (error);

	/* unlock */
	vortex_mutex_unlock (&connection->pending_errors_mutex);

	return axl_true;
}

/**
 * @internal Function used to record and error and then shutdown the
 * connection in the same step.
 *
 * @param conn The connection where the error was detected.
 * @param message The message to report
 * 
 */
void                __vortex_connection_shutdown_and_record_error (VortexConnection * conn,
								   VortexStatus       status,
								   const char       * message,
								   ...)
{
	va_list     args;
	char      * _msg;
	char      * aux;
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx * ctx;
#endif
	if (conn == NULL)
		return;

#if defined(ENABLE_VORTEX_LOG)
	/* get reference */
	ctx = conn->ctx;
#endif

	/* log error */
	if (status != VortexOk && status != VortexConnectionCloseCalled) {

		/* prepare message */
		va_start (args, message);
		_msg = axl_strdup_printfv (message, args);
		va_end (args);

		aux  = _msg;
		_msg = axl_strdup_printf ("%s (socket=%d, conn-id=%d, errno=%d, remote-ip=%s, serverName=%s, status=%d)", 
					  aux, conn->session, conn->id, errno, 
					  vortex_connection_get_host_ip (conn), vortex_connection_get_server_name (conn), 
					  status);
		axl_free (aux);

		vortex_log (VORTEX_LEVEL_CRITICAL, _msg);
		
		/* push an error into the connection */
		vortex_connection_push_channel_error (conn, status, _msg);

		/* release message */
		axl_free (_msg);
	} 
	
	/* shutdown connection */
	vortex_connection_shutdown (conn);

	return;
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
							      const char        * msg)
{
	VortexChannelError * error;

	/* check reference received */
	if (connection == NULL || msg == NULL)
		return;
	
	/* lock the connection during operations */
	vortex_mutex_lock (&connection->pending_errors_mutex);

	/* initialize pending errors stack on demand */
	if (connection->pending_errors == NULL) {
		connection->pending_errors = axl_stack_new ((axlDestroyFunc) __vortex_connection_free_channel_error);
		if (connection->pending_errors == NULL) {
			vortex_mutex_unlock (&connection->pending_errors_mutex);
			return;
		} /* end if */
	} /* end if */

	/* create the value */
	error       = axl_new (VortexChannelError, 1);
	if (error != NULL) {
		error->code = code;
		error->msg  = axl_strdup (msg);

		/* push the data */
		axl_stack_push (connection->pending_errors, error);
			
	} /* end if */
	
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
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx          * ctx;
#endif
	
	if (connection == NULL)
		return;

#if defined(ENABLE_VORTEX_LOG)
	/* get a reference to the context reference */
	ctx = connection->ctx;
#endif

	vortex_log (VORTEX_LEVEL_DEBUG, "freeing connection id=%d (%p)", connection->id, connection);

	/*
	 * NOTE: The order in which the channels and the channel pools
	 * are closed must be this way: first channels and the channel
	 * pools. Doing it other way will produce funny dead-locks.
	 */
	vortex_log (VORTEX_LEVEL_DEBUG, "freeing connection id=%d channels", connection->id);

	/* free channel resources */
	if (connection->channels) {
		/* call to notify remaining channels pending to be
		 * closed */
		vortex_hash_foreach2 (connection->channels, __vortex_connection_foreach_check_and_notify, connection, INT_TO_PTR(axl_false));

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
		axl_free (connection->local_addr);
		connection->host       = NULL;
		connection->local_addr = NULL;
	}
	axl_free (connection->host_ip);

	vortex_log (VORTEX_LEVEL_DEBUG, "freeing connection port id=%d", connection->id);

	if (connection->port != NULL) {
		axl_free (connection->port);
		axl_free (connection->local_port);
		connection->port       = NULL;
		connection->local_port = NULL;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "freeing connection profiles id=%d", connection->id);

	/* do not free remote profile list because they are handled by
	 * the greetings cache */
	connection->remote_supported_profiles = NULL;

	/* profile masks */
	axl_list_free (connection->profile_masks);
	connection->profile_masks = NULL;
	vortex_mutex_destroy (&connection->profile_masks_mutex);

	/* do not free features and localize because they are handled
	 * thourgh the cache */
	connection->features = NULL;
	connection->localize = NULL;

	/* free pending line */
	axl_free (connection->pending_line);
	connection->pending_line = NULL;

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
	if (connection->pending_errors) {
		while (! axl_stack_is_empty (connection->pending_errors)) {
			/* pop error */
			error = axl_stack_pop (connection->pending_errors);
			
			/* free the error */
			axl_free (error->msg);
			axl_free (error);
		} /* end if */
		/* free the stack */
		axl_stack_free (connection->pending_errors);
		connection->pending_errors = NULL;
	} /* end if */
	vortex_mutex_destroy (&connection->pending_errors_mutex);

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

	/* free posible frame and buffer */
	axl_free (connection->buffer);
	vortex_frame_free (connection->last_frame);

	/* release reference to context */
	vortex_ctx_unref2 (&connection->ctx, "end connection");

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
		if (! vortex_connection_is_profile_filtered (connection, -1, uri, NULL, 0, NULL, 0, NULL)) {
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
	if (node == NULL) {
		/* allocation failure */
		return -1;
	}
	/* lock during operation */
	vortex_mutex_lock (&connection->profile_masks_mutex);

	/* create list on demand */
	if (connection->profile_masks == NULL) {
		connection->profile_masks = axl_list_new (axl_list_always_return_1, axl_free);
		if (connection->profile_masks == NULL) {
			axl_free (node);
			vortex_mutex_unlock (&connection->profile_masks_mutex);
			return -1;
		} /* end if */
	}

	node->mask_id   = axl_list_length (connection->profile_masks);
	node->mask      = mask;
	node->user_data = user_data;

	/* install the profile mask */
	axl_list_append (connection->profile_masks, node);

	/* unlock now the item is removed */
	vortex_mutex_unlock (&connection->profile_masks_mutex);

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
 * @param encoding Signals the encoding used for profile_content received.
 *
 * @param serverName Optional parameter used to notify the serverName
 * provided at the start channel stage. You can safely provide a NULL
 * value if you are only checking if a particular profile is being
 * filtered on the particular connection.
 *
 * @param frame Optional parameter that defines the frame received
 * containing the start channel request (channel_num > 0).
 * 
 * @param error_msg Optional reference where the error message to be
 * returned can be configured.
 * 
 * @return axl_true if the if the profile is filtered, otherwise axl_false is
 * returned.
 */
axl_bool            vortex_connection_is_profile_filtered    (VortexConnection      * connection,
							      int                     channel_num,
							      const char            * uri,
							      const char            * profile_content,
							      VortexEncoding          encoding,
							      const char            * serverName,
							      VortexFrame           * frame,
							      char                 ** error_msg)
{
	int              iterator = 0;
	VortexMaskNode * node;

	/* check received data */
	if (connection == NULL || uri == NULL)
		return axl_false;

	/* look during the operation */
	vortex_mutex_lock (&connection->profile_masks_mutex);
	if (connection->profile_masks == NULL) {
		/* check finished */
		vortex_mutex_unlock (&connection->profile_masks_mutex);
		return axl_false;
	} /* end if */

	/* check all mask installed */
	while (iterator < axl_list_length (connection->profile_masks)) {
		
		/* get the mask reference */
		node = axl_list_get_nth (connection->profile_masks, iterator);

		/* check if the mask filter the provided profile */
		vortex_mutex_unlock (&connection->profile_masks_mutex);
		if (node->mask (connection, channel_num, uri, profile_content, encoding, serverName, frame, error_msg, node->user_data)) {

			/* uri filtered, report */
			return axl_true;
		}
		vortex_mutex_lock (&connection->profile_masks_mutex);

		/* update the iterator */
		iterator++;
		
	} /* end while */

	/* check finished */
	vortex_mutex_unlock (&connection->profile_masks_mutex);

	/* no mask have filtered the uri */
	return axl_false;
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
 * @return axl_true if the profile is supported by remote peer or axl_false if not.
 */
axl_bool           vortex_connection_is_profile_supported (VortexConnection * connection, 
							   const char       * uri)
{
	axl_bool    result;
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx * ctx;
#endif

	/* check reference received */
	if (connection == NULL || uri == NULL)
		return axl_false;

#if defined(ENABLE_VORTEX_LOG)
	/* get a reference to the context */
	ctx = connection->ctx;
#endif

	if (!connection->is_connected) {
		vortex_log (VORTEX_LEVEL_CRITICAL, 
		       "trying to get supported profile in a non-connected session");
		return axl_false;
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
 * @return axl_true if channel identified by <i>channel_num</i> exists,
 * otherwise axl_false.
 */
axl_bool                vortex_connection_channel_exists       (VortexConnection * connection, int  channel_num)
{
	axl_bool      result;

	if (channel_num < 0 || connection == NULL || ! connection->is_connected )
		return axl_false;

	/* channel 0 always exists, and cannot be closed. It's closed
	 * when connection (or session) is closed */
	if (channel_num == 0) 
		return axl_true;
	
	result = (vortex_hash_lookup (connection->channels, 
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
	if (connection == NULL || connection->channels == NULL)
		return -1;
	
	/* return number of channels */
	return axl_hash_items (connection->channels->table);
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
 * @return axl_true if the foreach operation was completed, otherwise
 * axl_false is returned.
 */
int             vortex_connection_foreach_channel        (VortexConnection * connection,
							  axlHashForeachFunc func,
							  axlPointer         user_data)
{
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx * ctx;
#endif

	/* do not operate */
	v_return_val_if_fail (connection, axl_false);
	v_return_val_if_fail (func, axl_false);

#if defined(ENABLE_VORTEX_LOG)
	/* get context */
	ctx = connection->ctx;
#endif

	/* try to ref the connection to avoid loosing the reference
	 * during the process. */
	if (! vortex_connection_ref (connection, "foreach-channel")) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Failed to foreach because ref operation over connection id=%d (%p) have failed",
			    connection->id, connection);
		return axl_false;
	}

	/* perform the foreach operation */
	vortex_hash_foreach (connection->channels, func, user_data);

	/* unref connection increased */
	vortex_connection_unref (connection, "foreach-channel");

	return axl_true;
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
	
	while (axl_true) {
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
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx     * ctx;
#endif

	/* check values returned */
	if (connection == NULL || ! connection->is_connected || channel_num < 0)
		return NULL;

	/* channel 0 always exists, and cannot be closed. It's closed
	 * when connection (or session) is closed */
	channel = vortex_hash_lookup (connection->channels, INT_TO_PTR(channel_num));
	
#if defined(ENABLE_VORTEX_LOG)
	if (channel == NULL) {
		/* get a reference to the ctx */
		ctx = connection->ctx;
		vortex_log (VORTEX_LEVEL_DEBUG, "failed to get channel=%d", channel_num);
	}
#endif
	return channel;
}

/** 
 * @internal Function supporting vortex_connection_get_channel_by_uri.
 */
axl_bool  __vortex_connection_get_by_uri_foreach (axlPointer key, axlPointer data, axlPointer user_data, axlPointer user_data2)
{
	VortexChannel  * channel  = data;
	VortexChannel ** result   = user_data;
	const char     * profile  = user_data2;

	/* check if the channel is running the profile requested */
	if (vortex_channel_is_running_profile (channel, profile)) {
		/* channel found, stop the foreach operation and
		 * update the reference */
		*result = channel;
		return axl_true;
	} /* end if */

	/* item not found */
	return axl_false;
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
axl_bool  __vortex_connection_get_by_func_foreach (axlPointer key, 
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
		return axl_true;
	} /* end if */

	/* item not found */
	return axl_false;
}

axl_bool  __vortex_connection_count_channel_foreach (axlPointer key, 
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
	return axl_false;
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
 * function returns axl_true. For example:
 *
 * \code
 * axl_bool  select_channel (VortexChannel * channel, axlPointer user_data)
 * {
 *       // implement here some selection pattern and return axl_true
 *       // to select
 *       return axl_true;
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
 * @param action axl_true if the given connection have to be close once a
 * Vortex Connection is closed. axl_false to avoid the socket to be
 * closed.
 */
void                vortex_connection_set_close_socket       (VortexConnection * connection, 
							      axl_bool           action)
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
axl_bool           vortex_connection_add_channel          (VortexConnection * connection, 
							   VortexChannel    * channel)
{
	/* call to common implementation */
	return vortex_connection_add_channel_common (connection, channel, axl_true);
}

/** 
 * @brief Adds a VortexChannel into an existing VortexConnection,
 * allowing to configure notification.
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
 *
 * @param channel the channel to add.
 *
 * @param do_notify axl_true to fire handlers configured at \ref
 * vortex_connection_set_channel_added_handler otherwise, this
 * notification is not done.
 */
axl_bool           vortex_connection_add_channel_common (VortexConnection * connection,
							 VortexChannel    * channel,
							 axl_bool           do_notify)
{
	VortexChannel * _channel;
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx     * ctx;
#endif
	
	/* perform some aditional checks */
	if (connection == NULL || channel == NULL || connection->channels == NULL || connection->role == VortexRoleMasterListener)
		return axl_false;

	/* lock channel hash */
	vortex_mutex_lock (&connection->channel_mutex);

	/* check if channel doesn't exists on this connection */
	_channel = vortex_hash_lookup (connection->channels, INT_TO_PTR (vortex_channel_get_number (channel)));
	if (_channel != NULL) {
		vortex_mutex_unlock (&connection->channel_mutex);
#if defined(ENABLE_VORTEX_LOG)
		/* get a reference to the ctx */
		ctx = connection->ctx;
#endif
		vortex_log (VORTEX_LEVEL_CRITICAL, "trying to add a channel on a connection which already have this channel");
		return axl_false;
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
	if (do_notify) 
		__vortex_connection_check_and_notify (connection, channel, axl_true);

	return axl_true;
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
	vortex_connection_remove_channel_common (connection, channel, axl_true);
	return;
}

/** 
 * @brief Removes the given channel from this connection. 
 * 
 * @param connection the connection where the channel will be removed.
 *
 * @param channel the channel to remove from the connection.
 *
 * @param do_notify Allows to configure if the channel removal
 * notification is done. If axl_false is provided, handlers configured
 * by \ref vortex_connection_set_channel_removed_handler aren't
 * called.
 */
void                vortex_connection_remove_channel_common  (VortexConnection * connection, 
							      VortexChannel    * channel,
							      axl_bool           do_notify)
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

	vortex_log (VORTEX_LEVEL_DEBUG, "removing channel id=%d (conn refs: %d, channels: %d)", channel_num,
		    vortex_connection_ref_count (connection), vortex_connection_channels_count (connection));

	/* check and notify the channel to be removed. */
	if (do_notify)
		__vortex_connection_check_and_notify (connection, channel, axl_false);

	/* remove channel from sequencer (if registered) */
	vortex_sequencer_remove_channel (ctx, channel);

	/* remove the channel */
	vortex_hash_remove (connection->channels, INT_TO_PTR (channel_num));

	vortex_log (VORTEX_LEVEL_DEBUG, "after channel id=%d remove (conn refs: %d, channels: %d)", channel_num,
		    connection->ref_count, vortex_connection_channels_count (connection));
	
	return;
}

/** 
 * @internal Function used by vortex to notify that this connection
 * has either writen or received content, setting a timestamp, which
 * allows checking if the connection was idle.
 *
 * @param conn The connection that received or produced content.
 * @param bytes Bytes received on this stamp.
 */
void                vortex_connection_set_receive_stamp            (VortexConnection * conn, long bytes_received, long bytes_sent)
{
	/* set that content was received */
	vortex_mutex_lock (&conn->ref_mutex);
	conn->last_idle_stamp  = (long) time (NULL);
	conn->bytes_received  += bytes_received;
	conn->bytes_sent      += bytes_sent;
	vortex_mutex_unlock (&conn->ref_mutex);

	return;
}

/** 
 * @internal allows to get bytes received so far and last idle stamp
 * (idle since that stamp) on the provided connection.
 */ 
void                vortex_connection_get_receive_stamp            (VortexConnection * conn, 
								    long             * bytes_received, 
								    long             * bytes_sent,
								    long             * last_idle_stamp)
{
	if (bytes_received != NULL)
		(*bytes_received) = 0;
	if (bytes_sent != NULL)
		(*bytes_sent) = 0;
	if (last_idle_stamp != NULL)
		(*last_idle_stamp) = 0;
	if (conn == NULL)
		return;
	vortex_mutex_lock (&conn->ref_mutex);
	if (bytes_received != NULL)
		(*bytes_received) = conn->bytes_received;
	if (bytes_sent != NULL)
		(*bytes_sent) = conn->bytes_sent;
	if (last_idle_stamp != NULL)
		(*last_idle_stamp) = conn->last_idle_stamp;
	vortex_mutex_unlock (&conn->ref_mutex);

	return;
}

/** 
 * @internal Function used to check idle status, calling the handler
 * defined if the idle status is reached. The function also resets the
 * idle status, so it can be called in the future. 
 */
void                vortex_connection_check_idle_status            (VortexConnection * conn, VortexCtx * ctx, long time_stamp)
{
	/* do not notify master listeners activity (they don't have) */
	if (conn->role == VortexRoleMasterListener)
		return;

	/* check if the connection was never checked */
	if (conn->last_idle_stamp == 0) {
		vortex_connection_set_receive_stamp (conn, 0, 0);
		return;
	} /* end if */

	/* check idle status */
	if ((time_stamp - conn->last_idle_stamp) > ctx->max_idle_period) {
		vortex_log (VORTEX_LEVEL_DEBUG, "Found idle connection id=%d, notifying..", vortex_connection_get_id (conn));
		/* notify idle ref */
		vortex_ctx_notify_idle (ctx, conn);

		return;
	} /* end if */
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
 * @internal Function used to setup manuall values returned by \ref
 * vortex_connection_get_host and \ref vortex_connection_get_port.
 */
void                vortex_connection_set_host_and_port      (VortexConnection * connection, 
							      const char       * host,
							      const char       * port,
							      const char       * host_ip)
{
	v_return_if_fail (connection && host && port);
	
	if (connection->host)
		axl_free (connection->host);
	if (connection->port)
		axl_free (connection->port);
	if (connection->host_ip)
		axl_free (connection->host_ip);
	
	/* set host, port and ip value */
	connection->host    = axl_strdup (host);
	connection->port    = axl_strdup (port);
	connection->host_ip = axl_strdup (host_ip);

	return;
}

/** 
 * @brief Allows to get the actual host ip this connection is
 * connected to.
 *
 * This function works like \ref vortex_connection_get_host_ip but
 * returning the actual ip in the case a name was used.
 *
 * @return A reference to the IP or NULL if it fails.
 */
const char        * vortex_connection_get_host_ip            (VortexConnection * connection)
{
	struct sockaddr_in sin;
#if defined(AXL_OS_WIN32)
	/* windows flavors */
	int                sin_size     = sizeof (sin);
#else
	/* unix flavors */
	socklen_t          sin_size     = sizeof (sin);
#endif
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx        * ctx;

	/* setup context */
	ctx = connection->ctx;
#endif

	/* check input parameters */
	if (connection == NULL)
		return NULL;

	/* acquire lock to check if host ip was defined previously */
	vortex_mutex_lock (&connection->op_mutex);
	if (connection->host_ip) {
		vortex_mutex_unlock (&connection->op_mutex);
		return connection->host_ip;
	} /* end if */

	if (connection->role == VortexRoleMasterListener) {
		if (getsockname (connection->session, (struct sockaddr *) &sin, &sin_size) < 0) {
			vortex_log (VORTEX_LEVEL_CRITICAL, "getsockname failed from socket=%d, errno=%d (%s)", 
				    connection->session, errno, vortex_errno_get_error (errno));
			vortex_mutex_unlock (&connection->op_mutex);
			return NULL;
		} /* end if */
	} else {
		/* get actual IP value */
		if (getpeername (connection->session, (struct sockaddr *) &sin, &sin_size) < 0) {
			vortex_log (VORTEX_LEVEL_CRITICAL, "getpeername failed from socket=%d, errno=%d (%s)", 
				    connection->session, errno, vortex_errno_get_error (errno));
			vortex_mutex_unlock (&connection->op_mutex);
			return NULL;
		} /* end if */
	}

	/* set local addr and local port */
	connection->host_ip = vortex_support_inet_ntoa (connection->ctx, &sin);

	/* unlock and return value created */
	vortex_mutex_unlock (&connection->op_mutex);
	return connection->host_ip;
}

/** 
 * @brief  Returns the connection unique identifier.
 *
 * The connection identifier is a unique integer assigned to all
 * connection created under Vortex Library. This allows Vortex programmer to
 * use this identifier for its own application purposes
 *
 * @param connection The connection to get the the unique integer
 * identifier from.
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
 * @brief Allows to get the serverName under which the remote BEEP
 * peer is working. 
 *
 * During the BEEP session, the first channel created under a provided
 * serverName attribute is meaningful for the rest of the
 * session. This means that the connection gets flagged with the
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
 * first caller. Rest of the callers will fail to set the name (the
 * function will do nothing) if the serverName is found to be
 * configured.
 * 
 * @param connection The connection to configure its serverName.
 * @param serverName The server name value to configured.
 */
void                vortex_connection_set_server_name         (VortexConnection * connection,
							       const char       * serverName)
{
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx * ctx = CONN_CTX (connection);
#endif
	int iterator = 0;

	/* check if the connection is null or the serverName is
	 * null */
	if (connection == NULL || serverName == NULL || connection->serverName != NULL)
		return;

	/* configure serverName */
	connection->serverName = axl_strdup (serverName);

	/* remove : values and the rest behind it */
	while (connection->serverName[iterator]) {
		if (connection->serverName[iterator] == ':')
			connection->serverName[iterator] = 0;

		/* next iterator*/
		iterator++;
	}
	vortex_log (VORTEX_LEVEL_DEBUG, "Received serverName %s and configured %s", serverName, connection->serverName);
	
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
 * @brief Allows to get local address used by the connection.
 * @param connection The connection to check.
 *
 * @return A reference to the local address used or NULL if it fails.
 */
const char        * vortex_connection_get_local_addr         (VortexConnection * connection)
{
	/* check reference received */
	if (connection == NULL)
		return NULL;

	return connection->local_addr;
}

/** 
 * @brief Allows to get the local port used by the connection. 
 * @param connection The connection to check.
 * @return A reference to the local port used or NULL if it fails.
 */
const char        * vortex_connection_get_local_port         (VortexConnection * connection)
{
	/* check reference received */
	if (connection == NULL)
		return NULL;

	return connection->local_port;
}

typedef struct __VortexOnCloseNotify {
	VortexConnection              * conn;
	VortexConnectionOnCloseFull     handler;
	axlPointer                      data;
	axl_bool                        is_full;
} VortexOnCloseNotify;

axlPointer __vortex_connection_on_close_do_notify (VortexOnCloseNotify * data)
{
#if defined(ENABLE_VORTEX_LOG) && ! defined(SHOW_FORMAT_BUGS)
	VortexCtx * ctx = CONN_CTX (data->conn);
#endif
	/* do notification */
	if (data->is_full) 
		data->handler (data->conn, data->data);
	else
		((VortexConnectionOnClose) data->handler) (data->conn);

	/* terminate thread */
	vortex_log (VORTEX_LEVEL_DEBUG, "async on close notification done for conn-id=%d..", vortex_connection_get_id (data->conn));

	vortex_connection_unref (data->conn, "on-close-notification");
	axl_free (data);
	return NULL;
}

void __vortex_connection_invoke_on_close_do_notify (VortexConnection            * conn, 
						    VortexConnectionOnCloseFull   handler, 
						    axlPointer                    user_data, 
						    axl_bool                      is_full)
{
	VortexOnCloseNotify * data;
#if defined(ENABLE_VORTEX_LOG) && ! defined(SHOW_FORMAT_BUGS)
	VortexCtx           * ctx = CONN_CTX (conn);
#endif
	VortexThread          thread_def;

	/* acquire data to launch the thread */
	data = axl_new (VortexOnCloseNotify, 1);
	if (data == NULL)
		return;

	/* set all data */
	data->conn    = conn;
	data->handler = handler;
	data->data    = user_data;
	data->is_full = is_full;
	
	/* increase reference counting during thread activation */
	vortex_connection_ref_internal (conn, "on-close-notification", axl_false);

	/* call to notify using a thread */
	vortex_log (VORTEX_LEVEL_DEBUG, "calling to create thread to implement on close notification for=%d", vortex_connection_get_id (conn));
	if (! vortex_thread_create (&thread_def, (VortexThreadFunc) __vortex_connection_on_close_do_notify, data, 
				    VORTEX_THREAD_CONF_DETACHED, VORTEX_THREAD_CONF_END)) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "failed to start thread to do connection-id=%d close notify", vortex_connection_get_id (conn));

		/* release data due to failure */
		vortex_connection_unref (conn, "on-close-notification");
		axl_free (data);
		return;
	}

	return;
} 

/** 
 * @internal 
 *
 * Function used to perform on close invocation
 * 
 * @param conneciton The connection where all on close handlers will
 * be notified.
 */
void __vortex_connection_invoke_on_close (VortexConnection * connection, 
					  axl_bool           is_full)
{
	VortexConnectionOnClose        on_close_handler;
	VortexConnectionOnCloseData  * handler;
	VortexCtx                    * ctx = connection->ctx;

	/* check if context is in process of finishing */
	if (ctx->vortex_exit) 
		return;

	/* lock now the op mutex is not blocked */
	vortex_mutex_lock (&connection->handlers_mutex);

	/* invoke full */
	if (is_full) {

		/* iterate over all full handlers and invoke them */
		while (axl_list_length (connection->on_close_full) > 0) {

			/* get a reference to the handler */
			handler = axl_list_get_first (connection->on_close_full);

			vortex_log (VORTEX_LEVEL_DEBUG, "running on close full handler %p conn-id=%d, remaining handlers: %d",
				    handler, connection->id, axl_list_length (connection->on_close_full));

			/* remove the handler from the list */
			axl_list_unlink_first (connection->on_close_full);

			/* unlock now the op mutex is not blocked */
			vortex_mutex_unlock (&connection->handlers_mutex);

			/* invoke */
			__vortex_connection_invoke_on_close_do_notify (connection, handler->handler, handler->data, axl_true);
			
			/* free handler node */
			axl_free (handler);

			/* reacquire the mutex */
			vortex_mutex_lock (&connection->handlers_mutex);
		} /* end if */

	} else {

		/* iterate over all full handlers and invoke them */
		while (axl_list_length (connection->on_close) > 0) {
			/* get a reference to the handler */
			on_close_handler = axl_list_get_first (connection->on_close);

			/* remove the handler from the list */
			axl_list_unlink_first (connection->on_close);

			/* unlock now the op mutex is not blocked */
			vortex_mutex_unlock (&connection->handlers_mutex);

			/* invoke */
			__vortex_connection_invoke_on_close_do_notify (connection, (VortexConnectionOnCloseFull) on_close_handler, NULL, axl_false);

			/* reacquire the mutex */
			vortex_mutex_lock (&connection->handlers_mutex);
		} /* end if */

	} /* if */

	/* unlock now the op mutex is not blocked */
	vortex_mutex_unlock (&connection->handlers_mutex);

	return;
} 

/** 
 * @brief Shutdown the connection provided immediately without doing
 * BEEP session close, flagging the connection as non connected
 * (without deallocating resources associated to the connection).
 * 
 * You can use this function to perform a forced connection close
 * (without meeting BEEP requirements while closing sessions). 
 * 
 * From a listener perspective (a connection created pasively), once
 * the connection is closed using this function, vortex library will
 * detect its status, removing all its references to the connection
 * (\ref vortex_connection_unref). 
 *
 * From a client perspective (the side that created the connection
 * using \ref vortex_connection_new) a call to \ref
 * vortex_connection_close after calling this function is required.
 *
 * How to know if you must call to \ref vortex_connection_close after
 * calling to \ref vortex_connection_shutdown? Your application must
 * perform a call to \ref vortex_connection_close if it issued a call
 * to \ref vortex_connection_new.
 * 
 * <b>Example 1</b>: Closing a connection at the client side:
 * \code
 * // Call to terminate the connection right now. No reference counting
 * // is updated.
 * vortex_connection_shutdown (connection);
 * 
 * // now close the connection (decreases the reference counting, that is
 * // the one assignated by vortex_connection_new) and terminates resources
 * // associated.
 * vortex_connection_close (connection);
 * 
 * // NOTE: a call to vortex_connection_close, after vortex_connection_shutdow
 * //       never fails.
 * \endcode
 *
 * <b>Example 2</b>: Closing a connection at the listener side:
 * \code
 * // Call to terminate the connection right now. No reference counting
 * // is updated.
 * vortex_connection_shutdown (connection);
 *
 * // Now let the system to collect the connection since the listener
 * // do not owns a reference (he didn't create the connection by doing
 * // a vortex_connection_new but it was received).
 * \endcode
 * 
 * The key of the function is that it do not updates the reference
 * counting. The caller will still have to do all required calls to
 * \ref vortex_connection_unref (as much as times called to \ref
 * vortex_connection_ref).
 * 
 * @param connection The connection to be shutted down.
 */
void    vortex_connection_shutdown           (VortexConnection * connection)
{
	/* call to internal set not connected */
	__vortex_connection_set_not_connected (connection, "(vortex connection shutdown)", VortexConnectionForcedClose);
	return;
	
} /* end vortex_connection_shutdown */

/** 
 * @brief Function used to just shutdown socket associated to the
 * connection.
 *
 * This function differs from \ref vortex_connection_shutdown in the
 * sense this one do not triggers all the internal connection close
 * (like calling connection close handlers, dropping frames associated
 * to this connection at the sequencer). 
 *
 * @param connection The connection where the socket will be closed.
 */
void    vortex_connection_shutdown_socket      (VortexConnection * connection)
{
	if (connection == NULL)
		return;
	vortex_close_socket (connection->session);
	/* do not set connection->session to -1 */
	return;
}

/** 
 * @brief Allows to configure a handler which is executed once a
 * channel is added to the provided connection. This could be used to
 * implement some init policy once the channel is added to the
 * connection. The function support adding several handlers.
 * 
 * @param connection The connection where the handler is going to be
 * installed. This parameter cannot be null.
 *
 * @param added_handler The handler to be called once the even
 * happens. This parameter cannot be null.
 *
 * @param user_data A reference to the user data to be passed to the
 * function.
 *
 * @return An identifier pointer that can be used later to remove the
 * handler to avoid receiving more notifications. To remove the
 * handler installed use \ref vortex_connection_remove_handler with
 * the handler type \ref CONNECTION_CHANNEL_ADD_HANDLER.
 */
axlPointer                vortex_connection_set_channel_added_handler   (VortexConnection                * connection,
									 VortexConnectionOnChannelUpdate   added_handler,
									 axlPointer                        user_data)
{
	VortexChannelStatusUpdate * update;

	/* check parameters received */
	if (connection == NULL || added_handler == NULL)
		return NULL;

	/* lock the connection */
	vortex_mutex_lock (&connection->channel_update_mutex);
	
	/* add the handler */
	update               = axl_new (VortexChannelStatusUpdate, 1);
	if (update == NULL) {
		/* unlock the connection */
		vortex_mutex_unlock (&connection->channel_update_mutex);
		/* allocation failure */
		return NULL;
	}
	update->handler      = added_handler;
	update->handler_data = user_data;

	/* check the list and add the item */
	if (connection->add_channel_handlers == NULL) {
		connection->add_channel_handlers = axl_list_new (axl_list_always_return_1, axl_free);
		if (connection->add_channel_handlers == NULL) {
			/* free allocated node */
			axl_free (update);

			/* unlock the connection */
			vortex_mutex_unlock (&connection->channel_update_mutex);
			return NULL;
		}
	}
	axl_list_add (connection->add_channel_handlers, update);

	/* unlock the connection */
	vortex_mutex_unlock (&connection->channel_update_mutex);

	return update;
}

/** 
 * @brief Allows to configure a handler which is executed once a
 * channel is removed from the provided connection. This could be used
 * to implement some init policy once the channel is removed from the
 * connection. The function support adding several handlers to be
 * called.
 * 
 * @param connection The connection where the handler is going to be
 * installed. This parameter cannot be null.
 *
 * @param removed_handler The handler to be called once the even
 * happens. This parameter cannot be null.
 *
 * @param user_data A reference to the user data to be passed to the
 * function.
 *
 * @return An identifier pointer that can be used later to remove the
 * handler to avoid receiving more notifications. To remove the
 * handler installed use \ref vortex_connection_remove_handler with
 * the handler type \ref CONNECTION_CHANNEL_REMOVE_HANDLER.
 */
axlPointer                vortex_connection_set_channel_removed_handler  (VortexConnection                * connection,
									  VortexConnectionOnChannelUpdate   removed_handler,
									  axlPointer                        user_data)
{
	VortexChannelStatusUpdate * update;
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx                 * ctx;
#endif

	/* check parameters received */
	if (connection == NULL || removed_handler == NULL)
		return NULL;

#if defined(ENABLE_VORTEX_LOG)
	/* get a reference to the context */
	ctx = connection->ctx;
#endif

	/* lock the connection */
	vortex_mutex_lock (&connection->channel_update_mutex);

	/* add the handler */
	update               = axl_new (VortexChannelStatusUpdate, 1);
	if (update == NULL) {
		/* unlock the connection */
		vortex_mutex_unlock (&connection->channel_update_mutex);
		/* allocation failure */
		return NULL;
	}
	update->handler      = removed_handler;
	update->handler_data = user_data;
	vortex_log (VORTEX_LEVEL_DEBUG, "Configure on channel removed handler at: %p",
		    removed_handler);

	/* check the list and add the item */
	if (connection->remove_channel_handlers == NULL) {
		connection->remove_channel_handlers = axl_list_new (axl_list_always_return_1, axl_free);
		if (connection->remove_channel_handlers == NULL) {
			/* free allocated node */
			axl_free (update);

			/* unlock the connection */
			vortex_mutex_unlock (&connection->channel_update_mutex);
			return NULL;
		}
	}
	axl_list_add (connection->remove_channel_handlers, update);

	/* unlock the connection */
	vortex_mutex_unlock (&connection->channel_update_mutex);

	return update;
}


/** 
 * @brief Allows to remove a particular handler (handler_type) with
 * the provided handler id.
 *
 * @param connection The connection where the handler will be removed. This value cannot be null.
 *
 * @param handler_type The handler type that is being removed.
 *
 * @param handler_id The handler unique identifier to be removed. This
 * value is returned by the function that added the handler. This value cannot be null.
 */
void                vortex_connection_remove_handler               (VortexConnection                * connection,
								    VortexConnectionHandler           handler_type,
								    axlPointer                        handler_id)
{
	/* check parameters received */
	if (connection == NULL || handler_id == NULL)
		return;
	switch (handler_type) {
	case CONNECTION_CHANNEL_ADD_HANDLER:
		/* lock especific mutex */
		vortex_mutex_lock (&connection->channel_update_mutex);

		/* remove and release memory hold by the handler id */
		axl_list_remove_ptr (connection->add_channel_handlers, handler_id);

		/* unlock especific mutex */
		vortex_mutex_unlock (&connection->channel_update_mutex);
		break;
	case CONNECTION_CHANNEL_REMOVE_HANDLER:
		/* lock especific mutex */
		vortex_mutex_lock (&connection->channel_update_mutex);

		/* remove and release memory hold by the handler id */
		axl_list_remove_ptr (connection->remove_channel_handlers, handler_id);

		/* unlock especific mutex */
		vortex_mutex_unlock (&connection->channel_update_mutex);
		break;
	default:
		/* not implemented */
		break;
	} /* end switch */
	
	return;
}

/** 
 * @brief Enable/disable connection blocking on the provided
 * reference. Activating the connection blocking will cause the BEEP
 * session associated to stop accepting any data. This is done by
 * signaling the vortex reader engine to not watch for status change
 * on the connection provided.
 *
 * The connection is still able to send content through its opened
 * channels but no content will be received.
 * 
 * @param conn The connection to (un)block.
 *
 * @param enable According to this value the connection will be
 * blocked (axl_true) otherwise axl_false must be used to make the connection
 * to accept incoming data.
 *
 * The function can also be used to block listener connections. The
 * function do not block connections accepted due to the listener.
 */
void                vortex_connection_block                        (VortexConnection * conn,
								    axl_bool           enable)
{
	v_return_if_fail (conn);
	
	/* set blocking state */
	conn->is_blocked = enable;

	/* call to restart the reader */
	vortex_reader_restart (vortex_connection_get_ctx (conn));

	return;
}

/** 
 * @brief Allows to check if the connection provided is blocked.
 * 
 * @param conn The connection to check for its associated blocking
 * state.
 * 
 * @return axl_true if the connection is blocked (no read I/O operation
 * available), otherwise axl_false is returned, signaling the connection
 * is fully I/O operational. Keep in mind the function returns axl_false
 * if the reference provided is NULL.
 */
axl_bool             vortex_connection_is_blocked                   (VortexConnection  * conn)
{
	v_return_val_if_fail (conn, axl_false);

	/* set blocking state */
	return conn->is_blocked;
}

/** 
 * @brief Allows to check if the provided connection is still in
 * transit of being accepted.
 *
 * @param conn The connection to be checked. In the case the function
 * returns axl_true it means the connection is still not fully
 * opened/accepted (still greetings phase wasn't finished).
 *
 * @return axl_true in the case the connection is half opened (BEEP
 * greetings not yet not completed) otherwise axl_false is
 * returned. The function also returns axl_false in the case or NULL
 * pointer received.
 */
axl_bool            vortex_connection_half_opened                  (VortexConnection  * conn)
{
	if (conn == NULL)
		return axl_false;
	return conn->initial_accept;
}

/** 
 * @brief Allows to get the amount of data to be used to build the
 * next frame. This function calls to the user defined configured
 * handler on the connection, acting globally to all channels. If no
 * handler is defined, the function returns -1.
 * 
 * @param connection The connection where a frame is about to be sent.
 *
 * @param channel The connection where the sending operation is taking
 * place.
 *
 * @param next_seq_no This value represent the next sequence number
 * for the first octect to be sent on the frame.
 *
 * @param message_size This value represent the size of the payload to
 * be sent.
 *
 * @param max_seq_no Is the maximum allowed seqno accepted by the
 * remote peer. Beyond this value, the remote peer will close the
 * connection.
 * 
 * @return The amount of payload to use into the next frame to be
 * built. The function will return -1 if no handler is defined or
 * connection reference received is null.
 */
int                 vortex_connection_get_next_frame_size          (VortexConnection * connection,
								    VortexChannel    * channel,
								    int                next_seq_no,
								    int                message_size,
								    int                max_seq_no)
{
	v_return_val_if_fail (connection, -1);

	/* return -1 to signal no handler is defined */
	if (connection->next_frame_size == NULL)
		return -1;

	/* call to configured handler */
	return connection->next_frame_size (channel, next_seq_no, message_size, max_seq_no, connection->next_frame_size_data);
}

/**
 * @brief Allows to control if the provided connection will produce
 * SEQ frame updates (send to the remote peer SEQ frames). 
 *
 * This function is useful for those profiles that requires a tune
 * operation, like TLS, which needs to avoid sending additional
 * information not controled by the application level (on top of
 * vortex).
 *
 * This function was introduced to avoid TLS negotiation to receive,
 * in the middle of the negotiation, a SEQ frame update do to previous
 * exchanges. However, this function is useful to other tunning
 * profiles that may require full control on that is being sent or
 * received from the wire.
 *
 * @param connection The connection to be configured.
 *
 * @param is_disabled axl_true to disable SEQ frames update. axl_false to allow SEQ
 * frames to be sent. (Default value is axl_false, that is, SEQ frames allowed).
 *
 * NOTE: Value configured by this function can be retrieved by \ref
 * vortex_connection_seq_frame_updates_status. 
 *
 * NOTE 2: This configuration affect to all channels created on the
 * connection provided.
 */
void                vortex_connection_seq_frame_updates            (VortexConnection * connection,
								    axl_bool           is_disabled)
{
#if defined(ENABLE_VORTEX_LOG) && ! defined(SHOW_FORMAT_BUGS)
	VortexCtx * ctx = CONN_CTX (connection);
#endif
	v_return_if_fail (connection);
	v_return_if_fail (vortex_connection_is_ok (connection, axl_false));

	/* set the flag */
	connection->seq_frames_disabled = is_disabled;

	vortex_log (VORTEX_LEVEL_DEBUG, "configuring SEQ frame generation status: is-disabled=%d", is_disabled);
	
	return;
}

/**
 * @brief Allows to get current configuration for SEQ frame
 * generation. See \ref vortex_connection_seq_frame_updates.
 *
 * @param connection The connection that is being checked for its SEQ
 * frame generation status.
 *
 * @return The function returns axl_true to signal that SEQ frame
 * generation is disabled. In the case axl_false is returned (default
 * state if nothing is changed), then SEQ frame generation is allowed.
 */
axl_bool            vortex_connection_seq_frame_updates_status     (VortexConnection * connection)
{
	v_return_val_if_fail (connection, axl_false);
	v_return_val_if_fail (vortex_connection_is_ok (connection, axl_false), axl_false);

	/* get the flag */
	return connection->seq_frames_disabled;

}

/** 
 * @brief Allows to configure the \ref VortexChannelFrameSize handler
 * to be used by the sequencer to decide how many data is used into
 * each frame produced (outstanding frames). 
 *
 * Configuring the handler at connection level will make all channels
 * to use this implementation unless the channel have its own \ref
 * VortexChannelFrameSize handler configured (\ref
 * vortex_channel_set_next_frame_size_handler).
 * 
 * @param connection The connection to be configured.
 *
 * @param next_frame_size The handler to be configured or NULL if the
 * default implementation is required.
 *
 * @param user_data User defined pointer to be passed to the \ref
 * VortexChannelFrameSize handler.
 * 
 * @return Returns previously configured handler or NULL if nothing
 * was set. The function does nothing and return NULL if connection
 * reference received is NULL.
 */
VortexChannelFrameSize  vortex_connection_set_next_frame_size_handler (VortexConnection        * connection,
								       VortexChannelFrameSize    next_frame_size,
								       axlPointer                user_data)
{
	VortexChannelFrameSize previous;
	v_return_val_if_fail (connection, NULL);

	/* get previous value */
	previous                         = connection->next_frame_size;

	/* configure new value (first data and then handler) */
	connection->next_frame_size_data = user_data;
	connection->next_frame_size      = next_frame_size;

	/* nullify data if a null handler is received */
	if (next_frame_size == NULL)
		connection->next_frame_size_data = NULL;

	/* return previous configuration */
	return previous;
}

/** 
 * @brief Configures default frame segmentation function (\ref
 * VortexChannelFrameSize) used for all connections and all channels
 * that do not have a segmentator defined.
 *
 * See \ref vortex_connection_set_next_frame_size_handler and \ref vortex_channel_set_next_frame_size_handler.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param next_frame_size The handler to be configured globally.
 *
 * @param user_data User defined pointer to be provided to the global
 * function to be configured.
 * 
 * @return Returns previously configured handler or NULL if nothing
 * was set. The function does nothing and return NULL if context
 * reference received is NULL.
 */
VortexChannelFrameSize  vortex_connection_set_default_next_frame_size_handler (VortexCtx               * ctx,
									       VortexChannelFrameSize    next_frame_size,
									       axlPointer                user_data)
{
	VortexChannelFrameSize    previous;

	/* check context received */
	v_return_val_if_fail (ctx, NULL);

	/* get previous value */
	previous                  = next_frame_size;

	/* configure new value (first data and then handler) */
	ctx->next_frame_size_data = user_data;
	ctx->next_frame_size      = next_frame_size;


	/* nullify data if a null handler is received */
	if (next_frame_size == NULL)
		ctx->next_frame_size_data = NULL;

	/* return previous configuration */
	return previous;
}

/** 
 * @internal Function used to perform notifications for a connection
 * created.
 *
 * @return An error was found during the processing.
 */
axl_bool            vortex_connection_actions_notify   (VortexCtx                * ctx,
							VortexConnection        ** caller_conn,
							VortexConnectionStage      stage)
{
	/* get current context */
	VortexConnection           * conn = (*caller_conn);
	int                          iterator;
	int                          result;
	VortexConnectionActionData * data;
	VortexConnection           * new_conn;
	VortexConnectionAction       action;
	axlPointer                   action_data;

	/* do not notify if the connection is not running */
	if (! vortex_connection_is_ok (conn, axl_false))
		return axl_false;

	/* get a reference to the context */
	ctx = conn->ctx;
	
	/* lock mutex */
	vortex_mutex_lock (&ctx->connection_actions_mutex);
	
	/* for each chandler stored */
	iterator = 0;
	while (iterator < axl_list_length (ctx->connection_actions)) {

		/* get data associated */
		data = axl_list_get_nth (ctx->connection_actions, iterator);

		/* call to notify if the stage matches */
		if (data->stage == stage) {
			new_conn    = NULL;
			action      = data->action;
			action_data = data->action_data;

			/* unlock during execution */
			vortex_mutex_unlock (&ctx->connection_actions_mutex);

			result   = action (ctx, conn, &new_conn, stage, action_data);
			vortex_log (VORTEX_LEVEL_DEBUG, "Action %p finished with result=%d, new_conn=%p and conn=%p",
				    action, result, new_conn, conn);

			/* (re)lock during execution */
			vortex_mutex_lock (&ctx->connection_actions_mutex);

			switch (result) {
			case -1:
				if (vortex_connection_is_ok (conn, axl_false)) {
					__vortex_connection_shutdown_and_record_error (
						conn, VortexConnectionCloseCalled,
						"connection action failed, closing session");
				}
				/* unlock mutex */
				vortex_mutex_unlock (&ctx->connection_actions_mutex);	
				return axl_false;
			case 0:
				vortex_log (VORTEX_LEVEL_DEBUG, "found connection action returning 0, blocking rest of connection actions");
				/* unlock mutex */
				vortex_mutex_unlock (&ctx->connection_actions_mutex);	
				return axl_true;
			case 1:
				/* nothing to do */
				break;
			case 2:
				/* request to replace received connection */
				(*caller_conn) = new_conn;
				conn           = new_conn;
				break;
			default:
				vortex_log (VORTEX_LEVEL_WARNING, "found unsupported value returned by a connection action=%d", result);
			} /* end if */
		}

		/* next iterator */
		iterator++;
	} /* end while */

	/* unlock mutex */
	vortex_mutex_unlock (&ctx->connection_actions_mutex);	

	return axl_true;
}



/** 
 * @internal
 * @brief Sets the given connection the not connected status.
 *
 * This internal vortex function allows library to set connection
 * status to axl_false for a given connection. This function should not be
 * used by vortex library consumers.
 *
 * This function is callable over and over again on the same
 * connection. The first time the function is called sets to axl_false
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
						      const char       * message,
						      VortexStatus       status)
{
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx * ctx;
#endif


	/* check reference received */
	if (connection == NULL || message == NULL)
		return;

#if defined(ENABLE_VORTEX_LOG)
	/* get a reference to the context */
	ctx = connection->ctx;
#endif

	/* set connection status to axl_false if weren't */
	vortex_mutex_lock (&connection->op_mutex);

	if (connection->is_connected) {


		/* acquire a reference to the connection during the
		   shutdown to avoid race conditions with listener
		   connections and the vortex reader loop */
		vortex_connection_ref_internal (connection, "set-not-connected", axl_false);

		vortex_log (VORTEX_LEVEL_DEBUG, "flagging the connection as non-connected: %s (close-session?: %d)", message ? message : "",
			    connection->close_session);

		/* set value consistently */
		vortex_mutex_lock  (&(connection->ref_mutex));
		connection->is_connected = axl_false;
		vortex_mutex_unlock  (&(connection->ref_mutex));

		/* renew the message */
		if (connection->message)
			axl_free (connection->message);
		connection->message = axl_strdup (message);

		/* configure status */
		connection->status  = status;

		/* unlock now the op mutex is not blocked */
		vortex_mutex_unlock (&connection->op_mutex);

		/* check to invoke on close handler */
		if (connection->on_close != NULL) {
			vortex_log (VORTEX_LEVEL_DEBUG, "notifying connection-id=%d close handlers", connection->id);
			/* invoking on close handler */
			__vortex_connection_invoke_on_close (connection, axl_false);
		}

		/* check for the close handler full definition */
		if (connection->on_close_full != NULL) {
			vortex_log (VORTEX_LEVEL_DEBUG, "notifying connection-id=%d close handlers", connection->id);
			/* invokin on close handler full */ 
			__vortex_connection_invoke_on_close (connection, axl_true);
		}

		/* close socket connection if weren't  */
		if (( connection->close_session) && (connection->session != -1)) {
			vortex_log (VORTEX_LEVEL_DEBUG, "closing connection id=%d to %s:%s (socket: %d)", 
				    connection->id,
				    axl_check_undef (connection->host), 
				    axl_check_undef (connection->port), connection->session);
			shutdown (connection->session, SHUT_RDWR); 
			vortex_close_socket (connection->session);  

			/* set session value */
			vortex_mutex_lock (&(connection->ref_mutex));
			connection->session      = -1;
			vortex_mutex_unlock (&(connection->ref_mutex));

 	        } /* end if */

		/* finish reference acquired after unlocking and doing
		   all close stuff */
		vortex_connection_unref (connection, "set-not-connected");

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
axl_bool   __vortex_connection_one_sending_round (axlPointer key,
						  axlPointer value,
						  axlPointer user_data)
{
	/* deprecated function */
	return axl_false;
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
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx * ctx;
#endif
	int         messages = 0;

	/* check connection status */
	if (! vortex_connection_is_ok (connection, axl_false))
		return 0;

#if defined(ENABLE_VORTEX_LOG)	
	/* get a reference to the ctx */
	ctx = connection->ctx;
#endif

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
 * The function allows to store arbitrary data associated to the given
 * connection. Data stored will be indexed by the provided key,
 * allowing to retrieve the information using: \ref
 * vortex_connection_get_data.
 *
 * If the value provided is NULL, this will be considered as a
 * removing request for the given key and its associated data.
 * 
 * See also \ref vortex_connection_set_data_full function. It is an
 * alternative API that allows configuring a set of destroy handlers
 * for key and data stored.
 *
 * @param connection The connection where data will be associated.
 *
 * @param key The string key indexing the data stored associated to
 * the given key.
 *
 * @param value The value to be stored. NULL to remove previous data
 * stored.
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
 * @brief Allows to remove a key/value pair installed by
 * vortex_connection_set_data and vortex_connection_set_data_full
 * without calling destroy functions associated.
 *
 * @param connection The connection where the key/value entry will be
 * removed without calling destroy function associated (if any) to
 * both (key and value).
 *
 * @param key The key that identifies the entry to be deleted.
 * 
 */
void                vortex_connection_delete_key_data        (VortexConnection * connection,
							      const char       * key)
{
	if (connection == NULL || connection->data == NULL)
		return;
	vortex_hash_delete (connection->data, (axlPointer) key);
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
 * @brief Allows to set a commonly used user land pointer associated
 * to the provided connection.
 *
 * Though you can use \ref vortex_connection_set_data_full or \ref vortex_connection_set_data, this function allows to set a pointer
 * that can be retreived by using \ref vortex_connection_get_hook with a low cpu usage.
 *
 * @param connection The connection where the user land pointer is associated.
 *
 * @param ptr The pointer that will be associated to the connection.
 */
void                vortex_connection_set_hook               (VortexConnection * connection,
							      axlPointer         ptr)
{
	if (connection == NULL)
		return;
	connection->hook = ptr;
	return;
}

/** 
 * @brief Allows to get the user land pointer configured by \ref vortex_connection_set_hook.
 *
 * @param connection The connection where the user land pointer is
 * being queried.
 *
 * @return The pointer stored.
 */
axlPointer          vortex_connection_get_hook               (VortexConnection * connection)
{
	if (connection == NULL)
		return NULL;
	return connection->hook;
}


/** 
 * @brief Allows to define custom actions to be implemented (by
 * calling the function provided) at the connection creation.
 * 
 * @param ctx The context that is going to be configured.
 *
 * @param stage The stage where the connection action will be
 * installed.
 *
 * @param action_handler The handler to be executed. The function can
 * have several actions registered on a stage. All of them will be
 * called in order as they were added.
 *
 * @param handler_data A user defined pointer to be passed to the
 * action function.
 *
 * See \ref VortexConnectionAction and \ref VortexConnectionStage for
 * more information.
 */
void                vortex_connection_set_connection_actions (VortexCtx              * ctx,
							      VortexConnectionStage    stage,
							      VortexConnectionAction   action_handler,
							      axlPointer               handler_data)
{
	/* get current context */
	VortexConnectionActionData * data;

	v_return_if_fail (ctx && action_handler);

	/* get a reference to the data */
	data              = axl_new (VortexConnectionActionData, 1);
	data->stage       = stage;
	data->action      = action_handler;
	data->action_data = handler_data;

	/* lock mutex */
	vortex_mutex_lock (&ctx->connection_actions_mutex);

	/* create the list if it is not defined */
	if (ctx->connection_actions == NULL)
		ctx->connection_actions = axl_list_new (axl_list_always_return_1, axl_free);

	/* store data */
	axl_list_add (ctx->connection_actions, data);

	/* unlock mutex */
	vortex_mutex_unlock (&ctx->connection_actions_mutex);

	return;
}

/** 
 * @brief Gets stored value indexed by the given key inside the given connection.
 *
 * The function returns stored data using \ref
 * vortex_connection_set_data or \ref vortex_connection_set_data_full.
 * 
 * @param connection the connection where the value will be looked up.
 * @param key the key to look up.
 * 
 * @return the value or NULL if fails.
 */
axlPointer         vortex_connection_get_data               (VortexConnection * connection,
							     const char       * key)
{
 	v_return_val_if_fail (connection,       NULL);
 	v_return_val_if_fail (key,              NULL);
 	v_return_val_if_fail (connection->data, NULL);

	return vortex_hash_lookup (connection->data, (axlPointer) key);
}

/** 
 * @brief Allows to get current data hash object used by the provided
 * connection. This way you can use this hash object directly with
 * \ref vortex_hash "vortex hash API".
 *
 * @param connection The connection where the hash has been requested..
 *
 * @return A reference to the hash or NULL if it fails.
 */
VortexHash        * vortex_connection_get_data_hash          (VortexConnection * connection)
{
	v_return_val_if_fail (connection, NULL);
	return connection->data;
}

/**
 * @internal Function that allows to get the internal reference used
 * by a connection to hold all channels available.
 *
 * @param connection The connection that is required to return its
 * channels hash structure.
 *
 * @return A reference to the hash table used to store channels or
 * NULL if it fails.
 */
VortexHash        * vortex_connection_get_channels_hash      (VortexConnection * connection)
{
 	v_return_val_if_fail (connection, NULL);
 
 	return connection->channels;
}

/** 
 * @brief Returns the channel pool identified by <i>pool_id</i>.
 *
 * If the connection has only one channel pool created, you can
 * access to it using pool_id = 1.
 * 
 * @param connection the connection where the channel pool is found.
 *
 * @param pool_id The channel pool id to look up. Remember that the
 * first pool is always have the pool id 1.
 * 
 * @return the channel pool or NULL if fails.
 *
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
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx * ctx;
#endif
	if (connection == NULL)
		return;

#if defined(ENABLE_VORTEX_LOG)
	/* get the ctx */
	ctx = connection->ctx;
#endif

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
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx * ctx;
#endif

	if (connection == NULL)
		return;

#if defined(ENABLE_VORTEX_LOG)	
	/* get the ctx */
	ctx = connection->ctx;
#endif

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

axl_bool  __vortex_connection_get_pending_msgs (axlPointer key, axlPointer value, axlPointer user_data) {

	VortexChannel * channel = value;
	int           * messages = user_data;

	/* count values */
	(*messages) = (*messages) + vortex_channel_pending_messages (channel);

	return axl_false; /* do not stop */
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
 * @return The number of pending messages to be sent by all channels in this connection.
 */
int                 vortex_connection_get_pending_msgs       (VortexConnection * connection)
{
	int  messages = 0;

	/* check reference */
	if (connection == NULL)
		return -1;

	/* get the first channel running the profile provided by
	 * foreaching all items */
	vortex_hash_foreach (connection->channels, __vortex_connection_get_pending_msgs, &messages);

	/* return the channel found */
	return messages;
}

/** 
 * @brief Allows to get current connection role.
 * 
 * @param connection The VortexConnection to get the current role from.
 * 
 * @return Current role represented by \ref VortexPeerRole. If the
 * function receives a NULL reference it will return \ref
 * VortexRoleUnknown.
 */
VortexPeerRole      vortex_connection_get_role               (VortexConnection * connection)
{
	/* if null reference received, return unknown role */
	v_return_val_if_fail (connection, VortexRoleUnknown);

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
 * NOTE: the value returned by this function is only the advised
 * features content sent by remote BEEP peer; in any way is a set of
 * features accepted or implemented. The application level must get
 * the value from this function, process it and decide to support or
 * not each feature. See \ref
 * CONNECTION_STAGE_PROCESS_GREETINGS_FEATURES and \ref
 * vortex_connection_set_connection_actions for a way to get a
 * notification of features available. In general features are
 * requested from initiator to listener, which means that this is only
 * useful at server side.
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
	if (!vortex_connection_is_ok (connection, axl_false))
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
	if (!vortex_connection_is_ok (connection, axl_false))
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
 * (\ref vortex_connection_is_ok (connection, axl_false)).
 */
int                 vortex_connection_get_opened_channels    (VortexConnection * connection)
{
	/* checke incoming data */
	if (connection == NULL || 
	    ! vortex_connection_is_ok (connection, axl_false))
		return 0;

	/* return number of channels */
	return axl_hash_items (connection->channels->table);
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
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx * ctx;
#endif

	/* check reference */
	if (connection == NULL)
		return;

#if defined(ENABLE_VORTEX_LOG)
	/* get a reference */
	ctx = connection->ctx;
#endif

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
 */
void vortex_connection_set_on_close       (VortexConnection * connection,
					   VortexConnectionOnClose on_close_handler)
{

	/* check reference received */
	v_return_if_fail (connection);
	v_return_if_fail (on_close_handler);

	/* lock until done */
	vortex_mutex_lock (&connection->handlers_mutex);

	/* init on demand */
	if (connection->on_close == NULL) {
		connection->on_close  = axl_list_new (axl_list_always_return_1, NULL);
		if (connection->on_close == NULL) {
			vortex_mutex_unlock (&connection->handlers_mutex);
			return;
		} /* end if */
	}

	/* save previous handler defined */
	axl_list_append (connection->on_close, on_close_handler);

	/* unlock now the item is removed */
	vortex_mutex_unlock (&connection->handlers_mutex);

	/* returns previous handler */
	return;
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
 *
 * NOTE: handler configured will be skipped in the case \ref VortexCtx
 * hosting the provided connection is being closed (a call to \ref
 * vortex_exit_ctx was done). 
 */
void vortex_connection_set_on_close_full  (VortexConnection * connection,
					   VortexConnectionOnCloseFull on_close_handler,
					   axlPointer data)
{
	/* insert the connection */
	vortex_connection_set_on_close_full2 (connection, on_close_handler, axl_true, data);
	return;
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
 * @param insert_last Allows to configure if the handler should be inserted at the last position.
 *
 * @param data User defined data to be passed to the handler.
 * 
 */
void                    vortex_connection_set_on_close_full2  (VortexConnection             * connection,
							       VortexConnectionOnCloseFull    on_close_handler,
							       axl_bool                       insert_last,
							       axlPointer                     data)
{
	VortexConnectionOnCloseData * handler;
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx                   * ctx;
#endif

	/* check reference received */
	if (connection == NULL || on_close_handler == NULL)
		return;

	/* lock during the operation */
	vortex_mutex_lock (&connection->handlers_mutex);

	/* init on close list on demand */
	if (connection->on_close_full == NULL) {
		connection->on_close_full  = axl_list_new (axl_list_always_return_1, axl_free);
		if (connection->on_close_full == NULL) {
			vortex_mutex_unlock (&connection->handlers_mutex);
			return;
		} /* end if */
	}

	/* save handler defined */
	handler          = axl_new (VortexConnectionOnCloseData, 1);
	handler->handler = on_close_handler;
	handler->data    = data;

	/* store the handler */
	if (insert_last)
		axl_list_append (connection->on_close_full, handler);
	else
		axl_list_prepend (connection->on_close_full, handler);
#if defined(ENABLE_VORTEX_LOG)
	ctx = connection->ctx;
	vortex_log (VORTEX_LEVEL_DEBUG, "on close full handlers %d on conn-id=%d, handler added %p (added %s)",
		    axl_list_length (connection->on_close_full), connection->id, handler, insert_last ? "last" : "first");
#endif

	/* unlock now it is done */
	vortex_mutex_unlock (&connection->handlers_mutex);

	/* finish */
	return;
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
 * @param data In order to remove the handler you must provide
 * the same pointer data as provided on handler installation. This is
 * done to avoid removing a handler that was installed twice but with
 * different contexts. Think about a function that install the same
 * handler but with different data. The appropiate handler to
 * uninstall and its associated data can only be matched this way.
 * 
 * @return axl_true if the function uninstalled the handler otherwise
 * axl_false is returned.
 */
axl_bool   vortex_connection_remove_on_close_full (VortexConnection              * connection, 
						   VortexConnectionOnCloseFull     on_close_handler,
						   axlPointer                      data)
{
	int                           iterator;
	VortexConnectionOnCloseData * handler;

	/* check reference received */
	v_return_val_if_fail (connection, axl_false);
	v_return_val_if_fail (on_close_handler, axl_false);

	/* look during the operation */
	vortex_mutex_lock (&connection->handlers_mutex);

	/* remove by pointer */
	iterator = 0;
	while (iterator < axl_list_length (connection->on_close_full)) {

		/* get a reference to the handler */
		handler = axl_list_get_nth (connection->on_close_full, iterator);
		
		if ((on_close_handler == handler->handler) && (data == handler->data)) {

			/* remove by pointer */
			axl_list_remove_ptr (connection->on_close_full, handler);

			/* unlock */
			vortex_mutex_unlock (&connection->handlers_mutex);

			return axl_true;
			
		} /* end if */

		/* update the iterator */
		iterator++;
		
	} /* end while */
	
	/* unlock */
	vortex_mutex_unlock (&connection->handlers_mutex);

	return axl_false;
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
	if (connection == NULL || buffer == NULL || ! vortex_connection_is_ok (connection, axl_false))
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
void                vortex_connection_sanity_socket_check (VortexCtx * ctx, axl_bool      enable)
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
 * @return axl_true if the process have finished properly, otherwise axl_false
 * is returned.
 */
axl_bool      vortex_connection_parse_greetings_and_enable (VortexConnection * connection, 
							    VortexFrame      * frame)
{
	VortexCtx * ctx;

	/* check parametes received */
	if (connection == NULL || frame == NULL)
		return axl_false;

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
			return axl_false;
		}
		vortex_log (VORTEX_LEVEL_DEBUG, "greetings parsed..");
		
		/* parse ok, free frame and establish new message */
		vortex_frame_unref (frame);

		/* free previous message and stablish the new one */
		if (connection->message)
			axl_free (connection->message);
		connection->message = axl_strdup ("session established and ready");
		connection->status  = VortexOk;
		
		vortex_log (VORTEX_LEVEL_DEBUG, "new connection created to %s:%s", connection->host, connection->port);

		/* now, every initial message have been sent, we need
		 * to send this to the reader manager */
		vortex_reader_watch_connection (ctx, connection); 
		return axl_true;
	}
	__vortex_connection_shutdown_and_record_error (
		connection, VortexProtocolError,
		"received a null frame (null reply) from remote side when expected a greetings reply, closing session");
	return axl_false;
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
	/* configure handler */
	connection->pre_accept_handler = pre_accept_handler;
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
								  axl_bool           status)
{
	/* flag this connection to be already TLS-ficated */
	vortex_connection_set_data (connection, "tls-fication:status", INT_TO_PTR ((int)status));
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
 * @return axl_true if TLS status is already activated, otherwise axl_false is returned.
 */
axl_bool                 vortex_connection_is_tlsficated              (VortexConnection * connection)
{
	return (PTR_TO_INT (vortex_connection_get_data (connection, "tls-fication:status")));
}


/** 
 * @brief Allows to check if there are an pre read handler defined on
 * the given connection.
 * 
 * @param connection The connection to check.
 * 
 * @return axl_true if the pre-read handler is defined, otherwise, axl_false is returned
 */
axl_bool                 vortex_connection_is_defined_preread_handler (VortexConnection * connection)
{
	return connection->pre_accept_handler != NULL;
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
	if (connection->pre_accept_handler != NULL)
		connection->pre_accept_handler (connection);
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
	if (ctx->connection_id == 0) {
		/* only init connection id in case it is 0 (clean
		 * start) leaving the value as is if we are in a fork
		 * operation, so child must keep connection ids as
		 * they were created at the parent before forking */
		ctx->connection_id                = 1;
	}
	ctx->connection_enable_sanity_check       = axl_true;
	ctx->connection_std_timeout               = 60000000;

	vortex_mutex_create (&ctx->connection_xml_cache_mutex);
	vortex_mutex_create (&ctx->connection_hostname_mutex);
	vortex_mutex_create (&ctx->connection_actions_mutex);

	/* init hashes */
	if (ctx->connection_xml_cache == NULL)
		ctx->connection_xml_cache = axl_hash_new (axl_hash_string, axl_hash_equal_string);
	if (ctx->connection_hostname == NULL)
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
	vortex_mutex_destroy (&ctx->connection_actions_mutex);

	/* drop hashes */
	axl_hash_free (ctx->connection_xml_cache);
	ctx->connection_xml_cache = NULL;

	axl_hash_free (ctx->connection_hostname);
	ctx->connection_hostname = NULL;

	/* free list */
	if (ctx->connection_actions != NULL)
		axl_list_free (ctx->connection_actions);
	ctx->connection_actions = NULL;

	return;
}

/** 
 * @brief Allows to get maximum segment size negociated.
 * 
 * @param connection The connection where to get maximum segment size.
 * 
 * @return The maximum segment size or -1 if fails. The function is
 * still not portable since the Microsoft Windows API do not allow
 * getting TCP maximum segment size.
 */
int                vortex_connection_get_mss                (VortexConnection * connection)
{
#if defined(AXL_OS_WIN32)
	/* no support */
	return -1;
#else
	/* unix flavors */
	socklen_t            optlen;
	long              max_seg_size;

	v_return_val_if_fail (connection, -1);

	/* clear values */
	optlen       = sizeof (long);
	max_seg_size = 0;
	
	/* call to get socket option */
	if (getsockopt (connection->session, IPPROTO_TCP, TCP_MAXSEG, &max_seg_size, &optlen) == 0) {
		return max_seg_size;
	}

	/* return value found */
	return -1;
#endif
}

/** 
 * @internal Function used to check if we have reached our socket
 * creation limit to avoid exhausting it. The idea is that we need to
 * have at least one bucket free before limit is reached so we can
 * still empty the listeners backlog to close them (accept ()).
 *
 * @return axl_true in the case the limit is not reached, otherwise
 * axl_false is returned.
 */
axl_bool vortex_connection_check_socket_limit (VortexCtx * ctx, VORTEX_SOCKET socket_to_check)
{
	int   soft_limit, hard_limit;
	VORTEX_SOCKET temp;

	/* create a temporal socket */
	temp = socket (AF_INET, SOCK_STREAM, 0);
	if (temp == VORTEX_INVALID_SOCKET) {
		/* uhmmn.. seems we reached our socket limit, we have
		 * to close the connection to avoid keep on iterating
		 * over the listener connection because its backlog
		 * could be filled with sockets we can't accept */
		shutdown (socket_to_check, SHUT_RDWR);
		vortex_close_socket (socket_to_check);

		/* get values */
		vortex_conf_get (ctx, VORTEX_SOFT_SOCK_LIMIT, &soft_limit);
		vortex_conf_get (ctx, VORTEX_HARD_SOCK_LIMIT, &hard_limit);
		
		vortex_log (VORTEX_LEVEL_CRITICAL, 
			    "droping socket connection, reached process limit: soft-limit=%d, hard-limit=%d\n",
			    soft_limit, hard_limit);
		return axl_false; /* limit reached */

	} /* end if */
	
	/* close temporal socket */
	vortex_close_socket (temp);

	return axl_true; /* connection check ok */
}

/** 
 * @internal Function used to change connection initial accept
 * status..
 */
void                vortex_connection_set_initial_accept     (VortexConnection * conn,
							      axl_bool           status)
{
	/* set status */
	conn->initial_accept = status;
	return;
}

/* @} */

