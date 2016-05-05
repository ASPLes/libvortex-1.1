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
#include <vortex.h>

/* local/private includes */
#include <vortex_ctx_private.h>
#include <vortex_connection_private.h>

#define LOG_DOMAIN "vortex-reader"

/**
 * \defgroup vortex_reader Vortex Reader: The module that reads you frames. 
 */

/**
 * \addtogroup vortex_reader
 * @{
 */

typedef enum {CONNECTION, 
	      LISTENER, 
	      TERMINATE, 
	      IO_WAIT_CHANGED,
	      IO_WAIT_READY,
	      FOREACH
} WatchType;

typedef struct _VortexReaderData {
	WatchType            type;
	VortexConnection   * connection;
	VortexForeachFunc    func;
	axlPointer           user_data;
	/* queue used to notify that the foreach operation was
	 * finished: currently only used for type == FOREACH */
	VortexAsyncQueue   * notify;
}VortexReaderData;

/** 
 * @internal
 *
 * @brief NUL specific frame checkings
 * 
 * @param frame The frame to check
 * @param connection The connection where the frame was received.
 * 
 * @return axl_true if all checks have been passed. Otherwise axl_false is returned.
 */
axl_bool      __vortex_reader_process_socket_check_nul_frame (VortexCtx        * ctx,
							      VortexFrame      * frame, 
							      VortexConnection * connection)
{
	/* call to update frame MIME status for NUL frame size we
	 * check the MIME body content to be 0 */
	if (! vortex_frame_mime_process (frame)) 
		vortex_log (VORTEX_LEVEL_CRITICAL, "failed to update MIME status for the NUL frame");
	
	switch (vortex_frame_get_type (frame)) {
	case VORTEX_FRAME_TYPE_NUL:
		/* check zero payload size at NUL frame */
		if (vortex_frame_get_payload_size (frame) != 0) {
			__vortex_connection_shutdown_and_record_error (
				connection, VortexProtocolError,"Received \"NUL\" frame with  non-zero payload (%d) content (%d)",
				vortex_frame_get_payload_size (frame), vortex_frame_get_content_size (frame));
			return axl_false;
		}

		/* check more flag at NUL frame reply. */
		if (vortex_frame_get_more_flag (frame)) {
			__vortex_connection_shutdown_and_record_error (
				connection, VortexProtocolError, "Received \"NUL\" frame with the continuator indicator: *");
			return axl_false;
		}
		
		break;
	default:
		return axl_true;
	}
	return axl_true;
}

/** 
 * @internal
 *
 * Updates current window size to track SEQ frames status. This
 * function not only updates current incoming buffer status but also
 * reports a new SEQ frame to notify remote side that the incoming
 * buffer have changed.
 * 
 * @param channel The channel where the incoming buffer change was detected.
 *
 * @param frame The frame that contains application data that is
 * meaningful to perform a right SEQ frame counting.
 */
axl_bool      __vortex_reader_update_incoming_buffer_and_notify (VortexCtx         * ctx,
								 VortexConnection  * connection,
								 VortexChannel     * channel,
								 VortexFrame       * frame)
{


	VortexChannel     * channel0;
	unsigned int        ackno;
	int                 window;
	VortexWriterData    writer;

	/* now, we have to update current incoming max seq no allowed
	 * for future checkings on this channel and to generate a SEQ
	 * frame signaling this state to the remote peer. */
	switch (vortex_frame_get_type (frame)) {
	case VORTEX_FRAME_TYPE_MSG:
	case VORTEX_FRAME_TYPE_ANS:
	case VORTEX_FRAME_TYPE_RPY:
	case VORTEX_FRAME_TYPE_ERR:
	case VORTEX_FRAME_TYPE_NUL:
		/* only perform this task for those frames that have
		 * actually payload content to report and the current
		 * window size have changed. */
		if (vortex_channel_update_incoming_buffer (channel, frame, &ackno, &window)) {
			/* It seems there are something to report.
			 * 
			 * Now create a SEQ frame with higher priority to
			 * report remote side that the current max seq no
			 * value have been updated. */
			writer.type        = VORTEX_FRAME_TYPE_SEQ;
			writer.msg_no      = 0;
			writer.the_frame   = vortex_frame_seq_build_up_from_params_buffer (vortex_channel_get_number (channel),
											   ackno,
											   window,
											   ctx->reader_seq_frame,
											   /* the following size value is found at the 
											      reader_seq_frame declaration found at vortex_ctx_private.h */
											   50,
											   &(writer.the_size));
			/* writer.the_size    = strlen (writer.the_frame); */
			writer.is_complete = axl_true;
			vortex_log (VORTEX_LEVEL_DEBUG, "notifying remote side that current buffer status is %s",
				    writer.the_frame);
			/* Queue the vortex writer message to be sent with
			 * higher priority, currently, the following function
			 * will check if the packet to send is a SEQ frame to
			 * apply this priority. */
			channel0 = vortex_connection_get_channel (connection, 0);
			if ((channel0 == NULL) || ! vortex_sequencer_direct_send (connection, channel0, &writer)) {
				vortex_log (VORTEX_LEVEL_CRITICAL, "unable to queue a SEQ frame");
				/* deallocate memory on error, because no one
				 * will do it. */
				vortex_frame_unref (frame);
				return axl_false;
			}
			
		} /* end if */
		break;
	default:
		/* do nothing */
		break;
	}
	
	return axl_true;
}

/** 
 * @internal
 *
 * @brief Allows to check if the given channel is expecting to
 * received the message number that is carrying the frame.
 *
 * This function not only perform the checks required but also allows
 * to be compatible with non-conformance BEEP implementations that
 * uses as starting message number the value 1 rather than 0 for the
 * very first one, even not being the channel 0.
 * 
 * @param channel The channel where the message was received.
 * @param frame   The frame received that has to be checked.
 * 
 * @return axl_true if the reader check is passed. Otherwise axl_false is
 * returned.
 */
axl_bool      vortex_reader_check_incoming_msgno (VortexCtx        * ctx,
						  VortexConnection * conn,
						  VortexChannel    * channel, 
						  VortexFrame      * frame)
{
  	vortex_log (VORTEX_LEVEL_DEBUG, "about to check expected message to be received on this channel");

  
 	/* check if the message is not already available in the
 	 * list */
 	if (! vortex_channel_check_msg_no (channel, frame)) {
 		/* drop a log */
		__vortex_connection_shutdown_and_record_error (
			conn, VortexProtocolError,
			"Found incoming message number %d that represents a previous message received but still not replied, protocol violation",
			vortex_frame_get_msgno (frame));

		/* unref frame here */
		vortex_frame_unref (frame);
		
  		return axl_false;
 	} /* end if */
 
  	/* check passed */
  	return axl_true;
}

/** 
 * @internal
 * 
 * The main purpose of this function is to dispatch received frames
 * into the appropriate channel. It also makes all checks to ensure the
 * frame receive have all indicators (seqno, channel, message number,
 * payload size correctness,..) to ensure the channel receive correct
 * frames and filter those ones which have something wrong.
 *
 * This function also manage frame fragment joining. There are two
 * levels of frame fragment managed by the vortex reader.
 * 
 * We call the first level of fragment, the one described at RFC3080,
 * as the complete frame which belongs to a group of frames which
 * conform a message which was splitted due to channel window size
 * restrictions.
 *
 * The second level of fragment happens when the vortex reader receive
 * a frame header which describes a frame size payload to receive but
 * not all payload was actually received. This can happen because
 * vortex uses non-blocking socket configuration so it can avoid DOS
 * attack. But this behavior introduce the asynchronous problem of
 * reading at the moment where the whole frame was not received.  We
 * call to this internal frame fragmentation. It is also supported
 * without blocking to vortex reader.
 *
 * While reading this function, you have to think about it as a
 * function which is executed for only one frame, received inside only
 * one channel for the given connection.
 *
 * @param connection the connection which have something to be read
 * 
 **/
void __vortex_reader_process_socket (VortexCtx        * ctx, 
				     VortexConnection * connection)
{

	VortexFrame      * frame;
	VortexFrame      * previous;
	VortexFrameType    type;
	VortexChannel    * channel;
	axl_bool           more;
#if defined(ENABLE_VORTEX_LOG)
	char             * raw_frame;
	int                frame_id;
#endif

	vortex_log (VORTEX_LEVEL_DEBUG, "something to read conn-id=%d", vortex_connection_get_id (connection));

	/* check if there are pre read handler to be executed on this 
	   connection. */
	if (vortex_connection_is_defined_preread_handler (connection)) {
		/* if defined preread handler invoke it and return. */
		vortex_connection_invoke_preread_handler (connection);
		return;
	} /* end if */

	/* before doing anything, check if the connection is broken */
	if (! vortex_connection_is_ok (connection, axl_false))
		return;

	/* check for unwatch requests */
	if (connection->reader_unwatch)
		return;

	/* read all frames received from remote site */
	frame   = vortex_frame_get_next (connection);
	if (frame == NULL) 
		return;

	/* get frame type to avoid calling everytime to
	 * vortex_frame_get_type */
	type    = vortex_frame_get_type (frame);

	/* NOTE: After this point, frame received is
	 * complete. vortex_frame_get_next function takes cares about
	 * joining frame fragments. */

	/* check for debug to throw some debug messages.*/
#if defined(ENABLE_VORTEX_LOG)
	/* get frame id */ 
	frame_id = vortex_frame_get_id (frame);

	if (vortex_log2_is_enabled (ctx)) {
		raw_frame = vortex_frame_get_raw_frame (frame);
		vortex_log (VORTEX_LEVEL_DEBUG, "frame id=%d received (before all filters)\n%s",
			    frame_id, raw_frame);
		axl_free (raw_frame);
	}
#endif
	
	/* check if this connection is being initially accepted */
	if (connection->initial_accept) {
		/* it doesn't matter to have a connection accepted or
		 * not to the vortex reader's mission, so simply call
		 * second step accept and return.  */
		__vortex_listener_second_step_accept (frame, connection);
		return;
	}
	vortex_log (VORTEX_LEVEL_DEBUG, "passed frame id=%d initial accept stage", frame_id);

	/* channel exists, get a channel reference */
	channel = vortex_connection_get_channel (connection,
						 vortex_frame_get_channel (frame));
	if (channel == NULL) {
		__vortex_connection_shutdown_and_record_error (
			connection, VortexProtocolError, "received a frame referring to a non-opened channel, closing session");
		/* free the frame */
		vortex_frame_unref (frame);
		return;		
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "passed frame id=%d connection id=%d existence stage", frame_id, vortex_connection_get_id (connection));
 	/* now update current incoming buffers to track SEQ frames */
 	if (! __vortex_reader_update_incoming_buffer_and_notify (ctx, connection, channel, frame)) {
 		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to notify SEQ channel status, connection broken or protocol violation");
 		return;
 	} /* end if */

	/* check message numbers and reply numbers sequencing */
	switch (type) {
	case VORTEX_FRAME_TYPE_MSG:
		/* MSG frame type: check if msgno is correct */
		if (!vortex_reader_check_incoming_msgno (ctx, connection, channel, frame))
			return;
		break;
	case VORTEX_FRAME_TYPE_RPY:
	case VORTEX_FRAME_TYPE_ERR:
	case VORTEX_FRAME_TYPE_ANS:
	case VORTEX_FRAME_TYPE_NUL:
		/* RPY or ERR frame type: check if reply is expected */
		if (vortex_channel_get_next_expected_reply_no (channel) != vortex_frame_get_msgno (frame)) {

			__vortex_connection_shutdown_and_record_error (
				connection, VortexProtocolError, "expected reply message %d but received %d",
				vortex_channel_get_next_expected_reply_no (channel),
				vortex_frame_get_msgno (frame));

			/* free the frame */
			vortex_frame_unref (frame);
			return;
		}
		break;
	case VORTEX_FRAME_TYPE_SEQ:
		/* manage incoming SEQ frames, check if the received
		 * ackno value is inside the seqno range for bytes
		 * already sent. */
		vortex_log (VORTEX_LEVEL_DEBUG, "received a SEQ frame: SEQ %d %u %d",
			    vortex_frame_get_channel (frame), vortex_frame_get_seqno (frame),
			    vortex_frame_get_payload_size (frame));

		/* update information about the buffer state of the
		 * remote peer for the given channel */
		vortex_channel_update_remote_incoming_buffer (channel, 
							      /* ackno */
							      vortex_frame_get_seqno (frame), 
							      /* window */
							      vortex_frame_get_content_size (frame)); 

		/* unref the SEQ frame because it is no longer
		 * needed */
		vortex_frame_unref (frame);

		/* signal vortex sequencer to start re-sequencing
		 * previous hold messages on the channel updated */
		vortex_sequencer_signal_update (channel, connection);
		return;
	default:
		/* do not make nothing (only checking msgno and
		 * rpyno) */
		break;
	}
	vortex_log (VORTEX_LEVEL_DEBUG, "passed frame id=%d message number checking stage", frame_id);

	/* Check next sequence number, this check is always applied,
	 * for SEQ frames received this case is not reached. */
	if (vortex_channel_get_next_expected_seq_no (channel) !=
	    vortex_frame_get_seqno (frame)) {

		/* drop a log */
		__vortex_connection_shutdown_and_record_error (
			connection, VortexProtocolError, "expected seq no %u for channel=%d in connection-id=%d, but received %u",
			vortex_channel_get_next_expected_seq_no (channel),
			vortex_channel_get_number (channel),
			vortex_connection_get_id (connection),
			vortex_frame_get_seqno (frame));

		/* free the frame */
		vortex_frame_unref (frame);
		return;
	}

	/* check other environment conditions */
	if ((type == VORTEX_FRAME_TYPE_NUL) && 
	    !__vortex_reader_process_socket_check_nul_frame (ctx, frame, connection)) {
		/* free the frame */
		vortex_frame_unref (frame);	
		return;
	}	

	/* update channel internal status for next incoming messages */
	switch (type) {
	case VORTEX_FRAME_TYPE_MSG:
		/* is a MSG frame type: update msgno and seqno */
		vortex_channel_update_status_received (channel, 
						       vortex_frame_get_content_size (frame), 
						       vortex_frame_get_msgno (frame),
						       vortex_frame_get_more_flag (frame) ?
						       UPDATE_SEQ_NO :
						       UPDATE_SEQ_NO | UPDATE_MSG_NO);
		break;
	case VORTEX_FRAME_TYPE_RPY:
	case VORTEX_FRAME_TYPE_ERR:
		/* flag the channel with reply_processed so, close
		 * channel process can avoid to close the
		 * channel. This is necessary because the channel close
		 * process can be waiting to received (and process)
		 * all replies for message sent to be able to close
		 * the channel.  
		 * 
		 * And we have to do this flagging before updating
		 * channel status because close process will compare
		 * messages sent to message replies receive. */
		vortex_channel_flag_reply_processed (channel, axl_false);
		

		/* is a ERR or RPY type: update rpy no and seqno */
		vortex_channel_update_status_received (channel, 
						       vortex_frame_get_content_size (frame), 
						       vortex_frame_get_msgno (frame),
						       UPDATE_SEQ_NO);

		/* remove the message replied from outstanding list */
		if (! vortex_frame_get_more_flag (frame))
			vortex_channel_remove_first_outstanding_msg_no (channel, vortex_frame_get_msgno (frame));

		break;
	case VORTEX_FRAME_TYPE_NUL:
		/* And we have to do this flagging before updating
		 * channel status because close process will compare
		 * messages sent to message replies receive. */
		vortex_channel_flag_reply_processed (channel, axl_false);

		/* is a ERR or RPY type: update rpy no and seqno */
		vortex_channel_update_status_received (channel, 
						       vortex_frame_get_content_size (frame), 
						       vortex_frame_get_msgno (frame),
						       UPDATE_SEQ_NO);

		/* remove the message replied from outstanding list */
		vortex_channel_remove_first_outstanding_msg_no (channel, vortex_frame_get_msgno (frame));
		
		break;
	case VORTEX_FRAME_TYPE_ANS:
		/* look above description */
		vortex_channel_flag_reply_processed (channel, axl_false);

		/* is not a MSG frame type: update seqno */
		vortex_channel_update_status_received (channel, 
						       vortex_frame_get_content_size (frame), 
						       0,
						       UPDATE_SEQ_NO);
		break;
	case VORTEX_FRAME_TYPE_UNKNOWN:
	default:
		/* nothing to do */
		break;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "passed channel update status due to frame id=%d received stage type=%d", frame_id, type);

	/* If we have a frame to be joined then threat it instead of
	 * invoke frame received handler. This is done by checking for
	 * more flag. */
	more = vortex_frame_get_more_flag (frame);

	/* check complete frames flags:
	 * If more is enabled, the frame signal that more(*) frames have to come, or
	 * The more(.) flag is not enabled but we have a previously stored frame and the channel
	 * have the complete_flag activated, that is, the last frame of a series of frames [****.]  */
	if (more) {
		/* get previous frame to perform check operations */
		previous = vortex_channel_get_previous_frame (channel);
		
		/* check if there are previous frame */
		if (previous == NULL) {
			/* no previous frame found, store if the
			 * complete flag is activated */
			if (vortex_channel_have_complete_flag (channel)) {
				vortex_log (VORTEX_LEVEL_DEBUG, "more flag on frame (%p) and complete flag detected on channel num=%d (%p), skipping to the next frame",
					    frame, vortex_channel_get_number (channel), frame);

				/* push the frame for later operation */
				vortex_channel_store_previous_frame (ctx, channel, frame);
				return;
			} /* end if */
		} else {
			/* we have a previous frame, check to store or
			 * deliver */
			if (! vortex_frame_are_joinable (previous, frame)) {
				__vortex_connection_shutdown_and_record_error (
					connection, VortexProtocolError, "frame fragment received is not valid, giving up for this session");
				vortex_frame_unref (frame);
				return;
			} /* end if */

			/* we don't have to check if the complete-flag
			 * is activated. Reached this point, it is
			 * already activated because a previous frame
			 * was found stored. Because the frame is
			 * joinable, store. */
			vortex_log (VORTEX_LEVEL_DEBUG, "more flag on frame (%p) and complete flag detected on channel num=%d (%p), skipping to the next frame",
				    frame, vortex_channel_get_number (channel), frame);
			
			/* push the frame for later operation */
			vortex_channel_store_previous_frame (ctx, channel, frame);
			return;
		} /* end if */

	} else {
		/* no more pending frames, check for stored frames
		 * previous */
		if (vortex_channel_have_previous_frame (channel)) {
			/* store the frame into the channel */
			vortex_channel_store_previous_frame (ctx, channel, frame);

			/* create one single frame with all stored frames */
			frame = vortex_channel_build_single_pending_frame (channel);

			vortex_log (VORTEX_LEVEL_DEBUG, "produced single consolidated frame id=%d due to complete flag enabled",
				    vortex_frame_get_id (frame));
#if defined(ENABLE_VORTEX_LOG)
			/* get frame id */
			frame_id = vortex_frame_get_id (frame);
#endif			
		} /* end if */
	} /* end if */

	vortex_log (VORTEX_LEVEL_DEBUG, "passed frame id=%d checking stage", frame_id);

 	/* if the frame is complete, apply mime processing */
 	if (vortex_frame_get_more_flag (frame) == 0 && type != VORTEX_FRAME_TYPE_NUL && vortex_channel_have_complete_flag (channel)) {
 		/* call to update frame MIME status */
 		if (! vortex_frame_mime_process (frame))
 			vortex_log (VORTEX_LEVEL_WARNING, "failed to update MIME status for the frame id=%d, continue delivery",
				    frame_id);
 	} 

	/* check for general frame received (channel != 0) */
	if (vortex_channel_get_number (channel) != 0 && 
	    vortex_reader_invoke_frame_received (ctx, connection, channel, frame)) {
		vortex_log (VORTEX_LEVEL_DEBUG, "frame id=%d delivered to global frame received handler", 
			    frame_id);
		return; /* frame was successfully delivered */
	}

	/* invoke frame received handler for second level and, if not
	 * defined, the first level handler */
	if (vortex_channel_invoke_received_handler (connection, channel, frame)) {
		vortex_log (VORTEX_LEVEL_DEBUG, "frame id=%d delivered on second (channel) level handler channel",
			    frame_id);
		return; /* frame was successfully delivered */
	}
	
	
	/* if not, try to deliver to first level. If second level
	 * invocation was ok, the frame have been freed. If not, the
	 * first level will do this */
	if (vortex_profiles_invoke_frame_received (vortex_channel_get_profile    (channel),
						   vortex_channel_get_number     (channel),
						   vortex_channel_get_connection (channel),
						   frame)) {
		vortex_log (VORTEX_LEVEL_DEBUG, "frame id=%d delivered on first (profile) level handler channel",
			    frame_id);
		return; /* frame was successfully delivered */
	}
	
	vortex_log (VORTEX_LEVEL_WARNING, 
		    "unable to deliver incoming frame id=%d, no first or second level handler defined, dropping frame",
		    frame_id);

	/* unable to deliver the frame, free it */
	vortex_frame_unref (frame);

	/* that's all I can do */
	return;
}

/** 
 * @internal 
 *
 * @brief Classify vortex reader items to be managed, that is,
 * connections or listeners.
 * 
 * @param data The internal vortex reader data to be managed.
 * 
 * @return axl_true if the item to be managed was clearly read or axl_false if
 * an error on registering the item was produced.
 */
axl_bool   vortex_reader_register_watch (VortexReaderData * data, axlList * con_list, axlList * srv_list)
{
	VortexConnection * connection;
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx        * ctx;
#endif

	/* get a reference to the connection (no matter if it is not
	 * defined) */
	connection = data->connection;
#if defined(ENABLE_VORTEX_LOG)
	ctx        = vortex_connection_get_ctx (connection);
#endif

	switch (data->type) {
	case CONNECTION:
		/* check the connection */
		if (!vortex_connection_is_ok (connection, axl_false)) {
			/* check if we can free this connection */
			vortex_connection_unref (connection, "vortex reader (watch)");
			vortex_log (VORTEX_LEVEL_DEBUG, "received a non-valid connection, ignoring it");

			/* release data */
			axl_free (data);
			return axl_false;
		}
			
		/* now we have a first connection, we can start to wait */
		vortex_log (VORTEX_LEVEL_DEBUG, "new connection (conn-id=%d) to be watched (%d)", 
			    vortex_connection_get_id (connection), vortex_connection_get_socket (connection));
		axl_list_append (con_list, connection);

		break;
	case LISTENER:
		vortex_log (VORTEX_LEVEL_DEBUG, "new listener connection to be watched (socket: %d --> %s:%s, conn-id: %d)",
			    vortex_connection_get_socket (connection), 
			    vortex_connection_get_host (connection), 
			    vortex_connection_get_port (connection),
			    vortex_connection_get_id (connection));
		axl_list_append (srv_list, connection);
		break;
	case TERMINATE:
	case IO_WAIT_CHANGED:
	case IO_WAIT_READY:
	case FOREACH:
		/* just unref vortex reader data */
		break;
	} /* end switch */
	
	axl_free (data);
	return axl_true;
}

/** 
 * @internal Vortex function to implement vortex reader I/O change.
 */
VortexReaderData * __vortex_reader_change_io_mech (VortexCtx        * ctx,
						   axlPointer       * on_reading, 
						   axlList          * con_list, 
						   axlList          * srv_list, 
						   VortexReaderData * data)
{
	/* get current context */
	VortexReaderData * result;

	vortex_log (VORTEX_LEVEL_DEBUG, "found I/O notification change");
	
	/* unref IO waiting object */
	vortex_io_waiting_invoke_destroy_fd_group (ctx, *on_reading); 
	*on_reading = NULL;
	
	/* notify preparation done and lock until new
	 * I/O is installed */
	vortex_log (VORTEX_LEVEL_DEBUG, "notify vortex reader preparation done");
	vortex_async_queue_push (ctx->reader_stopped, INT_TO_PTR(1));
	
	/* free data use the function that includes that knoledge */
	vortex_reader_register_watch (data, con_list, srv_list);
	
	/* lock */
	vortex_log (VORTEX_LEVEL_DEBUG, "lock until new API is installed");
	result = vortex_async_queue_pop (ctx->reader_queue);

	/* initialize the read set */
	vortex_log (VORTEX_LEVEL_DEBUG, "unlocked, creating new I/O mechanism used current API");
	*on_reading = vortex_io_waiting_invoke_create_fd_group (ctx, READ_OPERATIONS);

	return result;
}


/* do a foreach operation */
void vortex_reader_foreach_impl (VortexCtx        * ctx, 
				 axlList          * con_list, 
				 axlList          * srv_list, 
				 VortexReaderData * data)
{
	axlListCursor * cursor;

	vortex_log (VORTEX_LEVEL_DEBUG, "doing vortex reader foreach notification..");

	/* check for null function */
	if (data->func == NULL) 
		goto foreach_impl_notify;

	/* foreach the connection list */
	cursor = axl_list_cursor_new (con_list);
	while (axl_list_cursor_has_item (cursor)) {

		/* notify, if the connection is ok */
		if (vortex_connection_is_ok (axl_list_cursor_get (cursor), axl_false)) {
			data->func (axl_list_cursor_get (cursor), data->user_data);
		} /* end if */

		/* next cursor */
		axl_list_cursor_next (cursor);
	} /* end while */
	
	/* free cursor */
	axl_list_cursor_free (cursor);

	/* foreach the connection list */
	cursor = axl_list_cursor_new (srv_list);
	while (axl_list_cursor_has_item (cursor)) {
		/* notify, if the connection is ok */
		if (vortex_connection_is_ok (axl_list_cursor_get (cursor), axl_false)) {
			data->func (axl_list_cursor_get (cursor), data->user_data);
		} /* end if */

		/* next cursor */
		axl_list_cursor_next (cursor);
	} /* end while */

	/* free cursor */
	axl_list_cursor_free (cursor);

	/* notify that the foreach operation was completed */
 foreach_impl_notify:
	vortex_async_queue_push (data->notify, INT_TO_PTR (1));

	return;
}

/**
 * @internal Function that checks if there are a global frame received
 * handler configured on the context provided and, in the case it is
 * defined, the frame is delivered to that handler.
 *
 * @param ctx The context to check for a global frame received.
 *
 * @param connection The connection where the frame was received.
 *
 * @param channel The channel where the frame was received.
 *
 * @param frame The frame that was received.
 *
 * @return axl_true in the case the global frame received handler is
 * defined and the frame was delivered on it.
 */
axl_bool  vortex_reader_invoke_frame_received       (VortexCtx        * ctx,
						     VortexConnection * connection,
						     VortexChannel    * channel,
						     VortexFrame      * frame)
{
	/* check the reference and the handler */
	if (ctx == NULL || ctx->global_frame_received == NULL)
		return axl_false;
	
	/* no thread activation */
	ctx->global_frame_received (channel, connection, frame, ctx->global_frame_received_data);
	
	/* unref the frame here */
	vortex_frame_unref (frame);

	/* return delivered */
	return axl_true;
}

/** 
 * @internal
 * @brief Read the next item on the vortex reader to be processed
 * 
 * Once an item is read, it is check if something went wrong, in such
 * case the loop keeps on going.
 * 
 * The function also checks for terminating vortex reader loop by
 * looking for TERMINATE value into the data->type. In such case axl_false
 * is returned meaning that no further loop should be done by the
 * vortex reader.
 *
 * @return axl_true to keep vortex reader working, axl_false if vortex reader
 * should stop.
 */
axl_bool      vortex_reader_read_queue (VortexCtx  * ctx,
					axlList    * con_list, 
					axlList    * srv_list, 
					axlPointer * on_reading)
{
	/* get current context */
	VortexReaderData * data;
	int                should_continue;

	do {
		data            = vortex_async_queue_pop (ctx->reader_queue);

		/* check if we have to continue working */
		should_continue = (data->type != TERMINATE);

		/* check if the io/wait mech have changed */
		if (data->type == IO_WAIT_CHANGED) {
			/* change io mechanism */
			data = __vortex_reader_change_io_mech (ctx,
							       on_reading, 
							       con_list, 
							       srv_list, 
							       data);
		} else if (data->type == FOREACH) {
			/* do a foreach operation */
			vortex_reader_foreach_impl (ctx, con_list, srv_list, data);

		} /* end if */

	}while (!vortex_reader_register_watch (data, con_list, srv_list));

	return should_continue;
}

/** 
 * @internal Function used by the vortex reader main loop to check for
 * more connections to watch, to check if it has to terminate or to
 * check at run time the I/O waiting mechanism used.
 * 
 * @param con_list The set of connections already watched.
 *
 * @param srv_list The set of listener connections already watched.
 *
 * @param on_reading A reference to the I/O waiting object, in the
 * case the I/O waiting mechanism is changed.
 * 
 * @return axl_true to flag the process to continue working to to stop.
 */
axl_bool      vortex_reader_read_pending (VortexCtx  * ctx,
					  axlList    * con_list, 
					  axlList    * srv_list, 
					  axlPointer * on_reading)
{
	/* get current context */
	VortexReaderData * data;
	int                length;
	axl_bool           should_continue = axl_true;

	length = vortex_async_queue_length (ctx->reader_queue);
	while (length > 0) {
		length--;
		data            = vortex_async_queue_pop (ctx->reader_queue);

		vortex_log (VORTEX_LEVEL_DEBUG, "read pending type=%d",
			    data->type);

		/* check if we have to continue working */
		should_continue = (data->type != TERMINATE);

		/* check if the io/wait mech have changed */
		if (data->type == IO_WAIT_CHANGED) {
			/* change io mechanism */
			data = __vortex_reader_change_io_mech (ctx, on_reading, con_list, srv_list, data);

		} else if (data->type == FOREACH) {
			/* do a foreach operation */
			vortex_reader_foreach_impl (ctx, con_list, srv_list, data);

		} /* end if */

		/* watch the request received, maybe a connection or a
		 * vortex reader command to process  */
		vortex_reader_register_watch (data, con_list, srv_list);
		
	} /* end while */

	return should_continue;
}

/** 
 * @internal Auxiliar function that populates the reading set of file
 * descriptors (on_reading), returning the max fds.
 */
VORTEX_SOCKET __vortex_reader_build_set_to_watch_aux (VortexCtx     * ctx,
						      axlPointer      on_reading, 
						      axlListCursor * cursor, 
						      VORTEX_SOCKET   current_max)
{
	VORTEX_SOCKET      max_fds     = current_max;
	VORTEX_SOCKET      fds         = 0;
	VortexConnection * connection;
	long               time_stamp  = 0;

	/* get current time stamp if idle handler is defined */
	if (ctx->global_idle_handler)
		time_stamp = (long) time (NULL);
	
	axl_list_cursor_first (cursor);
	while (axl_list_cursor_has_item (cursor)) {

		/* get current connection */
		connection = axl_list_cursor_get (cursor);

		/* check for idle status */
		if (ctx->global_idle_handler)
			vortex_connection_check_idle_status (connection, ctx, time_stamp);

		/* check ok status */
		if (! vortex_connection_is_ok (connection, axl_false)) {

			/* FIRST: remove current cursor to ensure the
			 * connection is out of our handling before
			 * finishing the reference the reader owns */
			axl_list_cursor_unlink (cursor);

			/* connection isn't ok, unref it */
			vortex_connection_unref (connection, "vortex reader (build set)");

			continue;
		} /* end if */

		/* check if the connection must be unwatched */
		if (connection->reader_unwatch) {
			/* remove the unwatch flag from the connection */
			connection->reader_unwatch = axl_false;

			/* FIRST: remove current cursor to ensure the
			 * connection is out of our handling before
			 * finishing the reference the reader owns */
			axl_list_cursor_unlink (cursor);

			/* connection isn't ok, unref it */
			vortex_connection_unref (connection, "vortex reader (process: unwatch)");

			continue;
		} /* end if */

		/* check if the connection is blocked (no I/O read to
		 * perform on it) */
		if (vortex_connection_is_blocked (connection)) {
			/* vortex_log (VORTEX_LEVEL_DEBUG, "connection id=%d has I/O read blocked (vortex_connection_block)", 
			   vortex_connection_get_id (connection)); */
			/* get the next */
			axl_list_cursor_next (cursor);
			continue;
		} /* end if */

		/* get the socket to ge added and get its maximum
		 * value */
		fds        = vortex_connection_get_socket (connection);
		max_fds    = fds > max_fds ? fds: max_fds;

		/* add the socket descriptor into the given on reading
		 * group */
		if (! vortex_io_waiting_invoke_add_to_fd_group (ctx, fds, connection, on_reading)) {
			
			vortex_log (VORTEX_LEVEL_WARNING, 
				    "unable to add the connection to the vortex reader watching set. This could mean you did reach the I/O waiting mechanism limit.");

			/* FIRST: remove current cursor to ensure the
			 * connection is out of our handling before
			 * finishing the reference the reader owns */
			axl_list_cursor_unlink (cursor);

			/* set it as not connected */
			if (vortex_connection_is_ok (connection, axl_false))
				__vortex_connection_shutdown_and_record_error (connection, VortexError, "vortex reader (add fail)");
			vortex_connection_unref (connection, "vortex reader (add fail)");

			continue;
		} /* end if */

		/* get the next */
		axl_list_cursor_next (cursor);

	} /* end while */

	/* return maximum number for file descriptors */
	return max_fds;
	
} /* end __vortex_reader_build_set_to_watch_aux */

VORTEX_SOCKET   __vortex_reader_build_set_to_watch (VortexCtx     * ctx,
						    axlPointer      on_reading, 
						    axlListCursor * conn_cursor, 
						    axlListCursor * srv_cursor)
{

	VORTEX_SOCKET       max_fds     = 0;

	/* read server connections */
	max_fds = __vortex_reader_build_set_to_watch_aux (ctx, on_reading, srv_cursor, max_fds);

	/* read client connection list */
	max_fds = __vortex_reader_build_set_to_watch_aux (ctx, on_reading, conn_cursor, max_fds);

	/* return maximum number for file descriptors */
	return max_fds;
	
}

void __vortex_reader_check_connection_list (VortexCtx     * ctx,
					    axlPointer      on_reading, 
					    axlListCursor * conn_cursor, 
					    int             changed)
{

	VORTEX_SOCKET       fds        = 0;
	VortexConnection  * connection = NULL;
	int                 checked    = 0;

	/* check all connections */
	axl_list_cursor_first (conn_cursor);
	while (axl_list_cursor_has_item (conn_cursor)) {

		/* check changed */
		if (changed == checked)
			return;

		/* check if we have to keep on listening on this
		 * connection */
		connection = axl_list_cursor_get (conn_cursor);
		if (!vortex_connection_is_ok (connection, axl_false)) {
			/* FIRST: remove current cursor to ensure the
			 * connection is out of our handling before
			 * finishing the reference the reader owns */
			axl_list_cursor_unlink (conn_cursor);

			/* connection isn't ok, unref it */
			vortex_connection_unref (connection, "vortex reader (check list)");
			continue;
		}
		
		/* get the connection and socket. */
	        fds = vortex_connection_get_socket (connection);
		
		/* ask if this socket have changed */
		if (vortex_io_waiting_invoke_is_set_fd_group (ctx, fds, on_reading, ctx)) {

			/* call to process incoming data, activating
			 * all invocation code (first and second level
			 * handler) */
			__vortex_reader_process_socket (ctx, connection);

			/* update number of sockets checked */
			checked++;
		}

		/* get the next */
		axl_list_cursor_next (conn_cursor);

	} /* end for */

	return;
}

int  __vortex_reader_check_listener_list (VortexCtx     * ctx, 
					  axlPointer      on_reading, 
					  axlListCursor * srv_cursor, 
					  int             changed)
{

	int                fds      = 0;
	int                checked  = 0;
	VortexConnection * connection;

	/* check all listeners */
	axl_list_cursor_first (srv_cursor);
	while (axl_list_cursor_has_item (srv_cursor)) {

		/* get the connection */
		connection = axl_list_cursor_get (srv_cursor);

		if (!vortex_connection_is_ok (connection, axl_false)) {
			vortex_log (VORTEX_LEVEL_DEBUG, "vortex reader found listener id=%d not operational, unreference",
				    vortex_connection_get_id (connection));

			/* FIRST: remove current cursor to ensure the
			 * connection is out of our handling before
			 * finishing the reference the reader owns */
			axl_list_cursor_unlink (srv_cursor);

			/* connection isn't ok, unref it */
			vortex_connection_unref (connection, "vortex reader (process), listener closed");

			/* update checked connections */
			checked++;

			continue;
		} /* end if */
		
		/* get the connection and socket. */
		fds  = vortex_connection_get_socket (connection);

		/* check if the socket is activated */
		if (vortex_io_waiting_invoke_is_set_fd_group (ctx, fds, on_reading, ctx)) {
			/* init the listener incoming connection phase */
			vortex_log (VORTEX_LEVEL_DEBUG, "listener (%d) have requests, processing..", fds);
			vortex_listener_accept_connections (ctx, fds, connection);

			/* update checked connections */
			checked++;
		} /* end if */

		/* check to stop listener */
		if (checked == changed)
			return 0;

		/* get the next */
		axl_list_cursor_next (srv_cursor);
	}
	
	/* return remaining sockets active */
	return changed - checked;
}

/** 
 * @internal
 *
 * @brief Internal function called to stop vortex reader and cleanup
 * memory used.
 * 
 */
void __vortex_reader_stop_process (VortexCtx     * ctx,
				   axlPointer      on_reading, 
				   axlListCursor * conn_cursor, 
				   axlListCursor * srv_cursor)

{
	/* stop vortex reader process unreferring already managed
	 * connections */

	vortex_async_queue_unref (ctx->reader_queue);

	/* unref listener connections */
	vortex_log (VORTEX_LEVEL_DEBUG, "cleaning pending %d listener connections..", axl_list_length (ctx->srv_list));
	ctx->srv_list = NULL;
	axl_list_free (axl_list_cursor_list (srv_cursor));
	axl_list_cursor_free (srv_cursor);

	/* unref initiators connections */
	vortex_log (VORTEX_LEVEL_DEBUG, "cleaning pending %d peer connections..", axl_list_length (ctx->conn_list));
	ctx->conn_list = NULL;
	axl_list_free (axl_list_cursor_list (conn_cursor));
	axl_list_cursor_free (conn_cursor);

	/* unref IO waiting object */
	vortex_io_waiting_invoke_destroy_fd_group (ctx, on_reading); 

	/* signal that the vortex reader process is stopped */
	QUEUE_PUSH (ctx->reader_stopped, INT_TO_PTR (1));

	return;
}

void __vortex_reader_close_connection (axlPointer pointer)
{
	VortexConnection * conn = pointer;

	/* unref the connection */
	vortex_connection_shutdown (conn);
	vortex_connection_unref (conn, "vortex reader");

	return;
}

/** 
 * @internal Dispatch function used to process all sockets that have
 * changed.
 * 
 * @param fds The socket that have changed.
 * @param wait_to The purpose that was configured for the file set.
 * @param connection The connection that is notified for changes.
 */
void __vortex_reader_dispatch_connection (int                  fds,
					  VortexIoWaitingFor   wait_to,
					  VortexConnection   * connection,
					  axlPointer           user_data)
{
	/* cast the reference */
	VortexCtx * ctx = user_data;

	switch (vortex_connection_get_role (connection)) {
	case VortexRoleMasterListener:
		/* check if there are pre read handler to be executed on this 
		   connection. */
		if (vortex_connection_is_defined_preread_handler (connection)) {
			/* if defined preread handler invoke it and return. */
			vortex_connection_invoke_preread_handler (connection);
			return;
		} /* end if */

		/* listener connections */
		vortex_listener_accept_connections (ctx, fds, connection);
		break;
	default:
		/* call to process incoming data, activating all
		 * invocation code (first and second level handler) */
		__vortex_reader_process_socket (ctx, connection);
		break;
	} /* end if */
	return;
}

axl_bool __vortex_reader_detect_and_cleanup_connection (axlListCursor * cursor) 
{
	VortexConnection * conn;
	char               bytes[4];
	int                result;
	int                fds;
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx        * ctx;
#endif
	
	/* get connection from cursor */
	conn = axl_list_cursor_get (cursor);
	if (vortex_connection_is_ok (conn, axl_false)) {

		/* get the connection and socket. */
		fds    = vortex_connection_get_socket (conn);
#if defined(AXL_OS_UNIX)
		errno  = 0;
#endif
		result = recv (fds, bytes, 1, MSG_PEEK);
		if (result == -1 && errno == EBADF) {
			  
			/* get context */
#if defined(ENABLE_VORTEX_LOG)
			ctx = CONN_CTX (conn);
#endif
			vortex_log (VORTEX_LEVEL_CRITICAL, "Found connection-id=%d, with session=%d not working (errno=%d), shutting down",
				    vortex_connection_get_id (conn), fds, errno);
			/* close connection, but remove the socket reference to avoid closing some's socket */
			conn->session = -1;
			vortex_connection_shutdown (conn);
			
			/* connection isn't ok, unref it */
			vortex_connection_unref (conn, "vortex reader (process), wrong socket");
			axl_list_cursor_unlink (cursor);
			return axl_false;
		} /* end if */
	} /* end if */
	
	return axl_true;
}

void __vortex_reader_detect_and_cleanup_connections (VortexCtx * ctx)
{
	/* check all listeners */
	axl_list_cursor_first (ctx->conn_cursor);
	while (axl_list_cursor_has_item (ctx->conn_cursor)) {

		/* get the connection */
		if (! __vortex_reader_detect_and_cleanup_connection (ctx->conn_cursor))
			continue;

		/* get the next */
		axl_list_cursor_next (ctx->conn_cursor);
	} /* end while */

	/* check all listeners */
	axl_list_cursor_first (ctx->srv_cursor);
	while (axl_list_cursor_has_item (ctx->srv_cursor)) {

	  /* get the connection */
	  if (! __vortex_reader_detect_and_cleanup_connection (ctx->srv_cursor))
		   continue; 

	    /* get the next */
	    axl_list_cursor_next (ctx->srv_cursor); 
	} /* end while */

	/* clear errno after cleaning descriptors */
#if defined(AXL_OS_UNIX)
	errno = 0;
#endif

	return; 
}

axlPointer __vortex_reader_run (VortexCtx * ctx)
{
	VORTEX_SOCKET      max_fds     = 0;
	VORTEX_SOCKET      result;
	int                error_tries = 0;

	/* initialize the read set */
	if (ctx->on_reading != NULL)
		vortex_io_waiting_invoke_destroy_fd_group (ctx, ctx->on_reading);
	ctx->on_reading  = vortex_io_waiting_invoke_create_fd_group (ctx, READ_OPERATIONS);

	/* create lists */
	ctx->conn_list = axl_list_new (axl_list_always_return_1, __vortex_reader_close_connection);
	ctx->srv_list = axl_list_new (axl_list_always_return_1, __vortex_reader_close_connection);

	/* create cursors */
	ctx->conn_cursor = axl_list_cursor_new (ctx->conn_list);
	ctx->srv_cursor = axl_list_cursor_new (ctx->srv_list);

	/* first step. Waiting blocked for our first connection to
	 * listen */
 __vortex_reader_run_first_connection:
	if (!vortex_reader_read_queue (ctx, ctx->conn_list, ctx->srv_list, &(ctx->on_reading))) {
		/* seems that the vortex reader main loop should
		 * stop */
		__vortex_reader_stop_process (ctx, ctx->on_reading, ctx->conn_cursor, ctx->srv_cursor);
		return NULL;
	}

	while (axl_true) {
		/* reset descriptor set */
		vortex_io_waiting_invoke_clear_fd_group (ctx, ctx->on_reading);

		if ((axl_list_length (ctx->conn_list) == 0) && (axl_list_length (ctx->srv_list) == 0)) {
			/* check if we have to terminate the process
			 * in the case no more connections are
			 * available: useful when the current instance
			 * is running in the context of turbulence */
			vortex_ctx_check_on_finish (ctx);

			vortex_log (VORTEX_LEVEL_DEBUG, "no more connection to watch for, putting thread to sleep");
			goto __vortex_reader_run_first_connection;
		}

		/* build socket descriptor to be read */
		max_fds = __vortex_reader_build_set_to_watch (ctx, ctx->on_reading, ctx->conn_cursor, ctx->srv_cursor);
		if (errno == EBADF) {
			vortex_log (VORTEX_LEVEL_CRITICAL, "Found wrong file descriptor error...(max_fds=%d, errno=%d), cleaning", max_fds, errno);
			/* detect and cleanup wrong connections */
			__vortex_reader_detect_and_cleanup_connections (ctx);
			continue;
		} /* end if */
		
		/* perform IO blocking wait for read operation */
		result = vortex_io_waiting_invoke_wait (ctx, ctx->on_reading, max_fds, READ_OPERATIONS);

		/* do automatic thread pool resize here */
		__vortex_thread_pool_automatic_resize (ctx);  

		/* check for timeout error */
		if (result == -1 || result == -2)
			goto process_pending;

		/* check errors */
		if ((result < 0) && (errno != 0)) {

			error_tries++;
			if (error_tries == 2) {
				vortex_log (VORTEX_LEVEL_CRITICAL, 
					    "tries have been reached on reader, error was=(errno=%d): %s exiting..",
					    errno, vortex_errno_get_last_error ());
				return NULL;
			} /* end if */
			continue;
		} /* end if */

		/* check for fatal error */
		if (result == -3) {
			vortex_log (VORTEX_LEVEL_CRITICAL, "fatal error received from io-wait function, exiting from vortex reader process..");
			__vortex_reader_stop_process (ctx, ctx->on_reading, ctx->conn_cursor, ctx->srv_cursor);
			return NULL;
		}


		/* check for each listener */
		if (result > 0) {
			/* check if the mechanism have automatic
			 * dispatch */
			if (vortex_io_waiting_invoke_have_dispatch (ctx, ctx->on_reading)) {
				/* perform automatic dispatch,
				 * providing the dispatch function and
				 * the number of sockets changed */
				vortex_io_waiting_invoke_dispatch (ctx, ctx->on_reading, __vortex_reader_dispatch_connection, result, ctx);

			} else {
				/* call to check listener connections */
				result = __vortex_reader_check_listener_list (ctx, ctx->on_reading, ctx->srv_cursor, result);
			
				/* check for each connection to be watch is it have check */
				__vortex_reader_check_connection_list (ctx, ctx->on_reading, ctx->conn_cursor, result);
			} /* end if */
		}

		/* we have finished the connection dispatching, so
		 * read the pending queue elements to be watched */
		
		/* reset error tries */
	process_pending:
		error_tries = 0;

		/* read new connections to be managed */
		if (!vortex_reader_read_pending (ctx, ctx->conn_list, ctx->srv_list, &(ctx->on_reading))) {
			__vortex_reader_stop_process (ctx, ctx->on_reading, ctx->conn_cursor, ctx->srv_cursor);
			return NULL;
		}
	}
	return NULL;
}

/** 
 * @brief Function that returns the number of connections that are
 * currently watched by the reader.
 * @param ctx The context where the reader loop is located.
 * @return Number of connections watched. 
 */
int  vortex_reader_connections_watched         (VortexCtx        * ctx)
{
	if (ctx == NULL || ctx->conn_list == NULL || ctx->srv_list == NULL)
		return 0;
	
	/* return list */
	return axl_list_length (ctx->conn_list) + axl_list_length (ctx->srv_list);
}

/** 
 * @internal Function used to unwatch the provided connection from the
 * vortex reader loop.
 *
 * @param ctx The context where the unread operation will take place.
 * @param connection The connection where to be unwatched from the reader...
 */
void vortex_reader_unwatch_connection          (VortexCtx        * ctx,
						VortexConnection * connection)
{
	v_return_if_fail (ctx && connection);
	/* flag connection vortex reader unwatch */
	connection->reader_unwatch = axl_true;
	return;
}

/** 
 * @internal
 * 
 * Adds a new connection to be watched on vortex reader process. This
 * function is for internal vortex library use.
 **/
void vortex_reader_watch_connection (VortexCtx        * ctx,
				     VortexConnection * connection)
{
	/* get current context */
	VortexReaderData * data;

	v_return_if_fail (vortex_connection_is_ok (connection, axl_false));
	v_return_if_fail (ctx->reader_queue);

	if (!vortex_connection_set_nonblocking_socket (connection)) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to set non-blocking I/O operation, at connection registration, closing session");
 		return;
	}

	/* increase reference counting */
	if (! vortex_connection_ref (connection, "vortex reader (watch)")) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to increase connection reference count, dropping connection");
		return;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "Accepting conn-id=%d into reader queue %p, library status: %d", 
		    vortex_connection_get_id (connection),
		    ctx->reader_queue,
		    vortex_is_exiting (ctx));

	/* prepare data to be queued */
	data             = axl_new (VortexReaderData, 1);
	data->type       = CONNECTION;
	data->connection = connection;

	/* push data */
	QUEUE_PUSH (ctx->reader_queue, data);

	return;
}

/** 
 * @internal
 *
 * Install a new listener to watch for new incoming connections.
 **/
void vortex_reader_watch_listener   (VortexCtx        * ctx,
				     VortexConnection * listener)
{
	/* get current context */
	VortexReaderData * data;
	v_return_if_fail (listener > 0);
	
	/* prepare data to be queued */
	data             = axl_new (VortexReaderData, 1);
	data->type       = LISTENER;
	data->connection = listener;

	/* push data */
	QUEUE_PUSH (ctx->reader_queue, data);

	return;
}

/** 
 * @internal Function used to configure the connection so the next
 * call to terminate the list of connections will not close the
 * connection socket.
 */
axl_bool __vortex_reader_configure_conn (axlPointer ptr, axlPointer data)
{
	/* set the connection socket to be not closed */
	vortex_connection_set_close_socket ((VortexConnection *) ptr, axl_false);
	return axl_false; /* not found so all items are iterated */
}

/** 
 * @internal
 * 
 * Creates the reader thread process. It will be waiting for any
 * connection that have changed to read its connect and send it
 * appropriate channel reader.
 * 
 * @return The function returns axl_true if the vortex reader was started
 * properly, otherwise axl_false is returned.
 **/
axl_bool  vortex_reader_run (VortexCtx * ctx) 
{
	v_return_val_if_fail (ctx, axl_false);

	/* check connection list to be previously created to terminate
	   it without closing sockets associated to each connection */
	if (ctx->conn_list != NULL) {
		vortex_log (VORTEX_LEVEL_DEBUG, "releasing previous client connections, installed: %d",
			    axl_list_length (ctx->conn_list));
		ctx->reader_cleanup = axl_true;
		axl_list_lookup (ctx->conn_list, __vortex_reader_configure_conn, NULL);
		axl_list_cursor_free (ctx->conn_cursor);
		axl_list_free (ctx->conn_list);
		ctx->conn_list   = NULL;
		ctx->conn_cursor = NULL;
	} /* end if */
	if (ctx->srv_list != NULL) {
		vortex_log (VORTEX_LEVEL_DEBUG, "releasing previous listener connections, installed: %d",
			    axl_list_length (ctx->srv_list));
		ctx->reader_cleanup = axl_true;
		axl_list_lookup (ctx->srv_list, __vortex_reader_configure_conn, NULL);
		axl_list_cursor_free (ctx->srv_cursor);
		axl_list_free (ctx->srv_list);
		ctx->srv_list   = NULL;
		ctx->srv_cursor = NULL;
	} /* end if */

	/* clear reader cleanup flag */
	ctx->reader_cleanup = axl_false;

	/* reader_queue */
	if (ctx->reader_queue != NULL)
		vortex_async_queue_release (ctx->reader_queue);
	ctx->reader_queue   = vortex_async_queue_new ();

	/* reader stopped */
	if (ctx->reader_stopped != NULL) 
		vortex_async_queue_release (ctx->reader_stopped);
	ctx->reader_stopped = vortex_async_queue_new ();

	/* create the vortex reader main thread */
	if (! vortex_thread_create (&ctx->reader_thread, 
				    (VortexThreadFunc) __vortex_reader_run,
				    ctx,
				    VORTEX_THREAD_CONF_END)) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to start vortex reader loop");
		return axl_false;
	} /* end if */
	
	return axl_true;
}

/** 
 * @internal
 * @brief Cleanup vortex reader process.
 */
void vortex_reader_stop (VortexCtx * ctx)
{
	/* get current context */
	VortexReaderData * data;

	vortex_log (VORTEX_LEVEL_DEBUG, "stopping vortex reader ..");

	/* create a bacon to signal vortex reader that it should stop
	 * and unref resources */
	data       = axl_new (VortexReaderData, 1);
	data->type = TERMINATE;

	/* push data */
	vortex_log (VORTEX_LEVEL_DEBUG, "pushing data stop signal..");
	QUEUE_PUSH (ctx->reader_queue, data);
	vortex_log (VORTEX_LEVEL_DEBUG, "signal sent reader ..");

	/* waiting until the reader is stoped */
	vortex_log (VORTEX_LEVEL_DEBUG, "waiting vortex reader 60 seconds to stop");
	if (PTR_TO_INT (vortex_async_queue_timedpop (ctx->reader_stopped, 60000000))) {
		vortex_log (VORTEX_LEVEL_DEBUG, "vortex reader properly stopped, cleaning thread..");
		/* terminate thread */
		vortex_thread_destroy (&ctx->reader_thread, axl_false);

		/* clear queue */
		vortex_async_queue_unref (ctx->reader_stopped);
	} else {
		vortex_log (VORTEX_LEVEL_WARNING, "timeout while waiting vortex reader thread to stop..");
	}

	return;
}

/** 
 * @internal Allows to check notify vortex reader to stop its
 * processing and to change its I/O processing model. 
 * 
 * @return The function returns axl_true to notfy that the reader was
 * notified and axl_false if not. In the later case it means that the
 * reader is not running.
 */
axl_bool  vortex_reader_notify_change_io_api               (VortexCtx * ctx)
{
	VortexReaderData * data;

	/* check if the vortex reader is running */
	if (ctx == NULL || ctx->reader_queue == NULL)
		return axl_false;

	vortex_log (VORTEX_LEVEL_DEBUG, "stopping vortex reader due to a request for a I/O notify change...");

	/* create a bacon to signal vortex reader that it should stop
	 * and unref resources */
	data       = axl_new (VortexReaderData, 1);
	data->type = IO_WAIT_CHANGED;

	/* push data */
	vortex_log (VORTEX_LEVEL_DEBUG, "pushing signal to notify I/O change..");
	QUEUE_PUSH (ctx->reader_queue, data);

	/* waiting until the reader is stoped */
	vortex_async_queue_pop (ctx->reader_stopped);

	vortex_log (VORTEX_LEVEL_DEBUG, "done, now vortex reader will wait until the new API is installed..");

	return axl_true;
}

/** 
 * @internal Allows to notify vortex reader to continue with its
 * normal processing because the new I/O api have been installed.
 */
void vortex_reader_notify_change_done_io_api   (VortexCtx * ctx)
{
	VortexReaderData * data;

	/* create a bacon to signal vortex reader that it should stop
	 * and unref resources */
	data       = axl_new (VortexReaderData, 1);
	data->type = IO_WAIT_READY;

	/* push data */
	vortex_log (VORTEX_LEVEL_DEBUG, "pushing signal to notify I/O is ready..");
	QUEUE_PUSH (ctx->reader_queue, data);

	vortex_log (VORTEX_LEVEL_DEBUG, "notification done..");

	return;
}

/** 
 * @internal Function that allows to preform a foreach operation over
 * all connections handled by the vortex reader.
 * 
 * @param ctx The context where the operation will be implemented.
 *
 * @param func The function to execute on each connection. If the
 * function provided is NULL the call will produce to lock until the
 * reader tend the foreach, restarting the reader loop.
 *
 * @param user_data User data to be provided to the function.
 *
 * @return The function returns a reference to the queue that will be
 * used to notify the foreach operation finished.
 */
VortexAsyncQueue * vortex_reader_foreach                     (VortexCtx            * ctx,
							      VortexForeachFunc      func,
							      axlPointer             user_data)
{
	VortexReaderData * data;
	VortexAsyncQueue * queue;

	v_return_val_if_fail (ctx, NULL);

	/* queue an operation */
	data            = axl_new (VortexReaderData, 1);
	data->type      = FOREACH;
	data->func      = func;
	data->user_data = user_data;
	queue           = vortex_async_queue_new ();
	data->notify    = queue;
	
	/* queue the operation */
	vortex_log (VORTEX_LEVEL_DEBUG, "notify foreach reader operation..");
	QUEUE_PUSH (ctx->reader_queue, data);

	/* notification done */
	vortex_log (VORTEX_LEVEL_DEBUG, "finished foreach reader operation..");

	/* return a reference */
	return queue;
}

/** 
 * @internal Iterate over all connections currently stored on the
 * provided context associated to the vortex reader. This function is
 * only usable when the context is stopped.
 */
void               vortex_reader_foreach_offline (VortexCtx           * ctx,
						  VortexForeachFunc3    func,
						  axlPointer            user_data,
						  axlPointer            user_data2,
						  axlPointer            user_data3)
{
	/* first iterate over all client connextions */
	axl_list_cursor_first (ctx->conn_cursor);
	while (axl_list_cursor_has_item (ctx->conn_cursor)) {

		/* notify connection */
		func (axl_list_cursor_get (ctx->conn_cursor), user_data, user_data2, user_data3);

		/* next item */
		axl_list_cursor_next (ctx->conn_cursor);
	} /* end while */

	/* now iterate over all server connections */
	axl_list_cursor_first (ctx->srv_cursor);
	while (axl_list_cursor_has_item (ctx->srv_cursor)) {

		/* notify connection */
		func (axl_list_cursor_get (ctx->srv_cursor), user_data, user_data2, user_data3);

		/* next item */
		axl_list_cursor_next (ctx->srv_cursor);
	} /* end while */

	return;
}




/** 
 * @internal Allows to restart the vortex reader module, locking the
 * caller until the reader restart its loop.
 */
void vortex_reader_restart (VortexCtx * ctx)
{
	VortexAsyncQueue * queue;

	/* call to restart */
	queue = vortex_reader_foreach (ctx, NULL, NULL);
	vortex_async_queue_pop (queue);
	vortex_async_queue_unref (queue);
	return;
}

/* @} */
