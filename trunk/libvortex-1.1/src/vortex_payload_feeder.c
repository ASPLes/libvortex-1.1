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
	VortexPayloadFeederHandler handler;
	axlPointer                 user_data;
	VortexMutex                mutex;
	int                        ref_count;
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
	VortexCBuffer * buffer;
	VortexThread    thread;
	int             amount_read;
	axl_bool        failure_found;
} VortexPayloadFileFeeder;

int __vortex_payload_feeder_read_file (VortexCtx * ctx, VortexPayloadFileFeeder * feeder, char * buffer, int size)
{
	int bytes_read;

	/* check for failure found */
	if (feeder->failure_found) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Failed to feed content from file, failure found");
		return -1;
	}

	/* check if we have a buffer and read from it */
	if (feeder->buffer) {

		/* check if the size requested is bigger than the
		 * total amount of bytes to be transferred for a
		 * file */
		if ((feeder->size - feeder->amount_read) < size)
			size = feeder->size - feeder->amount_read;

		/* get requested content and block the caller until it
		   is received */
		if (vortex_cbuffer_is_empty (feeder->buffer, axl_true)) {
			printf ("F: found buffer empty..\n");
		} /* end if */
		bytes_read           = vortex_cbuffer_get (feeder->buffer, buffer, size, axl_true);
		feeder->amount_read += bytes_read;

		/* printf ("F: Reading from buffer %d bytes (requested: %d, amount served until now: %d)\n", bytes_read, size, feeder->amount_read); */
		return bytes_read; 
	}

	/* read requested content */
	bytes_read = fread (buffer, 1, size, feeder->file_to_feed);

	/* printf ("F: Reading directly from the file %d bytes (requested: %d, amount served until now: %d)\n", bytes_read, size, feeder->amount_read); */

	return bytes_read;
}

axl_bool __vortex_payload_feeder_file (VortexCtx               * ctx,
				       VortexPayloadFeederOp     op_type,
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
			(* size) = __vortex_payload_feeder_read_file (ctx, state, buffer + 2, (*size) - 2);
			
			/* ..and update (* size) to include two additional bytes */
			(*size) += 2;
		} else  {
			/* read the provided amount of bytes on the provided buffer */
			(* size) = __vortex_payload_feeder_read_file (ctx, state, buffer, (*size));
		}
		vortex_log (VORTEX_LEVEL_DEBUG, "Returning %d bytes content", (*size));
		return axl_true;
	case PAYLOAD_FEEDER_IS_FINISHED:
		/* return if the current feeder have no more content */
		(* size) = feof (state->file_to_feed) != 0;
		/* printf ("F: asked if it is finished: %d\n", (*size)); */

		/* check if we have a buffer installed */
		if (state->buffer) {
			(* size) = (* size ) && vortex_cbuffer_is_empty (state->buffer, axl_true);
			/* printf ("F: we have a buffer, asked if it is finished updated (bytes at the buffer %d): %d\n", 
			   vortex_cbuffer_available_bytes (state->buffer, axl_true), (*size)); */
		} /* end if */

		return axl_true;
	case PAYLOAD_FEEDER_RELEASE:
		/* release current feeder */
		fclose (state->file_to_feed);

		/* check it there is a buffer created */
		if (state->buffer) {
			/* finish buffer */
			vortex_cbuffer_unref (state->buffer);
			state->buffer = NULL;

			/* call to finish thread */
			vortex_thread_destroy (&state->thread, axl_false);
		}

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


axlPointer vortex_payload_feeder_file_thread (VortexPayloadFeeder * feeder)
{
	VortexPayloadFileFeeder * file_feeder = feeder->user_data;
	VortexCBuffer           * buffer      = file_feeder->buffer;
	char                      content[4096];
	int                       bytes_read;
	int                       total_bytes = 0;
	
	/* acquire a reference to the buffer */
	vortex_cbuffer_ref (buffer);
	vortex_payload_feeder_ref (feeder);

	/* acquire a refrence to the payload feeder */

	/* read the content and push it into the buffer */
	do {
		/* read content */
		bytes_read = fread (content, 1, 4095, file_feeder->file_to_feed);
		if (bytes_read == 0)
			break;

		/* accumulate bytes read */
		total_bytes += bytes_read;

		/* write into the buffer and lock until every thing is
		 * read */
		/* printf ("T:(1) Pushing %d bytes into the buffer (currently holding %d bytes, read until now %d)..\n",
		   bytes_read, vortex_cbuffer_available_bytes (buffer, axl_true), total_bytes); */
		if (vortex_cbuffer_put (buffer, content, bytes_read, axl_true) != bytes_read) {
			/* flag that a failure was found */
			file_feeder->failure_found = axl_true;
			/* printf ("ERROR: expected to write %d bytes but something different was found..\n", bytes_read); */
			break;
		}
		/* printf ("T:(2)   Pushed %d bytes into the buffer (currently holding %d bytes, read until now %d)..\n",
		   bytes_read, vortex_cbuffer_available_bytes (buffer, axl_true), total_bytes); */
	}while (1); /* end while */

	/* release reference to the buffer */
	vortex_cbuffer_unref (buffer);
	vortex_payload_feeder_unref (feeder, NULL);
	
	return NULL;
}

/** 
 * @brief Allows to configure the buffer size that will be used a \ref
 * VortexPayloadFeeder that was created with \ref
 * vortex_payload_feeder_file.
 *
 * @param feeder A payload feeder that was created by \ref vortex_payload_feeder_file
 *
 * @param buffer_size The amount of bytes that can hold at once the buffer.
 *
 * @return axl_true if the buffer was created and installed in the
 * feeder, otherwise axl_false is returned. The buffer can be added at
 * any time during the transfer process. A buffer added cannot be
 * later removed. The function will also fail if there is already a
 * buffer installed.
 */
axl_bool              vortex_payload_feeder_file_set_buffer (VortexPayloadFeeder * feeder, int buffer_size)
{
	VortexCBuffer           * buffer;
	VortexPayloadFileFeeder * file_feeder;

	/* check parameters */
	if (feeder == NULL || buffer_size <= 0 || feeder->user_data == NULL)
		return axl_false;

	/* get the file feeder */
	file_feeder = feeder->user_data;

	/* check if a buffer is already in place: still there is no
	   support for buffer resize */
	if (file_feeder->buffer)
		return axl_false;

	/* create the buffer and start feeding content */
	buffer = vortex_cbuffer_new (buffer_size);
	if (buffer == NULL)
		return axl_false;

	/* configure the buffer and create the thread that will feed content */
	file_feeder->buffer = buffer;
	if (! vortex_thread_create (&file_feeder->thread, (VortexThreadFunc) vortex_payload_feeder_file_thread, feeder,
				    VORTEX_THREAD_CONF_END)) {
		return axl_false;
	} /* end if */

	return axl_true;
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
	feeder->handler (ctx, PAYLOAD_FEEDER_GET_SIZE, &size, NULL, feeder->user_data);
	
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
	feeder->handler (ctx, PAYLOAD_FEEDER_GET_CONTENT, &size_to_copy, buffer, feeder->user_data);

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
	feeder->handler (ctx, PAYLOAD_FEEDER_IS_FINISHED, &is_finished, NULL, feeder->user_data);

	return is_finished;
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
		feeder->handler (ctx, PAYLOAD_FEEDER_RELEASE, NULL, NULL, feeder->user_data);
		feeder->handler = NULL;
	} /* end if */

	/* release mutex */
	vortex_mutex_unlock (&feeder->mutex);
	vortex_mutex_destroy (&feeder->mutex);

	/* free feeder */
	axl_free (feeder);

	return;
}
								 

