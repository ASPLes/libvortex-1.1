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
	int                  event_id;
	long                 bytes_received;
	struct timeval       when_started;
	int                  max_failure_period;
}VortexAliveData;

/** 
 * @internal Function that finishes data associated to a check enabled
 * on a connection.
 */
void __vortex_alive_free (axlPointer _data)
{
	VortexAliveData * data = _data;

	/* remove event */
	vortex_thread_pool_remove_event (CONN_CTX (data->conn), data->event_id);
	data->event_id = -1;

	if (data->conn) 
		vortex_connection_unref (data->conn, "alive-check");
	data->conn = NULL;

	/* unref channel */
	vortex_channel_unref2 (data->channel, "alive");

	/* free and data */
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
	VortexConnection * conn    = data->conn;
	VortexChannel    * channel = data->channel;
#if defined(ENABLE_VORTEX_LOG) && ! defined(SHOW_FORMAT_BUGS)
	VortexCtx        * ctx  = CONN_CTX (conn);
#endif

	/* nullify reference */
	data->conn = NULL;
	data->channel = NULL;

	/* release channel reference */
	vortex_channel_unref2 (channel, "alive");
	   

	/* now unref here */
	vortex_log (VORTEX_LEVEL_DEBUG, "releasing alive conn-id=%d ref (current: %d)", 
		    vortex_connection_get_id (conn), vortex_connection_ref_count (conn));
	vortex_connection_unref (conn, "alive-check");
	return;
}

void __vortex_alive_connection_closed (VortexConnection * conn, axlPointer user_data)
{
	VortexAliveData * data = user_data;
	VortexCtx       * ctx  = CONN_CTX(conn);

	/* remove events */
	vortex_thread_pool_remove_event (ctx, data->event_id);

	/* call to implement alive stop process */
	vortex_log (VORTEX_LEVEL_WARNING, "detected connection-id=%d close having alive activated",
		    vortex_connection_get_id (conn));
	__vortex_alive_free_reference (data);

	return;
}

void vortex_alive_frame_received (VortexChannel    * channel,
				  VortexConnection * conn,
				  VortexFrame      * frame,
				  axlPointer         user_data)
{
	/* check for incoming alive requests */
	if (vortex_frame_get_type (frame) == VORTEX_FRAME_TYPE_MSG) 
		vortex_channel_send_rpy (channel, "", 0, vortex_frame_get_msgno (frame));
	
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

axl_bool __vortex_alive_found_activity (VortexCtx * ctx, VortexConnection * conn, VortexAliveData * data)
{
	long              bytes_received;
	long              bytes_sent;
	long              last_idle_stamp;

	/* get current stamp and bytes received */
	vortex_connection_get_receive_stamp (conn, &bytes_received, &bytes_sent, &last_idle_stamp);

	/* check against values previously recorded */
	if (bytes_received > data->bytes_received) {
		vortex_log (VORTEX_LEVEL_WARNING, 
			    "Skipping failure check because bytes=%ld (previous %ld) received signals activity on conn-id=%d",
			    bytes_received, data->bytes_received, vortex_connection_get_id (conn));
		data->bytes_received  = bytes_received;
		return axl_true; /* activity found, skip alive failure checks */
	} /* end if */

	return axl_false; /* no activity, continue check */
}


/** 
 * @internal Function used to trigger failure after channel start
 * timeout or unreply count is reached.
 */
axl_bool __vortex_alive_trigger_failure (VortexAliveData * data)
{
#if defined(ENABLE_VORTEX_LOG) && ! defined(SHOW_FORMAT_BUGS)
	/* get ctx */
	VortexCtx * ctx = CONN_CTX (data->conn);
#endif
	axl_bool    release_ref;

	/* remove on close */
	release_ref = vortex_connection_remove_on_close_full (data->conn, __vortex_alive_connection_closed, data);

	/* check if we have a handler defined */
	if (data->failure_handler) {
		/* notify on handler */
		vortex_log (VORTEX_LEVEL_CRITICAL, 
			    "alive check max unreplied count reached=%d or alive channel not created, notify on failure handler for connection id=%d (release_ref=%d)",
			    data->max_unreply_count,
			    vortex_connection_get_id (data->conn), release_ref);

		/* call user space */
		data->failure_handler (data->conn, data->check_period, data->max_unreply_count);
		
		/* remove check alive data */
		if (release_ref) 
			__vortex_alive_free_reference (data);    
		
		/* request to remove this event */
		return axl_true;
	}
	
	vortex_log (VORTEX_LEVEL_CRITICAL, "alive check max unreplied count reached=%d < %d or alive channel not created, shutting down the connection id=%d (release_ref=%d)",
		    data->max_unreply_count, vortex_channel_get_outstanding_messages (data->channel, NULL), vortex_connection_get_id (data->conn),
		    release_ref);
	
	/* shutdown the connection */
	if (vortex_connection_is_ok (data->conn, axl_false)) {
		/* remove connection close */
		vortex_connection_shutdown (data->conn);
	}

 	/* remove check alive data */
	if (release_ref) 
		__vortex_alive_free_reference (data);  

	return axl_true;
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

		/* request system to remove this task */
		return axl_true;
	} /* end if */

	/* do not trigger alive failure if connection idle stamp was
	   updated or more bytes were received */
	if (__vortex_alive_found_activity (ctx, data->conn, data))
		return axl_false;

	/* if channel is ok, and so the connection, check if there are
	 * pending replies and if they exceed the max unreplied
	 * count */
	if (data->max_unreply_count <= vortex_channel_get_outstanding_messages (data->channel, NULL)) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "max unreply count reached (%d) < pending replies (%d), trigger failure",
			    data->max_unreply_count, vortex_channel_get_outstanding_messages (data->channel, NULL));
		/* call to trigger failure */
		return __vortex_alive_trigger_failure (data);
	} /* end if */

	/* ok, up to date or under the configured limits, request to send content (ping) */
	vortex_log (VORTEX_LEVEL_DEBUG, "doing alive check on connection id=%d", vortex_connection_get_id (data->conn));
	if (! vortex_channel_send_msg (data->channel, "", 0, NULL)) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Failed to send check alive message on connection id=%d, removing check alive event",
			    vortex_connection_get_id (data->conn));

		return axl_true; /* request to remove event */
	} /* end ok */

	/* request the system to not remove the alive check */
	return axl_false;
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
	VortexCtx        * ctx  = CONN_CTX (conn);

	if (data->conn == NULL || data->event_id == -1 || vortex_connection_get_data (conn, VORTEX_ALIVE_CHECK_ENABLED) == NULL) {
		vortex_log (VORTEX_LEVEL_WARNING, "received channel alive created after perioed was expired for conn-id=%d",
			    vortex_connection_get_id (conn));
		return;
	} /* end if */

	/* call to remove the create event that is triggering creating
	   the channel (and checkig it is done without the check
	   period configured) before continue to avoid closing the
	   connection because failure and that handler accessing to
	   the same reference */
	vortex_thread_pool_remove_event (ctx, data->event_id);
	data->event_id = -1;

	/* acquire a reference to it and update channel reference */
	if (! vortex_channel_ref2 (channel, "alive")) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "failed to acquire reference to the channel to implement ALIVE protocol, skiping");
		return;
	} /* end if */
	data->channel = channel;
	data->channel_in_progress = axl_false;

	/* log reporting */
	if (channel) {
		vortex_log (VORTEX_LEVEL_DEBUG, "alive channel created on connection id=%d", vortex_connection_get_id (conn));
		/* record current activity to have current reference
		 * of the connection's activity */
		__vortex_alive_found_activity (ctx, data->conn, data);

		/* now configure the event to implement the check */
		data->event_id = vortex_thread_pool_new_event (ctx, data->check_period, __vortex_alive_do_check, data, NULL);
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


	vortex_log (VORTEX_LEVEL_CRITICAL, "failed to create alive channel on connection id=%d, trigger failure", vortex_connection_get_id (conn));

	/* call to trigger failure */
	__vortex_alive_trigger_failure (data);
	

	return;
}

axl_bool   __vortex_alive_create_channel        (VortexCtx  * ctx, 
						 axlPointer   user_data,
						 axlPointer   user_data2)
{
	VortexAliveData * data = user_data;
	struct timeval    now;

	/* check connection status after continue with tests */
	if (! vortex_connection_is_ok (data->conn, axl_false)) {
		vortex_log (VORTEX_LEVEL_DEBUG, "finishing alive check on connection id=%d, found closed", 
			    vortex_connection_get_id (data->conn));

		/* request system to remove this task */
		return axl_true;
	} /* end if */

	/* check if channel is created to install the new event to do
	 * the alive check */
	if (data->channel)
		return axl_true; /* request to remove this event */

	if (! data->channel_in_progress) {
		/* notify we are in progress of creating a channel */
		data->channel_in_progress = axl_true;
		/* set max tries accepted during channel
		   creation: if channel is not created during
		   this tries, alive check will consider the
		   connection is failing */
		if (data->check_period <= 40000)
			data->channel_tries  = data->max_unreply_count + 10;
		
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
	} /* end if */

	vortex_log (VORTEX_LEVEL_WARNING, "Alive channel still not created, tries: %d (in_progress: %d, outstanding messages: %d)", 
		    data->channel_tries, data->channel_in_progress,
		    vortex_channel_get_outstanding_messages (data->channel, NULL));

	/* do not trigger alive failure if connection idle stamp was
	   updated or more bytes were received */
	if (__vortex_alive_found_activity (ctx, data->conn, data))
		return axl_false;

	/* check channel to be created within the max period
	 * established */
	gettimeofday (&now, NULL);
	if ((now.tv_sec - data->when_started.tv_sec) >= data->max_failure_period) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Alive channel was not created after waiting %d seconds (max failure period)",
			    data->max_failure_period);

		/* call to trigger failure */
		return __vortex_alive_trigger_failure (data);
	}
		
	
	/* check channel tries */
	data->channel_tries --;
	if (data->channel_tries == 0) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Alive channel was not created during check internal %ld + %d",
			    data->check_period, data->max_unreply_count + 10);

		/* call to trigger failure */
		return __vortex_alive_trigger_failure (data);
	} /* end if */
	
	
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
 * we accept. It must be > 0 or the function will return axl_false. If
 * max_unreply_count is reached, the failure is triggered.
 *
 * @param failure_handler Optional handler called when a failure is
 * detected.
 *
 * @return axl_true if the check was properly enabled on the
 * connection, otherwise axl_false is returned.
 *
 * <b>NOTE: about channel created to do alive tests</b>
 *
 * In order to implement ALIVE checks, a channel must be created over
 * the provided connection. Until that channel isn't working, ALIVE
 * checkign cannot be implemented.
 *
 * In this context, if the channel isn't created before
 * (max_unreply_count x check_period), with a minimum value of 3
 * seconds, ALIVE will trigger a failure too.
 */
axl_bool           vortex_alive_enable_check               (VortexConnection * conn,
							    long               check_period,
							    int                max_unreply_count,
							    VortexAliveFailure failure_handler)
{
	VortexAliveData  * data;
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
	data       = axl_new (VortexAliveData, 1);
	if (data == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "failed to acquire memory to hold VortexAliveData, unable to continue");
		return axl_false;
	}
	data->conn              = conn;
	data->failure_handler   = failure_handler;
	data->max_unreply_count = max_unreply_count;
	data->check_period      = check_period;

	/* get when we started to track channel creation */
	gettimeofday (&data->when_started, NULL);
	/* get how many seconds we can wait until channel is
	 * created */
	data->max_failure_period = (data->max_unreply_count * data->check_period) / 1000000;
	/* at least give 3 seconds to complete channel creation */
	if (data->max_failure_period < 3)
		data->max_failure_period = 3;
	
	if (! vortex_connection_ref (conn, "alive-check")) {
		/* release */
		axl_free (data);
		vortex_log (VORTEX_LEVEL_CRITICAL, "Failed to enable alive check, connection reference acquisition have failed");
		return axl_false;
	}

	/* create data */
	vortex_connection_set_data_full (conn, VORTEX_ALIVE_CHECK_ENABLED, data, NULL, __vortex_alive_free);
	
	/* also configure connection close to detect and react */
	vortex_connection_set_on_close_full (conn, __vortex_alive_connection_closed, data);

	/* record current bytes received and idle stamp */
	vortex_connection_get_receive_stamp (conn, &data->bytes_received, NULL, NULL);

	/* install event to create the channel */
	data->event_id = vortex_thread_pool_new_event (ctx, check_period > 40000 ? check_period : 40000, __vortex_alive_create_channel, data, NULL);

	return axl_true;
}


/* @} */
