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
#include <vortex_external.h>
#include <vortex_connection_private.h>

#define LOG_DOMAIN "vortex-external"

struct _VortexExternalSetup {

	axl_bool        enable_mutex;
	char          * host;
	char          * port;
	VortexMutex     mutex;
	int             ref_count;

	VortexExternalOnPrepare     on_prepare;
	axlPointer                  on_prepare_user_data;
	axlPointer                  on_prepare_user_data2;

};

typedef struct _VortexExternalData {
	
	VortexSendHandler           send_handler;
	VortexReceiveHandler        receive_handler;
	VortexMutex                 mutex;
	VortexExternalSetup       * setup;
	
} VortexExternalData;

/** 
 * \defgroup vortex_external Vortex External API: support for BEEP connections over external technologies
 */

/** 
 * \addtogroup vortex_external
 * @{
 */

/** 
 * @brief Allows to create a setup object to be used on a
 * VortexConnection external.
 *
 * @param ctx The context where this setup is to be created.
 *
 * @return A reference to the setup setup object created, NULL in case
 * of failure.
 */
VortexExternalSetup  * vortex_external_setup_new      (VortexCtx * ctx)
{
	VortexExternalSetup * setup = axl_new (VortexExternalSetup, 1);
	if (setup == NULL)
		return NULL;

	/* init mutex */
	vortex_mutex_create (&(setup->mutex));
	setup->ref_count = 1;

	return setup;
}

/** 
 * @brief Increase reference counting
 *
 * @param setup The setup object to handle 
 *
 * @return axl_true in the case it was increased (reference)
 * otherwise, axl_false is returned.
 */
axl_bool               vortex_external_setup_ref      (VortexExternalSetup * setup)
{
	int result;

	if (setup == NULL)
		return axl_false;

	vortex_mutex_lock (&setup->mutex);

	setup->ref_count++;
	result = (setup->ref_count > 1);

	vortex_mutex_unlock (&setup->mutex);

	return result;
}

/** 
 * @brief Decrease reference counting
 *
 * @param setup The setup object to handle 
 *
 */
void                   vortex_external_setup_unref    (VortexExternalSetup * setup)
{
	int result;

	if (setup == NULL)
		return;

	vortex_mutex_lock (&setup->mutex);

	/* call to decrease and check if we have to release */
	setup->ref_count--;
	result = setup->ref_count <= 0;

	if (result) {
		vortex_mutex_unlock (&setup->mutex);

		/* release internal data */
		vortex_mutex_destroy (&setup->mutex);
		axl_free (setup->host);
		axl_free (setup->port);
		axl_free (setup);

		return;
	} /* end if */

	vortex_mutex_unlock (&setup->mutex);


	return;
}

/** 
 * @brief Allows to configure setup object.
 *
 * @param setup The setup object to handle 
 *
 * @param item The configuration item to setup
 *
 * @param value The value to configure. It depends on the item configuration.
 *
 */
void                   vortex_external_setup_conf     (VortexExternalSetup      * setup,
						       VortexExternalConfItem     item,
						       axlPointer                 value)
{
	if (setup == NULL)
		return;
	switch (item) {
	case VORTEX_EXTERNAL_CONF_MUTEX_IO:
		/* configure mutex support */
		setup->enable_mutex = PTR_TO_INT (value);
		break;
	case VORTEX_EXTERNAL_CONF_HOST:
		/* set value */
		setup->host = axl_strdup (value);
		break;
	case VORTEX_EXTERNAL_CONF_PORT:
		/* set value */
		setup->port = axl_strdup (value);
		break;
	case VORTEX_EXTERNAL_ON_PREPARE_HANDLER:
		/* set value */
		setup->on_prepare = value;
		break;
	case VORTEX_EXTERNAL_ON_PREPARE_USER_DATA:
		/* set value */
		setup->on_prepare_user_data = value;
		break;
	case VORTEX_EXTERNAL_ON_PREPARE_USER_DATA2:
		/* set value */
		setup->on_prepare_user_data2 = value;
		break;
	} /* end switch */

	return;
}

void __vortex_external_data_release (axlPointer _data) {
	VortexExternalData * data = _data;

	/* destroy mutex and external setup (if any) */
	vortex_mutex_destroy (&data->mutex);
	vortex_external_setup_unref (data->setup);

	/* release data */
	axl_free (data);

	return;
}


/** 
 * @internal Send handler 
 */
int      __vortex_external_send_handler         (VortexConnection * connection,
						 const char       * buffer,
						 int                buffer_len)
{
	VortexCtx          * ctx  = CONN_CTX (connection);
	VortexExternalData * data = vortex_connection_get_data (connection, "vo:ex:da");
	int                  result;

	if (data == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Failed to send data (__vortex_external_send_handler), failed to get status data");
		return -1;
	} /* end if */

	if (data->setup && data->setup->enable_mutex) {
		/* call to get mutex */
		vortex_mutex_lock (&(data->setup->mutex));
	}

	/* call external method (send_handler) */
	result =  data->send_handler (connection, buffer, buffer_len);

	if (data->setup && data->setup->enable_mutex) {
		/* call to get mutex */
		vortex_mutex_unlock (&(data->setup->mutex));
	} /* end if */

	return result;
}

/** 
 * @internal Received handler 
 */
int      __vortex_external_received_handler     (VortexConnection * connection,
						 char             * buffer,
						 int                buffer_len)
{
	VortexCtx          * ctx  = CONN_CTX (connection);
	VortexExternalData * data = vortex_connection_get_data (connection, "vo:ex:da");
	int                  result;

	if (data == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Failed to send data (__vortex_external_received_handler), failed to get status data");
		return -1;
	} /* end if */

	if (data->setup && data->setup->enable_mutex) {
		/* call to get mutex */
		vortex_mutex_lock (&(data->setup->mutex));
	}

	/* call external method (receive_handler) */
	result =  data->receive_handler (connection, buffer, buffer_len);

	if (data->setup && data->setup->enable_mutex) {
		/* call to get mutex */
		vortex_mutex_unlock (&(data->setup->mutex));
	} /* end if */

	return result;
}




/** 
 * @brief Creates a new BEEP connection to a remote BEEP server using
 * a session that is already created and a set of handlers to do the
 * I/O.
 *
 * @param ctx The context where the operation will take place.
 *
 * @param _session The session to configure/wrap around Vortex/BEEP
 * API. This session must be working and has to be created before
 * calling to this function.
 *
 * @param _send_handler The handler that will be used to do the write operation
 *
 * @param _received_handler The handler that will be used to do read operation
 *
 * @param setup A reference (optional) to the setup object (\ref
 * vortex_external_setup_new).
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
 * provided at \ref vortex_external_setup_new). This means you'll have to
 * create different \ref VortexExternalSetup instances for each context
 * you have.</i>
 *
 */
VortexConnection * vortex_external_connection_new (VortexCtx                 * ctx,
						   VORTEX_SOCKET               _session,
						   VortexSendHandler           _send_handler,
						   VortexReceiveHandler        _received_handler,
						   VortexExternalSetup       * setup,
						   VortexConnectionNew         on_connected, 
						   axlPointer                  user_data)
{
	VortexConnection     * conn;
	VortexExternalData   * data;

	if (ctx == NULL)
		return NULL; /* no connection */

	if (_session < 0 || _session == VORTEX_INVALID_SOCKET) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Failed to create connection out of session received, wrong value received: %d", 
			    _session);
		return NULL; /* wrong socket */
	}

	/* create connection */
	conn = vortex_connection_new_empty_from_connection2 (ctx, _session, NULL, VortexRoleInitiator, axl_true);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Failed to create connection out of session received, vortex_connection_is_ok () failed");
		return NULL; /* wrong reference */
	} /* end if */

	/* create data and get a reference to the setup (if any) */
	data = axl_new (VortexExternalData, 1);
	if (data == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Failed to create connection unable to allocation memory for internal VortexExternalData");
		return NULL; /* wrong reference */
	} /* end if */

	/* configure data reference */
	if (setup)
		data->setup = setup;

	/* setup data to be usable and released when not needed
	   anymore */
	vortex_connection_set_data_full (conn, "vo:ex:da", data, NULL, __vortex_external_data_release);

	/* check and configure internal mutex (if needed) */
	if (setup && setup->enable_mutex) {
		/* create mutex */
		vortex_mutex_create (&data->mutex);

	} /* end if */

	/* ok, configure send and receive handlers */
	data->send_handler    = _send_handler;
	data->receive_handler = _received_handler;

	vortex_connection_set_send_handler (conn, __vortex_external_send_handler);
	vortex_connection_set_receive_handler (conn, __vortex_external_received_handler);

	/* configure connection here */
	conn->session = _session;

	/* configure host name if indicated by setup */

	/****** do not release setup here ******/

	/* do on prepare */
	if (setup && setup->on_prepare) 
		setup->on_prepare (ctx, conn, setup->on_prepare_user_data, setup->on_prepare_user_data2);

	/* do BEEP greetings */
	vortex_log (VORTEX_LEVEL_DEBUG, "Created BEEP connection from external session=%d (conn-id=%d, ref: %p), doing BEEP greetings",
		    _session, conn->id, conn);
	if (! vortex_connection_do_greetings_exchange (ctx, conn, NULL, 60)) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Created BEEP connection from external session=%d (conn-id=%d, ref: %p), but greetings failed",
			    _session, conn->id, conn);
		return conn;
	} /* end if */

	vortex_log (VORTEX_LEVEL_DEBUG, "Successfully created BEEP connection from external session=%d (conn-id=%d, ref: %p)",
		    _session, conn->id, conn);

	return conn;
}

typedef struct _VortexExternalListenerData {

	VORTEX_SOCKET               _session;
	VortexConnection          * listener;
	VortexExternalSetup       * setup;

	/* on accept handlers */
	VortexExternalOnAccept      on_accept_handler;
	axlPointer                  on_accept_data;

	/* send and received handler */
	VortexSendHandler           _send_handler;
	VortexReceiveHandler        _received_handler;

} VortexExternalListenerData;

void __vortex_external_listener_data_release (axlPointer _data)
{
	VortexExternalListenerData * data = _data;

	/* release socket session and unref data */
	vortex_close_socket (data->_session);
	vortex_external_setup_unref (data->setup);

	/* release data */
	axl_free (data);

	return;
}

void __vortex_external_listener_on_accept (VortexConnection * conn)
{
	VortexCtx                  * ctx        = CONN_CTX (conn);
	VortexExternalListenerData * data       = vortex_connection_get_data (conn, "vo:ext:data");
	VortexConnection           * new_conn   = NULL;
	VORTEX_SOCKET                _new_socket;

	vortex_log (VORTEX_LEVEL_DEBUG, "Called pre-read handler over a BEEP over External listener id=%d (socket=%d), doing initial accept",
		    vortex_connection_get_id (conn), data->_session);

	/* ok, accept connection and register it */
	_new_socket = data->on_accept_handler (ctx, conn, data->_session, data->on_accept_data);
	if (_new_socket == -1 || _new_socket == VORTEX_INVALID_SOCKET) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Received -1 or invalid socket from external on_accept handler, skipping accept");
		return;
	} /* end if */

	/* call for initial accept */
	new_conn = __vortex_listener_initial_accept (
		ctx, 
		_new_socket,
		conn,
		/* don't register the connection */
		axl_false);

	/* setup here the mutex if indicated by setup */

	/* configure I/O send and receive handlers */
	vortex_connection_set_send_handler (new_conn, data->_send_handler);
	vortex_connection_set_receive_handler (new_conn, data->_received_handler);

	/* called to accept listener inthe next step now we have it configured */
	vortex_listener_accept_connection (new_conn, axl_true);

	/* check connection status just to drop a log */
	if (vortex_connection_is_ok (new_conn, axl_false)) 
		vortex_log (VORTEX_LEVEL_DEBUG, "Received new BEEP over External connection id=%d, still required to complete BEEP greetings", 
			    vortex_connection_get_id (new_conn));
	
	return;
}


/** 
 * @brief Allows to create a new BEEP listener accepting connections
 * over External/unknown transport.
 *
 * 
 *
 * @param ctx The context where the operation is taking place.
 *
 * @param _session Already created transport represented by a watchable socket. 
 *
 * @param _send_handler The send/write handler that will be configured
 * on any accepted connection. For more information see \ref
 * VortexSendHandler, \ref vortex_connection_set_send_handler
 *
 * @param _received_handler The read/recv handler that will be
 * configured on any accepted connection. For more information see
 * \ref VortexReceiveHandler, \ref vortex_connection_set_receive_handler
 *
 * @param setup A reference (optional) to the setup object (\ref
 * vortex_external_setup_new).
 *
 * @param on_accept_handler The handler that will be called every time
 * an incoming connection is received on the provided master listener
 * socket (_session). The function must return a newly created socket
 * to allow Vortex Engine to create the BEEP session.
 *
 * @param on_accept_data A user defined pointer that will be passed to
 * on_accept_handler
 *
 *
 * @return The function returns a newly created BEEP listener (over
 * External) or NULL if the optional handler is provided
 * (on_ready). See \ref vortex_listener_new_full to know more about
 * the value returned by this function as this function is just a
 * wrapper to it, providing the needed bridging between BEEP and External.
 */
VortexConnection * vortex_external_listener_new   (VortexCtx                 * ctx,
						   VORTEX_SOCKET               _session,
						   VortexSendHandler           _send_handler,
						   VortexReceiveHandler        _received_handler,
						   VortexExternalSetup       * setup,
						   VortexExternalOnAccept      on_accept_handler,
						   axlPointer                  on_accept_data)
{

	VortexConnection           * result;
	VortexExternalListenerData * data;

	if (_send_handler == NULL || _received_handler == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Failed to create listener, send/receive handler is NULL");
		return NULL;
	} /* end if */

	if (on_accept_handler == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Failed to create listener, on accept handler is NULL");
		return NULL;
	} /* end if */

	if (_session < 0 || _session == -1 || _session == VORTEX_INVALID_SOCKET) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Failed to create listener, _session=%d doesn't have a valid value");
		return NULL;
	}


	/* check if this is a blocking or async listener creation */
	result = vortex_connection_new_empty_from_connection2 (ctx, _session, NULL, VortexRoleMasterListener, axl_true);

	/* operation ok */
	if (! vortex_connection_is_ok (result, axl_false)) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "EXTERNAL: failed to start listener, errno=%d, conn-id=%d, conn=%p, socket=%d",
			    errno, vortex_connection_get_id (result), result, _session);
		return result;
	} /* end if */

	data = axl_new (VortexExternalListenerData, 1);
	if (data == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "EXTERNAL: failed to start listener, allocation failure for external data, errno=%d, conn-id=%d, conn=%p, socket=%d",
			    errno, vortex_connection_get_id (result), result, _session);
		vortex_connection_shutdown (result);
		return result;
	} /* end if */

	vortex_log (VORTEX_LEVEL_DEBUG, "EXTERNAL: starting listener id=%d, session=%d (refs: %d)",
		    vortex_connection_get_id (result), _session);
	
	/* get setup */
	data->_session = _session;

	data->setup    = setup;
	data->listener = result;
	
	data->on_accept_data    = on_accept_data;
	data->on_accept_handler = on_accept_handler;

	data->_send_handler     = _send_handler;
	data->_received_handler = _received_handler;

	/* now configure here the on accept function that
	 * should be used to create new BEEP sessions on top
	 * of WebSocket */
	vortex_connection_set_preread_handler (result, __vortex_external_listener_on_accept);
	
	/* associate listener and context */
	vortex_connection_set_data_full (result, "vo:ext:data", data, NULL, __vortex_external_listener_data_release);
	
	/* register into the reader */
	vortex_reader_watch_listener (ctx, result);
	
	return result;
}



/** 
 * @} 
 */

