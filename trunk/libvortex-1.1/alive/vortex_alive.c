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

/** 
 * \defgroup vortex_alive Vortex ALIVE: A profile extension that allows to check peer alive status
 */

/** 
 * \addtogroup vortex_alive
 * @{
 */

#include <vortex_alive.h>

#define VORTEX_ALIVE_CHECK_ENABLED "vo:co:al"

typedef struct _VortexAliveData {
	long                 check_period;
	int                  max_unreply_count;
	VortexAliveFailure   failure_handler;
	VortexConnection   * conn;
	VortexChannel      * channel;
	axl_bool             channel_in_progress;
	int                  channel_tries;
}VortexAliveData;

/** 
 * @internal Function that finishes data associated to a check enabled
 * on a connection.
 */
void __vortex_alive_free (axlPointer _data)
{
	VortexAliveData * data = _data;
	if (data->conn) 
		vortex_connection_unref (data->conn, "alive-check");
	data->conn = NULL;
	axl_free (data);
	return;
}

/** 
 * @internal Function that implements vortex alive data reference
 * removal from connection. This function is used to avoid calling to
 * vortex_connection_unref for the last reference inside
 * __vortex_alive_free which in turn will execute inside
 * vortex_connection_free.
 */
void __vortex_alive_free_reference (VortexAliveData * data)
{
	VortexConnection * conn = data->conn;

	/* nullify reference */
	data->conn = NULL;

	/* call to remove reference from connection: the following
	   call will remove VortexAliveData from the connection
	   without calling to unref the connection. */
	vortex_connection_set_data (data->conn, VORTEX_ALIVE_CHECK_ENABLED, NULL);

	/* now unref here */
	vortex_connection_unref (conn, "alive-check");
	return;
}

void vortex_alive_frame_received (VortexChannel    * channel,
				  VortexConnection * conn,
				  VortexFrame      * frame,
				  axlPointer         user_data)
{
	/* check for incoming alive requests */
	if (vortex_frame_get_type (frame) == VORTEX_FRAME_TYPE_MSG) 
		vortex_channel_send_msg (channel, "", 0, NULL);
	
	return;
}

/** 
 * @brief Allows to init alive module on the current context. This is
 * required to receive alive requests, allowing remote peer to check
 * if we are reachable.
 *
 * @param ctx The context where the alive profile will be enabled. 
 *
 */
axl_bool           vortex_alive_init                       (VortexCtx * ctx)
{
	/* register profile */
	vortex_profiles_register (ctx,
				  VORTEX_ALIVE_PROFILE_URI,
				  /* on start */
				  NULL, NULL,
				  /* on close */
				  NULL, NULL,
				  /* on frame received */
				  vortex_alive_frame_received, NULL);
	return axl_true;
}

void __vortex_alive_channel_created (int                channel_num, 
				     VortexChannel    * channel, 
				     VortexConnection * conn, 
				     axlPointer         user_data)
{
	VortexAliveData * data = user_data;
	int                code;
	char             * msg;
	int                iterator;
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx        * ctx  = CONN_CTX (conn);
#endif

	/* update channel reference */
	data->channel = channel;
	data->channel_in_progress = axl_false;

	/* log reporting */
	if (channel) {
		vortex_log (VORTEX_LEVEL_DEBUG, "alive channel created on connection id=%d", vortex_connection_get_id (conn));
		return;
	}

	/* reached this point, error was found */
	vortex_log (VORTEX_LEVEL_CRITICAL, "failed to create alive channel on connection id=%d, error stack:", vortex_connection_get_id (conn));
	iterator = 0;
	while (vortex_connection_pop_channel_error (conn, &code, &msg)) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "  %d: code:%d message: %s",
			    iterator, code, msg);
		axl_free (msg);
		iterator++;
	} /* end while */

	return;
}


/** 
 * @internal function that implements the alive check, closing the
 * connection in case of delay.
 */
axl_bool __vortex_alive_do_check        (VortexCtx  * ctx, 
					 axlPointer   user_data,
					 axlPointer   user_data2)
{
	VortexAliveData * data = user_data;

	/* check connection status after continue with tests */
	if (! vortex_connection_is_ok (data->conn, axl_false)) {
		vortex_log (VORTEX_LEVEL_DEBUG, "finishing alive check on connection id=%d, found closed", 
			    vortex_connection_get_id (data->conn));

		/* call to remove data associated */
		__vortex_alive_free_reference (data);
		
		/* request system to remove this task */
		return axl_true;
	} /* end if */

	if (data->channel == NULL) {
		if (! data->channel_in_progress) {
			/* notify we are in progress of creating a channel */
			data->channel_in_progress = axl_true;
			/* set max tries accepted during channel
			   creation: if channel is not created during
			   this tries, alive check will consider the
			   connection is failing */
			data->channel_tries       = data->max_unreply_count + 5;

			vortex_log (VORTEX_LEVEL_DEBUG, "channel alive profile still not created, triggering async creation..");
			vortex_channel_new (data->conn, 0, VORTEX_ALIVE_PROFILE_URI, 
					    /* on close handler */
					    NULL, NULL, 
					    /* on frame received */
					    NULL, NULL,
					    /* async notification */
					    __vortex_alive_channel_created, data);

			/* still channel not created, unable to do checks */
			return axl_false;
		} 

		vortex_log (VORTEX_LEVEL_WARNING, "Alive channel still not created, tries: %d", data->channel_tries);

		/* check channel tries */
		data->channel_tries --;
		if (data->channel_tries == 0) {
			vortex_log (VORTEX_LEVEL_CRITICAL, "Alive channel was not created during check internal %ld x %d",
				    data->check_period, data->max_unreply_count + 5);
			goto trigger_failure;
		} /* end if */

		/* still channel not created, unable to do checks */
		return axl_false;
	} /* end if */

	/* if channel is ok, and so the connection, check if there are pending replies */
	if (data->max_unreply_count > vortex_channel_get_outstanding_messages (data->channel, NULL)) {
	trigger_failure:
		/* check if we have a handler defined */
		if (data->failure_handler) {
			/* notify on handler */
			vortex_log (VORTEX_LEVEL_CRITICAL, "alive check max unreplied count reached=%d or alive channel not created, notify on failure handler for connection id=%d",
				    vortex_connection_get_id (data->conn));
			data->failure_handler (data->conn, data->check_period, data->max_unreply_count);

			/* remove check alive data */
			__vortex_alive_free_reference (data);

			/* request to remove this event */
			return axl_true;
		}

		vortex_log (VORTEX_LEVEL_CRITICAL, "alive check max unreplied count reached=%d or alive channel not created, shutting down the connection id=%d",
			    data->max_unreply_count, vortex_connection_get_id (data->conn));

		/* close the connection */
		vortex_connection_shutdown (data->conn);

		/* remove check alive data */
		__vortex_alive_free_reference (data);

		/* request to remove this event */
		return axl_true;
	}

	/* request to send content (ping) */
	vortex_log (VORTEX_LEVEL_DEBUG, "doing alive check on connection id=%d", vortex_connection_get_id (data->conn));
	if (! vortex_channel_send_msg (data->channel, "", 0, NULL)) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Failed to send check alive message on connection id=%d, removing check alive event",
			    vortex_connection_get_id (data->conn));

		/* call to remove data associated */
		__vortex_alive_free_reference (data);
		
		return axl_true;
	} /* end ok */

	/* request the system to not remove the alive check */
	return axl_false;
}

/** 
 * @brief Allows to enable alive supervision on the provided connection.
 *
 * The function enables a permanent test every <b>check_period</b> to
 * ensure the connection is working. The check is implemented by a
 * simply request/reply with an unreplied tracking to ensure we detect
 * connection lost even when power failures or network cable unplug.
 *
 * The function accepts a max_unreply_count which usually is 0 (no
 * unreplied messages accepted) or the amount of unreplied messages we
 * are accepting until calling to failure handler or shutdown the
 * connection.
 *
 * The failure_handler is optional. If not provided, the alive check
 * will call to shutdown the connection (\ref
 * vortex_connection_shutdown), activating connection on close
 * configured (\ref vortex_connection_set_on_close_full) so close
 * handling is unified on the same place. 
 *
 * However, if failure_handler is defined, the alive check will not
 * shutdown the connection and will call the handler.
 *
 * @param conn The connection where the check will be enabled.
 *
 * @param check_period The check period. It is defined in microseconds
 * (1 second == 1000000 microseconds). To check every 20ms a
 * connection pass 20000. It must be > 0, or the function will return
 * axl_false.
 *
 * @param max_unreply_count The maximum amount of unreplied messages
 * we accept. It must be > 0 or the function will return axl_false.
 *
 * @param failure_handler Optional handler called when a failure is
 * detected.
 *
 * @return axl_true if the check was properly enabled on the
 * connection, otherwise axl_false is returned.
 */
axl_bool           vortex_alive_enable_check               (VortexConnection * conn,
							    long               check_period,
							    int                max_unreply_count,
							    VortexAliveFailure failure_handler)
{
	VortexAliveData * data;
	VortexCtx        * ctx = CONN_CTX (conn);

	if (check_period <= 0 || max_unreply_count < 0) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Expecified an unsupported check period or max unreply count. Both must be > 0.");
		return axl_false;
	}

	/* check if the connection is already checked */
	if (PTR_TO_INT (vortex_connection_get_data (conn, VORTEX_ALIVE_CHECK_ENABLED))) {
		vortex_log (VORTEX_LEVEL_WARNING, "Calling to enable connection alive check where it is already enabled");
		return axl_false;
	} /* end if */

	/* check to get a reference to the connection */
	if (! vortex_connection_ref (conn, "alive-check")) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Failed to enable alive check, connection reference acquisition have failed");
		return axl_false;
	}
	
	/* create data */
	data       = axl_new (VortexAliveData, 1);
	data->conn = conn;
	vortex_connection_set_data_full (conn, VORTEX_ALIVE_CHECK_ENABLED, data, NULL, __vortex_alive_free);

	/* install event to check */
	vortex_thread_pool_new_event (ctx, check_period, __vortex_alive_do_check, data, NULL);

	return axl_true;
}


/* @} */
