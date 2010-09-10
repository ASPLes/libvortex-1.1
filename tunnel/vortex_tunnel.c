/* 
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2010 Advanced Software Production Line, S.L.
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
#include <vortex_tunnel.h>

#define LOG_DOMAIN "vortex-tunnel"

#define VORTEX_TUNNEL_PARTNER_CONNECTION "vo:tu:con"
#define VORTEX_TUNNEL_BUFFER             "vo:tu:buf"

/* key for the accept tunnel and the tunnel resolver handler */
#define VORTEX_TUNNEL_ACCEPT             "vo:tu:ac"
#define VORTEX_TUNNEL_ACCEPT_DATA        "vo:tu:ac:da"
#define VORTEX_TUNNEL_RESOLVER           "vo:tu:re"
#define VORTEX_TUNNEL_RESOLVER_DATA      "vo:tu:re:da"


/**
 * \defgroup vortex_tunnel Vortex Tunnel: TUNNEL profile support, general application layer proxy for BEEP
 */

/**
 * \addtogroup vortex_tunnel
 * @{
 */

/** 
 * @internal Configuration for the tunnel settings.
 */
struct _VortexTunnelSettings{
	axlDoc               * doc;
	VortexCtx            * ctx;
	VortexConnectionOpts * options;
};

/** 
 * @brief Allows to create a new tunnel setting, a proxy configuration
 * for any BEEP session to be created (\ref VortexConnection).
 *
 * Once created all settings, you must use \ref
 * vortex_tunnel_settings_add_hop to configure the next hop that will
 * proxy your connection.
 * 
 * @return A newly created proxy setting, that must be deallocated
 * with \ref vortex_tunnel_settings_free. The function only returns
 * NULL in the case the context parameter provided is NULL.
 */
VortexTunnelSettings * vortex_tunnel_settings_new (VortexCtx * ctx)
{
	VortexTunnelSettings * settings;
	axlNode              * node;

	/* check context */
	v_return_val_if_fail (ctx, NULL);

	/* create the root document */
	settings      = axl_new (VortexTunnelSettings, 1);
	settings->doc = axl_doc_create (NULL, NULL, axl_true);

	/* configure context */
	settings->ctx = ctx;

	/* configure the inner most tunnel */
	node = axl_node_create ("tunnel");
	axl_doc_set_root (settings->doc, node);

	/* return settings created */
	return settings;
}

/** 
 * @brief Allows to configure connection options on the provided
 * tunnel settings object, allowing to modify connection behaviour.
 *
 * @param settings The tunnel settings object to configure.
 *
 * @param options The connection object option to be used.
 */
void                   vortex_tunnel_settings_set_options (VortexTunnelSettings * settings,
							   VortexConnectionOpts * options)
{
	v_return_if_fail (settings && options);
	settings->options = options;
	return;
}

/** 
 * @brief Allows to create a new BEEP proxy connection setting from
 * the content provided, that must be an xml meating the RFC3620
 * description.
 * 
 * @param ctx The context where the operation will be performed.
 * @param content The xml content to be parsed.
 * @param size The size of the xml content.
 * 
 * @return A reference to the TUNNEL settings created or NULL if it
 * fails.
 */
VortexTunnelSettings * vortex_tunnel_settings_new_from_xml (VortexCtx * ctx,
							    char      * content, 
							    int         size)
{
	VortexTunnelSettings * settings;
	axlError             * error;

	/* check reference received */
	v_return_val_if_fail (ctx, NULL);
	v_return_val_if_fail (content, NULL);

	/* create the root document */
	settings      = axl_new (VortexTunnelSettings, 1);
	settings->ctx = ctx;
	settings->doc = axl_doc_parse (content, size, &error);
	if (settings->doc == NULL) {
		/* drop-a-log */
		vortex_log (VORTEX_LEVEL_DEBUG, "failed to create the TUNNEL settings profile, error was: %s", 
			    axl_error_get (error));

		/* free the error and the settings */
		axl_error_free (error);
		vortex_tunnel_settings_free (settings);
		return NULL;
	} /* end if */

	/* return settings created */
	return settings;
}

/** 
 * @brief Allows to configure a new hop to be added to the current
 * proxy configuration. The hop added, will be appended to the current
 * configuration:
 *
 * \code
 *  [new hop] -> [next hop] -> [next hop]
 * \endcode
 *
 * You can configure one proxy to act as the first (and the only one)
 * hop for all you connections, or you can configure a serie of hops.
 * 
 * The hop configuration provided will be a combination of the
 * following items:
 * 
 * - \ref TUNNEL_FQDN + \ref TUNNEL_PORT : a full qualified domain name
 * followed by a tcp port.
 *
 * - \ref TUNNEL_FQDN + \ref TUNNEL_SRV : a full qualified domain name
 * followed by a DNS service record.
 *
 * - \ref TUNNEL_FQDN + \ref TUNNEL_SRV + \ref TUNNEL_PORT : a full qualified domain name
 * followed by a DNS service record plus a tcp port.
 *
 * - \ref TUNNEL_IP4 + \ref TUNNEL_PORT : an IPv4 address followed by a
 * tcp port configuration.
 *
 * - \ref TUNNEL_IP6 + \ref TUNNEL_PORT : an IPv6 address followed by a
 * tcp port configuration.
 *
 * - \ref TUNNEL_URI : the uri for the set of channels to be created,
 * that is, a remote end point BEEP peer that supports this
 * profile. This is only allowed at the but only on the innermost
 * element, that is, the last hop to be traversed.
 *
 * - \ref TUNNEL_ENDPOINT : a user defined string, provisioned at the
 * last proxy to be traversed, configuring a particular service to be
 * reached. Again, this is only allowed in the last hop used for a
 * proxy setting.
 *
 * The function must receive a set of \ref VortexTunnelItem followed
 * by its associated value, terminated with a \ref TUNNEL_END_CONF.
 *
 * For example:
 * \code
 * VortexTunnelSettings * settings;
 * 
 * // create the settings (using an initialized VortexCtx)
 * settings = vortex_tunnel_settings_new (ctx);
 *
 * // configure the default proxy 
 * vortex_tunnel_settings_add_hop (settings, 
 *                                 TUNNEL_IP4, "192.168.1.100", 
 *                                 TUNNEL_PORT, "604",
 *                                 TUNNEL_END_CONF);
 * \endcode
 * 
 * @param settings The setting that will be configured with a new hop.
 *
 *
 */
void vortex_tunnel_settings_add_hop (VortexTunnelSettings * settings,
				     ...)
{
	va_list            args;
	VortexTunnelItem   item;
	axlNode          * node;
	axlNode          * temp;
	VortexCtx        * ctx;
	const char       * value;
	char             * content;
	int                size;

	/* check parameter */
	v_return_if_fail (settings);

	/* get context */
	ctx = settings->ctx;

	/* create a new node */
	node = axl_node_create ("tunnel");

	/* open stdargs */
	va_start (args, settings);

	while ((item = va_arg (args, VortexTunnelItem)) != TUNNEL_END_CONF) {
		/* get the value */
		value = va_arg (args, const char *);

		/* configure the new value */
		switch (item) {
		case TUNNEL_END_CONF:
			/* shouldn't be reached at this point */
			goto wrong_tunnel_conf;
		case TUNNEL_FQDN:
			/* configure the FQDN */
			if (HAS_ATTR (node, "fqdn") || HAS_ATTR (node, "ip4") || 
			    HAS_ATTR (node, "ip6")  || HAS_ATTR (node, "uri") || 
			    HAS_ATTR (node, "endpoint"))
				goto wrong_tunnel_conf;

			/* configure the value */
			axl_node_set_attribute (node, "fqdn", value);
			break;
		case TUNNEL_PORT:
			/* configure the PORT */
			if (HAS_ATTR (node, "port") || HAS_ATTR (node, "uri") || 
			    HAS_ATTR (node, "endpoint"))
				goto wrong_tunnel_conf;

			/* configure the value */
			axl_node_set_attribute (node, "port", value);
			break;
		case TUNNEL_IP4:
			/* configure the IP4 */
			if (HAS_ATTR (node, "ip4") || HAS_ATTR (node, "fqdn") || 
			    HAS_ATTR (node, "srv") || HAS_ATTR (node, "endpoint") || 
			    HAS_ATTR (node, "uri"))
				goto wrong_tunnel_conf;

			/* configure the value */
			axl_node_set_attribute (node, "ip4", value);
			break;
		case TUNNEL_IP6:
			/* configure the IP6 */
			if (HAS_ATTR (node, "ip6") || HAS_ATTR (node, "fqdn")     || 
			    HAS_ATTR (node, "srv") || HAS_ATTR (node, "endpoint") || 
			    HAS_ATTR (node, "uri"))
				goto wrong_tunnel_conf;

			/* configure the value */
			axl_node_set_attribute (node, "ip6", value);
			break;
		case TUNNEL_SRV:
			/* configure the SRV */
			if (HAS_ATTR (node, "srv") || HAS_ATTR (node, "ip4") || 
			    HAS_ATTR (node, "ip6") || HAS_ATTR (node, "endpoint") || 
			    HAS_ATTR (node, "uri"))
				goto wrong_tunnel_conf;

			/* configure the value */
			axl_node_set_attribute (node, "srv", value);
			break;
		case TUNNEL_URI:
			/* configure the URI */
			if (HAS_ATTR (node, "uri")  || HAS_ATTR (node, "fqdn") || 
			    HAS_ATTR (node, "port") || HAS_ATTR (node, "srv") || 
			    HAS_ATTR (node, "ip4")  || HAS_ATTR (node, "ip6") || HAS_ATTR (node, "endpoint"))
				goto wrong_tunnel_conf;

			/* configure the value */
			axl_node_set_attribute (node, "uri", value);
			break;
		case TUNNEL_ENDPOINT:
			/* configure the URI */
			if (HAS_ATTR (node, "uri")  || HAS_ATTR (node, "fqdn") || 
			    HAS_ATTR (node, "port") || HAS_ATTR (node, "srv") || 
			    HAS_ATTR (node, "ip4")  || HAS_ATTR (node, "ip6") || HAS_ATTR (node, "endpoint"))
				goto wrong_tunnel_conf;

			/* configure the value */
			axl_node_set_attribute (node, "endpoint", value);
			break;
		} /* end switch */
		
	} /* end while */

	/* terminate stdargs */
	va_end (args);

	/* configure the node as the new root */
	temp = axl_doc_get_root (settings->doc);
	axl_doc_set_root (settings->doc, node);
	axl_node_set_child (node, temp);

	axl_doc_dump_pretty (settings->doc, &content, &size, 2);
	vortex_log (VORTEX_LEVEL_DEBUG, "tunnel hop added: \n%s",
		    content);
	axl_free (content);

	return;

 wrong_tunnel_conf:
	/* terminate stdargs */
	va_end (args);

	vortex_log (VORTEX_LEVEL_CRITICAL, "wrong tunnel configuration found!");

	/* free the node */
	axl_free (node);
	
	return;
}

/** 
 * @brief Creates a new BEEP session to the remote endpoint, using the
 * information provided by the tunnel setting (\ref
 * VortexTunnelSettings).
 * 
 * @param settings The tunnel settings to be used to create the connection.
 *
 * @param on_connected An optional handler to receive the connection
 * created due to the tunnel. It works the same as the async
 * notification handler that receives the \ref vortex_connection_new.
 * 
 * @param user_data User defined data that is provided to the previous
 * async notication handler.
 * 
 * @return A reference to the connection created.
 */
VortexConnection     * vortex_tunnel_new              (VortexTunnelSettings * settings,
						       VortexConnectionNew    on_connected,
						       axlPointer             user_data)
{
	/* call to common implementation */
	return __vortex_tunnel_new_common (settings, axl_true, on_connected, user_data);
}

/** 
 * @internal Function that implements the common part for a client
 * requesting to open a tunnel and a middle hop proxy that is
 * requesting to open a tunnel to the next hop.
 * 
 * @param settings The settings configuration to be used to open the
 * next hop.
 *
 * @param do_tunning_reset If the function must do the tunning reset or
 * just return after the <ok /> message from the next hop is received.
 *
 * @param on_connected An optional handler to receive the connection
 * created due to the tunnel. It works the same as the async
 * notification handler that receives the \ref vortex_connection_new.
 * 
 * @param user_data User defined data that is provided to the previous
 * async notication handler.
 * 
 * @return A reference to the connection created or NULL if it fails.
 */
VortexConnection     * __vortex_tunnel_new_common     (VortexTunnelSettings * settings,
						       axl_bool               do_tunning_reset,
						       VortexConnectionNew    on_connected,
						       axlPointer             user_data)
{
	VortexConnection      * connection;
	VortexConnection      * connection_aux;
	VortexCtx             * ctx;

	VortexChannel         * channel;
	VortexAsyncQueue      * queue;
	VortexFrame           * reply;

	const char            * host = NULL;
	const char            * port = NULL;
	char                  * content;
	int                     size;

	VORTEX_SOCKET           socket;
	axlNode               * node;
	axlDoc                * ok;
	VortexConnectionOpts  * options = settings->options;

	/* check parameters */
	v_return_val_if_fail (settings, NULL);
	
	/* get the context */
	ctx = settings->ctx;

	/* create a new connection to the first hop found at the
	 * settings */
	node = axl_doc_get_root (settings->doc);
	if (node == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "provided no information to connect to remote end point");
		return NULL;
	} /* end if */

	/* now check that the node have at least one child */
	if (axl_node_get_first_child (node) == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "found faulty PROXY configuration, you didn't configure the final destination");
		return NULL;
	} /* end if */

	/* check which is the type of next hop configuration found */
	if (HAS_ATTR (node, "fqdn")) {

		/* get the host and the port */
		host = ATTR_VALUE (node, "fqdn");
		port = ATTR_VALUE (node, "port");

	} else if (HAS_ATTR (node, "ip4")) {

		/* get the host and the port */
		host = ATTR_VALUE (node, "ip4");
		port = ATTR_VALUE (node, "port");
		
	} else if (HAS_ATTR (node, "ip6")) {
		/* yet not implemented IP6 support */
		
	} else if (HAS_ATTR (node, "endpoint")) {
		/* not implemented, see turbulence server */

	} else if (HAS_ATTR (node, "profile")) {
		/* not implemented, see turbulence server */

	} else if (HAS_ATTR (node, "srv")) {
		/* not implemented, see turbulence server */

	} /* end if */

	/* check values */
	if (host == NULL || port == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to figure out the next hop in transit");
		return NULL;
	} /* end if */

	/* now, get the content to be sent as next hop to the remote
	 * end point */
	node = axl_node_get_first_child (node);
	if (! axl_node_dump (node, &content, &size)) {
		vortex_log (VORTEX_LEVEL_WARNING, "unable to dump next hop content, unable to create the connection");
		return NULL;
	}

	/* now we have the content, create the channel using as
	 * piggyback the content received  */
	vortex_log (VORTEX_LEVEL_DEBUG, "connecting to the first end point of the TUNNEL conf, host=%s port=%s",
		    host, port);

	/* create the connection */
	connection = vortex_connection_new (ctx, host, port, NULL, NULL);
	if (! vortex_connection_is_ok (connection, axl_true)) {
		/* free piggyback */
		axl_free (content);

		vortex_log (VORTEX_LEVEL_WARNING, "unable to connect to remote end point");
		return NULL;
	} /* end if */

 	vortex_log (VORTEX_LEVEL_DEBUG, "connection created with host=%s port=%s, disabling SEQ frames, creating TUNNEL ",
  		    host, port);
 	vortex_connection_seq_frame_updates (connection, axl_true);
	
	/* create a new channel and a queue to receive all replies */
	queue   = vortex_async_queue_new ();
	channel = vortex_channel_new_full (
		/* the connection reference where the channel will be
		 * created (and the application level proxy) */
		connection, 
		/* the channel number to use, use auto */
		0,
		/* default serverName */
		NULL,
		/* profile */
		TUNNEL_PROFILE,
		/* no especial profile content
		 * ("piggyback") encoding */
		EncodingNone,
		/* profile content ("piggyback") and its size */
		content, size,
		/* close channel notification. */
		NULL, NULL,
		/* frame handler received notification */
		vortex_channel_queue_reply, queue,
		/* no channel creation notification */
		NULL, NULL);
	
	/* free piggyback */
	axl_free (content);

	if (channel == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "failed to create TUNNEL channel, unable to proxy BEEP connection");

		/* close the connection */
		vortex_connection_close (connection);

		/* dealloc queue */
		vortex_async_queue_unref (queue);
		return NULL;
	}
	
	/* now wait for the reply to the request for creating a BEEP
	 * application level tunnel */
	reply = vortex_channel_get_reply (channel, queue);

	/* dealloc queue */
	vortex_async_queue_unref (queue);

	/* check fast reply */
	if (axl_cmp (vortex_frame_get_payload (reply), "<ok />")) {
		/* free the frame */
		vortex_frame_free (reply);
		goto ok_reply;
	} /* end if */

	/* check the reply content */
	ok = axl_doc_parse (vortex_frame_get_payload (reply),
			    vortex_frame_get_payload_size (reply),
			    NULL);

	/* free the frame */
	vortex_frame_free (reply);

	/* check reply */
	if (ok == NULL || !NODE_CMP_NAME (axl_doc_get_root (ok), "ok")) {

		/* failed to create application level proxy */
		axl_doc_free (ok);
		
		/* close the connection */
		vortex_connection_close (connection);
		
		return NULL;
	} /* end if */

	/* free document */
	axl_doc_free (ok);

 ok_reply:
	vortex_log (VORTEX_LEVEL_DEBUG, "connection created to the next hop in TUNNEL ok");

	/* if the caller didn't ask to perform the tunning reset
	 * (likely to be a middle hop), just return */
	if (! do_tunning_reset) {
		return connection;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "now performing tunning reset");

	/* remote end point reported <ok /> init connection tunning
	 * reset */

	/***
	 *** TUNNING RESET 
	 ***/

	/* prevent vortex library to close the underlying socket once
	 * the connection is closed. */
	vortex_connection_set_close_socket (connection, axl_false);

	/* get the socket where the underlying transport negotiation
	 * is taking place. */
	socket = vortex_connection_get_socket (connection);

	/* get a reference to previous connection */
	connection_aux = connection;

	/* create a new connection object, set the socket, using the
	 * same connection reference. This is could be seen as not
	 * necessary but it is.  The \ref VortexConnection object have
	 * functions to to define what to do when the connection is
	 * closed, how the data is send and received, or some
	 * functions to store and retrieve data.  */
	connection = vortex_connection_new_empty_from_connection (ctx, socket, connection_aux, VortexRoleInitiator);

 	/* check connection */
 	if (! vortex_connection_is_ok (connection, axl_false)) {
 		vortex_log (VORTEX_LEVEL_CRITICAL, "failed to create initial connection to prepare TUNNEL");
 		return NULL;
 	}

	/* unref the vortex connection and create a new empty one */
	__vortex_connection_set_not_connected (connection_aux, 
					       "connection instance being closed, without closing session, due to underlying TUNNEL negotiation",
					       VortexConnectionCloseCalled);
	/* dealloc the connection */
	vortex_connection_unref (connection_aux, "(vortex tunnel process)");

 	/* reenable tunning reset */
 	vortex_log (VORTEX_LEVEL_DEBUG, "reenabling SEQ frames for the TUNNEL connection=%d created",
 		    vortex_connection_get_id (connection));
 	vortex_connection_seq_frame_updates (connection, axl_false);

	/* the the connection to be blocking during the TUNNEL
	 * negotiation, once called
	 * vortex_connection_parse_greetings_and_enable, the
	 * connection will be again non-blocking. */
	vortex_connection_set_blocking_socket (connection);
	
	/* Issue again initial greetings, greetings just like we were
	 * creating a connection. */
	if (!vortex_greetings_client_send (connection, options)) {

                vortex_log (VORTEX_LEVEL_CRITICAL, "failed while sending greetings after TUNNEL negotiation");

		/* because the greeting connection have failed, we
		 * have to close and drop the \ref VortexConnection
		 * object. */
		vortex_connection_unref (connection, "(vortex tunnel profile)");

		return NULL;
		
	} /* end if */

	vortex_log (VORTEX_LEVEL_DEBUG, "greetings sent, waiting for reply");

	/* 6. Wait to get greetings reply (again issued because the
	 * underlying transport reset the session state) */
	reply = vortex_greetings_client_process (connection, options);
	
	vortex_log (VORTEX_LEVEL_DEBUG, "reply received, checking content");

	/* 7. Check received greetings reply, read again supported
	 * profiles from remote site and register the connection into
	 * the vortex reader to start exchanging data. */
	if (!vortex_connection_parse_greetings_and_enable (connection, reply)) {

		/* flag the connection object to be not usable,
		 * unrefering it so eventually resources allocated
		 * will be collected. */
		vortex_connection_unref (connection, "(vortex tunnel profile)");

		return NULL;
	} /* end if */

	vortex_log (VORTEX_LEVEL_DEBUG, "greetings reply received is ok, connection is ready and registered into the vortex reader (that's all)");

	/* return the connection */
	return connection;
}

/** 
 * @brief Deallocates memory used by the settings created by \ref
 * vortex_tunnel_settings_new.
 * 
 * @param settings The proxy settings to be deallocated.
 */
void vortex_tunnel_settings_free (VortexTunnelSettings * settings)
{
	/* if null value received, just return */
	if (settings == NULL)
		return;

	/* free and return */
	axl_doc_free (settings->doc);
	axl_free (settings);

	return;
}

/** 
 * @internal Second step to prepare the listener connection to finally
 * accept the TUNNEL profile end point request.
 * 
 * @param connection The connection to be tweaked to support the
 * TUNNEL endpoint request.
 */
void __vortex_tunnel_prepare_listener (VortexConnection * connection)
{

	VORTEX_SOCKET       socket;
	VortexConnection  * new_connection;
	VortexCtx         * ctx = vortex_connection_get_ctx (connection);

	vortex_log (VORTEX_LEVEL_DEBUG, "executing pre-read handler for TUNNEL profile, terminate configuration");

	/*
	 * Reached this point, the client side have sent the initial
	 * greetings message, so we can perform some final operations
	 * to fully accept the TUNNEL request.
	 * 
	 * 1) Make vortex to notify us no more for the pre read
	 * handler:
	 */
	vortex_connection_set_preread_handler (connection, NULL);

	/* 
	 * 2) Create a new connection instance, from the previous one,
	 * using its socket, role, data, but not channels created. */
	 
	 /* prepare the new connection */
	socket         = vortex_connection_get_socket (connection);
	new_connection = vortex_connection_new_empty_from_connection (ctx, socket, connection, VortexRoleListener);
	
	/* 3) close the previous connection */
	__vortex_connection_set_not_connected (connection, 
					       "connection instance being closed, without closing session, due to underlaying TUNNEL negoctiation",
					       VortexConnectionCloseCalled);
	
	/* 4) Accept the connection in an initial step, sending the
	 * greetings. This function flag the connection to be on the
	 * initial stage and register the connection into the vortex
	 * reader. */
	vortex_listener_accept_connection (new_connection, axl_true);

	/* It is not required to perform any additional task here,
	 * once the client greetings are received, the second accept
	 * step will do the rest. */

	vortex_log (VORTEX_LEVEL_DEBUG, "TUNNEL profile end point configuration finished!");

	return;
}

/** 
 * @internal Then function that takes data from one connection and
 * write them into the partner connection.
 * 
 * @param connection The connection that was notified to have data to
 * be read.
 */
void __vortex_tunnel_pass_octets (VortexConnection * connection)
{
	/* get the connection partner */
	VortexConnection * partner = vortex_connection_get_data (connection, VORTEX_TUNNEL_PARTNER_CONNECTION);
	char             * buffer  = vortex_connection_get_data (connection, VORTEX_TUNNEL_BUFFER);
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx        * ctx     = vortex_connection_get_ctx (connection);
#endif
	int                read;

	/* read data from the connection, and write it directly on the
	 * connection partner */
	read = vortex_frame_receive_raw (connection, buffer, VORTEX_MAX_BUFFER_SIZE - 2);

	/* check data read */
	if (read == 0) {
		vortex_log (VORTEX_LEVEL_DEBUG, "TUNNEL, nothing read from the connection(%s:%s) close connection", 
			    vortex_connection_get_host (connection),
			    vortex_connection_get_port (connection));
		
		/* close connections */
		vortex_connection_shutdown (connection);
		vortex_connection_shutdown (partner);
		return;
	}

	buffer[read] = 0;
	vortex_log (VORTEX_LEVEL_DEBUG, "TUNNEL, passing octects (%s:%s --> %s:%s): %d", 
		    vortex_connection_get_host (connection),
		    vortex_connection_get_port (connection),
		    vortex_connection_get_host (partner),
		    vortex_connection_get_port (partner),
		    read);

	/* write data to the partner connection */
	if (! vortex_frame_send_raw (partner, buffer, read)) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "failed to write data, seems connection is broken");
	}
	
	/* there is nothing more to do over here */
	return;
}

/** 
 * @internal Implementation for the start tunnel request on the
 * connection provided.
 */
axl_bool  __vortex_tunnel_start_request (const char       * profile, 
					 int                channel_num, 
					 VortexConnection * connection, 
					 const char       * serverName, 
					 const char       * profile_content, 
					 char            ** profile_content_reply, 
					 VortexEncoding     encoding, 
					 axlPointer         user_data)
{

	axlDoc                 * doc = NULL;
	axlNode                * node;
	axlError               * error;
	VortexConnection       * new_connection;
	VortexTunnelSettings   * settings = NULL;
	char                   * buffer;
	axl_bool                 result;
	VortexCtx              * ctx;
	
	/* references to handlers */
	VortexOnAcceptedConnection         tunnel_accept;
	VortexTunnelLocationResolver       tunnel_location_resolver;

	v_return_val_if_fail (connection, axl_false);

	/* get context */
	ctx = vortex_connection_get_ctx (connection);
	v_return_val_if_fail (ctx, axl_false);

	vortex_log (VORTEX_LEVEL_DEBUG, "received request to tunnel..\n");

	/* try to do fast endpoint parsing, checking if the tunnel
	 * content received, applies to <tunnel /> */
	if (axl_cmp (profile_content, "<tunnel />")) {
		/* found endpoint configuration */
		result = axl_true;
		goto end_point_request;
	} /* end if */

	/* fast reply did not work, try to parse the content */
	doc = axl_doc_parse (profile_content, strlen (profile_content), &error);
	if (doc == NULL) {
		/* fill an error to be reported */
		*profile_content_reply = vortex_frame_get_error_message ("500", "failed to parse incoming tunnel spec.", NULL);

		/* free the error */
		axl_error_free (error);

		return axl_false;
	} /* end if */

	vortex_log (VORTEX_LEVEL_DEBUG, "document parsed, resolv tunnel");

	/* check if the tunnel request is to open a end point
	 * connection to the current host or we have to relay the
	 * connection to another point. */
	node   = axl_doc_get_root (doc);
	result = ! axl_node_have_childs (node);

	/* check previous result */
	if (result) {
		/* endpoint request found */
	end_point_request:
		
		/* end point case, we have to accept or not the tunnel
		 * request (a proxied connection) */
		vortex_log (VORTEX_LEVEL_DEBUG, "end point case found, requesting the application level to accept or not the TUNNNEL");
		
		/* call here to get the application level approval to
		 * accept the endpoint tunnel request */
		tunnel_accept = vortex_ctx_get_data (ctx, VORTEX_TUNNEL_ACCEPT);
		if (tunnel_accept != NULL) {

			if (! tunnel_accept (connection, vortex_ctx_get_data (ctx, VORTEX_TUNNEL_ACCEPT_DATA))) {

				/* free document */
				axl_doc_free (doc);

				/* reply error message */
				(*profile_content_reply) = vortex_frame_get_error_message ("537", 
											   "Application level have denied to accept a TUNNEL request",
											   NULL);
				return axl_false;
			} /* end if */
		} /* end if */

		vortex_log (VORTEX_LEVEL_DEBUG, "end point TUNNEL has been accepted, prepare session for tunning reset");

		/* Here goes the trick that makes tunning reset at the server
		 * side to be possible.
		 * 
		 * 1) we prepare the socket connection to not be closed even
		 * if the channel 0 is closed on the given connection. */
		vortex_connection_set_close_socket (connection, axl_false);

		/* 2) use the pre read handler to finish the TUNNEL
		 * configuration, issuing againt the greetings and
		 * reseting the connection.  */
		vortex_connection_set_preread_handler (connection, __vortex_tunnel_prepare_listener);

		/* free document */
		axl_doc_free (doc);

	} else {
		vortex_log (VORTEX_LEVEL_DEBUG, "middle hop proxy found, contact with next TUNNEL");

		/* perform here the location resolution if found a
		 * final hop with endpoint or profile attributes
		 * defined */
		tunnel_location_resolver = vortex_ctx_get_data (ctx, VORTEX_TUNNEL_RESOLVER);
		if (tunnel_location_resolver != NULL) {
			/* location resolution defined, call to the
			 * user space to create a new tunnel
			 * setting */
			settings = tunnel_location_resolver (profile_content, 
							     strlen (profile_content),
							     doc,
							     vortex_ctx_get_data (ctx, VORTEX_TUNNEL_RESOLVER_DATA));
		} /* end if */

		/* check if, at this point, the tunnel setting is
		 * defined to create one by using the provided  */
		if (settings == NULL) {
			/* create settings reusing reference */
			settings      = axl_new (VortexTunnelSettings, 1);
			settings->doc = doc;

			/* configure settings */
			settings->ctx = ctx;
		} else {
			/* free the document if it is not going to be
			 * used by the settings */
			axl_doc_free (doc);
		}
		
		/* check tunnel settings created */
		if (settings == NULL) {

			vortex_log (VORTEX_LEVEL_CRITICAL, "found that profile content for TUNNEL is poorly formed");
			/* reply error message */
			(*profile_content_reply) = vortex_frame_get_error_message ("500", 
										   "General syntax error, poorly-formed XML",
										   NULL);
			return axl_false;
		} /* end if */

		vortex_log (VORTEX_LEVEL_DEBUG, "connecting to the next TUNNEL HOP: \n%s", 
			    profile_content);

		/* we are in the middle of a tunnel hop configuration,
		 * we have to ask to the next hop to open a tunnel
		 * request, so, we have to create a new separated
		 * connection */
		new_connection = __vortex_tunnel_new_common (settings, axl_false, NULL, NULL);

		/* free settings no matter the result */
		vortex_tunnel_settings_free (settings);

		/* check the connection */
		if (! vortex_connection_is_ok (new_connection, axl_false)) {

			vortex_log (VORTEX_LEVEL_CRITICAL, "failed to contact next hop in TUNNEL configuration");

			/* reply error message */
			(*profile_content_reply) = vortex_frame_get_error_message ("450", 
										   "Request action not taken, failed to connect to the next hop.",
										   NULL);
			return axl_false;
		} /* end if */

		vortex_log (VORTEX_LEVEL_DEBUG, "reached next TUNNEL hop, and accepted tunnel, installing preread handlers");

		/* nice we have created the tunnel, now prepare both
		 * connections to be handled by the vortex pre read
		 * functions */
		vortex_connection_set_data (connection,     VORTEX_TUNNEL_PARTNER_CONNECTION, new_connection);
		vortex_connection_set_data (new_connection, VORTEX_TUNNEL_PARTNER_CONNECTION, connection);
		
		/* allocate buffers */
		buffer = axl_new (char, VORTEX_MAX_BUFFER_SIZE);
		vortex_connection_set_data_full (connection,     VORTEX_TUNNEL_BUFFER, buffer, NULL, axl_free);

		buffer = axl_new (char, VORTEX_MAX_BUFFER_SIZE);
		vortex_connection_set_data_full (new_connection, VORTEX_TUNNEL_BUFFER, buffer, NULL, axl_free);
		
		/* configure the pre read handlers */
		vortex_connection_set_preread_handler (connection,     __vortex_tunnel_pass_octets);
		vortex_connection_set_preread_handler (new_connection, __vortex_tunnel_pass_octets);

		/* Make the vortex reader to have the only one
		 * reference to this connection.  */
		vortex_connection_unref (new_connection, "tunnel-prepared");
		
	} /* end if */
	
	/* accept the channel to be created and fill the profile
	 * content reply sending the ok message */
	*profile_content_reply = axl_strdup ("<ok />");

	vortex_log (VORTEX_LEVEL_DEBUG, "accepted TUNNEL, replying <ok />");
	
	return axl_true;
}

/** 
 * @brief Activates the required support to accept incoming TUNNEL
 * requests, an application level request to tunnel a BEEP connection.
 *
 * This function is required to accept incoming requests for the
 * TUNNEL profile. 
 *
 * The first two parameters allows to provide an user space handler
 * that is called to accept or not an incoming final tunnel endpoint
 * request. This means that the listener side is receiving a request
 * to create a normal BEEP session that is going to be TUNNELed, at
 * least, by the peer requesting the session.
 * 
 * @param accept_tunnel The function to be executed at the proper time
 * to ask for accept tunnel the session or not. In the case no handler
 * is provided, the default behaviour is to accept the tunnel request.
 *
 * @param ctx The context where the operation will be performed.
 *
 * @param accept_tunnel_data User space pointer to data that will be
 * passed to the previous handler.
 * 
 * @return axl_true if the profile have been activated, or axl_false if
 * something have failed.
 */
axl_bool               vortex_tunnel_accept_negotiation (VortexCtx                  * ctx,
							 VortexOnAcceptedConnection   accept_tunnel,
							 axlPointer                   accept_tunnel_data)
{
	/* configure handlers */
	vortex_ctx_set_data (ctx, VORTEX_TUNNEL_ACCEPT, accept_tunnel);
	vortex_ctx_set_data (ctx, VORTEX_TUNNEL_ACCEPT_DATA, accept_tunnel_data);

	/* register the profile, using the basic handlers */
	vortex_profiles_register (
		/* context */
		ctx,
		/* profile uri */
		TUNNEL_PROFILE,
		/* tunnel starth handler, no basic start handler
		 * provided at this point */
		NULL, NULL, 
		/* no close channel provided because it is not
		 * necessary. */
		NULL, NULL,
		/* provide a first level frame receive handler */
		NULL, NULL);
	
	/* register the especial start extended handler */
	vortex_profiles_register_extended_start (ctx,
						 TUNNEL_PROFILE, 
						 __vortex_tunnel_start_request, NULL);

	/* report a log with tunnel profile activated */
	vortex_log (VORTEX_LEVEL_DEBUG, "activated TUNNEL profile");
	
	/* all required values installed */
	return axl_true;
}

/** 
 * @brief Allows to configure a tunnel settings run-time host location
 * resolver handler which is called to translate tunnel settings into
 * the desired values.
 * 
 * Because the library doesn't have a way to figure out how to
 * translate <b>endpoint</b> and <b>profile</b> TUNNEL configuration,
 * the task is delegated to the user space by using a resolver
 * handler. 
 * 
 * The handler is executed at the right time to get an appropiate \ref
 * VortexTunnelSettings reference provided a particular tunnel
 * configuration which can be translated. 
 * 
 * This method allows to not only translate the <b>endpoint</b> and
 * <b>profile</b> value. It can be used to perform connection
 * redirections, etc.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param resolver The handler to be executed to resolver TUNNEL settings.
 *
 * @param resolver_data A referece to user defined data to be provided
 * to the resolver handler.
 */
void                   vortex_tunnel_set_resolver       (VortexCtx                    * ctx,
							 VortexTunnelLocationResolver   resolver,
							 axlPointer                     resolver_data)
{
	v_return_if_fail (ctx);

	/* configure handlers */
	vortex_ctx_set_data (ctx, VORTEX_TUNNEL_RESOLVER, resolver);
	vortex_ctx_set_data (ctx, VORTEX_TUNNEL_RESOLVER_DATA, resolver_data);
	return;

}

/**
 * @} 
 */

