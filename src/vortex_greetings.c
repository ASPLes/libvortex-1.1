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
 *         C/ Dr. Michavila Nº 14
 *         Coslada 28820 Madrid
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://fact.aspl.es
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
 * Sends the BEEP init connection greeting over the channel 0 of this @connection
 * This initial greetings message reply is sent by actual listener role to remote beep client peer.
 * 
 * @param connection The connection where the greeting will be sent.
 *
 * @return true if greetings message was sent or false if not
 **/
bool     vortex_greetings_send (VortexConnection * connection)
{
	
	axlList       * registered_profiles;
	int             iterator;
	const char    * localize            = NULL;
	const char    * features            = NULL;
	char          * uri;
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
		       "unable to build and send greetings message: unable to found any profile registered");
		return false;
	}

	/* copy greetings */
	memcpy (greetings_buffer, "<greeting", 9);
	next_index += 9;
	features = vortex_greetings_get_features (ctx);
	if (features) {
		memcpy (greetings_buffer + next_index, " features='", 11);
		next_index += 11;

		memcpy (greetings_buffer + next_index, features, strlen (features));
		next_index += strlen (features);

		memcpy (greetings_buffer + next_index, "'", 1);
		next_index ++;
	} /* end features */

	localize = vortex_greetings_get_localize (ctx);
	if (localize) {
		memcpy (greetings_buffer + next_index, " localize='", 11);
		next_index += 11;

		memcpy (greetings_buffer + next_index, localize, strlen (localize));
		next_index += strlen (localize);

		memcpy (greetings_buffer + next_index, "'", 1);
		next_index ++;
	} /* end features */

	memcpy (greetings_buffer + next_index, ">\x0D\x0A", 3);
	next_index += 3;

	iterator = 0;	
	while (iterator < axl_list_length (registered_profiles)) {
		/* get the uri reference */
		uri = (char  *) axl_list_get_nth (registered_profiles, iterator);
		
		/* check if the profile is masked for this particular
		 * connection. */
		if (vortex_connection_is_profile_filtered (connection, -1, uri, NULL, NULL)) {
			vortex_log (VORTEX_LEVEL_DEBUG, "profile is filtered: %s", uri);
			
			/* update the iterator */
			iterator++;
		    
			continue;
		} /* end if */

		/* copy the profile content */
		memcpy (greetings_buffer + next_index, "   <profile uri='", 17);
		next_index += 17;

		/* copy the actual profile */
		memcpy (greetings_buffer + next_index, uri, strlen (uri));
		next_index += strlen (uri);

		/* terminate profile def */
		memcpy (greetings_buffer + next_index, "' />\x0D\x0A", 6);
		next_index += 6;

		/* update to the next iterator */
		iterator++;

	} /* end if */

	/* terminate greetings  */
	memcpy (greetings_buffer + next_index, "</greeting>\x0D\x0A", 13);
	next_index += 13;

	/* send the message */
	if (!vortex_channel_send_rpy (vortex_connection_get_channel (connection, 0),
				      greetings_buffer,
				      next_index,
				      0)) {
		__vortex_connection_set_not_connected (connection, "unable to send listener greetings message");
		return false;
	} /* end if */

	return true;
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
	VortexFrame        * frame;
#if defined(ENABLE_VORTEX_DEBUG)
	VortexCtx          * ctx   = vortex_connection_get_ctx (connection);
#endif

	/*
	 * Because this is a really especial case where we need to get
	 * totally blocked until the *WHOLE* frame is receive, we set
	 * the connection as blocking.
	 */
	frame = vortex_frame_get_next (connection);
	if (frame == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "no frame received from remote peer");
		return NULL;
	}

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
 * @return true if frame greeting reply is ok or false if not.
 **/
bool           vortex_greetings_is_reply_ok    (VortexFrame      * frame, VortexConnection * connection)
{
	VortexChannel * channel;
#if defined(ENABLE_VORTEX_DEBUG)
	VortexCtx     * ctx = vortex_connection_get_ctx (connection);
#endif

	/* check greetings reply */
	if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY) {
		__vortex_connection_set_not_connected (connection, "frame error, expected RPY frame type on greetings process");
		vortex_frame_unref (frame);
		return false;
	}

	/* check frame header spec */
	if (vortex_frame_get_channel (frame) != 0 || 
	    vortex_frame_get_msgno   (frame) != 0 || 
	    vortex_frame_get_seqno   (frame) != 0) {
		__vortex_connection_set_not_connected (connection, "frame error, expected greetings message on channel 0, message 0 or sequence number 0");
		vortex_frame_unref (frame);
		return false;
	}
	
	/* check content-type reply */
	if ((vortex_frame_get_content_type (frame) == NULL) ||
	    (! axl_cmp (vortex_frame_get_content_type (frame), "application/beep+xml"))) {

		__vortex_connection_set_not_connected (connection, 
						       "frame error, expected content type: application/beep+xml, not found");
		vortex_frame_unref (frame);
		return false;
	}

	/* update channel status */
	channel = vortex_connection_get_channel (connection, 0);
	vortex_channel_update_status_received (channel, 
					       vortex_frame_get_content_size (frame),
					       UPDATE_SEQ_NO | UPDATE_RPY_NO);

	vortex_log (VORTEX_LEVEL_DEBUG, "greetings frame header specification is ok");
	
	return true;	
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
 * @return true greetings message was sent or false if not.
 **/
bool          vortex_greetings_client_send     (VortexConnection * connection)
{
	char      * the_payload = NULL;
	char      * features    = NULL;
	char      * localize    = NULL;
	VortexCtx * ctx         = CONN_CTX (connection);
	
	/* check for features and localize */
	if (vortex_greetings_get_features (ctx) != NULL)
		features = axl_strdup_printf (" features='%s'", vortex_greetings_get_features (ctx));
	if (vortex_greetings_get_localize (ctx) != NULL)
		localize = axl_strdup_printf (" localize='%s'", vortex_greetings_get_localize (ctx));

	the_payload = axl_strdup_printf ("<greeting %s%s/>\x0D\x0A",
					 (features != NULL) ? features : "",
					 (localize != NULL) ? localize : "");
	/* free value */
	if (features != NULL)
		axl_free (features);
	if (localize != NULL)
		axl_free (localize);

	/* send the message */
	if (!vortex_channel_send_rpy (vortex_connection_get_channel (connection, 0),
				      the_payload,
				      strlen (the_payload),
				      0)) {
		axl_free (the_payload);
		vortex_log (VORTEX_LEVEL_CRITICAL,  "unable to send initial client greetings message");
		__vortex_connection_set_not_connected (connection, 
						       "unable to send initial client greetings message");
		return false;
	}
	
	axl_free (the_payload);				      
	return true;
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
	VortexFrame * frame = vortex_greetings_process (connection);

	return frame;
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
