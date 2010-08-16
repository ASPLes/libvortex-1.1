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
};

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

	return feeder;
}

typedef struct _VortexPayloadFileFeeder {
	int         size;
	FILE      * file_to_feed;
	axl_bool    mime_pending;
} VortexPayloadFileFeeder;

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
 * @param path The path to the file that will be feeded into the send operation.
 *
 * @return A reference to a \ref VortexPayloadFeeder object or NULL if
 * it fails. The function checks if the function exists and can be
 * opened. In such checks fails, function will return NULL.
 */ 
VortexPayloadFeeder * vortex_payload_feeder_file (const char * path)
{
	FILE                    * file_to_feed;
	VortexPayloadFileFeeder * state;
	struct stat               stats;

	v_return_val_if_fail (path, NULL);

	/* open file */
	file_to_feed = fopen (path, "r");
	if (file_to_feed == NULL)
		return NULL;

	/* get size */
	if (stat (path, &stats) != 0) {
		fclose (file_to_feed);
		return NULL;
	} /* end if */

	/* create state */
	state               = axl_new (VortexPayloadFileFeeder, 1);
	state->mime_pending = axl_true;
	state->file_to_feed = file_to_feed;
	state->size         = stats.st_size;

	/* ok, now create the feeder */
	return vortex_payload_feeder_new (__vortex_payload_feeder_file, state);
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
 * @internal Function that signal and releases all resources
 * associated to the feeder.
 */
void              vortex_payload_feeder_free (VortexPayloadFeeder * feeder,
					      VortexCtx           * ctx)
{
	if (feeder == NULL)
		return;

	/* call to get finished status */
	feeder->handler (ctx, PAYLOAD_FEEDER_RELEASE, NULL, NULL, feeder->user_data);

	/* free feeder */
	axl_free (feeder);

	return;
}
								 

