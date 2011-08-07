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
#include <vortex.h>

/* local include */
#include <vortex_ctx_private.h>
#include <vortex_connection_private.h>

/* include inline dtd */
#include <vortex-listener-conf.dtd.h>

#define LOG_DOMAIN "vortex-listener"

typedef struct _VortexListenerOnAcceptData {
	VortexOnAcceptedConnection on_accept;
	axlPointer                 on_accept_data;
}VortexListenerOnAcceptData;

int  __vortex_listener_get_port (const char  * port)
{
	return strtol (port, NULL, 10);
}

/** 
 * \defgroup vortex_listener Vortex Listener: Set of functions to create BEEP Listeners (server applications that accept incoming requests)
 */

/** 
 * \addtogroup vortex_listener
 * @{
 */

/** 
 * @brief Common task to be done to accept a connection before
 * greetings message is issued while working as a Listener.
 * 
 * @param connection The connection to accept.
 *
 * @param send_greetings If the greetings message must be sent or not.
 */
void vortex_listener_accept_connection    (VortexConnection * connection, axl_bool  send_greetings)
{
	VortexCtx                  * ctx;
	axl_bool                     result;
	int                          iterator;
	VortexListenerOnAcceptData * data;
	VortexConnection           * listener;
	VortexOnAcceptedConnection   on_accept;
	axlPointer                   on_accept_data;

	/* check received reference */
	if (connection == NULL)
		return;

	/* Get a reference to the connection, accept the new
	 * connection under the same domain as the context of the
	 * listener  */
	ctx = vortex_connection_get_ctx (connection);
	
	/* call to the handler defined */
	iterator = 0;
	result   = axl_true;

	/* init lock */
	vortex_mutex_lock (&ctx->listener_mutex);
	while (iterator < axl_list_length (ctx->listener_on_accept_handlers)) {
		/* call and check */
		data = axl_list_get_nth (ctx->listener_on_accept_handlers, iterator);

		/* internal check */
		if (data == NULL) {
			result = axl_false;
			break;
		} /* end if */

		/* get safe references to handlers */
		on_accept      = data->on_accept;
		on_accept_data = data->on_accept_data;

		/* unlock during notification */
		vortex_mutex_unlock (&ctx->listener_mutex);

		/* check if the following handler accept the incoming
		 * connection */
		vortex_log (VORTEX_LEVEL_DEBUG, "calling to accept connection, handler: %p, data: %p",
			    data->on_accept, data->on_accept_data);
		if (! on_accept (connection, on_accept_data)) {

			/* init lock */
			vortex_mutex_lock (&ctx->listener_mutex);

			vortex_log (VORTEX_LEVEL_DEBUG, "on accept handler have denied to accept the connection, handler: %p, data: %p",
				    on_accept, on_accept_data);

			/* found that at least one handler do not
			 * accept the incoming connection, dropping */
			result = axl_false;
			break;
		}

		/* init lock */
		vortex_mutex_lock (&ctx->listener_mutex);
		
		/* next iterator */
		iterator++;

	} /* end while */

	/* unlock */
	vortex_mutex_unlock (&ctx->listener_mutex);

	/* check result */
	if (! result) {
		/* check connection status */
		if (vortex_connection_is_ok (connection, axl_false)) {
			vortex_log (VORTEX_LEVEL_CRITICAL, "the server application level have dropped the provided connection");

			/* send greeints error */
			vortex_greetings_error_send (connection, 
						     NULL, "554",
						     "Transaction failed, peer have denied your request");
		} /* end if */

		/* flag the connection to be not connected */
		__vortex_connection_set_not_connected (connection, "connection filtered by on accept handler", VortexConnectionFiltered);

		vortex_connection_unref (connection, "vortex listener");
		return;

	} /* end if */

	/* check to send greetings but only if prered is not defined */
	if (! vortex_connection_is_defined_preread_handler (connection)) {
		/* get master listener (the connection that created this
		 * incoming connection) to check if we have to send greetings */
		listener = vortex_connection_get_data (connection, "_vo:li:master");
		if (vortex_connection_get_data (listener, "vo:li:send-greetings")) {
			vortex_log (VORTEX_LEVEL_DEBUG, "Sending BEEP greetings to client without waiting for theirs..");
			vortex_greetings_send (connection, NULL);
		} /* end if */
	} /* end if */

	/* call to complete incoming connection register operation */
	vortex_listener_complete_register (connection);
	
	/* close connection and free resources */
	vortex_log (VORTEX_LEVEL_DEBUG, "worker ended, connection registered on manager (initial accept)");

	return;
}

/** 
 * @internal Fucntion that allows to complete last parts once a
 * connection is accepted.
 */
void          vortex_listener_complete_register    (VortexConnection     * connection)
{
	VortexCtx * ctx;

	/* check connection received */
	if (connection == NULL)
		return;

	ctx = vortex_connection_get_ctx (connection);

	/* flag the connection to be on initial step */
	connection->initial_accept = axl_true;

	/*
	 * register the connection on vortex reader from here, the
	 * connection get a non-blocking state
	 */
	vortex_reader_watch_connection      (ctx, connection);

	/*
	 * Because this is a listener, we don't want to pay attention
	 * to free connection on errors. connection already have 1
	 * reference (reader), so let's reference counting to the job
	 * of free connection resources.
	 */
	vortex_connection_unref (connection, "vortex listener (initial accept)");

	return;
}

void __vortex_listener_release_master_ref (axlPointer ptr)
{
	/* release master reference */
	vortex_connection_unref ((VortexConnection *) ptr, "master release");
	return;
}

/** 
 * @internal
 *
 * Internal vortex library function. This function does the initial
 * accept for a new incoming connection. New connections are accepted
 * through two steps: an initial accept and final negotiation. A
 * connection to be totally accepted must step over two previous
 * steps.
 *
 * The reason to accept connections following this procedure is due to
 * Vortex Library way of reading data from all connections. 
 * 
 * Reading data from remote BEEP peers is done inside the vortex
 * reader which, basicly, is a loop executing a "select" call. 
 *
 * While reading data from those sockets accepted by the "select" call
 * is not a problem, however, it is actually a problem to accept new
 * connections because it implies reading data from remote peer: the
 * initial greeting, and writing data to remote peer: the initial
 * greeting response. 
 *
 * During the previous negotiation a malicious client can make
 * negotiation to be stopped, or sending data in an slow manner,
 * making the select loop to be blocked, even stopped. As a consequence
 * this malicious client have thrown down the reception for all
 * channels inside all connections.
 *
 * However, the vortex reader loop is prepared to avoid this problem
 * with already accepted connections because it doesn't pay attention
 * to those connection which are not sending any data and also support
 * to receive fragmented frames which are stored to be joined with
 * future fragment. Once connection is accepted the vortex reader is
 * strong enough to avoid DOS (denial of service) attacks (well, it
 * should be ;-).
 *
 * That way the mission for the first step: to only accept the new
 * connection and send the initial greeting to remote peer and *DO NOT
 * READING ANYTHING* to avoid DOS. On a second step, the response
 * reading is done and the connection is totally accepted in the
 * context of the vortex reader.
 *
 * A connection initial accepted is flagged to be on that step so
 * vortex reader can recognize it. 
 *
 * @param client_socket A new socket being accepted to be read.
 */
void __vortex_listener_initial_accept (VortexCtx        * ctx,
				       VORTEX_SOCKET      client_socket, 
				       VortexConnection * listener)
{
	VortexConnection     * connection = NULL;

	/* before doing anything, we have to create a connection */
	connection = vortex_connection_new_empty (ctx, client_socket, VortexRoleListener);
	vortex_log (VORTEX_LEVEL_DEBUG, "received connection from: %s:%s", 
		    vortex_connection_get_host (connection),
		    vortex_connection_get_port (connection));

	/* configure the relation between this connection and the
	 * master listener connection */
	if (vortex_connection_ref (listener, "master ref"))
		vortex_connection_set_data_full (connection, "_vo:li:master", listener, NULL, __vortex_listener_release_master_ref);

	/*
	 * Perform an initial accept, flagging the connection to be
	 * into the initial accept stage, and send the initial greetings.
	 */
	vortex_listener_accept_connection (connection, axl_true);

	return;
}

/** 
 * @internal
 *
 * This function is for internal vortex library purposes. This
 * function actually does the second accept step, that is, to read the
 * greeting response and finally accept the connection is that response
 * is ok.
 *
 * You can also read the doc for __vortex_listener_initial_accept to
 * get an idea about the initial step.
 *
 * Once the greeting response is ok, the function unflag this
 * connection to be "being accepted" so the connection starts to work.
 * 
 * @param frame The frame which should contains the greeting response
 * @param connection the connection being on initial accept step
 */
void __vortex_listener_second_step_accept (VortexFrame * frame, VortexConnection * connection)
{
	VortexFrame   * pending;
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx     * ctx = vortex_connection_get_ctx (connection);
#endif
	vortex_log (VORTEX_LEVEL_DEBUG, "called listener second step accept..");

	/* check if the connection have a pending frame (get the reference) */
	pending = vortex_connection_get_data (connection,
					      VORTEX_GREETINGS_PENDING_FRAME);
	/* check pending frame */
	if (pending) {
		pending = vortex_frame_join (pending, frame);
		vortex_frame_unref (frame);
		frame   = pending;
	} /* end if */

	/* check if the frame returned is not complete, to store in
	 * the connection and return NULL */
	if (vortex_frame_get_more_flag (frame)) {
		/* store the frame */
		vortex_connection_set_data_full (connection, 
						 /* key and data */
						 VORTEX_GREETINGS_PENDING_FRAME, frame,
						 NULL, (axlDestroyFunc) vortex_frame_unref);
		return;
	} /* end if */

	/* frame complete, clear connection content */
	vortex_connection_set_data (connection, 
				    /* key and data */
				    VORTEX_GREETINGS_PENDING_FRAME, NULL);


	/* call to update frame MIME status */
	if (! vortex_frame_mime_process (frame))
		vortex_log (VORTEX_LEVEL_WARNING, "failed to update MIME status for the frame, continue delivery");

	/* process greetings from init peer */
	if (!vortex_greetings_is_reply_ok (frame, connection, NULL)) {
		/* previous function already unref frame object
		 * received is something goes wrong */
		vortex_log (VORTEX_LEVEL_CRITICAL, "wrong greeting rpy from init peer, closing session");
		__vortex_connection_set_not_connected (connection, "wrong greeting rpy from init peer, closing session", VortexProtocolError);
		return;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "received initiator peer greetings...checking..");

	/* because the greeting is ok, parse it */
	if (!__vortex_connection_parse_greetings (connection, frame)) {
		/* previous function doesn't perform a frame
		 * deallocation, unref it*/
		vortex_frame_unref (frame);
		
		vortex_log (VORTEX_LEVEL_CRITICAL, "wrong greetings received, closing session");
		__vortex_connection_set_not_connected (connection, "wrong greetings received, closing session", VortexProtocolError);
		return;
	}

	/* if parse greetins ok, notify to process features and and
	   localize */
	if (! vortex_connection_actions_notify (&connection, CONNECTION_STAGE_PROCESS_GREETINGS_FEATURES)) {
		/* release_frame */
		vortex_frame_unref (frame);

		/* action reporting failure, unref the connection */
		vortex_log (VORTEX_LEVEL_CRITICAL, "vortex listener do to action failure = CONNECTION_STAGE_PROCESS_GREETINGS_FEATURES, connection closed id=%d",
			    vortex_connection_get_id (connection));
		__vortex_connection_set_not_connected (connection, "vortex listener do to action failure = CONNECTION_STAGE_PROCESS_GREETINGS_FEATURES", 
						       VortexConnectionFiltered);
		return;
	} /* end if */

	vortex_log (VORTEX_LEVEL_DEBUG, "greetings ok, sending listener greetings..");

	/* send greetings, get actual profile installation and report
	 * it to init peer */
	vortex_log (VORTEX_LEVEL_DEBUG, "sending greetings for a new connection..");
	if ((! vortex_greetings_send (connection, NULL))) {
		/* release frame */
		vortex_frame_unref (frame);

		vortex_log (VORTEX_LEVEL_CRITICAL, "vortex listener: failed to send initial listener greetings reply message");
		
		/*
		 * This unref sentence is properly defined. Opposite
		 * ref call done to this unref is actually done by
		 * vortex_connection_new_empty.
		 */ 
		__vortex_connection_set_not_connected (connection, "vortex listener: failed to send initial listener greetings reply message",
						       VortexProtocolError);
		return;
	} /* end if */

	/* frame accepted */
	vortex_log (VORTEX_LEVEL_DEBUG, "accepting connection on vortex_reader (second accept step)");
	
	/* free the last frame and watch connection on changes */
	vortex_frame_unref (frame);
	
	/*** CONNECTION COMPLETELY ACCEPTED ***/
	/* flag the connection to be totally accepted. */
	connection->initial_accept = axl_false;
	/* flag that the greetings message was already sent */
	vortex_connection_set_data (connection, "vo:greetings-sent", NULL);

	/* call to notify connection created */
	if (! vortex_connection_actions_notify (&connection, CONNECTION_STAGE_POST_CREATED)) {
		/* action reporting failure, unref the connection */
		vortex_log (VORTEX_LEVEL_CRITICAL, "vortex listener do to action failure = CONNECTION_STAGE_POST_CREATED, connection closed id=%d",
			    vortex_connection_get_id (connection));
		__vortex_connection_set_not_connected (connection, "vortex listener do to action failure = CONNECTION_STAGE_POST_CREATED", VortexConnectionFiltered);
		return;
	} /* end if */

	return;	
}

/** 
 * @brief Public function that performs a TCP listener accept.
 *
 * @param server_socket The listener socket where the accept() operation will be called.
 *
 * @return Returns a connected socket descriptor or -1 if it fails.
 */
VORTEX_SOCKET vortex_listener_accept (VORTEX_SOCKET server_socket)
{
	struct sockaddr_in inet_addr;
#if defined(AXL_OS_WIN32)
	int               addrlen;
#else
	socklen_t         addrlen;
#endif
	addrlen       = sizeof(struct sockaddr_in);

	/* accept the connection new connection */
	return accept (server_socket, (struct sockaddr *)&inet_addr, &addrlen);
}

void vortex_listener_accept_connections (VortexCtx        * ctx,
					 int                server_socket, 
					 VortexConnection * listener)
{
	int   soft_limit, hard_limit, client_socket;

	/* accept the connection new connection */
	client_socket = vortex_listener_accept (server_socket);
	if (client_socket == VORTEX_SOCKET_ERROR) {
		/* get values */
		vortex_conf_get (ctx, VORTEX_SOFT_SOCK_LIMIT, &soft_limit);
		vortex_conf_get (ctx, VORTEX_HARD_SOCK_LIMIT, &hard_limit);

		vortex_log (VORTEX_LEVEL_CRITICAL, "accept () failed, server_socket=%d, soft-limit=%d, hard-limit=%d: (errno=%d) %s\n",
			    server_socket, soft_limit, hard_limit, errno, vortex_errno_get_last_error ());
		return;
	}

	/* check we can support more sockets, if not close current
	 * connection: function already closes client socket in the
	 * case of failure */
	if (! vortex_connection_check_socket_limit (ctx, client_socket))
		return;

	/* instead of negotiate the connection at this point simply
	 * accept it to negotiate it inside vortex_reader loop.  */
	__vortex_listener_initial_accept (vortex_connection_get_ctx (listener), client_socket, listener);

	return;
}

typedef struct _VortexListenerData {
	char                     * host;
	int                        port;
	VortexListenerReady        on_ready;
	VortexListenerReadyFull    on_ready_full;
	axlPointer                 user_data;
	axl_bool                   threaded;
	axl_bool                   register_conn;
	VortexCtx                * ctx;
}VortexListenerData;

/** 
 * @brief Starts a generic TCP listener on the provided address and
 * port. This function is used internally by the vortex listener
 * module to startup the vortex listener TCP session associated,
 * however the function can be used directly to start TCP listeners.
 *
 * @param ctx The context where the listener is started.
 *
 * @param host Host address to allocate. It can be "127.0.0.1" to only
 * listen for localhost connections or "0.0.0.0" to listen on any
 * address that the server has installed. It cannot be NULL.
 *
 * @param port The port to listen on. It cannot be NULL and it must be
 * a non-zero string.
 *
 * @param error Optional axlError reference where a textual diagnostic
 * will be reported in case of error.
 *
 * @return The function returns the listener socket or -1 if it
 * fails. Optionally the axlError reports the textual especific error
 * found. If the function returns -2 then some parameter provided was
 * found to be NULL.
 */
VORTEX_SOCKET     vortex_listener_sock_listen      (VortexCtx   * ctx,
						    const char  * host,
						    const char  * port,
						    axlError   ** error)
{
	struct hostent     * he;
       struct in_addr     * haddr;
       struct sockaddr_in   saddr;
	struct sockaddr_in   sin;
	VORTEX_SOCKET        fd;
#if defined(AXL_OS_WIN32)
/*	BOOL                 unit      = axl_true; */
	int                  sin_size  = sizeof (sin);
#else    	
	int                  unit      = 1; 
	socklen_t            sin_size  = sizeof (sin);
#endif	
	uint16_t             int_port;
	int                  backlog   = 0;
	int                  bind_res;

	v_return_val_if_fail (ctx,  -2);
	v_return_val_if_fail (host, -2);
	v_return_val_if_fail (port || strlen (port) == 0, -2);

	/* resolve hostname */
	he = gethostbyname (host);
        if (he == NULL) {
		axl_error_report (error, VortexNameResolvFailure, "unable to get hostname by calling gethostbyname");
		return -1;
	} /* end if */

	haddr = ((struct in_addr *) (he->h_addr_list)[0]);
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) <= 2) {
		/* do not allow creating sockets reusing stdin (0),
		   stdout (1), stderr (2) */
		vortex_log (VORTEX_LEVEL_DEBUG, "failed to create listener socket: %d (errno=%d:%s)", fd, errno, vortex_errno_get_error (errno));
		axl_error_report (error, VortexSocketCreationError, 
				  "failed to create listener socket: %d (errno=%d:%s)", fd, errno, vortex_errno_get_error (errno));
		return -1;
        } /* end if */

#if defined(AXL_OS_WIN32)
	/* Do not issue a reuse addr which causes on windows to reuse
	 * the same address:port for the same process. Under linux,
	 * reusing the address means that consecutive process can
	 * reuse the address without being blocked by a wait
	 * state.  */
	/* setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char  *)&unit, sizeof(BOOL)); */
#else
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &unit, sizeof (unit));
#endif 

	/* get integer port */
	int_port  = (uint16_t) atoi (port);

	memset(&saddr, 0, sizeof(struct sockaddr_in));
	saddr.sin_family          = AF_INET;
	saddr.sin_port            = htons(int_port);
	memcpy(&saddr.sin_addr, haddr, sizeof(struct in_addr));

	/* call to bind */
	bind_res = bind(fd, (struct sockaddr *)&saddr,  sizeof (struct sockaddr_in));
	vortex_log (VORTEX_LEVEL_DEBUG, "bind(2) call returned: %d", bind_res);
	if (bind_res == VORTEX_SOCKET_ERROR) {
		vortex_log (VORTEX_LEVEL_DEBUG, "unable to bind address (port:%u already in use or insufficient permissions). Closing socket: %d", int_port, fd);
		axl_error_report (error, VortexBindError, "unable to bind address (port:%u already in use or insufficient permissions). Closing socket: %d", int_port, fd);
		vortex_close_socket (fd);
		return -1;
	}
	
	/* get current backlog configuration */
	vortex_conf_get (ctx, VORTEX_LISTENER_BACKLOG, &backlog);
	
	if (listen(fd, backlog) == VORTEX_SOCKET_ERROR) {
		axl_error_report (error, VortexSocketCreationError, "an error have occur while executing listen");
		return -1;
        } /* end if */

	/* notify listener */
	if (getsockname (fd, (struct sockaddr *) &sin, &sin_size) < -1) {
		axl_error_report (error, VortexNameResolvFailure, "an error have happen while executing getsockname");
		return -1;
	} /* end if */

	/* report and return fd */
	vortex_log  (VORTEX_LEVEL_DEBUG, "running listener at %s:%d (socket: %d)", inet_ntoa(sin.sin_addr), ntohs (sin.sin_port), fd);
	return fd;
}

axlPointer __vortex_listener_new (VortexListenerData * data)
{
	char               * host          = data->host;
	axl_bool             threaded      = data->threaded;
	axl_bool             register_conn = data->register_conn;
	char               * str_port      = axl_strdup_printf ("%d", data->port);
	axlPointer           user_data     = data->user_data;
	const char         * message       = NULL;
	VortexConnection   * listener      = NULL;
	VortexCtx          * ctx           = data->ctx;
	VortexStatus         status        = VortexOk;
	char               * host_used;
	axlError           * error         = NULL;
	VORTEX_SOCKET        fd;
	struct sockaddr_in   sin;

	/* handlers received (may be both null) */
	VortexListenerReady      on_ready       = data->on_ready;
	VortexListenerReadyFull  on_ready_full  = data->on_ready_full;
	
	/* free data */
	axl_free (data);

	/* allocate listener */
	fd = vortex_listener_sock_listen (ctx, host, str_port, &error);
	
	/* unref the host and port value */
	axl_free (str_port);
	axl_free (host);

	/* listener ok */
	/* seems listener to be created, now create the BEEP
	 * connection around it */
	listener = vortex_connection_new_empty (ctx, fd, VortexRoleMasterListener);

	vortex_log (VORTEX_LEVEL_DEBUG, "listener reference created (%p, id: %d, socket: %d)", listener, 
		    vortex_connection_get_id (listener), fd);

	/* handle returned socket or error */
	switch (fd) {
	case -2:
		vortex_log (VORTEX_LEVEL_CRITICAL, "Failed to start listener because vortex_listener_sock_listener reported NULL parameter received");
		status  = VortexWrongReference;
		message = "Failed to start listener because vortex_listener_sock_listener reported NULL parameter received";

		/* set status */
		__vortex_connection_set_not_connected (listener, message, status);
		break;
	case -1:
		vortex_log (VORTEX_LEVEL_CRITICAL, "Failed to start listener, vortex_listener_sock_listener reported (code: %d): %s",
			    axl_error_get_code (error), axl_error_get (error));
		status  = axl_error_get_code (error);
		message = axl_error_get (error);

		/* set status */
		__vortex_connection_set_not_connected (listener, message, status);
		break;
	default:
		/* register the listener socket at the Vortex Reader process.  */
		if (register_conn)
			vortex_reader_watch_listener (ctx, listener);
		if (threaded) {
			vortex_log (VORTEX_LEVEL_DEBUG, "doing listener notification (threaded mode)");
			/* notify listener created */
			host_used = vortex_support_inet_ntoa (ctx, &sin);
			if (on_ready != NULL) {
				on_ready (host_used, ntohs (sin.sin_port), VortexOk, "server ready for requests", user_data);
			} /* end if */
			
			if (on_ready_full != NULL) {
				on_ready_full (host_used, ntohs (sin.sin_port), VortexOk, "server ready for requests", listener, user_data);
			} /* end if */
			axl_free (host_used);
		} /* end if */

		/* call to notify connection created */
		if (! vortex_connection_actions_notify (&listener, CONNECTION_STAGE_POST_CREATED)) {
			/* action reporting failure, unref the connection */
			__vortex_connection_set_not_connected (listener, "vortex master listener post created action failed", VortexConnectionFiltered);
		} /* end if */

		/* the listener reference */
		vortex_log (VORTEX_LEVEL_DEBUG, "returning listener running at %s:%s (non-threaded mode)", 
			    vortex_connection_get_host (listener), vortex_connection_get_port (listener));
		return listener;
	} /* end switch */

	/* according to the invocation */
	if (threaded) {
		/* notify error found to handlers */
		if (on_ready != NULL) 
			on_ready      (NULL, 0, status, (char*) message, user_data);
		if (on_ready_full != NULL) 
			on_ready_full (NULL, 0, status, (char*) message, NULL, user_data);
	} else {
		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to start vortex server, error was: %s, unblocking vortex_listener_wait",
		       message);
		/* notify the listener that an error was found
		 * (because the server didn't suply a handler) */
		vortex_mutex_lock (&ctx->listener_unlock);
		QUEUE_PUSH (ctx->listener_wait_lock, INT_TO_PTR (axl_true));
		ctx->listener_wait_lock = NULL;
		vortex_mutex_unlock (&ctx->listener_unlock);
	} /* end if */

	/* unref error */
	axl_error_free (error);

	/* return listener created */
	return listener;
}

/** 
 * @internal Implementation to support listener creation functions vortex_listener_new*
 */
VortexConnection * __vortex_listener_new_common  (VortexCtx               * ctx,
						  const char              * host,
						  int                       port,
						  axl_bool                  register_conn,
						  VortexListenerReady       on_ready, 
						  VortexListenerReadyFull   on_ready_full,
						  axlPointer                user_data)
{
	VortexListenerData * data;

	/* check context is initialized */
	if (! vortex_init_check (ctx))
		return NULL;
	
	/* init listener module */
	vortex_listener_init (ctx);
	
	/* prepare function data */
	data                = axl_new (VortexListenerData, 1);
	data->host          = axl_strdup (host);
	data->port          = port;
	data->on_ready      = on_ready;
	data->on_ready_full = on_ready_full;
	data->user_data     = user_data;
	data->ctx           = ctx;
	data->register_conn = register_conn;
	data->threaded      = (on_ready != NULL) || (on_ready_full != NULL);
	
	/* make request */
	if (data->threaded) {
		vortex_log (VORTEX_LEVEL_DEBUG, "invoking listener_new threaded mode");
		vortex_thread_pool_new_task (ctx, (VortexThreadFunc) __vortex_listener_new, data);
		return NULL;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "invoking listener_new non-threaded mode");
	return __vortex_listener_new (data);	
}


/** 
 * @brief Creates a new Vortex Listener accepting incoming connections
 * on the given <b>host:port</b> configuration.
 *
 * If user provides an \ref VortexListenerReady "on_ready" callback,
 * the listener will be notified on it, in a separated thread, once
 * the process has finished. Check \ref VortexListenerReady handler
 * documentation which is on_ready handler type.
 * 
 * On that notification will also be passed the host and port actually
 * allocated. Think about using as host 0.0.0.0 and port 0. These
 * values will cause to \ref vortex_listener_new to allocate the
 * system configured hostname and a random free port. See \ref
 * vortex_handlers "this section" for more info about on_ready
 * parameter.
 *
 * Host and port value provided to this function could be unrefered
 * once returning from this function. The function performs a local
 * copy for those values, that are deallocated at the appropriate
 * moment.
 *
 * Keep in mind that you can actually call several times to this
 * function before calling to \ref vortex_listener_wait, to make your
 * process to be able to accept connections from several ports and
 * host names at the same time. 
 *
 * While providing the port information, make sure your process will
 * have enough rights to allocate the port provided. Usually, ports
 * from 1 to 1024 are reserved to listener programms that runs with
 * priviledges.
 *
 * There is an alternative API that perform the same function but
 * receive the TCP port value as integer: \ref vortex_listener_new2
 *
 * In the case the optional handler <b>on_ready</b> is not provided,
 * the function will return a reference to the \ref VortexConnection
 * representing the listener created. This reference will have the
 * role \ref VortexRoleMasterListener, (using \ref
 * vortex_connection_get_role) to indicate that the connection
 * reference is a listener.
 *
 * In the case the <b>on_ready</b> handler is provided, the function
 * will return NULL.
 *
 * Here is an example to start a vortex listener server:
 *
 * \code
 * // On this example you'll find:
 * //   - 3 handlers: frame_received, start_channel and
 * //                 close_channel (which handle the event
 * //                 they represent).
 * //   - An entry point (main function) which creates a
 * //     a simple listener server, using previous three
 * //     basic handlers.
 *
 * // vortex context
 * VortexCtx * ctx = NULL;
 *
 * // a frame handler
 * void frame_received (VortexChannel    * channel,
 *                      VortexConnection * connection,
 *                      VortexFrame      * frame,
 *                      axlPointer         user_data)
 * {
 *        // Received a frame from the remote side.
 *        // process it and reply to it if it is a MSG.
 *        return;
 * }
 *
 * axl_bool  start_channel (int                channel_num, 
 *                          VortexConnection * connection, 
 *                          axlPointer         user_data)
 * {
 *        printf ("Received an start message=%d!!\n",
 *                 channel_num);
 *        // if the async notifier returns axl_true, the channel
 *        // is implicitly created, if axl_false is returned the
 *        // channel creation is denied and a reply error
 *        // is sent.
 *        return axl_true;
 * }
 *
 * axl_bool      close_channel (int                channel_num, 
 *                              VortexConnection * connection, 
 *                              axlPointer         user_data)
 * {
 *        printf ("Got a close message notification!!\n");
 *
 *        // if axl_true is returned, the channel is
 *        // accepted to be closed. Otherwise the channel
 *        // will not be closed and an error reply will be
 *        // sent to the remote peer.
 *        return axl_true;
 * }
 * int main (int argc, char ** argv) {
 *
 *      VortexConnection * listener;
 *
 *      // create a context
 *      ctx = vortex_ctx_new ();
 *
 *      // enable log to see whats going on 
 *      vortex_log_enable (ctx, axl_true);
 *
 *      // init vortex library (and check its result!)
 *      if (! vortex_init_ctx (ctx)) {
 *           printf ("Unable to initialize vortex library\n");
 *           exit (-1);
 *      }
 * 
 *      // register a profile
 *      vortex_profiles_register (ctx, "http://vortex.aspl.es/profiles/example", 
 *                                start_channel, NULL,
 *                                close_channel, NULL,
 *                                frame_received, NULL);
 * 
 *      // now create a vortex server
 *      listener = vortex_listener_new (ctx, "0.0.0.0", "3000", NULL, NULL);
 *      if (! vortex_connection_is_ok (listener, axl_false)) {
 *             printf ("ERROR: failed to start listener, error found (code: %d) %s\n", 
 *                     vortex_connection_get_status (listener),
 *                     vortex_connection_get_message (listener));
 *             reutrn -1;
 *      } 
 *      
 *      // wait for listener to finish (maybe due to vortex_exit call)
 *      vortex_listener_wait (ctx);
 *  
 *      // end vortex internal subsystem (if no one have done it yet!)
 *      vortex_ctx_exit (ctx, axl_true);
 * 
 *      // that's all to start BEEPing!
 *      return 0;     
 * }  
 * \endcode
 *
 * @param ctx The context where the operation will be performed.
 *
 * @param host The host to listen on.
 *
 * @param port The port to listen on.
 *
 * @param on_ready A optional callback to get a notification when
 * vortex listener is ready to accept requests.
 *
 * @param user_data A user defined pointer to be passed in to
 * <i>on_ready</i> handler.
 *
 * @return The listener connection created (represented by a \ref
 * VortexConnection reference). You must use \ref
 * vortex_connection_is_ok to check if the server was started.
 * 
 * <b>NOTE:</b> the reference returned is only owned by the vortex
 * engine. This is not the case of \ref vortex_connection_new where
 * the caller acquires automatically a reference to the connection (as
 * well as the vortex engine). 
 * 
 * In this case, if your intention is to keep a reference for later
 * operations, you must call to \ref vortex_connection_ref to avoid
 * losing the reference if the system drops the connection. In the
 * same direction, you can't call to \ref vortex_connection_close if
 * you don't own the reference returned by this function.
 * 
 * To close immediately a listener you can use \ref
 * vortex_connection_shutdown. In the case the listener was not
 * started (because \ref vortex_connection_is_ok returned axl_false),
 * you must use \ref vortex_connection_close to terminate the
 * reference (NOT vortex_connection_shutdown).
 *
 * <b>Note about old connecting clients, previous to 1.1.3</b>
 *
 * Until Vortex Library 1.1.3, listener accepting incoming connections
 * were sending the BEEP greetings reply just after the remote peer
 * connects. However, this was changed to allow listener side
 * developers to react, modify or deny greetings at such phase (\ref CONNECTION_STAGE_PROCESS_GREETINGS_FEATURES),
 * hooking into that event, waiting first for the client BEEP greetings
 * (especially to check which features it is providing).
 *
 * This behaviour causes problems with clients from previous releases
 * which are waiting for the listener to issue its BEEP peer greetings
 * (where the listener is also waiting) to issue theirs. That is, old
 * client never connects because he never receives greetings from
 * BEEP listener, and BEEP listener never sends its greetings because
 * the client never sent its greetings (circular problem).
 *
 * To solve this issue you can either update the connecting client
 * software or configure your listener to don't wait for client
 * greetings to send its greetings:
 *
 * - \ref vortex_listener_send_greetings_on_connect
 *
 * This problem do not affect to new clients connecting to old servers.
 */
VortexConnection * vortex_listener_new (VortexCtx           * ctx,
					const char          * host, 
					const char          * port, 
					VortexListenerReady   on_ready, 
					axlPointer            user_data)
{
	/* call to int port API */
	return __vortex_listener_new_common (ctx, host, __vortex_listener_get_port (port), axl_true, on_ready, NULL, user_data);
}

/** 
 * @brief Creates a new listener, allowing to get the connection that
 * represents the listener created with the optional handler (\ref
 * VortexListenerReadyFull).
 *
 * This function provides the same functionality than \ref
 * vortex_listener_new and \ref vortex_listener_new2 but allowing to
 * get the connection (\ref VortexConnection) representing the
 * listener, by configuring the optional handler on_ready_full (\ref
 * VortexListenerReadyFull).
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param host The host to listen on.
 *
 * @param port The port to listen on.
 *
 * @param on_ready_full A optional callback to get a notification when
 * vortex listener is ready to accept requests.
 *
 * @param user_data A user defined pointer to be passed in to
 * <i>on_ready</i> handler.
 *
 * @return The listener connection created, or NULL if the optional
 * handler is provided (on_ready).
 *
 * <b>NOTE:</b> the reference returned is only owned by the vortex
 * engine. This is not the case of \ref vortex_connection_new where
 * the caller acquires automatically a reference to the connection (as
 * well as the vortex engine). 
 * 
 * In this case, if your intention is to own a reference to the
 * listener for later operations, you must call to \ref
 * vortex_connection_ref to avoid losing the reference if the system
 * drops the connection. In the same direction, you can't call to \ref
 * vortex_connection_close if you don't own the reference returned by
 * this function.
 * 
 * To close immediately a listener you can use \ref
 * vortex_connection_shutdown.
 */
VortexConnection * vortex_listener_new_full  (VortexCtx   * ctx,
					      const char  * host,
					      const char  * port,
					      VortexListenerReadyFull on_ready_full, 
					      axlPointer user_data)
{
	/* call to int port API */
	return __vortex_listener_new_common (ctx, host, __vortex_listener_get_port (port), axl_true, NULL, on_ready_full, user_data);
}

/** 
 * @brief Allows to create a BEEP listener optionally not registering
 * it on vortex reader. See \ref vortex_listener_new_full for more
 * details.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param host The host to listen on.
 *
 * @param port The port to listen on.
 *
 * @param register_conn axl_true makes the function to work like \ref
 * vortex_listener_new_full. Otherwise, axl_false makes the listener
 * created to be not registered on vortex reader process.
 *
 * @param on_ready_full A optional callback to get a notification when
 * vortex listener is ready to accept requests.
 *
 * @param user_data A user defined pointer to be passed in to
 * <i>on_ready</i> handler.
 *
 * @return The listener connection created, or NULL if the optional
 * handler is provided (on_ready).
 *
 * <b>IMPORTANT NOTE:</b>
 *
 * All vortex_listener_new* functions have a common behavior which is
 * reference returned is owned by the vortex engine. In this case, if
 * the caller passes register_conn = axl_false makes the reference
 * returned or notified to be owned by the caller.
 */
VortexConnection * vortex_listener_new_full2       (VortexCtx                * ctx,
						    const char               * host,
						    const char               * port,
						    axl_bool                   register_conn,
						    VortexListenerReadyFull    on_ready_full, 
						    axlPointer                 user_data)
{
	/* call to int port API */
	return __vortex_listener_new_common (ctx, host, __vortex_listener_get_port (port), register_conn, NULL, on_ready_full, user_data);
}

/** 
 * @brief Creates a new Vortex Listener accepting incoming connections
 * on the given <b>host:port</b> configuration, receiving the port
 * configuration as an integer value.
 *
 * See \ref vortex_listener_new for more information. 
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param host The host to listen to.
 *
 * @param port The port to listen to. Value defined for the port must be between 0 up to 65536.
 *
 * @param on_ready A optional notify callback to get when vortex
 * listener is ready to perform replies.
 *
 * @param user_data A user defined pointer to be passed in to
 * <i>on_ready</i> handler.
 *
 * @return The listener connection created, or NULL if the optional
 * handler is provided (on_ready).
 *
 * <b>NOTE:</b> the reference returned is only owned by the vortex
 * engine. This is not the case of \ref vortex_connection_new where
 * the caller acquires automatically a reference to the connection (as
 * well as the vortex engine).
 * 
 * In this case, if your intention is to keep a reference for later
 * operations, you must call to \ref vortex_connection_ref to avoid
 * losing the reference if the system drops the connection. In the
 * same direction, you can't call to \ref vortex_connection_close if
 * you don't own the reference returned by this function.
 * 
 * To close immediately a listener you can use \ref vortex_connection_shutdown.
 */
VortexConnection * vortex_listener_new2    (VortexCtx   * ctx,
					    const char  * host,
					    int           port,
					    VortexListenerReady on_ready, 
					    axlPointer user_data)
{

	/* call to common API */
	return __vortex_listener_new_common (ctx, host, port, axl_true, on_ready, NULL, user_data);
}



/** 
 * @brief Blocks a listener (or listeners) launched until vortex finish.
 * 
 * This function should be called after creating a listener (o
 * listeners) calling to \ref vortex_listener_new to block current
 * thread.
 * 
 * This function can be avoided if the program structure can ensure
 * that the programm will not exist after calling \ref
 * vortex_listener_new. This happens when the program is linked to (or
 * implements) and internal event loop.
 *
 * This function will be unblocked when the vortex listener created
 * ends or a failure have occur while creating the listener. To force
 * an unlocking, a call to \ref vortex_listener_unlock must be done.
 * 
 * @param ctx The context where the operation will be performed.
 */
void vortex_listener_wait (VortexCtx * ctx)
{
	VortexAsyncQueue * temp;

	/* check reference received */
	if (ctx == NULL)
		return;

	/* check and init listener_wait_lock if it wasn't: init
	   lock */
	vortex_mutex_lock (&ctx->listener_mutex);
	
	/* create listener locker */
	if (ctx->listener_wait_lock == NULL) 
		ctx->listener_wait_lock = vortex_async_queue_new ();

	/* unlock */
	vortex_mutex_unlock (&ctx->listener_mutex);

	/* double locking to ensure waiting */
	vortex_log (VORTEX_LEVEL_DEBUG, "Locking listener");
	if (ctx->listener_wait_lock != NULL) {
		/* get a local reference to the queue and work with it */
		temp = ctx->listener_wait_lock;

		/* get blocked until the waiting lock is released */
		vortex_async_queue_pop   (temp);
		
		/* unref the queue */
		vortex_async_queue_unref (temp);
	}
	vortex_log (VORTEX_LEVEL_DEBUG, "(un)Locked listener");

	return;
}

/** 
 * @brief Signals to unblock the listener blocked at the \ref vortex_listener_wait.
 * 
 * Under normal circumstances, this function must not be called
 * directly from user space. This function actually unblock waiting
 * listeners on vortex_listener_wait function.
 *
 * Vortex already call to this function once the \ref vortex_exit_ctx
 * function is called.
 *
 * Because some designs could require to control how memory is
 * released once the listener is unblocked from the \ref
 * vortex_listener_wait, this function could help to control an
 * trigger that unblock operation.
 *
 * Futher calls to this function will cause no operation. This
 * function is thread safe.
 *
 * @param ctx The context where the operation will be performed.
 **/
void vortex_listener_unlock (VortexCtx * ctx)
{
	/* check reference received */
	if (ctx == NULL || ctx->listener_wait_lock == NULL)
		return;

	/* unlock listener */
	vortex_mutex_lock (&ctx->listener_unlock);
	if (ctx->listener_wait_lock != NULL) {

		/* push to signal listener unblocking */
		vortex_log (VORTEX_LEVEL_DEBUG, "(un)Locking listener..");

		/* notify waiters */
		if (vortex_async_queue_waiters (ctx->listener_wait_lock) > 0) {
			QUEUE_PUSH (ctx->listener_wait_lock, INT_TO_PTR (axl_true));
		} else {
			/* unref */
			vortex_async_queue_unref (ctx->listener_wait_lock);
		} /* end if */

		/* nullify */
		ctx->listener_wait_lock = NULL;

		vortex_mutex_unlock (&ctx->listener_unlock);
		return;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "(un)Locking listener: already unlocked..");
	vortex_mutex_unlock (&ctx->listener_unlock);
	return;
}

/** 
 * @internal
 * 
 * Internal vortex function. This function allows
 * __vortex_listener_new_common to initialize vortex listener module
 * only if a listener is installed.
 **/
void vortex_listener_init (VortexCtx * ctx)
{
	/* do not lock if the data is already initialized */
	if (ctx != NULL && ctx->listener_wait_lock != NULL &&
	    ctx->listener_on_accept_handlers != NULL)
		return;

	/* init lock */
	vortex_mutex_lock (&ctx->listener_mutex);
	
	/* init the server on accept connection list */
	if (ctx->listener_on_accept_handlers == NULL)
		ctx->listener_on_accept_handlers = axl_list_new (axl_list_always_return_1, axl_free);

	/* unlock */
	vortex_mutex_unlock (&ctx->listener_mutex);
	return;
}

/** 
 * @brief Allows to configure a master listener connection (created
 * via \ref vortex_listener_new and similar) to not wait for client
 * BEEP peer greetings to issue listener BEEP greetings.
 *
 * See also \ref vortex_listener_new for more information. By default
 * BEEP listener waits for connecting client to send its BEEP
 * greetings and, once received and processed (usually using \ref
 * CONNECTION_STAGE_PROCESS_GREETINGS_FEATURES), the listener sends back its BEEP
 * greetings.
 *
 * This function allows to control such behavior. 
 *
 * @param listener The listener connection to configure.
 *
 * @param send_on_connect By default axl_false. axl_true to issue the
 * BEEP greetings on client connect without waiting for client BEEP
 * greetings.
 *
 */
void          vortex_listener_send_greetings_on_connect (VortexConnection * listener, 
							 axl_bool           send_on_connect)
{
	VortexCtx * ctx;

	if (listener == NULL)
		return;

	ctx = CONN_CTX (listener);
	vortex_log (VORTEX_LEVEL_DEBUG, "Setting BEEP listener greetings reply: %d", send_on_connect);
	vortex_connection_set_data (listener, "vo:li:send-greetings", INT_TO_PTR (send_on_connect));
	return;
}

/** 
 * @internal Allows to cleanup the vortex listener state.
 * 
 * @param ctx The vortex context to cleanup.
 */
void vortex_listener_cleanup (VortexCtx * ctx)
{
	VortexAsyncQueue * queue;
	v_return_if_fail (ctx);

	axl_list_free (ctx->listener_on_accept_handlers);
	ctx->listener_on_accept_handlers = NULL;
	
	axl_free (ctx->listener_default_realm);
	ctx->listener_default_realm = NULL;

	/* acquire queue and nullify */
	queue = ctx->listener_wait_lock;
	ctx->listener_wait_lock = NULL;
	vortex_log (VORTEX_LEVEL_DEBUG, "listener wait queue ref: %p", queue);
	if (queue) {
		/* remove pending items from the queue */
		while (vortex_async_queue_items (queue) > 0)
			vortex_async_queue_pop (queue);
		/* unref the queue */
		vortex_async_queue_unref (queue);
	}


	return;
}

/** 
 * @brief Allows to configure a handler that is executed once a
 * connection have been accepted.
 *
 * The handler to be configured could be used as a way to get
 * notifications about connections created, but also as a filter for
 * connections that must be dropped.
 *
 * @param ctx The context where the operation will be performed. 
 *
 * @param on_accepted The handler to be executed.
 *
 * @param _data User space data to be passed in to the handler
 * executed.
 *
 * Note this handler is called before any socket exchange to allow
 * denying as soon as possible. Though the handler receives a
 * reference to the \ref VortexConnection to be accepted/denied, it is only
 * provided to allow storing or reconfiguring the connection. 
 * 
 * In other words, when the handler is called, the BEEP session is
 * still not established. If you need to execute custom operations
 * once the connection is fully registered with the BEEP session
 * established, see \ref vortex_connection_set_connection_actions with
 * \ref CONNECTION_STAGE_POST_CREATED.
 *
 */
void          vortex_listener_set_on_connection_accepted (VortexCtx                  * ctx,
							  VortexOnAcceptedConnection   on_accepted, 
							  axlPointer                   _data)
{
	VortexListenerOnAcceptData * data;

	/* check reference received */
	if (ctx == NULL || on_accepted == NULL)
		return;

	/* init lock */
	vortex_mutex_lock (&ctx->listener_mutex);

	/* store handler configuration */
	data                 = axl_new (VortexListenerOnAcceptData, 1);
	if (data == NULL) {
		/* memory allocation failed. */
		vortex_mutex_unlock (&ctx->listener_mutex);
		return;
	}
	data->on_accept      = on_accepted;
	data->on_accept_data = _data;

	/* init the list if it wasn't */
	if (ctx->listener_on_accept_handlers == NULL) {
		/* alloc list */
		ctx->listener_on_accept_handlers = axl_list_new (axl_list_always_return_1, axl_free);

		/* check allocated list */
		if (ctx->listener_on_accept_handlers == NULL) {
			/* memory allocation failed. */
			vortex_mutex_unlock (&ctx->listener_mutex);
			return;
		} /* end if */
	}

	/* add the item */
	axl_list_add (ctx->listener_on_accept_handlers, data);

	vortex_log (VORTEX_LEVEL_DEBUG, "received new handler: %p with data %p, list: %d", 
		    on_accepted, data, axl_list_length (ctx->listener_on_accept_handlers));

	/* unlock */
	vortex_mutex_unlock (&ctx->listener_mutex);

	/* nothing more */
	return;
}


/** 
 * @brief Support function for Vortex Library listeners, that reads a
 * xml file that contains listener information and starts the
 * listener.
 *
 * The file the function will try to find is <b>conf.xml</b>. It will
 * lookup for all paths configured through: 
 * 
 *   - \ref vortex_support_find_data_file
 * 
 * Here is an example for a configuration file:
 * \code
 * <vortex-listener>
 *   <listener>
 *     <hostname>0.0.0.0</hostname>
 *     <port>44000</port>
 *   </listener>
 * </vortex-listener>
 * \endcode
 *
 * @param ctx The context where the operation will be performed.
 *
 * @return axl_true if the listener was started because the file was read
 * successfully otherwise axl_false is returned.
 */
axl_bool            vortex_listener_parse_conf_and_start (VortexCtx * ctx)
{
	/* listener xml configuration */
	axlDoc   * doc;
	axlDtd   * dtd;
	axlError * error;

	axlNode  * listener;
	axlNode  * aux;

	/* host and port references */
	char     * host;
	char     * port;

	/* a full path reference to the file */
	char     * full_path_file;

	/* check reference received */
	if (ctx == NULL)
		return axl_false;

	/* load the document */
	full_path_file = vortex_support_find_data_file (ctx, "conf.xml");
	doc            = axl_doc_parse_from_file (full_path_file, &error);
	axl_free (full_path_file);

	if (doc == NULL) {
		
		/* drop a log message */
		fprintf (stderr, "Unable to open conf.xml file, for host and port configuration. Error reported: %s\n",
			    axl_error_get (error));

		/* release the error reported */
		axl_error_free (error);
		
		return axl_false;
	}
	

	/* load the xml listener conf DTD */
	dtd            = axl_dtd_parse (VORTEX_LISTENER_CONF_DTD, -1, &error);
	if (dtd == NULL) {
		/* drop a log message */
		fprintf (stderr, "Unable to open conf.xml file, for host and port configuration. Error reported: %s\n",
			 axl_error_get (error));
		
		/* release the error reported */
		axl_error_free (error);

		/* release the document read */
		axl_doc_free (doc);
		
		return -1;
	}

	/* validate the xml listener document config */
	if (!axl_dtd_validate (doc, dtd, &error)) {
		fprintf (stderr, "Unable to validate listener configuration. Check your settings. Error reported: %s\n",
			 axl_error_get (error));

		/* release the error reported */
		axl_error_free (error);

		/* release the document read */
		axl_doc_free (doc);

		/* release the DTD read */
		axl_dtd_free (dtd);
		
		return -1;
	}

	/* get a reference to the fist <listener> configuration */
	listener = axl_doc_get (doc, "/vortex-listener/listener");

	do {
		/* get the <host> content */
		aux  = axl_node_get_child_nth (listener, 0);
		host = axl_node_get_content_trim (aux, NULL);

		/* get the <port> content */
		aux  = axl_node_get_child_nth (listener, 1);
		port = axl_node_get_content_trim (aux, NULL);

		/* starts a listener for the given configuration */
		vortex_listener_new (ctx, host, port, NULL, NULL);

		/* now go for the next listener configuration */
	}while ((listener = axl_node_get_next (listener)) != NULL);
	
	/* listener configuration already read, release document and DTD */
	axl_doc_free (doc);

	axl_dtd_free (dtd);

	return axl_true;	
}



/** 
 * @brief Allows to configure the default realm to be used at the
 * listener side, for all connections.
 *
 * Realm configuration is currently used by the SASL DIGEST-MD5
 * authentication. It is optional, but if defined, it must match the
 * realm configuration defined by the SASL login realm value provided
 * by the client peer.
 *
 * A value to be configured for the realm could be: <b>aspl.es</b>
 *
 * The value provided will to be copied, so it must be an static
 * string (or at least an string that is deallocated once the vortex
 * library stops its functions). 
 *
 * To get the value configured you must use \ref vortex_listener_get_default_realm.
 * 
 * @param realm The realm to be configured as a default value.
 *
 * @param ctx The context where the operation will be performed.
 */
void          vortex_listener_set_default_realm (VortexCtx   * ctx,
						 const char  * realm)
{
	/* check values received */
	if (ctx == NULL || realm == NULL)
		return;

	/* configure default realm */
	ctx->listener_default_realm = axl_strdup (realm);

	return;
}


/** 
 * @brief Allows to get current realm configuration, used for all
 * connections.
 * 
 * @param ctx The context where the operation will be performed.
 *
 * @return Current configuration. Result can be NULL, which means to
 * realm was configured.
 */
const char  * vortex_listener_get_default_realm (VortexCtx * ctx)
{
	/* check reference received */
	if (ctx == NULL)
		return NULL;

	/* return current realm */
	return ctx->listener_default_realm;
}

/** 
 * @internal Function used to shutdown connections associated to a
 * listener.
 */
void __vortex_listener_shutdown_foreach (VortexConnection * conn,
					 axlPointer         user_data)
{
	/* get the listener id associated to the connection */
	int         listener_id;
	VortexCtx * ctx;

	/* check the role (if it is a listener, skip) */
	if (vortex_connection_get_role (conn) == VortexRoleMasterListener)
		return;

	/* get the listener ctx */
	ctx = vortex_connection_get_ctx (conn);

	/* get the listener id associated to the connection */
	listener_id = vortex_connection_get_id (vortex_connection_get_listener (conn));

	/* check connection */
	vortex_log (VORTEX_LEVEL_DEBUG, "checking connection to shutdown: %d == %d", 
		    listener_id, PTR_TO_INT (user_data));
	if (listener_id == PTR_TO_INT (user_data)) { 
		vortex_log (VORTEX_LEVEL_DEBUG, "shutdown connection: %d..",
			    vortex_connection_get_id (conn));
		vortex_connection_shutdown (conn);
	}
	return;
}

/** 
 * @brief Allows to shutdown the listener provided and all connections
 * that were created due to its function.
 *
 * The function perform a shutdown (no BEEP close session phase) on
 * the listener and connections created.
 * 
 * @param listener The listener to shutdown.
 *
 * @param also_created_conns axl_true to shutdown all connections 
 */
void          vortex_listener_shutdown (VortexConnection * listener,
					axl_bool           also_created_conns)
{
	VortexCtx        * ctx;
	VortexAsyncQueue * notify = NULL;

	/* check parameters */
	if (! vortex_connection_is_ok (listener, axl_false))
		return;
	
	/* get ctx */
	ctx = vortex_connection_get_ctx (listener);

	vortex_log (VORTEX_LEVEL_DEBUG, "shutting down listener..");
	
	/* ref the listener during the operation */
	vortex_connection_ref (listener, "listener-shutdown");

	/* now do a foreach over all connections registered in the
	 * reader */
	if (also_created_conns) {
		/* call to shutdown all associated connections */
		notify = vortex_reader_foreach (ctx, 
						__vortex_listener_shutdown_foreach, 
						INT_TO_PTR (vortex_connection_get_id (listener)));
		/* wait to finish */
		vortex_async_queue_pop (notify);
		vortex_async_queue_unref (notify);
	} /* end if */

	/* shutdown the listener */
	vortex_connection_shutdown (listener);

	/* unref the listener now finished */
	vortex_connection_unref (listener, "listener-shutdown");

	return;
}

/* @} */
