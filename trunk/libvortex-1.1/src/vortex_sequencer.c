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
#include <vortex_payload_feeder_private.h>

#define LOG_DOMAIN "vortex-sequencer"

void vortex_sequencer_queue_data (VortexCtx * ctx, VortexSequencerData * data)
{
	v_return_if_fail (data);

	/* check not initialized queue */
	if (ctx->sequencer_queue == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "trying to queue a message to be sent, having the vortex sequencer stopped");
		axl_free (data->message);
		axl_free (data);
		return;
	} /* end if */

	vortex_log2 (VORTEX_LEVEL_DEBUG, "new message to be sent: msgno %d, channel %d (size: %d):\n%s",
		     data->msg_no, 
		     data->channel_num, 
		     data->message_size,
		     data->message ? data->message : "**** empty message ****");

	/* update channel reference */
	if (! vortex_channel_ref (data->channel)) {
		vortex_log (VORTEX_LEVEL_WARNING, "trying to queue a message to be sent over a channel not opened (vortex_channel_ref failed)");
		axl_free (data->message);
		axl_free (data);
		return;
	}

	/* get the connection for the data to be sent */
	data->conn = vortex_channel_get_connection (data->channel);

	/* queue data */
	QUEUE_PUSH (ctx->sequencer_queue, data);

	return;
}

void __vortex_sequencer_unref_and_clear (VortexConnection    * connection,
					 VortexSequencerData * data, 
					 axl_bool              unref)
{

	if (data != NULL) {

		/* free data no longer needed */
		axl_free (data->message); 
		data->message = NULL;

		/* unref channel */
		vortex_channel_unref (data->channel);
		data->channel = NULL;

		/* free feeder if defined */
		vortex_payload_feeder_free (data->feeder);
		data->feeder = NULL;
	
		/* free node */
		axl_free (data);

	} /* end if */

	/* Once done, release the reference from the vortex sequencer
	 * to the connection manipulated (if signaled). */
	if (unref) 
		vortex_connection_unref (connection, "(vortex sequencer)");
	
	return;
}


/** 
 * @internal Support function for the vortex sequencer loop. The
 * function is called when the message wasn't sent completely and some
 * status update is required in the message representation. It also
 * queues the pending message into the channel pending queue. The
 * function notifies the caller if he should keep on sending data.
 * 
 * @param data The message to be updated.
 *
 * @param resequence If we are on a resequence scenario.
 *
 * @param keep_on_sending A variable which is configured to signal if
 * the caller should keep on sending data or to stop.
 */
void vortex_sequencer_queue_message_and_update_status (VortexCtx            * ctx,
						       VortexSequencerData ** _data, 
						       unsigned int           size_to_copy,
						       unsigned int           max_seq_no,
						       axl_bool               resequence, 
						       int                  * keep_on_sending)
{
	VortexSequencerData * data = (*_data);

	/* well, it seems we didn't be able to send
	 * the hole message, so there are some
	 * remaining bytes. Just keep track about how
	 * many bytes we have sent. We will used this
	 * information once the message is enabled to
	 * be sent again on the next SEQ received. */
	data->step          = data->step         + size_to_copy;
	
	/* make next seq no value to point to the next byte to send */
	data->first_seq_no  = (data->first_seq_no + size_to_copy) % (MAX_SEQ_NO);
	
	/* make message size to be decreased the amount of bytes sent. */
	data->message_size  = data->message_size - size_to_copy;

	vortex_log (VORTEX_LEVEL_DEBUG, 
		    "updating message sequencing status: next seq no=%u max seq no accepted=%u message size=%d step=%u",
		    data->first_seq_no, max_seq_no, data->message_size, data->step);
	
	/* check if we can send more data */
	if ((data->first_seq_no < (max_seq_no - 60))) {
 		/* keep_on_sending variable is used to signal those
 		 * situations where a bigger message is pending to be
 		 * sent and the receiver window size is enough bigger
 		 * to support more content but, the frame about to be
 		 * sent (size_to_copy) is smaller. 
 		 * 
 		 * Think about sending frames of 2048 bytes (2K), that
 		 * taken together represents a message of 10240 bytes
 		 * (10K), with a remote receiver window of 8192
 		 * (8K). Under this situation it is required to keep
 		 * on sending to activate an efficient transfer. */
 
		vortex_log (VORTEX_LEVEL_DEBUG, "found space available to send more data, resequence.\n");
		(*keep_on_sending) = axl_true;

		/* return here to avoid nullfying the data
		 * reference */
		return;
	} else 
		(*keep_on_sending) = axl_false;


	/* do not queue the message again if we are
	 * re-sequencing, that is, sending pieces of a
	 * message that was queued because there were
	 * not enough buffer size at the remote peer
	 * for the given channel to hold it. */
	if (! resequence) {
		vortex_log (VORTEX_LEVEL_DEBUG, 
			    "because we are not re-sequencing, save the message to be pending");
		/* now queue the message to be pending to be sent. */
		vortex_channel_queue_pending_message (data->channel, data);
	}

	/* nullify sequencer data (the message) */
	(*_data) = NULL;

	return;
}


/** 
 * @internal Helper function used by the sequencer loop to check if a
 * channel is stalled and to store the message into the channel's
 * queue for future delivery.
 * 
 * @param data The message associated to a channel to be checked.
 * 
 * @return axl_true if the channel is stalled and the message was stored,
 * otherwise the loop can continue with the delivery of this message.
 */
axl_bool  vortex_sequencer_if_channel_stalled_queue_message (VortexCtx           * ctx,
							     VortexConnection    * connection, 
							     VortexSequencerData * data, 
							     axl_bool              resequence)
{
	/**    
	 * We need to check that remote buffer is willing to
	 * accept more data. On this context, the message
	 * received must be hold until a SEQ frame is received
	 * signaling that more bytes could be sent.
	 */
/*	vortex_log (VORTEX_LEVEL_DEBUG, "checking if channel is stalled: next seq no %u >= max seq no %u",
	data->first_seq_no, vortex_channel_get_max_seq_no_remote_accepted (data->channel));  */
	if (vortex_channel_is_stalled (data->channel)) {

		vortex_log (VORTEX_LEVEL_DEBUG, 
 			    "Seems that the next seq number to use is greater or equal than max seq no (%u >= %u), which means that this channel (%d) is stalled",
			    data->first_seq_no, 
			    vortex_channel_get_max_seq_no_remote_accepted (data->channel), 
			    vortex_channel_get_number (data->channel));
		/** 
		 * seems that this channel is stale, we can't
		 * do nothing more than queue the message and
		 * skip over to manage next message to
		 * sequence. But, do not queue anything if the
		 * message that is actually being checked is
		 * being managed as a resequence operation,
		 * which means that it is already queue.
		 */
		if (! resequence) {
			vortex_log (VORTEX_LEVEL_DEBUG, "queueing the message, because it is not a resequencing operation..");
			vortex_channel_queue_pending_message (data->channel, data);
			data = NULL;
		}
		
		/* unref the connection, we have previously
		 * increased the reference */
		vortex_connection_unref (connection, "(vortex sequencer)");
		return axl_true;
	}  /* end if */

	/* not stalled */
	return axl_false;
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

int vortex_sequencer_build_packet_to_send (VortexCtx * ctx, VortexChannel * channel, VortexSequencerData * data, VortexWriterData * packet)
{
 	int          size_to_copy;
 	unsigned int max_seq_no_accepted = vortex_channel_get_max_seq_no_remote_accepted (channel);
	char       * payload;
  
	/* calculate how many bytes to copy from the payload
	 * according to max_seq_no */
	size_to_copy = vortex_channel_get_next_frame_size (channel,  
							   data->first_seq_no, 
							   data->message_size, 
 							   max_seq_no_accepted);

	/* check that the next_frame_size do not report wrong values */
	if (size_to_copy > data->message_size || size_to_copy <= 0) {
		vortex_log (VORTEX_LEVEL_CRITICAL, 
			    "vortex_channel_get_next_frame_size is reporting wrong values (size to copy: %d > message size: %d), this will cause protocol failures...shutdown connection", 
			    size_to_copy, data->message_size);
		vortex_log (VORTEX_LEVEL_CRITICAL, "Context data: next seqno: %u, max seq no accepted: %u, channel is stalled: %d, message type: %d",
			    data->first_seq_no, max_seq_no_accepted, vortex_channel_is_stalled (channel), data->type);
		
		vortex_connection_shutdown (vortex_channel_get_connection (channel));
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
				if (data == vortex_channel_next_pending_message (channel))
					vortex_channel_remove_pending_message (channel);

				/* unref the connection and clear data (if
				 * user cancel feeder size_to_copy == -2) */
				__vortex_sequencer_unref_and_clear (vortex_channel_get_connection (channel), data, axl_true);

				/* signal caller to stop delivering this content */
				return -1;
			} /* end if */

			/* flag this feeder is about being cancelled/paused */
			size_to_copy = -1;
		} /* end if */
	} /* end if */
	
	vortex_log (VORTEX_LEVEL_DEBUG, "the channel is not stalled, continue with sequencing, about to send (size_to_copy:%d) bytes as payload (buffer:%d)...",
 		    size_to_copy, ctx->sequencer_send_buffer_size);
 	vortex_log (VORTEX_LEVEL_DEBUG, "channel remote max seq no accepted: %u (proposed: %u)...",
 		    vortex_channel_get_max_seq_no_remote_accepted (channel), max_seq_no_accepted);
	
	/* create the new package to be managed by the vortex writer */
	packet->type         = data->type;
	packet->msg_no       = data->msg_no;
 
	if (size_to_copy > 0) {
		/* check if we have to realloc buffer */
		CHECK_AND_INCREASE_BUFFER (size_to_copy, ctx->sequencer_send_buffer, ctx->sequencer_send_buffer_size);
	}
	
	/* we have the payload on buffer */
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
		!packet->is_complete, /* have more frames */
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
	
	/* return size used from the entire message */
	return size_to_copy;
}

/**
 * @internal Function that check if the channel have pending messages
 * to be sent. In that case, it queues current message handled and
 * updates the reference with the next message.
 */
axl_bool vortex_sequencer_previous_pending_messages (VortexCtx            * ctx,
						     VortexConnection     * conn,
						     VortexChannel        * channel, 
						     VortexSequencerData ** data, 
						     int                  * resequence)
{
	/* for the case it is a native delivery, not a
	 * resequencing operation, we have to check if
	 * there are pending messages to be sent for
	 * the selected channel, to avoid sending
	 * fresh packets */
	if (! vortex_channel_is_empty_pending_message (channel)) {
		
		/* seems there are pending messages to
		 * be sent, queue current message at
		 * the last position of the pending
		 * message queue and use the next
		 * first pending message */
		vortex_channel_queue_pending_message (channel, (*data));
		
		/* get a reference to the next pending message */
		(*data)      = vortex_channel_next_pending_message (channel);

		if ((*data) == NULL) {
			vortex_log (VORTEX_LEVEL_DEBUG, "signaling to stop processing for channel=%d, next pending message returned NULL",
				    vortex_channel_get_number (channel));

			/* unref the connection, we
			 * have previously increased
			 * the reference */
			vortex_connection_unref (conn, "(vortex sequencer)");

			return axl_false;
		} /* end if */
		
		/* stop if the channel is stalled */
		if (vortex_channel_is_stalled (channel)) {
			vortex_log (VORTEX_LEVEL_DEBUG, 
 				    "channel=%d is still stalled (seqno: %u, allowed seqno: %u), queued message for future delivery",
				    vortex_channel_get_number (channel), 
				    (*data)->first_seq_no, vortex_channel_get_max_seq_no_remote_accepted (channel));
			/* unref the connection, we
			 * have previously increased
			 * the reference */
			vortex_connection_unref (conn, "(vortex sequencer)");

			/* signal to stop processing */
			return axl_false;
		} /* end if */
		
		/* flag this attempt as a resequence
		 * to avoid adding twice the data
		 * item */
		(*resequence) = axl_true;
	} /* end if */
	
	/* continue processing */
	return axl_true;
}


/** 
 * @internal Function that checks the connection and channel
 * status. It also checks status for the data reference. If everything
 * is ok, connection and channel references are updated.
 */
axl_bool vortex_sequencer_check_status_and_get_references (VortexCtx             * ctx,
							   VortexSequencerData   * data, 
							   VortexConnection     ** connection, 
							   VortexChannel        ** channel)
{
	/* new message to be sent */
	(*channel) = data->channel;

	/* check to drop the sequence */
	if (data->discard) {
		vortex_log (VORTEX_LEVEL_WARNING, "discarding sequencer data (found flag activated)");
		__vortex_sequencer_unref_and_clear (NULL, data, axl_false);

		/* do not continue */
		return axl_false;
	} /* end if */

	/** 
	 * Increase reference counting for the connection holding the channel 
	 * to avoid race deallocation condition during the message fragmentation
	 * on the channel queue. connection ref must be performed before any other
	 * operation to avoid unrefering (vortex_connection_unref) a connection not 
	 * referred (vortex_connection_ref).
	 */
	(*connection) = data->conn;
	if ((*connection) != NULL && ! vortex_connection_ref ((*connection), "(vortex sequencer)")) {
		vortex_log (VORTEX_LEVEL_CRITICAL, 
			    "detectd message to be sequenced over a channel installed on a non connected session, dropping message");
		
		/* clear data without unrefering the connection */
		__vortex_sequencer_unref_and_clear (NULL, data, axl_false);
		return axl_false;
	}
	
	/* check if the channel exists on the connection */
	if (vortex_connection_get_channel ((*connection), data->channel_num) == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, 
			    "detect message to be sequenced over a channel not available at this point, dropping message");
		
		/* clear data, unrefering the connection (since it was
		 * referenced previously) */
		__vortex_sequencer_unref_and_clear ((*connection), data, axl_true);
		return axl_false;
	}
	
	/* check channel number for deliver */
	if (data->channel_num != vortex_channel_get_number ((*channel))) {
		vortex_log (VORTEX_LEVEL_CRITICAL, 
			    "channel number for frame sending mismatch, dropping this frame");

		/* unref the connection and clear data */
		__vortex_sequencer_unref_and_clear ((*connection), data, axl_true);
		return axl_false;
	}
	
	/* keep processing */
	return axl_true;
}
	
axlPointer __vortex_sequencer_run (axlPointer _data)
{
	/* get current context */
	VortexCtx           * ctx = _data;
	VortexSequencerData * data                   = NULL;
	VortexWriterData      packet;
	VortexChannel       * channel                = NULL;
	VortexConnection    * connection             = NULL;
	int                   message_size;
	int                   max_seq_no            = 0;
	int                   size_to_copy;
	axl_bool              keep_on_sending;
	axl_bool              resequence;
	VortexAsyncQueue    * queue;

	while (axl_true) {
		/* block until receive a new message to be sent */
		data    = vortex_async_queue_pop (ctx->sequencer_queue);

		/* check if it was requested to stop the vortex
		 * sequencer operation */
		if (data == INT_TO_PTR(1)) {
			vortex_log (VORTEX_LEVEL_DEBUG, "vortex sequencer stopped..");

			/* unref the queue */
			queue                  = ctx->sequencer_queue;
			ctx->sequencer_queue   = NULL;
			vortex_async_queue_unref (queue);

			/* signal the stop function that the vortex
			 * sequencer was stoped */
			QUEUE_PUSH (ctx->sequencer_stopped, INT_TO_PTR (1));
			return NULL;
		}
		
		vortex_log (VORTEX_LEVEL_DEBUG, "a new message to be sequenced: (channel=%d, size=%d, sequencer-queue=%d)..",
			    data->channel_num, data->message_size, vortex_async_queue_items (ctx->sequencer_queue));

 		/* check (data) connection and channel status, and if
 		 * they are ok, update connection and channel
 		 * references */
 		if (! vortex_sequencer_check_status_and_get_references (ctx, data, &connection, &channel))
  			continue;

 		/** 
		 * Check if we have received a resequenced signal from
		 * vortex reader, that is, to try to re-sequence
		 * previous pending messages because we have received
		 * a SEQ frame. 
		 */
		resequence = data->resequence;
		if (data->resequence) {
			vortex_log (VORTEX_LEVEL_DEBUG, "detected a re-sequencing operation..");

			/* free signal, and flag the variable to be
			 * NULL */
			axl_free (data);
			
		start_resequence:

			/* now get the very first pending message  */
			data       = vortex_channel_next_pending_message (channel);
			if (data == NULL) {
				vortex_log (VORTEX_LEVEL_DEBUG, "nothing to re-sequence, skipping..");
				
				/* unref the connection */
				__vortex_sequencer_unref_and_clear (connection, NULL, axl_true);

				continue;
			}

 			/* we have to flag the resequence variable this way because we can't use
 			 * data->resequence. This is because the ->resequence flag is only used by the
 			 * vortex reader to wakeup the vortex sequencer, but not by normal vortex
			 * sequencer packets (data variable). */
			resequence = axl_true;
		}else {
			/* check for previous messages and update
			 * (data) reference */
 			if (! vortex_sequencer_previous_pending_messages (ctx, connection, channel, &data, &resequence))
 				continue;
		} /* end if */

		/* create the frame (or frames to send) (splitter
		 * process) */
	keep_sending:
		/* if feeder is defined, get pending message size */
		if (data->feeder)
			data->message_size = vortex_payload_feeder_get_pending_size (data->feeder);

		/* refresh all sending data */
		data->first_seq_no     = vortex_channel_get_next_seq_no (channel);
		message_size           = data->message_size;
		max_seq_no             = vortex_channel_get_max_seq_no_remote_accepted (channel);
		keep_on_sending        = axl_false;

 		vortex_log (VORTEX_LEVEL_DEBUG, "sequence operation (%p): type=%d, msgno=%d, next seq no=%u message size=%d max seq no=%u step=%u",
 			    data, data->type, data->msg_no, data->first_seq_no, message_size, max_seq_no, data->step);
  		
 		/* check if the channel is stalled and queue the
 		 * message if it is found stalled */
 		if (vortex_sequencer_if_channel_stalled_queue_message (ctx, connection, data, resequence)) {
 			continue; /* no required to unref the
 				   * connection here since it is done
 				   * by previous function */
		}
		
 		/* build the packet to send */
 		size_to_copy = vortex_sequencer_build_packet_to_send (ctx, channel, data, &packet);

		/* check if the transfer is cancelled or paused */
		if (size_to_copy < 0)
			continue;
			
		/* STEP 1: now queue the rest of the message if it
		 * wasn't completly sequence. We do this before
		 * sending the frame to avoid a possible race
		 * condition between sending the message sequenced, be
		 * processed at the remote side, the remote side
		 * generate a SEQ frame, send it, and our vortex
		 * reader process it, finding that there is not
		 * pending, not yet completely sequenced message.
		 *
		 * Incredible, but true! 
		 */
		if (! packet.is_complete) {
			vortex_log (VORTEX_LEVEL_DEBUG, "the message sequenced is not going to be sent completely (%d != %d)", 
			       size_to_copy, message_size);

 			/* queue the rest of message still not sent
 			 * for later delivery; also update message
 			 * representation to signal the remaining
 			 * message content to be sent */
 			vortex_sequencer_queue_message_and_update_status (ctx, &data, 
 									  size_to_copy, max_seq_no, resequence, 
 									  &keep_on_sending);
 			/* watch keep_on_sending which is used
 			 * later... */

		} /* end if */

		/* because we have sent the message, update remote seqno buffer used */
		vortex_channel_update_status (channel, size_to_copy, 0, UPDATE_SEQ_NO);
		
		/* STEP 2: now, send the package built, queueing it at
		 * the channel queue. At this point, we have prepared
		 * the rest to be sequenced message. */
		/* now, perform a send operation for the frame built */
		vortex_log (VORTEX_LEVEL_DEBUG, "frame built, send the frame directly"); 
		if (! vortex_sequencer_direct_send (connection, channel, &packet)) {
			vortex_log (VORTEX_LEVEL_WARNING, "unable to send data at this moment");
			
			if (! resequence) {
				vortex_log (VORTEX_LEVEL_WARNING, "queuing data to be sent later because we aren't resending");
				
				/* queue data to be sent later */
				vortex_channel_queue_pending_message (channel, data);
			}
			
			/* in both cases nullify data variable
			 * because it has been queued into the
			 * channel pending queue (resequence =
			 * axl_false) or it is already stored on
			 * the channel pending queue */
			data = NULL;
			
			/* unref the connection and clear data */
			__vortex_sequencer_unref_and_clear (connection, data, axl_true);
			continue;
		}

		/* check for keep on sending flag activated */
		if (keep_on_sending)
			goto keep_sending;

		/** 
		 * STEP 3: now we have sent the last frame generated,
		 * we have to check if the message previously was
		 * completely sent so we could remove it from the
		 * pending messages to be resequenced, in the case we
		 * are doing so, try to keep on re-sequencing all
		 * messages pending until the current max seq no value
		 * gets exhausted.
		 */
		if (packet.is_complete) {
			vortex_log (VORTEX_LEVEL_DEBUG, "it seems the message was sent completely");
			/* the message was sent completely, check for
			 * re-sequence to remove the first pending
			 * message. */
			if (resequence) {
				/* only remove the first */
				vortex_channel_remove_pending_message (channel);

				/* check for previous messages that could be resequenced */
				if (vortex_channel_next_pending_message (channel) != NULL) {
					vortex_log (VORTEX_LEVEL_DEBUG, "seems that there are pending messages to be resequenced..");
					/* unref the connection and clear data */
					__vortex_sequencer_unref_and_clear (connection, data, axl_false);

					goto start_resequence;
				} /* end if */
			} /* end if */
		} /* end if */

		/* unref the connection and clear data */
		__vortex_sequencer_unref_and_clear (connection, data, axl_true); 
		
	} /* end while */
	
	/* that's all vortex sequencer process can do */
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
	if (ctx->sequencer_queue != NULL)
		vortex_async_queue_release (ctx->sequencer_queue);
	ctx->sequencer_queue  = vortex_async_queue_new ();

	/* auxiliar queue used to synchronize the vortex sequencing
	 * shutting down process */
	if (ctx->sequencer_stopped != NULL)
		vortex_async_queue_release (ctx->sequencer_stopped);
	ctx->sequencer_stopped      = vortex_async_queue_new ();

	/* init sequencer buffer */
	ctx->sequencer_send_buffer_size = 4096 + 100;
	if (ctx->sequencer_send_buffer == NULL)
		ctx->sequencer_send_buffer = axl_new (char, ctx->sequencer_send_buffer_size);

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
	v_return_if_fail (ctx);

	/**
	 * Currently it does nothing. In the past was required. Due to
	 * API compat and because in the future the function could
	 * implement some deallocation code, it is not removed.
	 */
	vortex_log (VORTEX_LEVEL_DEBUG, "stopping vortex sequencer");

	/* signal the sequencer to stop */
	QUEUE_PUSH  (ctx->sequencer_queue, INT_TO_PTR (1));

	/* wait until the sequencer stops */
	vortex_log (VORTEX_LEVEL_DEBUG, "waiting vortex sequencer 60 seconds to stop");
	if (PTR_TO_INT (vortex_async_queue_timedpop (ctx->sequencer_stopped, 60000000))) {
		vortex_log (VORTEX_LEVEL_DEBUG, "vortex sequencer properly stopped, cleaning thread..");
		vortex_thread_destroy       (&ctx->sequencer_thread, axl_false);

		vortex_log (VORTEX_LEVEL_DEBUG, "vortex sequencer completely stopped");
		vortex_async_queue_unref    (ctx->sequencer_stopped);
	} else {
		vortex_log (VORTEX_LEVEL_WARNING, "timeout while waiting vortex sequencer thread to stop..");
	}
	
	/* free sequencer buffer */
	axl_free (ctx->sequencer_send_buffer);
	axl_free (ctx->sequencer_feeder_buffer);

	return; 
}



axl_bool      vortex_sequencer_direct_send (VortexConnection * connection,
					    VortexChannel    * channel,
					    VortexWriterData * packet)
{
	/* reply number */
	axl_bool    result = axl_true;
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx * ctx    = vortex_connection_get_ctx (connection);
#endif

	/* reference the channel during the transmission */
	vortex_channel_ref (channel);

#if defined(ENABLE_VORTEX_LOG)
	/* send the frame */
	if (vortex_log2_is_enabled (ctx))
		vortex_log2 (VORTEX_LEVEL_DEBUG, "Sending message, size (%d) over connection id=%d, errno=%d, Content: \n%s",
			     packet->the_size,  
			     vortex_connection_get_id (connection), errno, packet->the_frame);
	else
		vortex_log (VORTEX_LEVEL_DEBUG, "Sending message, size (%d) over connection id=%d, errno=%d",
			    packet->the_size,  
			    vortex_connection_get_id (connection), errno);
#endif

	if (! vortex_frame_send_raw (connection, packet->the_frame, packet->the_size)) {
		/* drop a log */
		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to send the frame: errno=(%d): %s", 
			    errno, vortex_errno_get_error (errno));
		
		/* set as non connected and flag the result */
		result = axl_false;
	}
	
	/* signal the message have been sent */
	if ((packet->type == VORTEX_FRAME_TYPE_RPY || packet->type == VORTEX_FRAME_TYPE_NUL) && packet->is_complete) {  

		/* update reply sent */
		vortex_channel_update_status (channel, 0, packet->msg_no, UPDATE_RPY_NO_WRITTEN);

		/* unblock waiting thread for replies sent */
		vortex_channel_signal_reply_sent_on_close_blocked (channel);
		
		/* signal reply sent */
		vortex_channel_signal_rpy_sent (channel, packet->msg_no);
	}

	/* unref the channel now finished the operation */
	vortex_channel_unref (channel);
	
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
	VortexCtx           * ctx = vortex_channel_get_ctx (channel);
	VortexSequencerData * data;

	vortex_log (VORTEX_LEVEL_DEBUG, "about to check for pending queue message not sequenced..");

	/* do not update any thing if found that the queue is not available */
	if (ctx->sequencer_queue == NULL)
		return;

	/* check if we have data to be resequenced */
	if (vortex_channel_next_pending_message (channel)) {

		/* prepare data, flagging re-sequencing .. */
		data                       = axl_new (VortexSequencerData, 1);
		data->resequence           = axl_true;
		data->channel_num          = vortex_channel_get_number (channel);
		data->channel              = channel;
		data->conn                 = connection;
	
		/* queue data */
		vortex_log (VORTEX_LEVEL_DEBUG, 
			    "doing PRIORITY push to skip current stored items=%d (it seems that we have pending messages not sequenced, re-sequence them)",
			    vortex_async_queue_items (ctx->sequencer_queue));
		QUEUE_PRIORITY_PUSH (ctx->sequencer_queue, data);
	}else {
		vortex_log (VORTEX_LEVEL_DEBUG, "there is no messages pending to be sequenced, over the channel=%d",
		       vortex_channel_get_number (channel));
	} /* end if */

	return;
}

void vortex_sequencer_drop_connections_foreach (VortexAsyncQueue * queue,
						axlPointer         item_stored,
						int                position,
						axlPointer         user_data)
{
	VortexConnection    * conn = user_data;
	VortexSequencerData * data = item_stored;

	/* check for termination beacon */
	if (data == INT_TO_PTR(1))
		return;

	/* flag as discard those messages having the connection provided */
	if (data->conn == conn &&
	    vortex_connection_get_id (conn) == vortex_connection_get_id (data->conn)) {
		
		data->discard = axl_true;
	} /* end if */

	return;
}
						

/** 
 * @internal Function used to drop all pending messages to be
 * sequenced for this connection.
 * 
 * @param conn The connection that is about to be closed.
 */
void	vortex_sequencer_drop_connection_messages (VortexConnection * conn)
{
	/* get current context */
	VortexCtx * ctx = vortex_connection_get_ctx (conn);

	/* check context and queue. Also check if the reader is
	   re-initializing which means we are in the middle of some
	   sort of process creation (fork ()) */
	if (ctx == NULL || ctx->sequencer_queue == NULL || ctx->reader_cleanup || ctx->vortex_exit)
		return;

	/* call to discard pending messages */
	vortex_log (VORTEX_LEVEL_DEBUG, 
		    "calling to discard messages pending to be sequenced for connection id=%d",
		    vortex_connection_get_id (conn));
	vortex_async_queue_ref (ctx->sequencer_queue);
	vortex_async_queue_foreach (ctx->sequencer_queue,
				    vortex_sequencer_drop_connections_foreach,
				    conn);
	vortex_async_queue_unref (ctx->sequencer_queue);
	return;
}
