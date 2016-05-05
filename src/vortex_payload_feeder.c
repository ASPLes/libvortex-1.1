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
#include <vortex_payload_feeder.h>
#include <vortex_payload_feeder_private.h>

/** 
 * @brief Allows to create a new payload feeder object that will be
 * defined by feeder handler provided.
 *
 * Once created the feeder, you can use use the sending API for feeder objects, see:
 *
 * - \ref vortex_channel_send_msg_from_feeder
 * - \ref vortex_channel_send_rpy_from_feeder
 * - \ref vortex_channel_send_ans_rpy_from_feeder
 *
 * The handler provided must implement feeder events defined by \ref
 * VortexPayloadFeederOp. As an example, you can take a look on how is
 * implemented vortex_payload_feeder_file. 
 *
 * Once a feeder is used to initiate a send operation, the user is not
 * required to release it. This is already done by the vortex
 * sequencer thread.
 *

 *
 * @param handler The feeder handler that will define how this instance will work.
 *
 * @param user_data User defined pointer passed to the feeder handler when it is called.
 *
 * @return A reference to the payload feeder ready to use or NULL it
 * if fails. 
 */
VortexPayloadFeeder * vortex_payload_feeder_new (VortexPayloadFeederHandler handler,
						 axlPointer                 user_data)
{
	VortexPayloadFeeder * feeder;
	if (handler == NULL)
		return NULL;

	/* build feeder */
	feeder            = axl_new (VortexPayloadFeeder, 1);
	feeder->handler   = handler;
	feeder->user_data = user_data;

	/* init mutex and reference counting */
	vortex_mutex_create (&feeder->mutex);
	feeder->ref_count = 1;

	return feeder;
}

typedef struct _VortexPayloadFileFeeder {
	int             size;
	FILE          * file_to_feed;
	axl_bool        mime_pending;
} VortexPayloadFileFeeder;

axl_bool __vortex_payload_feeder_file (VortexCtx               * ctx,
				       VortexPayloadFeederOp     op_type,
				       VortexPayloadFeeder     * feeder,
				       axlPointer                param1,
				       axlPointer                param2,
				       axlPointer                user_data)
{
	VortexPayloadFileFeeder * state  = user_data;
	int                     * size   = param1;
	char                    * buffer = param2;

	switch (op_type) {
	case PAYLOAD_FEEDER_GET_SIZE:
		vortex_log (VORTEX_LEVEL_DEBUG, "Requesting to return size: %d", state->size);
		/* printf ("F: Requesting to return size: %d\n", state->size); */
		/* return feeder size */
		(* size) = state->size + 2;
		return axl_true;
	case PAYLOAD_FEEDER_GET_CONTENT:
		vortex_log (VORTEX_LEVEL_DEBUG, "Requesting to return %d bytes content", (*size));

		/* check to place initial mime headers */
		if (state->mime_pending) {
			/* set mime handled */
			state->mime_pending = axl_false;

			buffer [0] = '\r';
			buffer [1] = '\n';
			(* size) = fread (buffer + 2, 1, (*size) - 2, state->file_to_feed);
			
			/* ..and update (* size) to include two additional bytes */
			(*size) += 2;
		} else  {
			/* read the provided amount of bytes on the provided buffer */
			(* size) = fread (buffer, 1, (*size), state->file_to_feed);
		}
		vortex_log (VORTEX_LEVEL_DEBUG, "Returning %d bytes content", (*size));
		return axl_true;
	case PAYLOAD_FEEDER_IS_FINISHED:
		/* return if the current feeder have no more content */
		(* size) = feof (state->file_to_feed) != 0;
		/* printf ("F: asked if it is finished: %d\n", (*size)); */

		return axl_true;
	case PAYLOAD_FEEDER_RELEASE:
		/* release current feeder */
		fclose (state->file_to_feed);

		axl_free (state);
		return axl_true;
	} /* end switch */

	/* this code is never reached */
	return axl_true;
}

/** 
 * @brief Creates a feeder object connected to the file found at the
 * path provided.
 *
 * Once you have created the feeder object you initialize a send
 * operation providing the feeder. This will cause to send a frame or
 * the desired type, making its content to be filled from content
 * found at the \ref VortexPayloadFeeder. The the following functions:
 *
 * - \ref vortex_channel_send_msg_from_feeder
 * - \ref vortex_channel_send_rpy_from_feeder
 * - \ref vortex_channel_send_ans_rpy_from_feeder
 *
 * <b>NOTE:</b> there is a \ref vortex_payload_feeder_free
 * function. This is usually not required because the send operation
 * already finishes the object when the send operation has been
 * completed. 
 *
 * @param path The path to the file that will be feeded into the send operation.
 *
 * @param add_mime_head In general is recommend that the feeder add an
 * empty mime header before sending the content found on the
 * file. However, it may be required to avoid producing such mime
 * header in the case remote BEEP peer will not process such
 * header. For example, if remote peer disable \ref
 * vortex_channel_set_complete_flag "complete flag", it will require
 * the content to be without a mime header or to detect the first
 * frame fragment and skip the initial MIME header.
 *
 * @return A reference to a \ref VortexPayloadFeeder object or NULL if
 * it fails. The function checks if the function exists and can be
 * opened. In such checks fails, function will return NULL.
 *
 * <b>Important note about reference returned by this function</b>
 *
 * \ref VortexPayloadFeeder object support reference counting. Once
 * the feeder object is "sent" (using some of available function like
 * \ref vortex_channel_send_msg_from_feeder) then the reference is
 * considered to be owned by the vortex engine (in particular by the
 * vortex sequencer loop).
 *
 * This means that you must call to \ref vortex_payload_feeder_ref in
 * the case you want to pause a feeder transfer (\ref
 * vortex_payload_feeder_pause) or to check its transfer status (\ref
 * vortex_payload_feeder_status).
 *
 * In the case you use the feeder to just send content, then you don't
 * have to worry about \ref vortex_payload_feeder_ref. In the other
 * hand, as much calls are done to \ref vortex_payload_feeder_ref as much
 * as required to \ref vortex_payload_feeder_unref.
 */
VortexPayloadFeeder * vortex_payload_feeder_file (const char * path,
						  axl_bool     add_mime_head)
{
	FILE                    * file_to_feed;
	VortexPayloadFileFeeder * state;
	struct stat               stats;

	v_return_val_if_fail (path, NULL);

	/* open file */
#if defined(AXL_OS_UNIX)
	file_to_feed = fopen (path, "r");
#elif defined(AXL_OS_WIN32)
	file_to_feed = fopen (path, "rb");
#endif
	if (file_to_feed == NULL)
		return NULL;

	/* get size */
	if (stat (path, &stats) != 0) {
		fclose (file_to_feed);
		return NULL;
	} /* end if */

	/* create state */
	state               = axl_new (VortexPayloadFileFeeder, 1);
	state->mime_pending = add_mime_head;
	state->file_to_feed = file_to_feed;
	state->size         = stats.st_size;

	/* ok, now create the feeder */
	return vortex_payload_feeder_new (__vortex_payload_feeder_file, state);
}

/** 
 * @internal Makes the feeder to return pending content size to be
 * feeded.
 *
 * @param feeder The feeder object where the operation will be
 * applied.
 *
 * @return The pending size. The function must always return > 0.
 */
int                   vortex_payload_feeder_get_pending_size (VortexPayloadFeeder * feeder)
{
	int      size = -1;
	
	/* call to get pending size */
	feeder->handler (feeder->ctx, PAYLOAD_FEEDER_GET_SIZE, feeder, &size, NULL, feeder->user_data);
	
	/* return size */
	return size;
}

/** 
 * @internal Makes the feeder to return next payload to be sent. The
 * function receives the max amount of bytes that can be sent at this
 * moment and the buffer where the content must be placed.
 */
int                   vortex_payload_feeder_get_content (VortexPayloadFeeder * feeder, 
							 int                   size_to_copy, 
							 char                * buffer)
{
	/* check for pause or cancel */
	if (feeder->status < 0)
		return feeder->status; /* return directly the state of the 
					  should continue either because 
					  it is paused or cancel */
	/* call to get content */
	feeder->handler (feeder->ctx, PAYLOAD_FEEDER_GET_CONTENT, feeder, &size_to_copy, buffer, feeder->user_data);
	
	/* accumulate bytes transferred */
	if (size_to_copy > 0)
		feeder->bytes_transferred += size_to_copy;

	/* return updated size to copy */
	return size_to_copy;
}

axlPointer __vortex_payload_feeder_is_finished_notification (VortexPayloadFeeder * feeder)
{
	/* call to notify */
	feeder->finish_handler (feeder->channel, feeder, feeder->finish_user_data);

	/* release references */
	vortex_payload_feeder_unref (feeder);

	return NULL;
}

/** 
 * @brief Allows to query the feeder if it has more content to be
 * send.
 * @param feeder The feeder to be checked.
 *
 * @return axl_true if the feeder has no more content, otherwise
 * axl_false is returned.
 */
axl_bool              vortex_payload_feeder_is_finished (VortexPayloadFeeder * feeder)
{
	axl_bool                           is_finished = axl_false;

	if (feeder == NULL)
		return axl_false;

	/* call to get finished status */
	feeder->handler (feeder->ctx, PAYLOAD_FEEDER_IS_FINISHED, feeder, &is_finished, NULL, feeder->user_data);
	
	/* check if it is finished and if there are handlers
	 * configured */
	if (is_finished && feeder->finish_handler && ! feeder->notification_done) {
		/* flag that the notification is already done */
		feeder->notification_done = axl_true;

		/* acquire a reference */
		vortex_payload_feeder_ref (feeder);

		/* call to report finished handler */
		vortex_thread_pool_new_task (feeder->ctx, (VortexThreadFunc)__vortex_payload_feeder_is_finished_notification, feeder);
	} /* end if */

	return is_finished;
}


/** 
 * @brief Allows to configure a finished handler that will be called
 * when the feeder is done sending all its content.
 *
 * @param feeder The feeder to be configured with the finished
 * handler.
 *
 * @param on_finished The on finished handler to configure.
 *
 * @param user_data Optional user defined pointer to be passed to the on finished handler
 */
void                  vortex_payload_feeder_set_on_finished (VortexPayloadFeeder                * feeder,
							     VortexPayloadFeederFinishedHandler   on_finished,
							     axlPointer                           user_data)
{
	/* check input data */
	if (feeder == NULL || on_finished == NULL)
		return;

	/* configure handler */
	feeder->finish_handler     = on_finished;
	feeder->finish_user_data   = user_data;
	return;
}


/** 
 * @brief Flags the feeder to be paused/cancelled as soon as possible.
 *
 * The pause/cancel operation does not happens immediately because the
 * sequencer thread needs to access again to the feeder to check the
 * pause/cancel status.
 *
 * To pause/cancel, the caller must "own" a reference to the feeder because
 * the default reference is owned by the vortex sequencer. This will
 * avoid race conditions.
 *
 * @param feeder The feeder to be paused/cancelled
 *
 * @param close_transfer In the case the feeder is paused/cancelled it may be
 * required to send an empty frame to close the series of frames that
 * may be sent. The idea is that pausing current transfer leaves the
 * channel in such state that a frame with more flag is active so it
 * is required at least another frame with the same msgno and more
 * flag set to false (.) to complete this particular transfer. This is
 * achieved by setting close_transfer = axl_true. In the other hand,
 * if close_transfer = axl_false, the caller will block the channel
 * until this feeder is resumed (because having an pending transfer
 * makes not possible other transfers). If you don't know how to
 * proceed, in general is a good idea to close_transfer = axl_true so
 * your channel is available for other transfer.
 *
 * @return The function returns axl_true if the feeder was flagged to
 * be paused/cancelled. Otherwise axl_false is returned. The function also
 * returns axl_false in the case NULL is received.
 *
 * <b>How do I resume a transfer?</b>
 *
 * To resume a transfer just call again to transfer the feeder as
 * usual (for example \ref vortex_channel_send_msg_from_feeder). Because 
 * the feeder object holds its internal state, the transfer will continue 
 * at the place it was suspended.
 *
 */
axl_bool                   vortex_payload_feeder_pause       (VortexPayloadFeeder * feeder,
							      axl_bool              close_transfer)
{
	if (feeder == NULL)
		return axl_false;

	/* flag the feeder to be cancelled */
	feeder->status         = -1;
	feeder->close_transfer = close_transfer;

	return axl_true;
}

/** 
 * @brief Returns current status of the provided payload feeder.
 *
 * The status object contains information about how many bytes has
 * being transferred so far, total bytes to be transferred (if
 * supported by the feeder), if is cancelled or paused.
 *
 * NOTE: you must call first \ref vortex_payload_feeder_ref to own a
 * reference to avoid a race condition between your access and vortex
 * sequencer finishing its reference.
 */
void                  vortex_payload_feeder_status      (VortexPayloadFeeder       * feeder, 
							 VortexPayloadFeederStatus * status)
{
	if (feeder == NULL || status == NULL)
		return;
		
	/* configure data */
	status->is_finished       = vortex_payload_feeder_is_finished (feeder);
	status->total_size        = vortex_payload_feeder_get_pending_size (feeder);
	status->bytes_transferred = feeder->bytes_transferred;
	status->is_paused         = (feeder->status == -1);

	return;
}

/** 
 * @brief Allows to increase the reference counting associated to the
 * payload feeder. A reference account acquired can be released by
 * using \ref vortex_payload_feeder_unref.
 *
 * @param feeder The feeder to increase its reference counting.
 *
 * @return axl_true in the case reference counting was updated,
 * otherwise axl_false is returned.
 */
axl_bool              vortex_payload_feeder_ref         (VortexPayloadFeeder * feeder)
{
	if (feeder == NULL || feeder->handler == NULL || feeder->ref_count == 0)
		return axl_false;

	/* acquire mutex */
	vortex_mutex_lock (&feeder->mutex);

	/* increase reference counting */
	feeder->ref_count++;

	/* release mutex */
	vortex_mutex_unlock (&feeder->mutex);

	return axl_true;
}

/** 
 * @brief Allows to reduce a reference from the provided payload
 * feeder.
 *
 * @param feeder The payload feeder that will be reduced.
 *
 */ 
void              vortex_payload_feeder_unref       (VortexPayloadFeeder * feeder)
{
	if (feeder == NULL)
		return;

	/* call to current implementation */
	vortex_payload_feeder_free (feeder);
	return;
}

/** 
 * @brief Function that signal and releases all resources
 * associated to the feeder if the reference counting reaches 0.
 *
 * This function is equivalent to \ref vortex_payload_feeder_unref.
 *
 * <b>NOTE:</b> in most cases you don't need to call this function
 * because release operation is usually done by the send operation
 * (including when the send operation fails).
 *
 * @param feeder The feeder to be released.
 */
void              vortex_payload_feeder_free (VortexPayloadFeeder * feeder)
{
	if (feeder == NULL)
		return;

	/* acquire mutex */
	vortex_mutex_lock (&feeder->mutex);

	/* increase reference counting */
	feeder->ref_count--;
	if (feeder->ref_count != 0) {
		/* release mutex */
		vortex_mutex_unlock (&feeder->mutex);
		return;
	} /* end if */

	/* call to get finished status */
	if (feeder->handler) {
		feeder->handler (feeder->ctx, PAYLOAD_FEEDER_RELEASE, feeder, NULL, NULL, feeder->user_data);
		feeder->handler = NULL;
	} /* end if */

	/* call to finish channel ref */
	vortex_channel_unref2 (feeder->channel, "payload feeder");
	feeder->channel = NULL;

	/* release mutex */
	vortex_mutex_unlock (&feeder->mutex);
	vortex_mutex_destroy (&feeder->mutex);

	/* free feeder */
	axl_free (feeder);

	return;
}
								 

