/* 
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2013 Advanced Software Production Line, S.L.
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
#include <vortex_websocket.h>

#define VORTEX_WEBSOCKET_ENABLED "vo:websocket:en"
#define VORTEX_TLS_WEBSOCKET_ENABLED "vo:websocket:tls"

#define LOG_DOMAIN "vortex-websocket"

/** 
 * \defgroup vortex_websocket Vortex WebSocket API: support for BEEP connections over WebSocket
 */

/** 
 * \addtogroup vortex_websocket
 * @{
 */

struct _VortexWebsocketSetup {
	/** 
	 * @internal Context reference.
	 */
	VortexCtx * ctx;

	/** 
	 * @internal reference counting.
	 */
	int           ref_count;
	VortexMutex   mutex;

	/** 
	 * @internal Proxy location.
	 */
	char * proxy_host;
	char * proxy_port;

	/** 
	 * @internal Optional connection options to be used on
	 * connection setup.
	 */
	VortexConnectionOpts * options;

	/** 
	 * @internal Support for Origin and Host header 
	 */
	char * origin;
	char * host_header;
	/** 
	 * @internal Use WebSocket TLS support.
	 */
	axl_bool use_wss;
};

/** 
 * @internal Internal function to release and free the mutex memory
 * allocated.
 * 
 * @param mutex The mutex to destroy.
 */
void __vortex_websocket_free_mutex (VortexMutex * mutex)
{
	/* free mutex */
	vortex_mutex_destroy (mutex);
	axl_free (mutex);
	return;
}

/** 
 * @brief Allows to create a \ref VortexWebsocketSetup object which is used
 * to configure the WEBSOCKET CONNECT implementation.
 *
 * @param ctx The vortex context to associate. This parameter can't be
 * NULL.
 *
 * @return A newly created reference to \ref VortexWebsocketSetup. Use \ref
 * vortex_websocket_setup_unref to terminate it. Use \ref
 * vortex_websocket_setup_conf to setup required values.
 */
VortexWebsocketSetup  * vortex_websocket_setup_new      (VortexCtx * ctx)
{
	/* at this moment nothing special */
	VortexWebsocketSetup * setup;

	v_return_val_if_fail_msg (ctx, NULL, "Failed to create VortexWebsocketSetup, received NULL reference to VortexCtx");

	setup             =  axl_new (VortexWebsocketSetup, 1);
	setup->ctx        = ctx;
	setup->ref_count  = 1;
	vortex_mutex_create (&setup->mutex);

	return setup;
}

/** 
 * @brief Allows to increment the reference counting associated to the
 * \ref VortexWebsocketSetup object.
 *
 * @param setup Reference to update reference counting.
 *
 * @return axl_true in the case the reference counting is updated,
 * otherwise axl_false is returned.
 */
axl_bool           vortex_websocket_setup_ref      (VortexWebsocketSetup * setup)
{
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx * ctx = setup ? setup->ctx : NULL;
#endif

	v_return_val_if_fail_msg (setup, axl_false, "Unable to update reference counting, setup reference is NULL");

	vortex_mutex_lock (&setup->mutex);

	setup->ref_count++;

	vortex_mutex_unlock (&setup->mutex);

	return axl_true;
}

/** 
 * @brief Terminates the \ref VortexWebsocketSetup reference provided.
 *
 * @param setup The reference to finish.
 */
void               vortex_websocket_setup_unref    (VortexWebsocketSetup * setup)
{
	if (setup == NULL)
		return;


	/* lock mutex */
	vortex_mutex_lock (&setup->mutex);

	/* decrease ref count */
	setup->ref_count--;
	if (setup->ref_count != 0) {
		vortex_mutex_unlock (&setup->mutex);
		return;
	}
	vortex_mutex_unlock (&setup->mutex);

	axl_free (setup->proxy_host);
	axl_free (setup->proxy_port);
	axl_free (setup->host_header);
	axl_free (setup->origin);
	vortex_mutex_destroy (&setup->mutex);

	axl_free (setup);
	return;
}

/** 
 * @brief Allows to configure a particular value (\ref
 * VortexWebsocketConfItem) on the provided setup (\ref VortexWebsocketSetup).
 *
 * @param setup The setup to configure.
 *
 * @param item The configuration that will be modified.
 *
 * @param value The value to configure.
 */
void               vortex_websocket_setup_conf     (VortexWebsocketSetup      * setup,
					       VortexWebsocketConfItem     item,
					       axlPointer             value)
{
	char      * str_aux;
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx * ctx = (setup ? setup->ctx : NULL);
#endif

	v_return_if_fail_msg (setup, "Failed to setup because it was received a NULL VortexWebsocketSetup reference");

	switch (item) {
	case VORTEX_WEBSOCKET_CONF_ITEM_PROXY_HOST:
		/* record previous value */
		str_aux = setup->proxy_host;

		/* configure new value */
		setup->proxy_host = axl_strdup (value);

		/* dealloc previous value */
		if (str_aux)
			axl_free (str_aux);
		break;
	case VORTEX_WEBSOCKET_CONF_ITEM_PROXY_PORT:
		/* record previous value */
		str_aux = setup->proxy_port;

		/* configure new value */
		setup->proxy_port = axl_strdup (value);

		/* dealloc previous value */
		if (str_aux)
			axl_free (str_aux);
		break;
	case VORTEX_WEBSOCKET_CONF_ITEM_CONN_OPTS:
		/* configure connection options to be used on the
		   connection setup */
		setup->options = value;
		break;
	case VORTEX_WEBSOCKET_CONF_ITEM_ORIGIN:
		/* store origin */
		setup->origin = axl_strdup (value);
		break;
	case VORTEX_WEBSOCKET_CONF_ITEM_HOST:
		/* store origin */
		setup->host_header = axl_strdup (value);
		break;
	case VORTEX_WEBSOCKET_CONF_ITEM_ENABLE_TLS:
		/* enable wss support */
		setup->use_wss = PTR_TO_INT (value);
		break;
	} /* end switch */

	return;
}

typedef struct _VortexWebsocketConnectionData {
	VortexCtx            * ctx;
	char                 * host;
	char                 * port;
	VortexWebsocketSetup * setup;
	VortexConnectionNew    on_connected;
	axlPointer             user_data;
	VortexConnectionOpts * options;
} VortexWebsocketConnectionData;


/**
 * @internal Function used to release data associated to \ref
 * VortexWebsocketConnectionData
 */
void vortex_websocket_connection_data_free (VortexWebsocketConnectionData * data)
{
	if (data == NULL)
		return;
	axl_free (data->host);
	axl_free (data->port);

	vortex_websocket_setup_unref (data->setup);
	axl_free (data);
	return;
}

int vortex_websocket_read (VortexConnection * conn,
			   char             * buffer,
			   int                buffer_len)
{
	noPollConn * _conn = vortex_connection_get_hook (conn);
	int          result;
	VortexCtx   * ctx  = CONN_CTX (conn);
	VortexMutex * mutex;

	/* check if the connection has the greetings completed and it
	 * is initiator role */
	if (_conn == NULL)
		_conn = vortex_connection_get_data (conn, "nopoll-conn");

	/* get mutex */
	mutex = vortex_connection_get_data (conn, "ws:mutex");
	if (mutex == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to find mutex to protect ssl object to read data");
		return -1;
	} /* end if */

	/* call to acquire mutex, read and release */
	vortex_mutex_lock (mutex);
	result = nopoll_conn_read (_conn, buffer, buffer_len, nopoll_false, 0);
	vortex_mutex_unlock (mutex);

	vortex_log (VORTEX_LEVEL_DEBUG, "nopoll_conn_read returned result=%d, noPollConn status=%d",
		    result, nopoll_conn_is_ok (_conn));
	if (result == -1) {
		/* check connection status to notify that no data was
		 * available  */
		if (nopoll_conn_is_ok (_conn))
			return -2; 
	} /* end if */
	return result;
}

int vortex_websocket_send (VortexConnection * conn,
			   const char       * buffer,
			   int                buffer_len)
{
	noPollConn  * _conn = vortex_connection_get_hook (conn);
	VortexCtx   * ctx   = CONN_CTX (conn);
	VortexMutex * mutex;
	int           result;

	if (_conn == NULL)
		_conn = vortex_connection_get_data (conn, "nopoll-conn");

	/* get mutex */
	mutex = vortex_connection_get_data (conn, "ws:mutex");
	if (mutex == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to find mutex to protect ssl object to read data");
		return -1;
	} /* end if */

	/* acquire lock, operate and release */
	vortex_mutex_lock (mutex);
	result = nopoll_conn_send_text (_conn, buffer, buffer_len);
	vortex_mutex_unlock (mutex);

	return result;
}

void __vortex_websocket_conn_close (axlPointer ptr)
{
	noPollConn       * conn = ptr;
	/* noPollCtx        * nopoll_ctx = nopoll_conn_ctx (conn);*/
	VortexConnection * v_conn = nopoll_conn_get_hook (conn);
	VortexCtx        * ctx    = CONN_CTX (v_conn); 

	/* nopoll_log_enable (nopoll_ctx, nopoll_true);
	   nopoll_log_color_enable (nopoll_ctx, nopoll_true); */

	vortex_log (VORTEX_LEVEL_DEBUG, "Calling to close noPoll conn-id=%d (%p, socket: %d), associated to conn-id=%d (%p)",
		    nopoll_conn_get_id (conn), conn, nopoll_conn_socket (conn), vortex_connection_get_id (v_conn), v_conn);
	
	nopoll_conn_set_socket (conn, -1);
	nopoll_conn_close (conn);

	return;
}

void __vortex_websocket_release_ctx (axlPointer ptr)
{
	noPollCtx * ctx = ptr;

	nopoll_ctx_unref (ctx);

	return;
}


/**
 * @internal Implementation of vortex_websocket_connection_new.
 */
axlPointer __vortex_websocket_connection_new (VortexWebsocketConnectionData * data)
{
	VortexConnection * conn = NULL;

	/* paramters */
	VortexCtx            * ctx     = data->ctx;
	const char           * host    = data->host;
	const char           * port    = data->port;
	VortexWebsocketSetup * setup   = data->setup;
	VortexConnectionOpts * options = setup->options;
	noPollConn           * nopoll_conn;
	noPollCtx            * nopoll_ctx;
	char                 * custom_origin = NULL;

	VortexMutex          * mutex;
	
	/* create first a basic webSocket connection */
	nopoll_ctx = nopoll_ctx_new ();
	if (nopoll_ctx == NULL)
		goto report_conn;

	/* nopoll_log_enable (nopoll_ctx, axl_true);
	   nopoll_log_color_enable (nopoll_ctx, axl_true);    */

	if (! setup->origin)
		custom_origin = axl_strdup_printf ("http://%s", host);

	vortex_log (VORTEX_LEVEL_DEBUG, "Creating web socket connection, context %p (refs: %d)..",
		    nopoll_ctx, nopoll_ctx_ref_count (nopoll_ctx));

	/* creat the connection */
	if (setup->use_wss) {
		nopoll_conn = nopoll_conn_tls_new (nopoll_ctx, 
						   NULL, /* tls options, still not in use */
						   host, 
						   port,
						   setup->host_header ? setup->host_header : host,
						   /* get url */
						   NULL, 
						   /* protocols */
						   NULL,
						   setup->origin ? setup->origin : custom_origin);
	} else {
		nopoll_conn = nopoll_conn_new (nopoll_ctx, 
					       host, 
					       port,
					       setup->host_header ? setup->host_header : host,
					       /* get url */
					       NULL, 
					       /* protocols */
					       NULL,
					       setup->origin ? setup->origin : custom_origin);
	} /* end if */

	axl_free (custom_origin);

	vortex_log (VORTEX_LEVEL_DEBUG, "Created, waiting it to be completed (context %p, refs: %d, use TLS: %d)..",
		    nopoll_ctx, nopoll_ctx_ref_count (nopoll_ctx), setup->use_wss);

	/* wait until the connection is ready */
	if (! nopoll_conn_wait_until_connection_ready (nopoll_conn, vortex_connection_get_connect_timeout (ctx) / 1000000)) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Failed to create connection with %s:%s, nopoll connection is not working. Is there a listener at %s:%s?",
			    host, port, host, port);
		nopoll_conn_close (nopoll_conn);
		nopoll_ctx_unref (nopoll_ctx);
		goto report_conn;
	} /* end if */

	vortex_log (VORTEX_LEVEL_DEBUG, "Ok, it seems websocket connection was created, now enable BEEP on top of it (socket %d)....",
		    nopoll_conn_socket (nopoll_conn));
	
	/* create an empty connection object */
	conn = vortex_connection_new_empty (ctx, nopoll_conn_socket (nopoll_conn), VortexRoleInitiator);

	/* setup serverName value */
	vortex_connection_set_server_name (conn, setup->host_header ? setup->host_header : host);

	vortex_log (VORTEX_LEVEL_DEBUG, "Created BEEP conn-id=%d (%p) <-> nopoll conn-id=%d (%p), socket=%d, serverName=%s, setting up I/O handlers..",
		    vortex_connection_get_id (conn), conn, nopoll_conn_get_id (nopoll_conn), nopoll_conn, nopoll_conn_socket (nopoll_conn), vortex_connection_get_server_name (conn));

	/* associate VortexConnection <-> noPollConn */
	vortex_connection_set_data_full (conn, "nopoll-conn", nopoll_conn, NULL, __vortex_websocket_conn_close);
	vortex_connection_set_hook (conn, nopoll_conn);
	nopoll_conn_set_hook (nopoll_conn, conn);

	/* setup I/O handlers */
	mutex = axl_new (VortexMutex, 1);
	vortex_mutex_create (mutex);
	vortex_connection_set_data_full (conn, "ws:mutex", mutex,
					 NULL, (axlDestroyFunc) __vortex_websocket_free_mutex);

	vortex_connection_set_send_handler (conn, vortex_websocket_send);
	vortex_connection_set_receive_handler (conn, vortex_websocket_read);

	/* associate context */
	vortex_connection_set_data_full (conn, "nopoll-ctx", nopoll_ctx, NULL, __vortex_websocket_release_ctx);

	/* do BEEP greetings */
	if (! vortex_connection_do_greetings_exchange (ctx, conn, options, 60))
		goto report_conn;

 report_conn:
	if (conn) {
		/* do connection notification */
		vortex_log (VORTEX_LEVEL_DEBUG, "BEEP over WebSocket connection status %d, use wss: %d",
			    vortex_connection_is_ok (conn, axl_false), setup->use_wss);
		if (setup->use_wss && vortex_connection_is_ok (conn, axl_false)) {
			/* notify this connection is using TLS mode */
			vortex_connection_set_data (conn, VORTEX_TLS_WEBSOCKET_ENABLED, INT_TO_PTR(1));
			vortex_connection_set_data (conn, "tls-fication:status", INT_TO_PTR(1));
		} /* end if */

		/* flag that this connection was created by the websocket module */
		vortex_connection_set_data (conn, VORTEX_WEBSOCKET_ENABLED, INT_TO_PTR(1));

		/* drop a log */
		if (vortex_connection_is_ok (conn, axl_false)) {
			vortex_log (VORTEX_LEVEL_DEBUG, "BEEP over WebSocket connection id=%d setup ok%s", 
				    vortex_connection_get_id (conn),
				    vortex_websocket_connection_is_tls_running (conn) ? " (TLS activated)" : "");
		} /* end if */
	} /* end if */

	if (data->on_connected) {
		data->on_connected (conn, data->user_data);
		/* free data */
		vortex_websocket_connection_data_free (data);
		return NULL;
	} /* end if */

	/* free data */
	vortex_websocket_connection_data_free (data);
	return conn;
}

/** 
 * @brief Creates a new BEEP connection to a remote BEEP server, by
 * connecting to a WEBSOCKET server supporting WEBSOCKET CONNECT method (proxy
 * server).
 *
 * @param host The remote peer to connect to. This value will be used
 * for the Host WEBSOCKET header. In the case you want to use a
 * different Host header value than the value provided here use the
 * configuration item \ref VORTEX_WEBSOCKET_CONF_ITEM_HOST (\ref VortexWebsocketConfItem and \ref vortex_websocket_setup_conf).
 *
 * @param port The peer's port to connect to. This value will be used
 * for the port part of the Host header.
 *
 * @param setup Additional connection options. It must configure at least the origin value (\ref VORTEX_WEBSOCKET_CONF_ITEM_ORIGIN). Th reference you pass to this function will be owned by the connection created. This means you don't have to release the setup object created. If you want to reuse the same setup object, increase the reference counting each time you call this function (\ref vortex_websocket_setup_new).
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
 * <i><b>NOTE:</b> The \ref VortexCtx object to be used on this
 * function will be the one configured on setup parameter (reference
 * provided at \ref vortex_websocket_setup_new). This means you'll have to
 * create different \ref VortexWebsocketSetup instances for each context
 * you have.</i>
 *
 */
VortexConnection * vortex_websocket_connection_new (const char            * host, 
						    const char            * port,
						    VortexWebsocketSetup  * setup,
						    VortexConnectionNew     on_connected,
						    axlPointer              user_data)
{
	VortexWebsocketConnectionData * data;

	/* ctx reference */
	VortexCtx                * ctx = (setup) ? setup->ctx : NULL;

	/* check direct references */
	v_return_val_if_fail_msg (ctx,   NULL, "Unable to create connection, received NULL vortex context reference");
	v_return_val_if_fail_msg (host,  NULL, "Unable to create connection, received NULL host reference");
	v_return_val_if_fail_msg (port,  NULL, "Unable to create connection, received NULL port reference");
	v_return_val_if_fail_msg (setup, NULL, "Unable to create connection, received NULL setup reference");

	/* create invocation object */
	data               = axl_new (VortexWebsocketConnectionData, 1);
	data->ctx          = setup->ctx;
	data->host         = axl_strdup (host);
	data->port         = axl_strdup (port);
	data->setup        = setup;

	/* ref setup to avoid the user destroying it during connection
	 * setup */
	data->on_connected = on_connected;
	data->user_data    = user_data;

	if (on_connected) {
		vortex_log (VORTEX_LEVEL_DEBUG, "doing WEBSOCKET connection in threaded mode");
		vortex_thread_pool_new_task (ctx, (VortexThreadFunc) __vortex_websocket_connection_new, data);
		return NULL;
	} 
	
	vortex_log (VORTEX_LEVEL_DEBUG, "doing WEBSOCKET connection in blocking mode");
	return __vortex_websocket_connection_new (data);
}

/** 
 * @internal Function used to setup Host: header received over the
 * WebSocket into the BEEP serverName session value.
 */
axl_bool __vortex_websocket_setup_server_name_from_header (noPollConn * _new_conn, VortexConnection * new_conn)
{
	VortexCtx * ctx = CONN_CTX (new_conn);

	v_return_val_if_fail (_new_conn && new_conn, axl_false);

	if (nopoll_conn_get_host_header (_new_conn)) {
		
		if (nopoll_conn_is_tls_on (_new_conn)) {
			/* drop a log */
			vortex_log (VORTEX_LEVEL_DEBUG, "Flagging WebSocket connection id=%d with TLS on",
				    vortex_connection_get_id (new_conn));
			
			/* configure tls enabled */
			vortex_connection_set_data (new_conn, VORTEX_TLS_WEBSOCKET_ENABLED, INT_TO_PTR(1));
			vortex_connection_set_data (new_conn, "tls-fication:status", INT_TO_PTR(1));
		} /* end if */
		
		/* configure server name and drop a log */
		vortex_connection_set_server_name (new_conn, nopoll_conn_get_host_header (_new_conn));
		vortex_log (VORTEX_LEVEL_DEBUG, "Virtual hosting Host: %s header received, set as BEEP serverName=%s value on conn-id=%d",
			    nopoll_conn_get_host_header (_new_conn), vortex_connection_get_server_name (new_conn), 
			    vortex_connection_get_id (new_conn));

		/* return ok operation */
		return axl_true;
	} /* end if */

	return axl_false;
}

int vortex_websocket_listener_post_actions (VortexCtx               * ctx, 
					    VortexConnection        * new_conn, 
					    VortexConnection       ** __new_ref, 
					    VortexConnectionStage     stage,
					    axlPointer                user_data)
{
	noPollConn  * _new_conn   = vortex_connection_get_hook (new_conn);

	/* skip for connections that aren't final listeners */
	if (_new_conn == NULL || vortex_connection_get_role (new_conn) != VortexRoleListener || ! vortex_websocket_connection_is (new_conn))
		return 1;

	/* setup serverName BEEP attribute taking that value from
	 * WebSocket Host: header */

	/* check if the WebSocket handshake completed (where Host:
	 * header could be sent) and check it */
	if (__vortex_websocket_setup_server_name_from_header (_new_conn, new_conn))
		return 1;

	/* request to shutdown connection because host header wasn't
	 * received */
	vortex_log (VORTEX_LEVEL_CRITICAL, "WebSocket Host: header wasn't received over BEEP conn-id=%d, noPoll conn-id=%d, current serverName=%s (role: %d), requesting to shutdown connection",
		    vortex_connection_get_id (new_conn), nopoll_conn_get_id (_new_conn), 
		    vortex_connection_get_server_name (new_conn) ? vortex_connection_get_server_name (new_conn) : "(null)", 
		    vortex_connection_get_role (new_conn));
	return -1;
}

/** 
 * @internal Setup and link the provided listener connection accepted
 * with the provided associated new no poll connection.
 */
void __vortex_websocket_setup_listener_connection (VortexConnection * new_conn, noPollConn * _new_conn)
{
	VortexMutex        * mutex;

	/* set this connection was accepted under the WebSocket interface */
	vortex_connection_set_data (new_conn, VORTEX_WEBSOCKET_ENABLED, INT_TO_PTR (1));

	/* setup I/O handlers */
	mutex = axl_new (VortexMutex, 1);
	vortex_mutex_create (mutex);
	vortex_connection_set_data_full (new_conn, "ws:mutex", mutex,
					 NULL, (axlDestroyFunc) __vortex_websocket_free_mutex);

	vortex_connection_set_send_handler (new_conn, vortex_websocket_send);
	vortex_connection_set_receive_handler (new_conn, vortex_websocket_read);

	vortex_connection_set_data_full (new_conn, "nopoll-conn", _new_conn, NULL, __vortex_websocket_conn_close);
	vortex_connection_set_hook (new_conn, _new_conn);

	return;
}

void vortex_websocket_listener_accept (VortexConnection * conn)
{
	VortexCtx          * ctx        = CONN_CTX (conn);
	noPollConn         * listener   = vortex_connection_get_hook (conn);
	noPollCtx          * nopoll_ctx = nopoll_conn_ctx (listener);
	noPollConn         * _new_conn  = NULL;
	VortexConnection   * new_conn   = NULL;

	vortex_log (VORTEX_LEVEL_DEBUG, "Called pre-read handler over a BEEP over WebSocket listener id=%d (socket=%d, noPollConn %p), doing initial accept",
		    vortex_connection_get_id (conn), vortex_connection_get_socket (conn), listener);
	/* nopoll_log_enable (nopoll_ctx, nopoll_true);
	   nopoll_log_color_enable (nopoll_ctx, nopoll_true); */

	/* ok, accept connection and register it */
	_new_conn = nopoll_conn_accept (nopoll_ctx, listener);
	if (_new_conn == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Received null pointer from nopoll_conn_accept, that's strange. Failure found at listener %s:%s",
			    vortex_connection_get_host (conn), vortex_connection_get_port (conn));
		return;
	} /* end if */

	/* call for initial accept */
	new_conn = __vortex_listener_initial_accept (
		ctx, 
		nopoll_conn_socket (_new_conn), 
		conn,
		/* don't register the connection */
		axl_false);

	__vortex_websocket_setup_listener_connection (new_conn, _new_conn);

	/* set this connection was accepted under the WebSocket interface */
	vortex_connection_set_data (new_conn, VORTEX_WEBSOCKET_ENABLED, INT_TO_PTR (1));

	/* called to accept listener inthe next step now we have it configured */
	vortex_listener_accept_connection (new_conn, axl_true);

	/* check connection status just to drop a log */
	if (vortex_connection_is_ok (new_conn, axl_false)) 
		vortex_log (VORTEX_LEVEL_DEBUG, "Received new BEEP over WebSocket connection id=%d, still required to complete BEEP greetings", vortex_connection_get_id (new_conn));
	
	return;
}

/** 
 * @internal Function used to check and install post action function
 * (if it wasn't found).
 */
void __vortex_websocket_install_post_actions (VortexCtx * ctx)
{
	if (! vortex_ctx_get_data (ctx, "vo:web:post-action")) {
		/* setup handler to be called for each connection
		 * accepted on every listener started */
		vortex_connection_set_connection_actions (ctx, CONNECTION_STAGE_POST_CREATED, vortex_websocket_listener_post_actions, NULL);
		
		/* setup we have already setup the handler */
		vortex_ctx_set_data (ctx, "vo:web:post-action", INT_TO_PTR (1));
	} /* end if */

	return;
}

/** 
 * @brief Allows to create a new BEEP listener accepting connections over WebSocket.
 *
 * The function accepts the noPollConn reference of the WebSocket
 * listener already running through which BEEP clients will connect
 * using webSocket as transport protocol.
 *
 * @param ctx The context where the operation is taking place.
 *
 * @param listener The noPollConn listener on top this new BEEP listener is going to run. 
 *
 * @param on_ready_full The on ready function as defined by \ref vortex_listener_new_full.
 *
 * @param user_data User defined pointer to be passed in into
 * on_ready_full function when called.
 *
 * @return The function returns a newly created BEEP listener (over
 * WebSocket) or NULL if the optional handler is provided
 * (on_ready). See \ref vortex_listener_new_full to know more about
 * the value returned by this function as this function is just a
 * wrapper to it, providing the needed bridging between BEEP and WebSocket.
 */
VortexConnection * vortex_websocket_listener_new   (VortexCtx                * ctx,
						    noPollConn               * listener,
						    VortexListenerReadyFull    on_ready_full,
						    axlPointer                 user_data)
{
	VortexConnection * result;

	v_return_val_if_fail (ctx || listener, NULL);

	/* nopoll_log_enable (nopoll_conn_ctx (listener), axl_true);
	   nopoll_log_color_enable (nopoll_conn_ctx (listener), axl_true);      */

	/* check if we have installed post actions function */
	__vortex_websocket_install_post_actions (ctx);

	/* check if this is a blocking or async listener creation */
	result = vortex_connection_new_empty (ctx, nopoll_conn_socket (listener), VortexRoleMasterListener);
	/* operation ok */
	if (vortex_connection_is_ok (result, axl_false)) {
		/* now configure here the on accept function that
		 * should be used to create new BEEP sessions on top
		 * of WebSocket */
		vortex_connection_set_preread_handler (result, vortex_websocket_listener_accept);

		/* associate listener and context */
		vortex_connection_set_data_full (result, "nopoll-conn", listener, NULL, __vortex_websocket_conn_close);
		vortex_connection_set_hook (result, listener);

		/* register into the reader */
		vortex_reader_watch_listener (ctx, result);
	}

	if (on_ready_full == NULL) 
		return result;

	/* do an async notification on the onready handler here and return NULL */
	return NULL;
}

/** 
 * @brief Allows to check if the provided connection was created by
 * using \ref vortex_websocket_connection_new or \ref vortex_websocket_listener_new.
 *
 * @param conn The connection to check for being created though an
 * WEBSOCKET proxy server.
 *
 * @return axl_true in the case the connection was created using
 * WebSocket API, otherwise axl_false is returned.
 */
axl_bool           vortex_websocket_connection_is (VortexConnection * conn)
{
	/* current status */
	return PTR_TO_INT (vortex_connection_get_data (conn, VORTEX_WEBSOCKET_ENABLED));
}

/** 
 * @brief Allows to check if the provided connection has TLS enabled
 * (WebSocket over TLS).
 *
 * @param conn The connection that is asked about its connection type.
 *
 * @return axl_true in the case the connection is running WebSocket
 * over TLS.
 */
axl_bool           vortex_websocket_connection_is_tls_running (VortexConnection * conn)
{
	noPollConn * _conn;
	VortexCtx  * ctx = CONN_CTX (conn);

	/* check incoming reference */
	if (conn == NULL)
		return axl_false;

	/* get connection hook */
	_conn = vortex_connection_get_hook (conn);
	vortex_log (VORTEX_LEVEL_DEBUG, "Checking connection hook: %p -> %p", conn, _conn);
	if (_conn == NULL)
		return axl_false;

	/* check if the noPoll connection is running TLS */
	vortex_log (VORTEX_LEVEL_DEBUG, "Checking connection TLS state: %p (%d)", _conn, nopoll_conn_is_tls_on (_conn));
	if (! nopoll_conn_is_tls_on (_conn)) 
		return axl_false;

	/* current status */
	vortex_log (VORTEX_LEVEL_DEBUG, "Checking connection TLS vortex flag: %p (%d)", conn, 
		    PTR_TO_INT (vortex_connection_get_data (conn, VORTEX_TLS_WEBSOCKET_ENABLED)));
	return PTR_TO_INT (vortex_connection_get_data (conn, VORTEX_TLS_WEBSOCKET_ENABLED));
}

int __vortex_websocket_detect_and_prepare_transport (VortexCtx        * ctx, 
						     VortexConnection * listener,
						     VortexConnection * conn, 
						     VORTEX_SOCKET      _session,
						     const char       * bytes,
						     axlPointer         user_data)
{
	noPollCtx  * nopoll_ctx;
	noPollConn * nopoll_conn;
	noPollConn * nopoll_listener;
	axl_bool     is_tls_conn;

	/* detect tls conn */
	is_tls_conn = bytes[0] == 22 && bytes[1] == 3 && bytes[2] == 1;

	if (! axl_memcmp ("GET", bytes, 3) && ! is_tls_conn)
		return 1; /* nothing detected here (it doesn't seems
			     to be a websocket connection) */

	vortex_log (VORTEX_LEVEL_DEBUG, "WEB-SOCKET: activated port share handler for conn-id=%d (socket=%d)",
		    vortex_connection_get_id (conn), _session);

	/* get context associated to the listener */
	nopoll_ctx = vortex_ctx_get_data (ctx, "nopoll-ctx");
	if (nopoll_ctx == NULL) {
		/* no context found, create we and set it into the the
		   listener */

		/* create a context */
		nopoll_ctx = nopoll_ctx_new ();

		/* associate context */
		vortex_ctx_set_data_full (ctx, "nopoll-ctx", nopoll_ctx, NULL, __vortex_websocket_release_ctx);
	} /* end if */

	/* MASTER LISTENER: now get the noPoll listener associated to this Vortex listener */
	nopoll_listener = vortex_connection_get_data (listener, "nopoll-conn");
	if (nopoll_listener == NULL) {
		/* create the listener */
		nopoll_listener = nopoll_listener_from_socket (nopoll_ctx, vortex_connection_get_socket (listener));

		/* associate it to the connection */
		vortex_connection_set_data_full (listener, "nopoll-conn", nopoll_listener, NULL, __vortex_websocket_conn_close);
	} /* end if */

	/* ACCEPTED CONNECTION: create connection accepted noPollConn object */
	nopoll_conn = nopoll_listener_from_socket (nopoll_ctx, vortex_connection_get_socket (conn));
	if (nopoll_conn == NULL) {
		/* failed to create connection, this is a failure */
		vortex_log (VORTEX_LEVEL_CRITICAL, "Unable to create noPollConn object for accepted conn-id=%d, socket=%d",
			    vortex_connection_get_id (conn), vortex_connection_get_socket (conn));
		return -1;
	} /* end if */

	/* complete websocket conn setup */
	if (! nopoll_conn_accept_complete (nopoll_ctx, nopoll_listener, nopoll_conn, _session, is_tls_conn)) {
		/* failed to create connection, this is a failure */
		vortex_log (VORTEX_LEVEL_CRITICAL, "noPoll accept process failed (nopoll_conn_accept_complete) conn-id=%d, socket=%d",
			    vortex_connection_get_id (conn), vortex_connection_get_socket (conn));
		return -1;
	} /* end if */

	/* now prepare all I/O associated to the listener */
	vortex_log (VORTEX_LEVEL_DEBUG, "WEB-SOCKET: Detected and activated websocket transport for conn-id=%d, serverName=%s (is tls on=%d)", 
		    vortex_connection_get_id (conn), 
		    vortex_connection_get_server_name (conn) ? vortex_connection_get_server_name (conn) : "<not defined>",
		    is_tls_conn);
	__vortex_websocket_setup_listener_connection (conn, nopoll_conn);
	return 2; /* web socket connection detected */
}

/** 
 * @brief Allows to activate Vortex Library port sharing feature on
 * the provided local listener address and local listener port.
 *
 * Vortex Library port sharing is a feature provided by the core
 * library that allows running pure BEEP protocol (as defined by RFC
 * 3081, that is, running BEEP on top of TCP) but also activate other
 * previous transports before running BEEP like BEEP over WebSocket or
 * BEEP over TLS webSocket.
 *
 * This allows to have on a single port an unified configuration that
 * allows receiving normal BEEP conections and BEEP over WebSocket
 * (with or without TLS), thus saving configurations. 
 *
 * @param ctx The context where the port sharing feature will be
 * enabled.
 *
 * @param nopoll_ctx Reference to the noPoll context that should be
 * used by all noPollConn listeners. If you provide a NULL value, the
 * function will create a new noPollCtx object. The function will
 * acquire a reference to the noPollCtx object passed in. The caller
 * still owns all references to this object.
 *
 * @param local_addr The local address to limit the port sharing
 * operation (or NULL if it is required to be applied to any address).
 *
 * @param local_port The local port to limit the port sharing feature
 * or NULL to enable it over all listeners ports.
 *
 * @param enable axl_true to activate port sharing status or axl_false
 * to disable it.
 *
 * @return The function returns a handler of the port share
 * installed. This handler can later be used to remove this port share
 * detector. The function returns NULL in the case of failure.
 */
axlPointer         vortex_websocket_listener_port_sharing (VortexCtx  * ctx, 
							   noPollCtx  * nopoll_ctx,
							   const char * local_addr, 
							   const char * local_port)
{
	v_return_val_if_fail (ctx, NULL);

	/* associate nopoll-ctx if found */
	if (nopoll_ctx) {
		if (! nopoll_ctx_ref (nopoll_ctx)) {
			vortex_log (VORTEX_LEVEL_DEBUG, "Unable to acquire noPollCtx reference, failed to activate port sharing feature..");
			return NULL;
		} /* end if */
		vortex_ctx_set_data_full (ctx, "nopoll-ctx", nopoll_ctx, NULL, __vortex_websocket_release_ctx);
	}

	/* check if we have installed post actions function */
	__vortex_websocket_install_post_actions (ctx);

	/* call to enable/disable according to the enable status */
	return vortex_listener_set_port_sharing_handling (
		ctx, 
		local_addr, local_port, 
		__vortex_websocket_detect_and_prepare_transport, NULL);
}

/** 
 * @} 
 */

