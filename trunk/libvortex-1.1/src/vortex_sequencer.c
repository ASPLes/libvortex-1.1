/*
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2005 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 *  
 *  You may find a copy of the license under this software is released
 *  at COPYING file. This is LGPL software: you are welcome to
 *  develop proprietary applications using this library without any
 *  royalty or fee but returning back any change, improvement or
 *  addition in the form of source code, project image, documentation
 *  patches, etc. 
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

#define LOG_DOMAIN "vortex-sequencer"

void vortex_sequencer_queue_data (VortexCtx * ctx, VortexSequencerData * data)
{
	v_return_if_fail (data);

	/* check not initialized queue */
	if (ctx->sequencer_queue == NULL) {
		vortex_log (VORTEX_LEVEL_DEBUG, "trying to queue a message to be sent, having the vortex sequencer stopped");
		axl_free (data->message);
		axl_free (data);
		return;
	} /* end if */

	vortex_log2 (VORTEX_LEVEL_DEBUG, "new message to be sent: msgno %d, channel %d (size: %d):\n%s",
		     data->msg_no, 
		     data->channel_num, 
		     data->message_size,
		     data->message ? data->message : "**** empty message ****");

	/* get the connection for the data to be sent */
	data->conn = vortex_channel_get_connection (data->channel);
	
	/* queue data */
	QUEUE_PUSH (ctx->sequencer_queue, data);

	return;
}

void __vortex_sequencer_unref_and_clear (VortexConnection * connection,
					 VortexSequencerData * data, 
					 bool                  unref)
{

	/** 
	 * Once done, release the reference from the vortex
	 * sequencer to the connection manipulated.
	 */
	if (unref)
		vortex_connection_unref (connection, "(vortex sequencer)");
		
	/* free data no longer needed */
	if ((data != NULL) && (data->message != NULL))
		axl_free (data->message);
	
	/* free data it self */
	if (data != NULL)
		axl_free (data);
	
	return;
}

axlPointer __vortex_sequencer_run (axlPointer _data)
{
	/* get current context */
	VortexCtx           * ctx = _data;
	VortexSequencerData * data                   = NULL;
	VortexWriterData      packet;
	VortexChannel       * channel                = NULL;
	VortexConnection    * connection             = NULL;
	int                   next_seq_no;
	int                   message_size;
	int                   max_seq_no            = 0;
	int                   size_to_copy;
	char                * a_frame;
	bool                  resequence;
	VortexAsyncQueue    * queue;

	while (true) {
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
			QUEUE_PUSH (ctx->sequencer_stoped, INT_TO_PTR (1));
			return NULL;
		}
		
		vortex_log (VORTEX_LEVEL_DEBUG, "a new message to be sequenced..");

		/* new message to be sent */
		channel = data->channel;

		/* check to drop the sequence */
		if (data->discard) {
			vortex_log (VORTEX_LEVEL_WARNING, "discarding sequencer data (found flag activated)");
			__vortex_sequencer_unref_and_clear (NULL, data, false);
			continue;
		} /* end if */

		/**
		 * Increase reference counting for the connection holding the channel 
		 * to avoid race deallocation condition during the message fragmentation
		 * on the channel queue. connection ref must be performed before any other
		 * operation to avoid unrefering (vortex_connection_unref) a connection not 
		 * referred (vortex_connection_ref).
		 */
		connection = vortex_channel_get_connection (channel);
		if (! vortex_connection_ref (connection, "(vortex sequencer)")) {
			vortex_log (VORTEX_LEVEL_CRITICAL, 
				    "detect message to be sequenced over a channel installed on a non connected session, dropping message");

			/* clear data without unrefering the connection */
			__vortex_sequencer_unref_and_clear (connection, data, false);
			continue;
		}

		/* check channel number for deliver */
		if (data->channel_num != vortex_channel_get_number (channel)) {
			vortex_log (VORTEX_LEVEL_CRITICAL, 
			       "channel number for frame sending mismatch, dropping this frame");
			/* unref the connection and clear data */
			__vortex_sequencer_unref_and_clear (connection, data, true);
			continue;
		}

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
				__vortex_sequencer_unref_and_clear (connection, NULL, true);
				continue;
			}
			
			/* we have to flag the resequence variable
			 * this way because we can't use
			 * data->resequence. This is because the
			 * ->resequence flag is only used by the
			 * vortex reader to wakeup the vortex
			 * sequencer, but not by normal vortex
			 * sequencer packets (data variable). */
			resequence = true;
		}else {
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
				vortex_channel_queue_pending_message (channel, data);
				
				/* get a reference to the next pending message */
				data       = vortex_channel_next_pending_message (channel);
				
				/* flag this attempt as a resequence
				 * to avoid adding twice the data
				 * item */
				resequence = true;
			}
			
		}

		/* create the frame (or frames to send) (splitter
		 * process) */
		next_seq_no  = data->first_seq_no;
		message_size = data->message_size;
		max_seq_no   = vortex_channel_get_max_seq_no_remote_accepted (channel);
		vortex_log (VORTEX_LEVEL_DEBUG, "sequence operation: next seq no=%d message size=%d max seq no=%d step=%d",
		       next_seq_no, message_size, max_seq_no, data->step);

		
		/**    
		 * We need to check that remote buffer is willing to
		 * accept more data. On this context, the message
		 * received must be hold until a SEQ frame is received
		 * signaling that more bytes could be sent.
		 */
		if (next_seq_no >= max_seq_no) {
			
			vortex_log (VORTEX_LEVEL_WARNING, 
			       "Seems that the next seq number to use is greater or equal than max seq no (%d >= %d), which means that this channel (%d) is stalled",
			       next_seq_no, max_seq_no, vortex_channel_get_number (channel));
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
				vortex_channel_queue_pending_message (channel, data);
				data = NULL;
			}

			/* unref the connection, we have previously
			 * increased the reference */
			vortex_connection_unref (connection, "(vortex sequencer)");
			continue;
		} 
		vortex_log (VORTEX_LEVEL_DEBUG, "the channel is not stalled, continue with sequencing ...");

		/* calculate how many bytes to copy from the payload
		 * according to max_seq_no */
		if ((next_seq_no + message_size) > max_seq_no)
			size_to_copy = max_seq_no - next_seq_no + 1;
		else
			size_to_copy = message_size;

		/* create the new package to be managed by the vortex writer */
		packet.type         = data->type;
		packet.msg_no       = data->msg_no;
			
		/* we have the payload on buffer */
		vortex_log (VORTEX_LEVEL_DEBUG, "sequencing next message: type=%d, channel num=%d, msgno=%d, more=%d, next seq=%d size=%d ansno=%d",
		       data->type, data->channel_num, data->msg_no, !(message_size == size_to_copy), 
		       next_seq_no, size_to_copy, data->ansno);
		a_frame = vortex_frame_build_up_from_params_s (data->type,        /* frame type to be created */
							       data->channel_num, /* channel number the frame applies to */
							       data->msg_no,      /* the message number */
							       /* frame payload size to be created */
							       !(message_size == size_to_copy), /* have more frames */
							       /* sequence number for the frame to be created */
							       next_seq_no,
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
							       (data->message != NULL) ? (data->message + data->step) : NULL,
							       /* calculated frame size */
							       &(packet.the_size));
		/* set frame returned to the package */
		packet.the_frame = a_frame;
		
			
		/* STEP 1: new queue the rest of the message if it
		 * wasn't completly sequence. We do this before
		 * sending the frame to avoid a possible race condition
		 * between sending the message sequenced, be processed
		 * at the remote side, the remote side generate a SEQ
		 * frame, send it, and our vortex reader process it
		 * and found that there is not pending, not yet
		 * completely sequenced message. 
		 *
		 * Incredible, but true! 
		 */
		if (size_to_copy != message_size) {
			vortex_log (VORTEX_LEVEL_DEBUG, "the message sequenced is not going to be sent completely (%d != %d)", 
			       size_to_copy, message_size);

			/* well, it seems we didn't be able to send
			 * the hole message, so there are some
			 * remaining bytes. Just keep track about how
			 * many bytes we have sent. We will used this
			 * information once the message is enabled to
			 * be sent again on the next SEQ received. */
			data->step          = data->step         + size_to_copy;

			/* make next seq no value to point to the next byte to send */
			data->first_seq_no  = data->first_seq_no + size_to_copy;
			
			/* make message size to be decreased the amount of bytes sent. */
			data->message_size  = data->message_size - size_to_copy;

			vortex_log (VORTEX_LEVEL_DEBUG, 
			       "updating message sequencing status: next seq no=%d message size=%d step=%d",
			       data->first_seq_no, data->message_size, data->step);

			/* do not queue the message again if we are
			 * re-sequencing, that is, sending pieces of a
			 * message that was queued because there were
			 * not enough buffer size at the remote peer
			 * for the given channel to hold it. */
			if (! resequence) {
				vortex_log (VORTEX_LEVEL_DEBUG, 
				       "because we are not re-sequencing, save the message to be pending");
				/* now queue the message to be pending to be sent. */
				vortex_channel_queue_pending_message (channel, data);
			}
			
			/* because we have queued the message, we want
			 * to avoid the message to be deleted from the
			 * last instruction this while has.
			 */
			data = NULL;

		} /* end if */
		
		/* STEP 2: now, send the package built, queueing it at
		 * the channel queue. At this point, we have prepared
		 * the rest to be sequenced message. */
		if ((a_frame != NULL) && (packet.the_size != -1)) {
			/* now, perform a send operation for the frame built */
			vortex_log (VORTEX_LEVEL_DEBUG, "frame built, send the frame directly"); 
			if (! vortex_sequencer_direct_send (connection, channel, &packet)) {
				vortex_log (VORTEX_LEVEL_WARNING, "unable to send data at this moment");

				/* free frame */
				axl_free (packet.the_frame);
				
				if (! resequence) {
					vortex_log (VORTEX_LEVEL_WARNING, "queuing data to be sent later because we aren't resending");
					
					/* queue data to be sent later */
					vortex_channel_queue_pending_message (channel, data);
					data = NULL;
				}
				
				/* unref the connection and clear data */
				__vortex_sequencer_unref_and_clear (connection, data, false);
				
				continue;
			}

			/* free frame */
			axl_free (packet.the_frame);

		}else {
			/* something has happend with the given
			 * connection. There was a problem queueing
			 * the message. */
			vortex_log (VORTEX_LEVEL_WARNING, 
			       "unable to queue a frame to be sent, dropping entire message (frame == NULL)=%d (frame size == -1)=%d",
			       (a_frame == NULL), (packet.the_size == -1));

			/* because we didn't notify the vortex writer
			 * the current package, we release it. */
			axl_free (packet.the_frame);

			/* unref the connection and clear data */
			__vortex_sequencer_unref_and_clear (connection, data, true);
			continue;
		}

		/** 
		 * STEP 3: now we have sent the last frame generated,
		 * we have to check if the message previously was
		 * completely sent so we could remove it from the
		 * pending messages to be resequenced, in the case we
		 * are doing so, and then try to keep on re-sequencing
		 * all messages pending until the current max seq no
		 * value gets exhausted.
		 */
		if (size_to_copy == message_size) {
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
					__vortex_sequencer_unref_and_clear (connection, data, false);

					goto start_resequence;
				} /* end if */
			} /* end if */
		} /* end if */

		/* unref the connection and clear data */
		__vortex_sequencer_unref_and_clear (connection, data, true);

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
 * @return true if the sequencer was init, otherwise false is
 * returned.
 **/
bool vortex_sequencer_run (VortexCtx * ctx)
{

	v_return_val_if_fail (ctx, false);

	/* sequencer queue where all data is received */
	ctx->sequencer_queue  = vortex_async_queue_new ();

	/* auxiliar queue used to synchronize the vortex sequencing
	 * shutting down process */
	ctx->sequencer_stoped = vortex_async_queue_new ();

	/* starts the vortex sequencer */
	if (! vortex_thread_create (&ctx->sequencer_thread,
				    (VortexThreadFunc) __vortex_sequencer_run,
				    ctx,
				    VORTEX_THREAD_CONF_END)) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to initialize the sequencer thread");
		return false;
	} /* end if */

	/* ok, sequencer initialized */
	return true;
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
	vortex_async_queue_pop   (ctx->sequencer_stoped);
	vortex_async_queue_unref (ctx->sequencer_stoped);
	vortex_thread_destroy    (&ctx->sequencer_thread, false);

	vortex_log (VORTEX_LEVEL_DEBUG, "vortex sequencer completely stoped");

	return; 
}



bool     vortex_sequencer_direct_send (VortexConnection * connection,
				       VortexChannel    * channel,
				       VortexWriterData * packet)
{
	/* reply number */
	bool        result = true;
	VortexCtx * ctx    = vortex_connection_get_ctx (connection);

	/* reference the channel during the transmission */
	vortex_channel_ref (channel);

	/* send the frame */
	if (vortex_log2_is_enabled (ctx))
		vortex_log2 (VORTEX_LEVEL_DEBUG, "Sending message, size (%d) over connection id=%d, Content: \n%s",
			     packet->the_size,  
			     vortex_connection_get_id (connection), packet->the_frame);
	else
		vortex_log (VORTEX_LEVEL_DEBUG, "Sending message, size (%d) over connection id=%d",
			    packet->the_size,  
			    vortex_connection_get_id (connection));

	if (! vortex_frame_send_raw (connection, packet->the_frame, packet->the_size)) {
		/* drop a log */
		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to send the frame: errno=(%d): %s", 
			    errno, vortex_errno_get_error (errno));
		
		/* set as non connected and flag the result */
		result = false;
	}

	/* signal the message have been sent */
	if (packet->type == VORTEX_FRAME_TYPE_RPY) {

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
void     vortex_sequencer_signal_update        (VortexChannel       * channel)
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
		vortex_log (VORTEX_LEVEL_DEBUG, "it seems that we have pending messages not sequenced, re-sequence them");
		/* prepare data, flagging re-sequencing .. */
		data                       = axl_new (VortexSequencerData, 1);
		data->resequence           = true;
		data->channel_num          = vortex_channel_get_number (channel);
		data->channel              = channel;
	
		/* queue data */
		QUEUE_PUSH (ctx->sequencer_queue, data);
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
		
		data->discard = true;
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

	/* check context and queue */
	if (ctx == NULL || ctx->sequencer_queue == NULL)
		return;

	/* call to discard pending messages */
	vortex_log (VORTEX_LEVEL_DEBUG, 
		    "calling to discard messages pending to be sequenced for connection id=%d",
		    vortex_connection_get_id (conn));
	vortex_async_queue_foreach (ctx->sequencer_queue,
				    vortex_sequencer_drop_connections_foreach,
				    conn);
}
