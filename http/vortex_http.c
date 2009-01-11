/* 
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2008 Advanced Software Production Line, S.L.
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
#include <vortex_http.h>

struct _VortexHttpSetup {
	/**
	 * @internal Context reference.
	 */
	VortexCtx * ctx;

	/**
	 * @internal reference counting.
	 */
	int    ref_count;

	/**
	 * @internal Proxy location.
	 */
	char * proxy_host;
	char * proxy_port;

	
};

/**
 * @brief Allows to create a \ref VortexHttpSetup object which is used
 * to configure the HTTP CONNECT implementation.
 *
 * @param ctx The vortex context to associate. This parameter can't be
 * NULL.
 *
 * @return A newly created reference to \ref VortexHttpSetup. Use \ref
 * vortex_http_setup_unref to terminate it. Use \ref
 * vortex_http_setup_conf to setup required values.
 */
VortexHttpSetup  * vortex_http_setup_new      (VortexCtx * ctx)
{
	/* at this moment nothing special */
	VortexHttpSetup * setup;

	v_return_val_if_fail_msg (ctx, NULL, "Failed to create VortexHttpSetup, received NULL reference to VortexCtx");

	setup             =  axl_new (VortexHttpSetup, 1);
	setup->ctx        = ctx;
	setup->ref_count  = 1;

	return setup;
}

/**
 * @brief Terminates the \ref VortexHttpSetup reference provided.
 *
 * @param setup The reference to finish.
 */
void               vortex_http_setup_unref    (VortexHttpSetup * setup)
{
	if (setup == NULL)
		return;

	/* decrease ref count */
	setup->ref_count--;
	if (setup->ref_count != 0)
		return;

	axl_free (setup->proxy_host);
	axl_free (setup->proxy_port);
	axl_free (setup);
	return;
}

/**
 * @brief Allows to configure a particular value (\ref
 * VortexHttpConfItem) on the provided setup (\ref VortexHttpSetup).
 *
 * @param setup The setup to configure.
 *
 * @param item The configuration that will be modified.
 *
 * @param str_value The string value to configure.
 */
void               vortex_http_setup_conf     (VortexHttpSetup      * setup,
					       VortexHttpConfItem     item,
					       const char           * str_value)
{
	char      * str_aux;
	VortexCtx * ctx = (setup ? setup->ctx : NULL);

	v_return_if_fail_msg (setup, "Failed to setup because it was received a NULL VortexHttpSetup reference");

	switch (item) {
	case VORTEX_HTTP_CONF_ITEM_PROXY_HOST:
		/* record previous value */
		str_aux = setup->proxy_host;

		/* configure new value */
		setup->proxy_host = axl_strdup (str_value);

		/* dealloc previous value */
		if (str_aux)
			axl_free (str_aux);
		break;
	case VORTEX_HTTP_CONF_ITEM_PROXY_PORT:
		/* record previous value */
		str_aux = setup->proxy_port;

		/* configure new value */
		setup->proxy_port = axl_strdup (str_value);

		/* dealloc previous value */
		if (str_aux)
			axl_free (str_aux);
		break;
	} /* end switch */

	return;
}

/**
 * @brief Creates a new BEEP connection to a remote HTTP server
 * supporting HTTP CONNECT method. 
 *
 * The function also allows connecting to a remote BEEP server using a
 * HTTP proxy that also support CONNECT method.
 *
 * @param ctx The context where the operation will be performed.
 *
 * @param host The remote peer to connect to. This value will be used
 * for the Host HTTP header.
 *
 * @param port The peer's port to connect to. This value will be used
 * for the port part of the Host header.
 *
 * @param setup Additional connection options. This can be used to
 * configure a HTTP proxy with CONNECT support, authenticaion,
 * etc. This reference is not optional. It must configure, at least,
 * proxy host and its port.
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
 */
VortexConnection * vortex_http_connection_new (VortexCtx            * ctx,
					       const char           * host, 
					       const char           * port,
					       VortexHttpSetup      * setup,
					       VortexConnectionNew    on_connected,
					       axlPointer             user_data)
{
	VORTEX_SOCKET      socket;
	int                timeout;
	axlError         * error = NULL;
	VortexConnection * conn;
	char             * http_header;
	char               buffer[1024];
	int                bytes_read;

	v_return_val_if_fail_msg (ctx,   NULL, "Unable to create connection, received NULL vortex context reference");
	v_return_val_if_fail_msg (host,  NULL, "Unable to create connection, received NULL host reference");
	v_return_val_if_fail_msg (port,  NULL, "Unable to create connection, received NULL port reference");
	v_return_val_if_fail_msg (setup, NULL, "Unable to create connection, received NULL setup reference");

	/* check host and port */
	v_return_val_if_fail (setup->proxy_host, NULL);
	v_return_val_if_fail (setup->proxy_port, NULL);

	/* create an empty connection object */
	conn = vortex_connection_new_empty (ctx, -1, VortexRoleInitiator);

	/* do a connection against the host */
	vortex_log (VORTEX_LEVEL_DEBUG, "connecting to HTTP proxy: %s:%s", setup->proxy_port, setup->proxy_port);
	timeout = vortex_connection_get_connect_timeout (ctx);
	socket  = vortex_connection_sock_connect (ctx, setup->proxy_host, setup->proxy_port, &timeout, &error);
	if (socket == -1) {
		/* set message and free error */
		__vortex_connection_set_not_connected (conn, axl_error_get (error), axl_error_get_code (error));
		axl_error_free (error);
		return conn;
	} /* end if */
	
	/* set socket and simulate host and port (not the host and
	 * port from the socket) */
	vortex_log (VORTEX_LEVEL_DEBUG, "connection ok, how associate socket..");
	if (! vortex_connection_set_socket (conn, socket, host, port)) {
		__vortex_connection_set_not_connected (conn, "Failed to set socket to the BEEP connection created", VortexError);
		return conn;
	} /* end if */

	/* create CONNECT header */
	http_header = axl_strdup_printf ("CONNECT %s:%s HTTP/1.1\r\nHost: %s:%s\r\n\r\n",
					 host, port, host, port);

	/* send connect info */
	vortex_log (VORTEX_LEVEL_DEBUG, "sending CONNECT method headers");
	if (! vortex_frame_send_raw (conn, http_header, strlen (http_header))) {
		axl_free (http_header);
		
		__vortex_connection_set_not_connected (conn, "Failed to send initial CONNECT http header", VortexProtocolError);
		return conn;
	}
	axl_free (http_header);
	
	/* now read from the socket the HTTP 200 OK status */
	vortex_log (VORTEX_LEVEL_DEBUG, "reading reply");

	/* set socket in blocking mode to read content */
	vortex_connection_set_sock_block (socket, axl_true);
	
	/* read line */
	bytes_read = vortex_frame_readline (conn, buffer, 1024);
	switch (bytes_read) {
	case 0:
		__vortex_connection_set_not_connected (conn, "Remote server (either the proxy or the destination BEEP server) have closed the connection", VortexError);
		return conn;
	case -1:
		__vortex_connection_set_not_connected (conn, "An error have happen while reading content", VortexError);
		return conn;
	default:
		if (bytes_read > 0) {
			vortex_log (VORTEX_LEVEL_DEBUG, "reply received: %s, processing", buffer);
			if (! axl_memcmp (buffer, "HTTP/1.0 2", 10) &&
			    ! axl_memcmp (buffer, "HTTP/1.1 2", 10)) {
				__vortex_connection_set_not_connected (conn, "Received an negative reply. Some administrative configuration prevents from connecting to the destination requested", VortexError);
				return conn;
			} /* end if */

			vortex_log (VORTEX_LEVEL_DEBUG, "Read the next empty line");
			bytes_read = vortex_frame_readline (conn, buffer, 1024);
			if (bytes_read > 2) 
				vortex_log (VORTEX_LEVEL_WARNING, "received more than HTTP header termination, expected to find \\r\\n sequence..");
			break;
		} /* end if */

		__vortex_connection_set_not_connected (conn, "Failed to read reply after CONNECT method request", VortexError);
		return conn;
	} /* end switch */
		
	/* reached this point the remote HTTP/1.1 proxy have switched
	 * to tunnel mode, now init greetins phase */
	if (! vortex_connection_do_greetings_exchange (ctx, conn, timeout))
		return conn;
	
	vortex_log (VORTEX_LEVEL_DEBUG, "connection through HTTP proxy setup ok");
	return conn;
}

/**
 * @} 
 */

