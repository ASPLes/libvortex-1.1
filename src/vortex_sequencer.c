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

/* local include */
#include <vortex_ctx_private.h>
#include <vortex_payload_feeder_private.h>

#define LOG_DOMAIN "vortex-sequencer"

void __vortex_sequencer_channel_unref (axlPointer channel)
{
	vortex_channel_unref2 (channel, "sequencer");
	return;
}

/* note: look at vortex_ctx_private.h for VortexSequencerState
 * definition */
void vortex_sequencer_release_state (VortexSequencerState * state)
{
	vortex_mutex_destroy (&state->mutex);
	vortex_cond_destroy (&state->cond);

	axl_hash_cursor_free (state->ready_cursor);
	axl_hash_free (state->ready);

	axl_free (state);

	return;
}

/* creates a new vortex sequencer state */
VortexSequencerState * vortex_sequencer_create_state (void)
{
	VortexSequencerState * result;

	result          = axl_new (VortexSequencerState, 1);

	/* create hashes */
	result->ready   = axl_hash_new (axl_hash_int, axl_hash_equal_int);

	/* create cursors */
	result->ready_cursor   = axl_hash_cursor_new (result->ready);

	/* init mutex and cond */
	vortex_mutex_create (&result->mutex);
	vortex_cond_create (&result->cond);

	return result;
}

axl_bool vortex_sequencer_add_channel (VortexCtx * ctx, VortexSequencerData * data)
{
	VortexSequencerState * state;

	/* get state reference */
	state = ctx->sequencer_state;

	/* lock */
	vortex_mutex_lock (&state->mutex);

	/* check if the channel is already added */
	if (axl_hash_get (state->ready, data->channel)) {

		/* queue message into the channel's pending structure */
		vortex_channel_queue_pending_message (data->channel, data);

		vortex_mutex_unlock (&state->mutex);
		return axl_true;
	}

	/* seems channel is not added */
	if (! vortex_channel_is_stalled (data->channel)) {
		/* update channel reference (this reference is
		   associated to the channel used by the sequencer) */
		if (! vortex_channel_ref2 (data->channel, "sequencer")) {
			/* release data */
			vortex_payload_feeder_unref (data->feeder);
			axl_free (data->message);
			axl_free (data);

			vortex_log (VORTEX_LEVEL_CRITICAL, "Failed to acquire reference to queue channel into sequencer");
			vortex_mutex_unlock (&state->mutex);
			return axl_false;
		} /* end if */

		/* add channel */
		axl_hash_insert_full (state->ready, data->channel, (axlDestroyFunc) __vortex_sequencer_channel_unref, INT_TO_PTR (1), NULL);
	} /* end if */

	/* queue message into the channel's pending structure */
	vortex_channel_queue_pending_message (data->channel, data);

	/* unlock */
	vortex_mutex_unlock (&state->mutex);

	return axl_true;
}

void vortex_sequencer_signal (VortexCtx * ctx)
{
	if (ctx == NULL || ctx->sequencer_state == NULL)
		return;

	vortex_cond_signal (&ctx->sequencer_state->cond);

	return;
}

axl_bool vortex_sequencer_queue_data (VortexCtx * ctx, VortexSequencerData * data)
{
	axl_bool is_stalled;

	v_return_val_if_fail (data, axl_false);

	/* check state before handling this message with the sequencer */
	if (ctx->vortex_exit || ctx->sequencer_state == NULL || ctx->sequencer_state->exit) {
		vortex_payload_feeder_unref (data->feeder);
		axl_free (data->message);
		axl_free (data);
		return axl_false;
	}

	/* update channel reference (this reference is associated to
	   the data) */
	/* if (! vortex_channel_ref (data->channel)) {
		vortex_log (VORTEX_LEVEL_WARNING, "trying to queue a message to be sent over a channel not opened (vortex_channel_ref failed)");
		vortex_payload_feeder_unref (data->feeder);
		axl_free (data->message);
		axl_free (data);
		return axl_false;
		} */

	vortex_log2 (VORTEX_LEVEL_DEBUG, "new message to be sent: msgno %d, channel %d, conn-id=%d, is-stalled: %d (size: %d):\n%s",
		     data->msg_no, 
		     data->channel_num, 
		     vortex_connection_get_id (vortex_channel_get_connection (data->channel)),
		     vortex_channel_is_stalled (data->channel),
		     data->message_size,
		     data->message ? data->message : "**** empty message ****");

	/* get current is stalled status */
	is_stalled = vortex_channel_is_stalled (data->channel);

	/* add the channel to the sequencer structure */
	if (! vortex_sequencer_add_channel (ctx, data)) 
		return axl_false;

	/* signal sequencer (but only if the channel is not stalled) */
	if (! is_stalled)
		vortex_sequencer_signal (ctx);

	return axl_true;
}

/** 
 * @brief Allows to get current number of pending channel operations.
 *
 * @param ctx The context where the operation is being requested.
 *
 * @return Return the number of channels with pending operations or 0
 * if nothing is pending. If nothing is pending, it means vortex
 * sequencer module has nothing to send at this point. Function
 * returns -1 in the case or NULL pointer received.
 */
int      vortex_sequencer_channels_pending_ops     (VortexCtx           * ctx)
{

	VortexSequencerState * state;
	int                    result;

	if (ctx == NULL)
		return -1;

	/* get state reference */
	state = ctx->sequencer_state;

	/* lock */
	vortex_mutex_lock (&state->mutex);

	result = axl_hash_items (state->ready);

	/* unlock */
	vortex_mutex_unlock (&state->mutex);

	/* report number of items */
	return result;
	
}

#define CHECK_AND_INCREASE_BUFFER(size_to_copy, buffer, buffer_size) \
	do {								\
		if ((size_to_copy + 100) > buffer_size) {		\
			vortex_log (VORTEX_LEVEL_DEBUG, "Found vortex sequencer buffer size not enough for current send operation: (size to copy:%d) > (buffer_size:%d)", \
				    (size_to_copy + 100), buffer_size);	\
			if (buffer_size > 100) {			\
				while ((size_to_copy + 100) > buffer_size) { \
					/* update buffer size */	\
					buffer_size = (buffer_size - 100) * 2 + 100; \
				} /* end if */				\
			} else {					\
				buffer_size = size_to_copy + 100;	\
			}						\
			vortex_log (VORTEX_LEVEL_DEBUG, "Updated sequencer buffer size for send operation to: (size to copy:%d) <= (buffer_size:%d)", \
				    (size_to_copy + 100),  buffer_size); \
			/* free current buffer */			\
			axl_free (buffer);				\
			/* allocate new buffer */			\
			buffer      = axl_new (char, buffer_size);	\
		} /* end if */						\
	} while(0)

axl_bool vortex_sequencer_remove_message_sent (VortexCtx * ctx, VortexChannel * channel)
{
	VortexSequencerData * data;
	axl_bool              is_empty;

	/* get pending message */
	data  = vortex_channel_remove_pending_message (channel);
	if (data == NULL)
		return axl_true; /* is_empty=axl_true : notify caller
				    that this channel has nothing to
				    send anymore */

	/* get if the queue is empty */
	is_empty = (vortex_channel_next_pending_message (channel) == NULL);

	/* log here */
	vortex_log (VORTEX_LEVEL_DEBUG, "Last message on channel=%d (%p) was sent completly, pending messages: %d (%d)",
		    vortex_channel_get_number (channel), channel, vortex_channel_pending_messages (channel), is_empty);

	/* release a reference */
	/* vortex_channel_unref (channel); */

	/* release feeder */
	vortex_payload_feeder_unref (data->feeder);

	/* release data */
	axl_free (data->message);
	axl_free (data);

	return is_empty;
}

int vortex_sequencer_build_packet_to_send (VortexCtx           * ctx, 
					   VortexChannel       * channel, 
					   VortexConnection    * conn, 
					   VortexSequencerData * data, 
					   VortexWriterData    * packet)
{
 	int          size_to_copy        = 0;
 	unsigned int max_seq_no_accepted = vortex_channel_get_max_seq_no_remote_accepted (channel);
	char       * payload             = NULL;

	/* clear packet */
	memset (packet, 0, sizeof (VortexWriterData));

	/* flag as not complete until something different is set */
	packet->is_complete = axl_false;

	/* check particular case where an empty message is to be sent
	 * and the message is NUL */
	if (data->message_size == 0 && data->type == VORTEX_FRAME_TYPE_NUL)
		goto build_frame;
  
	/* calculate how many bytes to copy from the payload
	 * according to max_seq_no */
	size_to_copy = vortex_channel_get_next_frame_size (channel,  
							   data->first_seq_no, 
							   data->message_size, 
 							   max_seq_no_accepted);

	/* check that the next_frame_size do not report wrong values */
	if (size_to_copy > data->message_size || size_to_copy <= 0) {
		__vortex_connection_shutdown_and_record_error (
			conn, VortexProtocolError,
			"vortex_channel_get_next_frame_size is reporting wrong values (size to copy: %d > message size: %d), this will cause protocol failures...shutdown connection id=%d (channel stalled: %d)", 
			size_to_copy, data->message_size, 
			vortex_connection_get_id (conn), vortex_channel_is_stalled (channel));
		return 0;
	}

	/* check here if we have a feeder defined */
	if (data->feeder) {
		if (data->feeder->status == 0) {
			/* check and increase buffer */
			CHECK_AND_INCREASE_BUFFER (size_to_copy, ctx->sequencer_feeder_buffer, ctx->sequencer_feeder_buffer_size);

			/* get content available at this moment to be sent */
			size_to_copy = vortex_payload_feeder_get_content (data->feeder, size_to_copy, ctx->sequencer_feeder_buffer);
		} else {
			vortex_log (VORTEX_LEVEL_DEBUG, "feeder cancelled, close transfer status is: %d", data->feeder->close_transfer);
			if (! data->feeder->close_transfer) {
				/* also record current msgno to
				   continue in the future */
				data->feeder->msg_no = data->msg_no;

				/* remove queued request if found same pointer */
				if (data == vortex_channel_next_pending_message (channel)) {
					/* remove the queued request but ensure we are the only thread touching this */
					vortex_sequencer_remove_message_sent (ctx, channel);
				}

				/* signal caller to stop delivering this content */
				return -1;
			} /* end if */

			/* flag this feeder is about being cancelled/paused */
			size_to_copy = -1;
		} /* end if */
	} /* end if */
	
	vortex_log (VORTEX_LEVEL_DEBUG, "the channel=%d (on conn-id=%d) is not stalled, continue with sequencing, about to send (size_to_copy:%d) bytes as payload (buffer:%d)...",
 		    vortex_channel_get_number (channel), 
		    vortex_connection_get_id (vortex_channel_get_connection (channel)), size_to_copy, ctx->sequencer_send_buffer_size);
 	vortex_log (VORTEX_LEVEL_DEBUG, "channel remote max seq no accepted: %u (proposed: %u)...",
 		    vortex_channel_get_max_seq_no_remote_accepted (channel), max_seq_no_accepted);
	
	/* create the new package to be managed by the vortex writer */
	packet->msg_no = data->msg_no;
 
	if (size_to_copy > 0) {
		/* check if we have to realloc buffer */
		CHECK_AND_INCREASE_BUFFER (size_to_copy, ctx->sequencer_send_buffer, ctx->sequencer_send_buffer_size);
	}
	
	/* we have the payload on buffer */
build_frame:
	/* set datatype for this package */
	packet->type = data->type;

	/* report what we are going to sequence */
	vortex_log (VORTEX_LEVEL_DEBUG, "sequencing next message: type=%d, channel num=%d, msgno=%d, more=%d, next seq=%u size=%d ansno=%d",
		    data->type, data->channel_num, data->msg_no, !(data->message_size == size_to_copy), 
		    data->first_seq_no, size_to_copy, data->ansno);
	vortex_log (VORTEX_LEVEL_DEBUG, "                         message=%p, step=%u, message-size=%u",
		    data->message, data->step, data->message_size);

	/* point to payload */
	if (data->feeder) {
		payload = (size_to_copy > 0) ? ctx->sequencer_feeder_buffer : NULL;
	} else
		payload = (data->message != NULL) ? (data->message + data->step) : NULL;

	/* check if the packet is complete (either last frame or all
	 * the payload fits into a single frame */
	if (data->feeder) {
		/* prepare is complete flag */
		packet->is_complete = (data->feeder->status != 0 && data->feeder->close_transfer) ? axl_true : vortex_payload_feeder_is_finished (data->feeder);

		/* normalize size_to_copy to avoid the caller to skipp ending this empty frame */
		if (size_to_copy < 0)
			size_to_copy = 0;
	} else
		packet->is_complete = (size_to_copy == data->message_size);

	/* build frame */
	packet->the_frame = vortex_frame_build_up_from_params_s_buffer (
		data->type,        /* frame type to be created */
		data->channel_num, /* channel number the frame applies to */
		data->msg_no,      /* the message number */
		/* frame payload size to be created */
		!packet->is_complete || data->fixed_more, /* have more frames */
		data->first_seq_no,   /* sequence number for the frame to be created */
		/* size for the payload starting from previous sequence number */
		size_to_copy,
		/* an optional ansno value, only used for ANS frames */
		data->ansno,
		/* no mime configuration, already handled from vortex channel module */
		NULL, 
		/* no mime configuration for transfer encoding, 
		 * already handler by vortex channel module */
		NULL, 
		/* the frame payload itself */
		payload,
		/* calculated frame size */
		&(packet->the_size),
		/* buffer and its size */
		ctx->sequencer_send_buffer, ctx->sequencer_send_buffer_size);

	/* update fixed more flag on packet */
	packet->fixed_more = data->fixed_more;
	
	/* return size used from the entire message */
	return size_to_copy;
}

/** 
 * @internal Function that does a send round for a channel. The
 * function assumes the channel is not stalled (but can end stalled
 * after the function finished).
 *
 */ 
void __vortex_sequencer_do_send_round (VortexCtx * ctx, VortexChannel * channel, VortexConnection * conn, axl_bool * paused, axl_bool * complete)
{
	VortexSequencerData  * data;
#if defined(ENABLE_VORTEX_LOG)
	int                    message_size;
	int                    max_seq_no = 0;
#endif
	int                    size_to_copy;
	VortexWriterData       packet;

	/* get data from channel */
	data  = vortex_channel_next_pending_message (channel);
	if (data == NULL) {
		/* no pending message on this channel */
		vortex_log (VORTEX_LEVEL_DEBUG, "no data were found to sequence on this channel, remove channel");
		return;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "a new message to be sequenced: (conn-id=%d, channel=%d, size=%d)..",
		    vortex_connection_get_id (conn), data->channel_num, data->message_size);
	
	/* if feeder is defined, get pending message size */
	if (data->feeder)
		data->message_size = vortex_payload_feeder_get_pending_size (data->feeder);

	/* refresh all sending data */
	data->first_seq_no     = vortex_channel_get_next_seq_no (channel);
#if defined(ENABLE_VORTEX_LOG)
	message_size           = data->message_size;
	max_seq_no             = vortex_channel_get_max_seq_no_remote_accepted (channel);
#endif

	vortex_log (VORTEX_LEVEL_DEBUG, "sequence operation (%p): type=%d, msgno=%d, next seq no=%u message size=%d max seq no=%u step=%u",
		    data, data->type, data->msg_no, data->first_seq_no, message_size, max_seq_no, data->step);
  		
	/* build the packet to send */
	size_to_copy = vortex_sequencer_build_packet_to_send (ctx, channel, conn, data, &packet);
	*complete    = packet.is_complete;

	/* check if the transfer is cancelled or paused */
	if (size_to_copy < 0) {
		*paused = axl_true;
		return;
	}
			
	/* STEP 1: now queue the rest of the message if it wasn't
	 * completly sequence. We do this before sending the frame to
	 * avoid a possible race condition between sending the message
	 * sequenced, be processed at the remote side, the remote side
	 * generate a SEQ frame, send it, and our vortex reader
	 * process it, finding that there is not pending, not yet
	 * completely sequenced message.
	 *
	 * Incredible, but true!
	 */
	if (! packet.is_complete) {
		
		/* well, it seems we didn't be able to send the hole
		 * message, so there are some remaining bytes. Just
		 * keep track about how many bytes we have sent. We
		 * will used this information once the message is
		 * enabled to be sent again on the next SEQ
		 * received. */
		data->step          = data->step + size_to_copy;
		/* make next seq no value to point to the next byte to send */
		data->first_seq_no  = (data->first_seq_no + size_to_copy) % (MAX_SEQ_NO);
		/* make message size to be decreased the amount of bytes sent. */
		data->message_size  = data->message_size - size_to_copy;

		vortex_log (VORTEX_LEVEL_DEBUG, 
			    "updating message sequencing status: next seq no=%u max seq no accepted=%u message size=%d step=%u",
			    data->first_seq_no, max_seq_no, data->message_size, data->step);

		vortex_log (VORTEX_LEVEL_DEBUG, "the message sequenced is not going to be sent completely (%d != %d)", 
			    size_to_copy, message_size);

	} /* end if */

	/* because we have sent the message, update remote seqno buffer used */
	vortex_channel_update_status (channel, size_to_copy, 0, UPDATE_SEQ_NO);
		
	/* STEP 2: now, send the package built, queueing it at the
	 * channel queue. At this point, we have prepared the rest to
	 * be sequenced message. */
	/* now, perform a send operation for the frame built */
	vortex_log (VORTEX_LEVEL_DEBUG, "frame built, send the frame directly (over channel=%d, conn-id=%d)",
		    vortex_channel_get_number (channel), vortex_connection_get_id (conn));

	if (! vortex_sequencer_direct_send (conn, channel, &packet)) {
		vortex_log (VORTEX_LEVEL_WARNING, "unable to send data at this moment");
		return;
	}

	/* that's all vortex sequencer process can do */
	return;
}

void vortex_sequencer_process_channels (VortexCtx * ctx, VortexSequencerState * state, axl_bool process_channel_0)
{
	axl_bool               paused;
	axl_bool               complete;
	axl_bool               is_empty;
	axl_bool               is_stalled;	
	VortexChannel        * channel          = NULL;
	VortexConnection     * conn             = NULL;

	/* now iterate all ready channels */
	axl_hash_cursor_first (state->ready_cursor);
	while (axl_hash_cursor_has_item (state->ready_cursor)) {
		
		/* get the channel and manage it, unlocking
		 * during the process */
		channel = axl_hash_cursor_get_key (state->ready_cursor);
		
		/* check for remove flag */
		if (PTR_TO_INT (vortex_channel_get_data (channel, "vo:seq:del"))) {
			axl_hash_cursor_remove (state->ready_cursor);
			continue;
		}
		
		/* check if channel is stalled */
		if (vortex_channel_is_stalled (channel)) {
			vortex_log (VORTEX_LEVEL_DEBUG, "channel=%d (%p) is stalled, removing from ready set",
				    vortex_channel_get_number (channel), channel);
			axl_hash_cursor_remove (state->ready_cursor);
			continue;
		} /* end if */

		/* check what kind of channel is this to know if we
		 * have to process it */
		if (process_channel_0 && vortex_channel_get_number (channel) != 0) {
			axl_hash_cursor_next (state->ready_cursor);
			continue;
		}
		if (!process_channel_0 && vortex_channel_get_number (channel) == 0) {
			axl_hash_cursor_next (state->ready_cursor);
			continue;
		}

		/* get connection reference */
		conn = vortex_channel_get_connection (channel);
		
		/* acquire connection */
		if (! vortex_connection_ref (conn, "vortex-sequencer")) {
			vortex_log (VORTEX_LEVEL_CRITICAL, "Unable to acquire reference to the connection (%p) inside vortex sequencer to do sending round, dropping channel (%p)",
				    conn, channel);
			axl_hash_cursor_remove (state->ready_cursor);
			continue;
		} /* end if */

		vortex_log (VORTEX_LEVEL_DEBUG, "handling next send channel=%d (%p), conn-id=%d (%p)",
			    vortex_channel_get_number (channel), channel, vortex_connection_get_id (conn), conn);
		
		/* unlock and call */
		vortex_mutex_unlock (&state->mutex);
		
		/* call to do send operation */
		paused   = axl_false;
		complete = axl_false;
		is_empty = axl_false;
		__vortex_sequencer_do_send_round (ctx, channel, conn, &paused, &complete);
		
		vortex_log (VORTEX_LEVEL_DEBUG, "it seems the message was sent completely over conn-id=%d, channel=%d (%p)",
			    vortex_connection_get_id (conn), vortex_channel_get_number (channel), channel);
		
		/* release connection (unlock mutex to allow
		 * connection close process to reenter into
		 * sequencer module) */
		vortex_connection_unref (conn, "vortex-sequencer");
		
		vortex_mutex_lock (&state->mutex);
		
		/* remove message sent */
		is_stalled = vortex_channel_is_stalled (channel);
		if (complete) {
			/* remove message sent */
			is_empty = vortex_sequencer_remove_message_sent (ctx, channel);
		} /* end if */
		
		/* check for remove flag */
		if (PTR_TO_INT (vortex_channel_get_data (channel, "vo:seq:del"))) {
			axl_hash_cursor_remove (state->ready_cursor);
			continue;
		}
		
		/* check channel after send operation */
		if (is_empty) {
			vortex_log (VORTEX_LEVEL_DEBUG, "Channel %p is empty (no more pending messages), removing from sequencer", channel);
			/* no more send operations, remove channel from our registry but check
			 * first it wasn't removed during the unlock  */ 				
			axl_hash_cursor_remove (state->ready_cursor);
			continue;
		}
		
		/* now check stalled channel */
		if (is_stalled || paused) {
			vortex_log (VORTEX_LEVEL_DEBUG, "Channel %p is stalled or paused, removing from sequencer", channel);
			/* no more send operations, remove channel from our registry but check
			 * first it wasn't removed during the unlock  */ 				
			axl_hash_cursor_remove (state->ready_cursor);
			continue;
		} /* end if */
		
		/* next item */
		axl_hash_cursor_next (state->ready_cursor);
		
	} /* end while */

	return;
}
	
axlPointer __vortex_sequencer_run (axlPointer _data)
{
	/* get current context */
	VortexCtx            * ctx = _data;
	VortexSequencerState * state;

	/* get the state */
	state = ctx->sequencer_state;

	/* lock mutex (acquire) */
	vortex_mutex_lock (&state->mutex);

	while (axl_true) {
		/* block until receive a new message to be sent (but
		 * only if there are no ready events) */
		vortex_log (VORTEX_LEVEL_DEBUG, "sequencer locking (ready channels: %d, exit: %d)",
			    axl_hash_items (state->ready), state->exit);
		while ((axl_hash_items (state->ready) == 0) && (! state->exit )) {
			vortex_cond_timedwait (&state->cond, &state->mutex, 10000);
		} /* end if */

		vortex_log (VORTEX_LEVEL_DEBUG, "sequencer unlocked (ready channels: %d, exit: %d)",
			    axl_hash_items (state->ready), state->exit);

		/* check if it was requested to stop the vortex
		 * sequencer operation */
		if (state->exit) {

			/* release unlock now we are finishing */
			vortex_mutex_unlock (&state->mutex);
			
			vortex_log (VORTEX_LEVEL_DEBUG, "exiting vortex sequencer thread ..");

			/* release reference acquired here */
			vortex_ctx_unref (&ctx);

			return NULL;
		} /* end if */

		/* process all ready administrative channels (channel 0) */
		vortex_sequencer_process_channels (ctx, state, axl_true);

		/* now process the rest */
		vortex_sequencer_process_channels (ctx, state, axl_false);
		
	} /* end while */

	/* never reached */
	return NULL;
}

/** 
 * @internal
 * 
 * Starts the vortex sequencer process. This process with the vortex
 * writer process conforms the subsystem which actually send a message
 * inside vortex. While vortex reader process threats all incoming
 * message and dispatch them to appropriate destination, the vortex
 * sequencer waits for messages to be send over channels.
 *
 * Once the vortex sequencer receive a petition to send a message, it
 * checks if actual channel window size is appropriate to actual
 * message being sent. If not, the vortex sequencer splits the message
 * into many pieces until all pieces can be sent using the actual
 * channel window size.
 *
 * Once the message is segmented, the vortex sequencer build up the
 * frames and enqueue the messages inside the channel waiting
 * queue. Once the sequencer have queue the first frame to be sent, it
 * signal the vortex writer to starting to send messages.
 *
 *
 * @return axl_true if the sequencer was init, otherwise axl_false is
 * returned.
 **/
axl_bool  vortex_sequencer_run (VortexCtx * ctx)
{

	v_return_val_if_fail (ctx, axl_false);

	/* sequencer queue where all data is received */
	if (ctx->sequencer_state != NULL)
		vortex_sequencer_release_state (ctx->sequencer_state);
	ctx->sequencer_state  = vortex_sequencer_create_state ();

	/* init sequencer buffer */
	ctx->sequencer_send_buffer_size = 4096 + 100;
	if (ctx->sequencer_send_buffer == NULL)
		ctx->sequencer_send_buffer = axl_new (char, ctx->sequencer_send_buffer_size);

	/* acquire a reference to the context to avoid loosing it
	 * during a log running sequencer not stopped */
	vortex_ctx_ref2 (ctx, "sequencer");

	/* starts the vortex sequencer */
	if (! vortex_thread_create (&ctx->sequencer_thread,
				    (VortexThreadFunc) __vortex_sequencer_run,
				    ctx,
				    VORTEX_THREAD_CONF_END)) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to initialize the sequencer thread");
		return axl_false;
	} /* end if */

	/* ok, sequencer initialized */
	return axl_true;
}

/** 
 * @internal
 * @brief Stop vortex sequencer process.
 */
void vortex_sequencer_stop (VortexCtx * ctx)
{
	VortexSequencerState * state;

	v_return_if_fail (ctx);

	/**
	 * Currently it does nothing. In the past was required. Due to
	 * API compat and because in the future the function could
	 * implement some deallocation code, it is not removed.
	 */
	vortex_log (VORTEX_LEVEL_DEBUG, "stopping vortex sequencer");

	/* get reference tot he sequencer state */
	state = ctx->sequencer_state;

	/* signal the sequencer to stop */
	vortex_mutex_lock (&state->mutex);
	state->exit = axl_true;
	vortex_cond_signal (&state->cond);
	vortex_mutex_unlock (&state->mutex);

	/* wait until the sequencer stops */
	vortex_log (VORTEX_LEVEL_DEBUG, "vortex sequencer properly stopped, cleaning thread..");
	vortex_thread_destroy  (&ctx->sequencer_thread, axl_false);

	/* release state */
	vortex_sequencer_release_state (state);
	ctx->sequencer_state = NULL;

	/* free sequencer buffer */
	axl_free (ctx->sequencer_send_buffer);
	axl_free (ctx->sequencer_feeder_buffer);

	return; 
}



axl_bool      vortex_sequencer_direct_send (VortexConnection    * connection,
					    VortexChannel       * channel,
					    VortexWriterData    * packet)
{
	/* reply number */
	axl_bool    result = axl_true;
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx * ctx    = vortex_connection_get_ctx (connection);
#endif

#if defined(ENABLE_VORTEX_LOG)
	/* send the frame */
	if (vortex_log2_is_enabled (ctx))
		vortex_log2 (VORTEX_LEVEL_DEBUG, "Sending message, size (%d) over channel=%d (%p), connection id=%d, errno=%d, Content: \n%s",
			     packet->the_size,  
			     vortex_channel_get_number (channel), channel, vortex_connection_get_id (connection), errno, packet->the_frame);
	else
		vortex_log (VORTEX_LEVEL_DEBUG, "Sending message, size (%d) over channel=%d (%p), connection id=%d, errno=%d",
			    packet->the_size,  
			    vortex_channel_get_number (channel), channel, vortex_connection_get_id (connection), errno);
#endif

	if (! vortex_frame_send_raw (connection, packet->the_frame, packet->the_size)) {
		/* drop a log */
		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to send frame over connection id=%d: errno=(%d): %s", 
			    vortex_connection_get_id (connection),
			    errno, vortex_errno_get_error (errno));
		
		/* set as non connected and flag the result */
		result = axl_false;
	}
	
	/* signal the message have been sent */
	if ((packet->type == VORTEX_FRAME_TYPE_RPY || packet->type == VORTEX_FRAME_TYPE_NUL) && packet->is_complete && ! packet->fixed_more) {  

		/* update reply sent */
		vortex_channel_update_status (channel, 0, packet->msg_no, UPDATE_RPY_NO_WRITTEN);

		/* unblock waiting thread for replies sent */
		vortex_channel_signal_reply_sent_on_close_blocked (channel);
		
		/* signal reply sent */
		vortex_channel_signal_rpy_sent (channel, packet->msg_no);
	}

	/* nothing more */
	return result;
}

/** 
 * @internal
 *
 * @brief Internal function that allows the vortex reader to
 * re-sequence messages that are hold due to channel being stalled
 * 
 * @param channel The channel to notify
 */
void     vortex_sequencer_signal_update        (VortexChannel       * channel,
						VortexConnection    * connection)
{
	/* get current context */
	VortexCtx            * ctx = vortex_channel_get_ctx (channel);
	VortexSequencerState * state;

	vortex_log (VORTEX_LEVEL_DEBUG, "about to check for pending queue messages=%d not sequenced for channel=%d (%p)",
		    vortex_channel_pending_messages (channel), vortex_channel_get_number (channel), channel);

	/* check if we have data to be resequenced */
	if (vortex_channel_next_pending_message (channel)) {

		/* get reference to the state */
		state = ctx->sequencer_state;

		/* move to ready */
		vortex_mutex_lock (&state->mutex);
		if (! axl_hash_get (state->ready, channel)) {
			/* update channel reference */
			if (! vortex_channel_ref2 (channel, "sequencer")) {
				vortex_log (VORTEX_LEVEL_CRITICAL, "unable to acquire channel %p reference, failed to to signal sequencer for SEQ frame update",
					    channel);
				vortex_mutex_unlock (&state->mutex);
				return;
			} /* end if */

			/* insert hash */
			axl_hash_insert_full (state->ready, channel, (axlDestroyFunc) __vortex_sequencer_channel_unref, INT_TO_PTR (1), NULL);
		} /* end if */
		/* signal */
		vortex_cond_signal (&state->cond);
		vortex_mutex_unlock (&state->mutex);

	}else {
		vortex_log (VORTEX_LEVEL_DEBUG, "there is no messages pending to be sequenced, over the channel=%d",
		       vortex_channel_get_number (channel));
	} /* end if */

	return;
}

/** 
 * @internal Function used to unregister the connection from the
 * sequencer if it is found.
 */
void     vortex_sequencer_remove_channel           (VortexCtx        * ctx,
						    VortexChannel    * channel)
{
	vortex_log (VORTEX_LEVEL_DEBUG, "removing channel %p from sequencer supervision", channel);
	vortex_channel_set_data (channel, "vo:seq:del", INT_TO_PTR (axl_true));

	return;
}

