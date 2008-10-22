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
#include <vortex.h>

/* local include */
#include <vortex_ctx_private.h>

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
void vortex_listener_accept_connection    (VortexConnection * connection, int  send_greetings)
{
	VortexCtx                  * ctx;
	int                          result;
	int                          iterator;
	VortexListenerOnAcceptData * data;

	/* check received reference */
	if (connection == NULL)
		return;

	/* Get a reference to the connection, accept the new
	 * connection under the same domain as the context of the
	 * listener  */
	ctx = vortex_connection_get_ctx (connection);
	
	/* call to the handler defined */
	iterator = 0;
	result   = true;

	/* init lock */
	vortex_mutex_lock (&ctx->listener_mutex);
	while (iterator < axl_list_length (ctx->listener_on_accept_handlers)) {
		/* call and check */
		data = axl_list_get_nth (ctx->listener_on_accept_handlers, iterator);

		/* check if the following handler accept the incoming
		 * connection */
		vortex_log (VORTEX_LEVEL_DEBUG, "calling to accept connection, handler: %p, data: %p",
			    data->on_accept, data->on_accept_data);
		if (! data->on_accept (connection, data->on_accept_data)) {

			vortex_log (VORTEX_LEVEL_DEBUG, "on accept handler have denied to accept the connection, handler: %p, data: %p",
				    data->on_accept, data->on_accept_data);

			/* found that at least one handler do not
			 * accept the incoming connection, dropping */
			result = false;
			break;
		}
		
		/* next iterator */
		iterator++;

	} /* end while */

	/* unlock */
	vortex_mutex_unlock (&ctx->listener_mutex);

	/* check result */
	if (result == false) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "the server application level have dropped the provided connection");
		/* send the error reply message */
		vortex_channel_send_err (vortex_connection_get_channel (connection, 0), 
					 "<error code='554'>transaction failed, peer have denied your request</error>",
					 75, 0);
					 
		/* flag the connection to be not connected */
		__vortex_connection_set_not_connected (connection, "connection filtered by on accept handler", VortexConnectionFiltered);
		vortex_connection_unref (connection, "vortex listener");
		return;

	} /* end if */

	/* send greetings, get actual profile installation and report
	 * it to init peer */
	if (send_greetings && (! vortex_greetings_send (connection))) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to send initial greeting message");
		
		/*
		 * This unref sentence is properly defined. Opposite
		 * ref call done to this unref is actually done by
		 * vortex_connection_new_empty.
		 */ 
		vortex_connection_unref (connection, "vortex listener");
		return;
	} /* end if */

	/* flag the connection to be on initial step */
	vortex_connection_set_data (connection, "initial_accept", INT_TO_PTR (true));

	/* call to notify connection created */
	vortex_connection_actions_notify (&connection, CONNECTION_STAGE_POST_CREATED);

	/*
	 * register the connection on vortex reader from here, the
	 * connection get a non-blocking state
	 */
	vortex_reader_watch_connection      (ctx, connection);

	/* close connection and free resources */
	vortex_log (VORTEX_LEVEL_DEBUG, "worker ended, connection registered on manager (initial accept)");

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
	vortex_connection_set_data (connection, "_vo:li:master", listener);

	/*
	 * Perform an initial accept, flagging the connection to be
	 * into the initial accept stage, and send the initial greetings.
	 */
	vortex_listener_accept_connection (connection, true);

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
	vortex_log (VORTEX_LEVEL_DEBUG, "greetings sent, waiting reply");

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
	if (!vortex_greetings_is_reply_ok (frame, connection)) {
		/* previous function already unref frame object
		 * received is something goes wrong */
		vortex_log (VORTEX_LEVEL_CRITICAL, "wrong greeting rpy from init peer, closing session");
		__vortex_connection_set_not_connected (connection, "wrong greeting rpy from init peer, closing session", VortexProtocolError);
		goto unref;
	}

	/* because the greeting is ok, parse it */
	if (!__vortex_connection_parse_greetings (connection, frame)) {
		/* previous function doesn't perform a frame
		 * deallocation, unref it*/
		vortex_frame_unref (frame);
		
		vortex_log (VORTEX_LEVEL_CRITICAL, "wrong greetings received, closing session");
		__vortex_connection_set_not_connected (connection, "wrong greetings received, closing session", VortexProtocolError);
		goto unref;
	}

	/* frame accepted */
	vortex_log (VORTEX_LEVEL_DEBUG, "accepting connection on vortex_reader (second accept step)");
	
	/* free the last frame and watch connection on changes */
	vortex_frame_unref (frame);
	
	/* flag the connection to be totally accepted. */
	vortex_connection_set_data (connection, "initial_accept", NULL);

 unref:
	/*
	 * Because this is a listener, we don't want to pay attention
	 * to free connection on errors. connection already have 1
	 * reference (reader), so let's reference counting to the job
	 * of free connection resources.
	 */
	vortex_connection_unref (connection, "vortex listener (initial accept)");

	return;	
}



void vortex_listener_accept_connections (VortexCtx        * ctx,
					 int                server_socket, 
					 VortexConnection * listener)
{
	struct sockaddr_in inet_addr;
#if defined(AXL_OS_WIN32)
	int               addrlen;
#else
	socklen_t         addrlen;
#endif
	int               soft_limit, hard_limit;

	VORTEX_SOCKET     client_socket;
	VORTEX_SOCKET     temp;

	addrlen       = sizeof(struct sockaddr_in);
	/* accept the connection new connection */
	client_socket = accept (server_socket, (struct sockaddr *)&inet_addr, &addrlen);
	if (client_socket == VORTEX_SOCKET_ERROR) {
		/* get values */
		vortex_conf_get (ctx, VORTEX_SOFT_SOCK_LIMIT, &soft_limit);
		vortex_conf_get (ctx, VORTEX_HARD_SOCK_LIMIT, &hard_limit);

		vortex_log (VORTEX_LEVEL_CRITICAL, "accept () failed, server_socket=%d, soft-limit=%d, hard-limit=%d: %s\n",
			    server_socket, soft_limit, hard_limit, vortex_errno_get_last_error ());
		return;
	}

	/* check we can support more sockets, if not close current
	 * connection */
	temp = socket (AF_INET, SOCK_STREAM, 0);
	if (temp == VORTEX_INVALID_SOCKET) {
		/* uhmmn.. seems we reached our socket limit, we have
		 * to close the connection to avoid keep on iterating
		 * over the listener connection because its backlog
		 * could be filled with sockets we can't accept */
		shutdown (client_socket, SHUT_RDWR);
		vortex_close_socket (client_socket);

		/* get values */
		vortex_conf_get (ctx, VORTEX_SOFT_SOCK_LIMIT, &soft_limit);
		vortex_conf_get (ctx, VORTEX_HARD_SOCK_LIMIT, &hard_limit);
		
		vortex_log (VORTEX_LEVEL_CRITICAL, 
			    "droping incoming client connection, reached process limit: soft-limit=%d, hard-limit=%d\n",
			    soft_limit, hard_limit);
		return;

	} /* end if */
	
	/* close temporal socket */
	vortex_close_socket (temp);

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
	int                        threaded;
	VortexCtx                * ctx;
}VortexListenerData;

axlPointer __vortex_listener_new (VortexListenerData * data)
{
	/* get current context */
	struct hostent     * he;
        struct in_addr     * haddr;
        struct sockaddr_in   saddr;
	struct sockaddr_in   sin;
	VORTEX_SOCKET        fd;
#if defined(AXL_OS_WIN32)
	BOOL                 unit      = true;
	int                  sin_size  = sizeof (sin);
#else    	
	int                  unit      = 1;
	socklen_t            sin_size  = sizeof (sin);
#endif	
	char               * host      = data->host;
	int                  threaded  = data->threaded;
	uint16_t             int_port  = (uint16_t) data->port;
	axlPointer           user_data = data->user_data;
	char               * message   = NULL;
	VortexConnection   * listener;
	VortexCtx          * ctx       = data->ctx;
	int                  backlog   = 0;
	VortexStatus         status    = VortexOk;
	char               * host_used;

	/* handlers received (may be both null) */
	VortexListenerReady      on_ready       = data->on_ready;
	VortexListenerReadyFull  on_ready_full  = data->on_ready_full;
	
	/* free data */
	axl_free (data);

	he = gethostbyname (host);
        if (he == NULL) {
		message = "unable to get hostname by calling gethostbyname";
		status  = VortexNameResolvFailure;
		goto error;
	}

	haddr = ((struct in_addr *) (he->h_addr_list)[0]);
	if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		message = "unable to create a new socket";
		status  = VortexSocketCreationError;
		goto error;
        }

#if defined(AXL_OS_WIN32)
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char  *)&unit, sizeof(BOOL));
#else
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &unit, sizeof (unit));
#endif

	memset(&saddr, 0, sizeof(struct sockaddr_in));
	saddr.sin_family          = AF_INET;
        saddr.sin_port            = htons(int_port);
        memcpy(&saddr.sin_addr, haddr, sizeof(struct in_addr));

        if (bind(fd, (struct sockaddr *)&saddr,  sizeof (struct sockaddr_in)) == VORTEX_SOCKET_ERROR) {
		vortex_close_socket (fd);
		message = "unable to bind address";
		status  = VortexBindError;
		goto error;
        }

	/* get current backlog configuration */
	vortex_conf_get (ctx, VORTEX_LISTENER_BACKLOG, &backlog);

	if (listen(fd, backlog) == VORTEX_SOCKET_ERROR) {
		message = "an error have occur while executing listen";
		status  = VortexSocketCreationError;
		goto error;
        }

	/* notify listener */
	if (getsockname (fd, (struct sockaddr *) &sin, &sin_size) < -1) {
		message = "an error have occur while executing getsockname";
		status  = VortexNameResolvFailure;
		goto error;
	}

	/* unref the host and port value */
	axl_free (host);

	/* seems listener to be created, now create the BEEP
	 * connection around it */
	listener = vortex_connection_new_empty (ctx, fd, VortexRoleMasterListener);

	/* register the listener socket at the Vortex Reader process.  */
	vortex_reader_watch_listener (ctx, listener);
	if (threaded) {
		/* notify listener created */
		host_used = vortex_support_inet_ntoa (ctx, &sin);
		if (on_ready != NULL) {
			on_ready (host_used, ntohs (sin.sin_port), VortexOk,
				  "server ready for requests", user_data);
		} /* end if */
		
		if (on_ready_full != NULL) {
			on_ready_full (host_used, ntohs (sin.sin_port), VortexOk,
				       "server ready for requests", listener, user_data);
		} /* end if */
		axl_free (host_used);
	} /* end if */

	/* the listener reference */
	return listener;

 error:
	/* unref the host and port */
	axl_free (host);
	if (threaded) {
		/* notify error found to handlers */
		if (on_ready != NULL) 
			on_ready      (NULL, 0, status, message, user_data);
		if (on_ready_full != NULL) 
			on_ready_full (NULL, 0, status, message, NULL, user_data);
	} else {
		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to start vortex server, error was: %s, unblocking vortex_listener_wait",
		       message);
		/* notify the listener that an error was found
		 * (because the server didn't suply a handler) */
		vortex_mutex_lock (&ctx->listener_unlock);
		QUEUE_PUSH (ctx->listener_wait_lock, INT_TO_PTR (true));
		ctx->listener_wait_lock = NULL;
		vortex_mutex_unlock (&ctx->listener_unlock);
	} /* end if */
	return NULL;	
}

/** 
 * @internal Implementation to support listener creation functions vortex_listener_new*
 */
VortexConnection * __vortex_listener_new_common  (VortexCtx               * ctx,
						  const char              * host,
						  int                       port,
						  VortexListenerReady       on_ready, 
						  VortexListenerReadyFull   on_ready_full,
						  axlPointer                user_data)
{
	VortexListenerData * data;
	
	v_return_val_if_fail (host, NULL);
	v_return_val_if_fail (port >= 0 && port <= 65536, NULL);

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
 * If user provies an \ref VortexListenerReady "on_ready" callback,
 * it will be notified on it, in a separated thread, once the process
 * have finished. Check documentation for the \ref VortexListenerReady
 * handler which is the async definition for \ref VortexListenerReady "on_ready" handler.
 * 
 * On that notification will also be passed the host and port actually
 * allocated. Think about using as host 0.0.0.0 and port 0. These
 * values will cause to \ref vortex_listener_new to allocate the
 * system configured hostname and a randomly free port. See \ref
 * vortex_handlers "this section" for more info about on_ready.
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
 * Here is an little example to start a vortex listener server:
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
 * int      start_channel (int                channel_num, 
 *                        VortexConnection * connection, 
 *                        axlPointer         user_data)
 * {
 *        printf ("Received an start message=%d!!\n",
 *                 channel_num);
 *        // if the async notifier returns true, the channel
 *        // is implicitly created, if false is returned the
 *        // channel creation is denied and a reply error
 *        // is sent.
 *        return true;
 * }
 *
 * int      close_channel (int                channel_num, 
 *                         VortexConnection * connection, 
 *                         axlPointer         user_data)
 * {
 *        printf ("Got a close message notification!!\n");
 *
 *        // if true is returned, the channel is
 *        // accepted to be closed. Otherwise the channel
 *        // will not be closed and an error reply will be
 *        // sent to the remote peer.
 *        return true;
 * }
 * int main (int argc, char ** argv) {
 *
 *      // create a context
 *      ctx = vortex_ctx_new ();
 *
 *      // enable log to see whats going on 
 *      vortex_log_enable (ctx, true);
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
 *      vortex_listener_new (ctx, "0.0.0.0", "3000", NULL, NULL);
 *      
 *      // wait for listener to finish (maybe due to vortex_exit call)
 *      vortex_listener_wait (ctx);
 *  
 *      // end vortex internal subsystem (if no one have done it yet!)
 *      vortex_ctx_exit (ctx, true);
 * 
 *      // that's all to start BEEPing!
 *      return 0;     
 * }  
 * \endcode
 *
 * @param ctx The context where the operation will be performed.
 *
 * @param host The host to listen to.
 *
 * @param port The port to listen to.
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
VortexConnection * vortex_listener_new (VortexCtx           * ctx,
					const char          * host, 
					const char          * port, 
					VortexListenerReady   on_ready, 
					axlPointer            user_data)
{
	/* call to int port API */
	return __vortex_listener_new_common (ctx, host, __vortex_listener_get_port (port), on_ready, NULL, user_data);
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
 * If your intention isn't getting a reference to the connection
 * created, you can safely use \ref vortex_listener_new and \ref
 * vortex_listener_new2.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param host The host to listen to.
 *
 * @param port The port to listen to.
 *
 * @param on_ready_full A optional notify callback to get when vortex
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
VortexConnection * vortex_listener_new_full  (VortexCtx   * ctx,
					      const char  * host,
					      const char  * port,
					      VortexListenerReadyFull on_ready_full, 
					      axlPointer user_data)
{
	/* call to int port API */
	return __vortex_listener_new_common (ctx, host, __vortex_listener_get_port (port), NULL, on_ready_full, user_data);
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
	return __vortex_listener_new_common (ctx, host, port, on_ready, NULL, user_data);
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
			QUEUE_PUSH (ctx->listener_wait_lock, INT_TO_PTR (true));
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
	
	/* create listener locker */
	if (ctx->listener_wait_lock == NULL) {
		ctx->listener_wait_lock = vortex_async_queue_new ();
	}

	/* init the server on accept connection list */
	if (ctx->listener_on_accept_handlers == NULL)
		ctx->listener_on_accept_handlers = axl_list_new (axl_list_always_return_1, axl_free);

	/* unlock */
	vortex_mutex_unlock (&ctx->listener_mutex);
	return;
}

/** 
 * @internal Allows to cleanup the vortex listener state.
 * 
 * @param ctx The vortex context to cleanup.
 */
void vortex_listener_cleanup (VortexCtx * ctx)
{
	v_return_if_fail (ctx);

	axl_list_free (ctx->listener_on_accept_handlers);
	ctx->listener_on_accept_handlers = NULL;
	
	axl_free (ctx->listener_default_realm);
	ctx->listener_default_realm = NULL;

	if (ctx->listener_wait_lock)
		vortex_async_queue_unref (ctx->listener_wait_lock);
	ctx->listener_wait_lock = NULL;

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
	data->on_accept      = on_accepted;
	data->on_accept_data = _data;

	/* init the list if it wasn't */
	if (ctx->listener_on_accept_handlers == NULL)
		ctx->listener_on_accept_handlers = axl_list_new (axl_list_always_return_1, axl_free);

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
 * @return true if the listener was started because the file was read
 * successfully otherwise false is returned.
 */
int               vortex_listener_parse_conf_and_start (VortexCtx * ctx)
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
		return false;

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
		
		return false;
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

	return true;	
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
 * @param also_created_conns true to shutdown all connections 
 */
void          vortex_listener_shutdown (VortexConnection * listener,
					int                also_created_conns)
{
	VortexCtx        * ctx;
	VortexAsyncQueue * notify = NULL;

	/* check parameters */
	if (! vortex_connection_is_ok (listener, false))
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
