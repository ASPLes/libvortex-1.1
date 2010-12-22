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
#include <vortex_payload_feeder.h>

struct _VortexPayloadFeeder {
	VortexCtx                  * ctx;
	VortexPayloadFeederHandler   handler;
	axlPointer                   user_data;
	VortexMutex                  mutex;
	int                          ref_count;
};

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
 * @param ctx     The context where the feeder will be created
 *
 * @param handler The feeder handler that will define how this instance will work.
 *
 * @param user_data User defined pointer passed to the feeder handler when it is called.
 *
 * @return A reference to the payload feeder ready to use or NULL it
 * if fails. 
 */
VortexPayloadFeeder * vortex_payload_feeder_new (VortexCtx                * ctx,
						 VortexPayloadFeederHandler handler,
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

	/* set context */
	feeder->ctx       = ctx;

	return feeder;
}

typedef struct _VortexPayloadFileFeeder {
	int             size;
	FILE          * file_to_feed;
	axl_bool        mime_pending;
	int             cancel_status;
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

		/* check for pause or cancel */
		if (state->cancel_status < 0)
			return state->cancel_status; /* return directly the state of the 
							should continue either because 
							it is paused or cancel */

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
 */
VortexPayloadFeeder * vortex_payload_feeder_file (VortexCtx  * ctx,
						  const char * path,
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
	return vortex_payload_feeder_new (ctx, __vortex_payload_feeder_file, state);
}

/** 
 * @internal Makes the feeder to return pending content size to be feeded.
 * @param feeder The feeder object where the operation will be applied.
 *
 * @return The pending size. The function must always return > 0.
 */
int                   vortex_payload_feeder_get_pending_size (VortexPayloadFeeder * feeder, VortexCtx * ctx)
{
	int      size = -1;
	
	/* call to get pending size */
	feeder->handler (ctx, PAYLOAD_FEEDER_GET_SIZE, feeder, &size, NULL, feeder->user_data);
	
	/* return size */
	return size;
}

/** 
 * @internal Makes the feeder to return next payload to be sent. The
 * function receives the max amount of bytes that can be sent at this
 * moment and the buffer where the content must be placed.
 */
int                   vortex_payload_feeder_get_content (VortexPayloadFeeder * feeder, 
							 VortexCtx           * ctx,
							 int                   size_to_copy, 
							 char                * buffer)
{
	/* call to get content */
	feeder->handler (ctx, PAYLOAD_FEEDER_GET_CONTENT, feeder, &size_to_copy, buffer, feeder->user_data);

	/* return updated size to copy */
	return size_to_copy;
}

/** 
 * @internal Allows to query the feeder if it has more content to be
 * send.
 */
axl_bool              vortex_payload_feeder_is_finished (VortexPayloadFeeder * feeder,
							 VortexCtx           * ctx)
{
	axl_bool is_finished = axl_false;

	/* call to get finished status */
	feeder->handler (ctx, PAYLOAD_FEEDER_IS_FINISHED, feeder, &is_finished, NULL, feeder->user_data);

	return is_finished;
}

/** 
 * @brief Flags the feeder to be cancelled as soon as possible. 
 *
 * The cancel operation does not happens immediately because the
 * sequencer thread needs to access again to the feeder to check the
 * cancel status.
 *
 * To cancel this function the caller must "own" a reference to the
 * feeder because the default reference is owned by the vortex
 * sequencer.
 *
 * @param feeder The feeder to be cancelled. 
 *
 * @return The function returns axl_true if the feeder was flagged to
 * be cancelled. Otherwise axl_false is returned.
 *
 * 
 */
axl_bool                   vortex_payload_feeder_cancel      (VortexPayloadFeeder * feeder)
{
	return -1;
}

axl_bool                   vortex_payload_feeder_pause       (VortexPayloadFeeder * feeder)
{
	return -1;
}

/** 
 * @brief Returns current status of the provided payload feeder.
 *
 * The status object contains information about how many bytes has
 * being transferred so far, total bytes to be transferred (if
 * supported by the feeder), if is cancelled or paused.
 */
void                  vortex_payload_feeder_status      (VortexPayloadFeeder       * feeder, 
							 VortexPayloadFeederStatus * status)
{
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
 * @param ctx The context where the operation will take place.
 *
 */ 
void              vortex_payload_feeder_unref       (VortexPayloadFeeder * feeder,
						     VortexCtx           * ctx)
{
	/* call to current implementation */
	vortex_payload_feeder_free (feeder, ctx);
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
 * @param ctx The context where the feeder release will happen. 
 */
void              vortex_payload_feeder_free (VortexPayloadFeeder * feeder,
					      VortexCtx           * ctx)
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
		feeder->handler (ctx, PAYLOAD_FEEDER_RELEASE, feeder, NULL, NULL, feeder->user_data);
		feeder->handler = NULL;
	} /* end if */

	/* release mutex */
	vortex_mutex_unlock (&feeder->mutex);
	vortex_mutex_destroy (&feeder->mutex);

	/* free feeder */
	axl_free (feeder);

	return;
}
								 

