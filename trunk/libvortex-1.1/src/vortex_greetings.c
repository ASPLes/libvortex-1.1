/* 
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2008 Advanced Software Production Line, S.L.
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

/* loca include */
#include <vortex_ctx_private.h>

#define LOG_DOMAIN "vortex-greetings"

/**
 * \defgroup vortex_greetings Vortex Greetings: Related function to Initial Greeting messages.
 */

/**
 * \addtogroup vortex_greetings
 * @{
 */

/**
 * @internal
 * 
 * Build the greetings message for the provided @connection.
 * 
 * @param connection The connection where the greeting will be sent.
 * @param greetings_buffer The buffer used to build the message.
 *
 * @return the number of bytes in the built message.
 **/
int __vortex_greetings_build_message (VortexConnection * connection, char * greetings_buffer, int buffer_size)
{
	VortexCtx     * ctx                 = CONN_CTX (connection);
	axlList       * registered_profiles = vortex_profiles_get_actual_list_ref (ctx);
	int             iterator;
	int             next_index          = 0;
	const char    * localize            = NULL;
	const char    * features            = NULL;
	char          * uri;
	int             features_size, localize_size, size;
	

	/* check features and localize here (including all additional
	 * size required to build the greetings header) */
	features      = vortex_greetings_get_features (ctx);
	features_size = features ? strlen (features) : 0;

	localize      = vortex_greetings_get_localize (ctx);
	localize_size = localize ? strlen (localize) : 0;

	/* count features and localize size plus 9, 11, 1, 11, 1 ,3 */
	if ((features_size + localize_size + 36) >= buffer_size) {
		vortex_log (VORTEX_LEVEL_CRITICAL,  
			    "found buffer to build greetings to be not enough to hold current features (%d bytes)/localize (%d bytes) configuration",
			    features_size, localize_size);
		return -1;
	} /* end if */
	
	
	/* copy greetings */
	memcpy (greetings_buffer, "<greeting", 9);
	next_index += 9;
	if (features) {
		memcpy (greetings_buffer + next_index, " features='", 11);
		next_index += 11;
		
		memcpy (greetings_buffer + next_index, features, features_size);
		next_index += features_size;
		
		memcpy (greetings_buffer + next_index, "'", 1);
		next_index ++;
	} /* end features */
	

	if (localize) {
		memcpy (greetings_buffer + next_index, " localize='", 11);
		next_index += 11;
		
		memcpy (greetings_buffer + next_index, localize, localize_size);
		next_index += localize_size;
		
		memcpy (greetings_buffer + next_index, "'", 1);
		next_index ++;
	} /* end features */
	
	memcpy (greetings_buffer + next_index, ">\x0D\x0A", 3);
	next_index += 3;
	if (registered_profiles != NULL) {
		iterator = 0;   
		while (iterator < axl_list_length (registered_profiles)) {
			/* get the uri reference */
			uri = (char  *) axl_list_get_nth (registered_profiles, iterator);
			
			/* check if the profile is masked for this particular
			 * connection. */
			if (vortex_connection_is_profile_filtered (connection, -1, uri, NULL, NULL, NULL)) {
				vortex_log (VORTEX_LEVEL_DEBUG, "profile is filtered: %s", uri);
				
				/* update the iterator */
				iterator++;
				
				continue;
			} /* end if */

			/* check here the size to produce */
			size = strlen (uri);

			/* count profile uri size and accumulated
			 * counting plus 17, 6, 13) */
			if ((next_index + size + 36) >= buffer_size) {
				vortex_log (VORTEX_LEVEL_CRITICAL,  
					"found buffer to build greetings to be not enough to hold current profiles to advertise");
				return -1;
			} /* end if */
			
			/* copy the profile content */
			memcpy (greetings_buffer + next_index, "   <profile uri='", 17);
			next_index += 17;
			
			/* copy the actual profile */
			memcpy (greetings_buffer + next_index, uri, size);
			next_index += size;
			
			/* terminate profile def */
			memcpy (greetings_buffer + next_index, "' />\x0D\x0A", 6);
			next_index += 6;
			
			/* update to the next iterator */
			iterator++;
			
		} /* end if */
	} /* end if */
	
	/* terminate greetings  */
	memcpy (greetings_buffer + next_index, "</greeting>\x0D\x0A", 13);
	next_index += 13;
    
	return next_index;
}

/**
 * @internal
 * 
 * Sends the BEEP init connection greeting over the channel 0 of this @connection
 * This initial greetings message reply is sent by actual listener role to remote beep client peer.
 * 
 * @param connection The connection where the greeting will be sent.
 *
 * @return axl_true if greetings message was sent or axl_false if not
 **/
axl_bool      vortex_greetings_send (VortexConnection * connection)
{
	
	axlList       * registered_profiles;
	VortexCtx     * ctx                 = vortex_connection_get_ctx (connection);

	/* tecnically, the greetings initial message can't be larger
	 * than 4096 initial window. */
	char            greetings_buffer[5100];
	int             next_index = 0;

	/* get registered profiles */
	registered_profiles = vortex_profiles_get_actual_list_ref (ctx);

	/* build up supported registered profiles */
	if (registered_profiles == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, 
			    "unable to build and send greetings message: unable to find any profile registered");
		__vortex_connection_set_not_connected (connection, 
						       "unable to build and send greetings message: unable to find any profile registered", VortexError);
		return axl_false;
	}

	/* Build the greetings message with localization features and filtered profiles*/
	next_index = __vortex_greetings_build_message (connection, greetings_buffer, 5100);
	if (next_index == -1) {
		/* log */
		vortex_log (VORTEX_LEVEL_CRITICAL, "failed to build greetings message, closing the connection");
		/* and drop */
		__vortex_connection_set_not_connected (connection, 
						       "failed to build greetings message, closing the connection",
						       VortexError);
		return axl_false;
	} /* end if */

	/* send the message */
	if (!vortex_channel_send_rpy (vortex_connection_get_channel (connection, 0),
				      greetings_buffer,
				      next_index,
				      0)) {
		__vortex_connection_set_not_connected (connection, "unable to send listener greetings message", 
						       VortexError);
		return axl_false;
	} /* end if */
	
	return axl_true;
}

/**
 * @internal
 * 
 * These functions helps vortex library to parse and process greetings
 * message response.  The greetings message is a important piece of
 * the BEEP protocol. On this message, profiles for listener peer are
 * sent, so actual BEEP client can figure out about is possibility of
 * connection to remote site.
 *
 * This function should not be useful for vortex library consumer. It
 * is used mainly by vortex internal purposes.
 *
 * @param connection connection where a greeting reply is expected.
 * 
 * @return The frame read or NULL if it fails.
 **/
VortexFrame *  vortex_greetings_process (VortexConnection * connection)
{
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx          * ctx   = vortex_connection_get_ctx (connection);
#endif
	VortexFrame        * frame;
 	VortexFrame        * pending;
  
 	/* check if the connection have a pending frame (get the
	 * reference) */
 	pending = vortex_connection_get_data (connection,
 					      VORTEX_GREETINGS_PENDING_FRAME);

	/*
	 * Because this is a really especial case where we need to get
	 * totally blocked until the *WHOLE* frame is receive, we set
	 * the connection as blocking.
	 */
	frame = vortex_frame_get_next (connection);
	if (frame == NULL) {
		vortex_log (VORTEX_LEVEL_WARNING, "no frame received from remote peer");
		return NULL;
	}

	/* check pending frame */
	if (pending) {
		vortex_log (VORTEX_LEVEL_WARNING, "have pending flag at greetings process (previous incomplete frame received).");
		pending = vortex_frame_join (pending, frame);
		vortex_frame_unref (frame);
		frame   = pending;
	} /* end if */

	/* check if the frame returned is not complete, to store in
	 * the connection and return NULL */
	if (vortex_frame_get_more_flag (frame) > 0) {
		/* store the frame */
		vortex_connection_set_data_full (connection, 
						 /* key and data */
						 VORTEX_GREETINGS_PENDING_FRAME, frame,
						 NULL, (axlDestroyFunc) vortex_frame_unref);
		return NULL;
	} /* end if */

	/* call to update frame MIME status */
	if (! vortex_frame_mime_process (frame))
		vortex_log (VORTEX_LEVEL_WARNING, "failed to update MIME status for the frame, continue delivery");

	/* frame complete, clear connection content */
	vortex_connection_set_data (connection, 
				    /* key and data */
				    VORTEX_GREETINGS_PENDING_FRAME, NULL);

	/* ensure from this point to be frame not NULL  */
	if (vortex_greetings_is_reply_ok (frame, connection))
		return frame;
	return NULL;
}

/**
 * @internal
 * 
 * This function should not be useful for vortex library
 * consumers.This function returns if the greeting reply is ok. The
 * greeting reply is supposed to be held the the received frame.
 *
 * If the frame is wrong this function free the frame.
 * 
 * @param frame the frame which contains the greetings reply.
 *
 * @return axl_true if frame greeting reply is ok or axl_false if not.
 **/
axl_bool            vortex_greetings_is_reply_ok    (VortexFrame      * frame, VortexConnection * connection)
{
	VortexChannel * channel;
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx     * ctx = vortex_connection_get_ctx (connection);
#endif

	/* check greetings reply */
	if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY) {
		vortex_log (VORTEX_LEVEL_CRITICAL, 
			    "frame error, expected RPY frame type on greetings process");
		__vortex_connection_set_not_connected (connection, "frame error, expected RPY frame type on greetings process",
						       VortexProtocolError);
		vortex_frame_unref (frame);
		return axl_false;
	}

	/* check frame header spec */
	if (vortex_frame_get_channel (frame) != 0 || 
	    vortex_frame_get_msgno   (frame) != 0 || 
	    vortex_frame_get_seqno   (frame) != 0) {
		vortex_log (VORTEX_LEVEL_CRITICAL, 
			    "frame error, expected greetings message on channel 0, message 0 or sequence number 0");
		__vortex_connection_set_not_connected (connection, "frame error, expected greetings message on channel 0, message 0 or sequence number 0",
						       VortexProtocolError);
		vortex_frame_unref (frame);
		return axl_false;
	}
	
	/* check content-type reply */
	if ((vortex_frame_get_content_type (frame) == NULL) ||
	    (! axl_cmp (vortex_frame_get_content_type (frame), "application/beep+xml"))) {
		vortex_log (VORTEX_LEVEL_CRITICAL, 
			    "frame error, expected content type: application/beep+xml, not found");
		__vortex_connection_set_not_connected (connection, 
						       "frame error, expected content type: application/beep+xml, not found",
						       VortexProtocolError);
		vortex_frame_unref (frame);
		return axl_false;
	}

	/* update channel status */
	channel = vortex_connection_get_channel (connection, 0);
	vortex_channel_update_status_received (channel, 
					       vortex_frame_get_content_size (frame),
					       0,
					       UPDATE_SEQ_NO | UPDATE_RPY_NO);

	vortex_log (VORTEX_LEVEL_DEBUG, "greetings frame header specification is ok");
	
	return axl_true;	
}

/**
 * @internal
 * 
 * This function helps vortex library to be able to complete initial
 * session negotiation. It send the session greetings for a client
 * role. You can see vortex_greetings_send documentation for more
 * details.
 *
 * This function is mainly used by vortex library for its internal
 * purposes, so it should not be useful for vortex library consumers.
 * 
 * @return axl_true greetings message was sent or axl_false if not.
 **/
axl_bool           vortex_greetings_client_send     (VortexConnection * connection)
{
	/* tecnically, the greetings initial message can't be larger
	 * than 4096 initial window. */
	char        greetings_buffer[5100];
	int         next_index = 0;
	VortexCtx * ctx         = CONN_CTX (connection);
	axlList   * registered_profiles;
	
	/* build up supported registered profiles */
	registered_profiles = vortex_profiles_get_actual_list_ref (ctx);
	if (registered_profiles == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, 
			    "unable to build and send greetings message: unable to found any profile registered");
		return axl_false;
	}
	
	/* Build the greetings message with localization features and filtered profiles*/
	next_index = __vortex_greetings_build_message (connection, greetings_buffer, 5100);
	if (next_index == -1) {
		/* log */
		vortex_log (VORTEX_LEVEL_CRITICAL, "failed to build greetings message, closing the connection");
		/* and drop */
		__vortex_connection_set_not_connected (connection, 
						       "failed to build greetings message, closing the connection",
						       VortexError);
		return axl_false;
	} /* end if */

	/* send the message */
	if (!vortex_channel_send_rpy (vortex_connection_get_channel (connection, 0),
				      greetings_buffer,
				      next_index,
				      0)) {
		vortex_log (VORTEX_LEVEL_CRITICAL,  "unable to send initial client greetings message");
		__vortex_connection_set_not_connected (connection, 
						       "unable to send initial client greetings message",
						       VortexError);
		return axl_false;
	} /* end if */

	return axl_true;
}

/**
 * @internal
 * 
 * This function helps vortex library to be able to process client
 * greetings response process.  This function is mainly used by vortex
 * library for its internal purposes. This function should not be
 * useful for vortex library consumers.
 * 
 * @return The vortex frame read or NULL if it fails..
 **/
VortexFrame *  vortex_greetings_client_process (VortexConnection * connection)
{
	return vortex_greetings_process (connection);
}


/** 
 * @brief Allows to set advertised, optional, features attributes
 * while sending greeting element.
 *
 * Inside the 2.3.1.1, RFC 3080, The Greeting Message is defined that
 * the greeting message support an optional attribute called
 * "features". This attribute contains optional features that could be
 * advertised to the remote peer.
 *
 * This function allows to set the content of the "features"
 * attribute. It will be used by any greeting generated. These features
 * is only to be used to modify the channel management BEEP profile. 
 *
 * The function doesn't make any copy of the received value. You
 * should not free passed in value.
 *
 * @param ctx The context where the operation will be performed.
 *
 * @param features The string to be set as features. If provided a
 * NULL value, previous features will cleared.
 */
void           vortex_greetings_set_features   (VortexCtx * ctx, const char  * features)
{

	/* clear previous features installed */
	if (ctx->greetings_features)
		axl_free (ctx->greetings_features);
	ctx->greetings_features = NULL;

	/* return if no feature was provided */
	if (features == NULL)
		return;

	ctx->greetings_features = axl_strdup (features);

	return;
}

/** 
 * @brief Allows to get current "features" greetings attribute.
 *
 * Check documentation for: \ref vortex_greetings_set_features.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @return Current "features" status.
 */
const char        *  vortex_greetings_get_features   (VortexCtx * ctx)
{
	/* check context */
	if (ctx == NULL)
		return NULL;

	return ctx->greetings_features;
}

/** 
 * @brief Allows to set current "localize" greetings attribute. This
 * attribute allows to advertise to the remote peer the language
 * desired to generate error code responses.
 *
 * For values to identify your preferred language you must check: RFC
 * 3066, January 2001, "Tags for the Identification of Languages".
 *
 * This function doesn't make any copy of the received value. You
 * should not free passed in value.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param localize The localize value.
 */
void           vortex_greetings_set_localize   (VortexCtx * ctx, const char  * localize)
{
	/* check references */
	if (ctx == NULL || localize == NULL)
		return;

	/* clear previous localize installed */
	if (ctx->greetings_localize)
		axl_free (ctx->greetings_localize);
	ctx->greetings_localize = NULL;

	/* return if no feature was provided */
	if (localize == NULL)
		return;

	ctx->greetings_localize = axl_strdup (localize);
}

/** 
 * @brief Return current "localize" greetings optional attribute status.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @return Current localize value.
 */
const char        *  vortex_greetings_get_localize   (VortexCtx * ctx)
{
	if (ctx == NULL)
		return NULL;

	return ctx->greetings_localize;
}

/** 
 * @internal Cleanup the greetings module.
 * 
 * @param ctx The context where the state will be cleared.
 */
void           vortex_greetings_cleanup        (VortexCtx * ctx)
{
	v_return_if_fail (ctx);

	/* cleaup greetings module */
	axl_free (ctx->greetings_features);
	ctx->greetings_features = NULL;

	axl_free (ctx->greetings_localize);
	ctx->greetings_localize = NULL;

	return;
}

/* @} */
