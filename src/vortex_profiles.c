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

/* local include */
#include <vortex_ctx_private.h>

#define LOG_DOMAIN "vortex-profiles"

typedef struct _VortexProfile
{
	char                         * profile_name;
	VortexOnStartChannel           start;
	axlPointer                     start_user_data;
	VortexOnStartChannelExtended   start_extended;
	axlPointer                     start_extended_user_data;
	VortexOnCloseChannel           close;
	axlPointer                     close_user_data;
	VortexOnFrameReceived          received;
	axlPointer                     received_user_data;
	char                         * mime_type;
	char                         * transfer_encoding;
	int                            automatic_mime;
} VortexProfile;

/**
 * \defgroup vortex_profiles Vortex Profiles: Related functions to manage profiles inside Vortex Library.
 */

/**
 * \addtogroup vortex_profiles
 * @{
 */

void __vortex_profiles_destroy_profile_item (axlPointer data)
{
	VortexProfile * profile = data;

	/* free the profile name if were defined */
	if (profile->profile_name != NULL)
		axl_free (profile->profile_name);
	
	/* free the mime type value if were defined */
	if (profile->mime_type != NULL)
		axl_free (profile->mime_type);

	/* free the transfer encoding value if were defined */
	if (profile->transfer_encoding != NULL)
		axl_free (profile->transfer_encoding);

	/* free the profile node itself */
	axl_free (profile);
	
	return;
}

/** 
 * @internal
 *
 * @brief Default start handler, that is, accept to create a channel
 * always.
 */
axl_bool      __vortex_profiles_default_start (int                channel_num,
					       VortexConnection * connection,
					       axlPointer         user_data)
{
	return axl_true;
}

/** 
 * @internal
 *
 * @brief Default close handler, that is, accept to close the channel
 * always.
 */
axl_bool      __vortex_profiles_default_close (int                channel_num,
					       VortexConnection * connection,
					       axlPointer         user_data)
{
	return axl_true;
}


/** 
 * @brief Allows to register a new profile inside the Vortex Library.
 *
 * Register a profile to be used on channel creation. Profiles are
 * used by Vortex channels to know which message to exchange and the
 * channel semantics. To be able to create vortex channels you must
 * register at least one profile.
 *
 * On vortex session establishment, vortex peer acting as server sends
 * to vortex peer acting as client (or initiator) its registered
 * profiles. This enable both sides to know if they can talk together.
 *
 * In order to get an idea about profile names to use you can see
 * actual reserved profiles name defined by BEEP RFC. Some on them
 * are:
 *
 *  <ul>
 *   <li>The one time password profile: <i>"http://iana.org/beep/SASL/OTP"</i>
 *   <li>The TLS profile: <i>"http://iana.org/beep/TLS"</i>
 *   <li>The profile used by Coyote layer at Af-Arch: <i>"http://fact.aspl.es/profiles/coyote_profile"</i>
 *  </ul>
 *
 * Associated to the profile being registered are the handlers. There
 * are three handlers to be executed during the profile life.
 * 
 * When a remote peer wants to create a channel sends a start channel
 * message. On that event, when an start message is received, the \ref
 * VortexOnStartChannel "start" handler will be executed. 
 * 
 * When a remote peer wants to close an already created channel it
 * sends the close channel message. The \ref VortexOnCloseChannel "close" 
 * handler is executed on that event.
 * 
 * For all frames received, \ref VortexOnFrameReceived "received" handler will be executed.
 *
 * You can get more info about these handlers \ref vortex_handlers
 * "here". You can also read the following \ref profile_example
 * "document to know more about profiles".
 * 
 * If you don't provide handlers for \ref VortexOnStartChannel "start"
 * and \ref VortexOnCloseChannel "close", a default handler will be used.
 * 
 * These default handlers always return axl_true, so, on channel creation
 * it will accept always and on channel closing it will accept
 * always.
 *
 * If you don't provide a received handler, all data received will be
 * acknowledged but dropped.
 *
 * There are some exception to previous information. First one is that
 * there are two levels of handlers to be executed on events for
 * channels with a profile. The first level is defined by previous
 * ones handlers. But a second level of handlers exists on per-channel
 * basis.
 *
 * This second level of handlers are executed for the same events
 * before the first level is executed.  If a handler for a particular
 * event is not found on second level, then the first handler is
 * executed.
 *
 * If a handler for a particular event is found on second level, the
 * is executed and first level handler not.
 *
 * This allows you to have several levels of handlers to be executed
 * in a general way and handlers to be executed for a particular
 * channel.
 * 
 * Another exceptions comes with the \ref VortexOnStartChannel "start"
 * handler. It only allows you to get notified about the channel
 * number requested to be created and the connection where it was
 * received the petition. On most cases it is not needed more
 * information. 
 * 
 * However, the start message have several optional attributes and
 * content element that are used by profile definitions to implement
 * things such as TLS. On that case it is need to get notified with
 * all data available. You can check \ref
 * vortex_profiles_register_extended_start function to know more about
 * this issue.
 *
 * You can check \ref vortex_manual_dispatch_schema "this section" to know more
 * about how second level, first level and wait reply method 
 * implemented by Vortex Library to receive frames.
 *
 * You can also check \ref profile_example "this tutorial" about
 * creating new profiles for your application. 
 *
 * 
 * @param ctx                 The context where the operation will be performed.
 * @param uri                 A profile name to register
 * @param start               A handler to control channel creation
 * @param start_user_data     User defined data to be passed in to start handler
 * @param close               A handler to control channel termination
 * @param close_user_data     User defined data to be passed in to close handler
 * @param received            A handler to control incoming frames
 * @param received_user_data  User defined data to be passed in to received handler
 *
 * @return The function return axl_true if the profile was properly
 * registered. Otherwise axl_false is returned.
 */
int  vortex_profiles_register (VortexCtx             * ctx,
			       const char            * uri,
			       VortexOnStartChannel    start,
			       axlPointer              start_user_data,
			       VortexOnCloseChannel    close,
			       axlPointer              close_user_data,
			       VortexOnFrameReceived   received,
			       axlPointer              received_user_data)
{
	/* get current context */
	VortexProfile * profile;

	if (ctx == NULL)
		return axl_false;

	/* find out if we already have registered this profile */
	profile = vortex_hash_lookup (ctx->registered_profiles, (axlPointer) uri);
	if (profile == NULL) {
		vortex_log (VORTEX_LEVEL_DEBUG, "profile %s is not registered, storing settings",
			    uri);

		/* create a new profile */
		profile                     = axl_new (VortexProfile, 1);
		profile->profile_name       = axl_strdup (uri);
		profile->start              = start ? start : __vortex_profiles_default_start;
		profile->start_user_data    = start_user_data;
		profile->close              = close ? close : __vortex_profiles_default_close;
		profile->close_user_data    = close_user_data;
		profile->received           = received;
		profile->received_user_data = received_user_data;

		/* register the new profile */
		vortex_hash_replace (ctx->registered_profiles, profile->profile_name, profile);

		/* register in the list */
		axl_list_append (ctx->profiles_list, profile->profile_name);

		return axl_true;
	}

		vortex_log (VORTEX_LEVEL_DEBUG, "profile %s is already registered, updating its settings",
			    uri);
	
	/* set new data for the given profile */
	profile->start              = start ? start : __vortex_profiles_default_start;
	profile->start_user_data    = start_user_data;
	profile->close              = close ? close : __vortex_profiles_default_close;
	profile->close_user_data    = close_user_data;
	profile->received           = received;
	profile->received_user_data = received_user_data;

	return axl_true;
}

/** 
 * @brief Allows to unregister a profile already registered with \ref
 * vortex_profiles_register.
 *
 * The function is able to support calling several times to unregister
 * the same profile (or unregistering profiles not registered).
 * 
 * @param ctx The context where the operation will be performed.
 * @param uri The profile uri to unregister.
 *
 * @return axl_true if the profile was unregistered, otherwise axl_false is
 * returned.
 */
int      vortex_profiles_unregister              (VortexCtx             * ctx,
						  const char            * uri)
{
	/* get current context */
	VortexProfile * profile;

	if (ctx == NULL)
		return axl_false;

	/* find out if we already have registered this profile */
	profile = vortex_hash_lookup (ctx->registered_profiles, (axlPointer) uri);
	if (profile == NULL)
		return axl_false;

	/* remove by pointer */
	axl_list_remove_ptr (ctx->profiles_list, profile->profile_name);

	/* unregister the profile */
	vortex_hash_remove (ctx->registered_profiles, (axlPointer) uri);

	return axl_true;
}

/** 
 * @brief Allows to configure which is the default mime type to be set
 * for those channels created under the given profile.
 *
 * Default mime type and content transfer encoding used under beep is:
 *  - Content-Type: "application/octet-stream"
 *  - Content-Transfer-Encoding: binary
 *
 * They are used by default if no other value is set. 
 *
 * The function will perform a local copy of all the values provided
 * on this function, so you could safely unref them once the function
 * had finished.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param uri The profile which the given values for the mime
 * configuration will be applied.
 *
 * @param mime_type Content-Type configuration value. You can use NULL
 * values for parameter. In the case the value is not, previous value
 * is conserved.
 *
 * @param transfer_encoding Content-Transfer-Encoding configuration
 * value. You can use NULL values for parameter. In the case the value
 * is not, previous value is conserved.
 *
 * @return axl_true if the basic MIME configuration was done, otherwise
 * axl_false is returned.
 */
int      vortex_profiles_set_mime_type           (VortexCtx             * ctx,
						  const char            * uri,
						  const char            * mime_type,
						  const char            * transfer_encoding)
{
	/* get current context */
	VortexProfile * profile;

	if (ctx == NULL)
		return axl_false;

	/* find out if we already have registered this profile */
	profile = vortex_hash_lookup (ctx->registered_profiles, (axlPointer) uri);
	if (profile == NULL) {
		vortex_log (VORTEX_LEVEL_WARNING, "Attempting to configure mime and content type configuration over a profile not registered (%s)",
		       uri);
		return axl_false;
	}

	/* save current mime type if it is defined */
	if (mime_type != NULL) {
		if (profile->mime_type != NULL)
			axl_free (profile->mime_type);
		profile->mime_type = axl_strdup (mime_type);
	}
	
	/* save current transfer encoding type if it is defined */
	if (transfer_encoding != NULL) {
		if (profile->transfer_encoding != NULL)
			axl_free (profile->transfer_encoding);
		profile->transfer_encoding = axl_strdup (transfer_encoding);
	}
	
	/* just return */
	return axl_true;
}

/** 
 * @brief Allows to get current configuration for the default mime
 * type used for the given uri profile.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param uri The BEEP unique profile identification to get mime type
 * configuration from.
 * 
 * @return Current mime type configuration or NULL. Value returned
 * must not be unrefered. It is a internal copy. NULL is returned only
 * if an internal error have happen, in this case, the profile
 * requested doesn't exist.
 */
const char  * vortex_profiles_get_mime_type (VortexCtx * ctx, const char  * uri)
{
	/* get current context */
	VortexProfile * profile;

	if (ctx == NULL)
		return NULL;

	/* find out if we already have registered this profile */
	profile = vortex_hash_lookup (ctx->registered_profiles, (axlPointer) uri);
	if (profile == NULL) {
		vortex_log (VORTEX_LEVEL_WARNING, "Attempting to get mime type configuration over a profile not registered (%s)",
		       uri);
		return NULL;
	} /* end if */

	return (profile->mime_type != NULL) ? profile->mime_type : "application/octet-stream";
}

/** 
 * @brief Allows to get current configuration for the default content
 * transfer encoding used for the given uri profile.
 *
 * @param ctx The context where the operation will be performed.
 *
 * @param uri The BEEP unique profile identification to get content
 * transfer encoding configuration from.
 * 
 * @return Current content transfer encoding configuration or
 * NULL. Value returned must not be unrefered. It is a internal
 * copy. NULL is returned only if an internal error have happen, in
 * this case, the profile requested doesn't exist.
 */
const char  * vortex_profiles_get_transfer_encoding (VortexCtx   * ctx,
						     const char  * uri)
{
	/* get current context */
	VortexProfile * profile;
	
	/* check context received */
	if (ctx == NULL)
		return NULL;

	/* find out if we already have registered this profile */
	profile = vortex_hash_lookup (ctx->registered_profiles, (axlPointer) uri);
	if (profile == NULL) {
		vortex_log (VORTEX_LEVEL_WARNING, "Attempting to get mime type configuration over a profile not registered (%s)",
		       uri);
		return NULL;
	}

	return (profile->transfer_encoding != NULL) ? profile->transfer_encoding : "binary";
}

/** 
 * @brief Allows to register an extended version for the start message. 
 * 
 * Extended version supports the same than \ref VortexOnStartChannel
 * handler but also giving support for the profile requested, the
 * serverName attribute, the profile content and the profile content
 * encoding.
 *
 * Most profile definitions doesn't need the extended version for the
 * start message received. If the profile being implemented doesn't
 * need the profile uri, the serverName, the profile content and the
 * encoding you should use the \ref VortexOnStartChannel "start handler" 
 * defined at \ref vortex_profiles_register.
 *
 * Once defined the extended handler, the start handler defined at
 * \ref vortex_profiles_register will be ignored.
 *
 * @param ctx                      The context where the operation will be performed.
 * @param uri                      The uri profile to register the start channel extended
 * @param extended_start           The handler to be invoked when an start message is received
 * @param extended_start_user_data User defined data to be passed in to the handler.
 *
 * @return axl_true if the extended start handler was configured,
 * otherwise axl_false is returned.
 */
int      vortex_profiles_register_extended_start (VortexCtx                    * ctx,
						  const char                   * uri,
						  VortexOnStartChannelExtended   extended_start,
						  axlPointer                     extended_start_user_data)
{
	/* get current context */
	VortexProfile * profile;

	/* check reference */
	if (ctx == NULL)
		return axl_false;

	/* find out if we already have registered this profile */
	profile     = vortex_hash_lookup (ctx->registered_profiles, (axlPointer) uri);
	if (profile == NULL) {
		/* create a new profile if it weren't defined */
		profile                           = axl_new (VortexProfile, 1);
		profile->profile_name             = axl_strdup (uri);

		/* register extended start message */
		profile->start_extended           = extended_start;
		profile->start_extended_user_data = extended_start_user_data;

		/* save data */
		vortex_hash_replace (ctx->registered_profiles, profile->profile_name, profile);

		/* configure start and close default handlers */
		profile->start              = __vortex_profiles_default_start;
		profile->close              = __vortex_profiles_default_close;

		/* register in the list */
		axl_list_append (ctx->profiles_list, profile->profile_name);
	}
	
	/* register extended start message */
	profile->start_extended           = extended_start;
	profile->start_extended_user_data = extended_start_user_data;

	return axl_true;
}

/** 
 * @internal
 *
 * Invoke start handler for selected profile. As defined for
 * \ref vortex_profiles_register, the \ref VortexOnStartChannel "start handler"
 * must be defined or a default will be provided. Start handler is
 * invoked in order to know if a new channel using selected profile
 * can be created.
 * 
 * First it is check for an extended start handler. If not defined,
 * the start handlers is executed.
 * 
 * @param uri                   The uri profile identifying the channel requested to be created.
 * @param channel_num           The channel num requested to be created.
 * @param connection            The connection where the channel creation request has been received.
 * @param serverName            The serverName start attribute received.
 * @param profile_content       The profile content received.
 * @param profile_content_reply Optional profile content to be set on the start channel reply.
 * @param encoding              The profile content encoding.
 * 
 * @return axl_true if the channel was allowed to be created or axl_false if it fails.
 */
axl_bool      vortex_profiles_invoke_start (char             * uri, 
					    int                channel_num, 
					    VortexConnection * connection,
					    char             * serverName, 
					    char             * profile_content, 
					    char            ** profile_content_reply, 
					    VortexEncoding     encoding)
{
	VortexProfile * profile;
	VortexCtx     * ctx = vortex_connection_get_ctx (connection);

	v_return_val_if_fail (connection,                      axl_false);
	v_return_val_if_fail (channel_num >= 0,                axl_false);
	v_return_val_if_fail (uri,                             axl_false);
	v_return_val_if_fail (ctx && ctx->registered_profiles, axl_false);

	/* look up for the profile definition */
	profile = vortex_hash_lookup (ctx->registered_profiles, uri);
	if (profile == NULL) {
		vortex_log (VORTEX_LEVEL_DEBUG, "requiring to invoke start handler on a not registered profile");
		return axl_false;
	}

	/* check for a start extended */
	if (profile->start_extended != NULL)
		return profile->start_extended (uri, channel_num, connection, serverName, 
						profile_content, profile_content_reply, encoding,
						profile->start_extended_user_data);

	/* if no defined, exec start handler (no check is required
	 * because it has, at least, the default start handler
	 * defined) */
	return profile->start (channel_num, connection, profile->start_user_data);
}

/** 
 * @brief Returns if the given profiles have actually defined the start
 * handler.
 * 
 * This function check for the definition of the \ref
 * VortexOnStartChannel "start" handler defined at \ref
 * vortex_profiles_register or an \ref VortexOnStartChannelExtended "extended start" 
 * handler defined at \ref vortex_profiles_register_extended_start.
 *
 * It also returns no start handler defined if <i>uri</i> is not a
 * registered profile.
 * 
 * @param ctx The context where the operation will be performed.
 * @param uri The uri to check.
 * 
 * @return axl_true if close handler is defined or axl_false if not
 */
axl_bool      vortex_profiles_is_defined_start (VortexCtx   * ctx,
						const char  * uri)
{
	VortexProfile * profile;

	/* check references received */
	if (ctx == NULL || uri == NULL)
		return axl_false;

	profile = vortex_hash_lookup (ctx->registered_profiles, (axlPointer) uri);
	if (profile == NULL) {
		vortex_log (VORTEX_LEVEL_DEBUG, "looking up for a start handler on a non-registered profile");
		return axl_false;
	}

	return ((profile->start != NULL) || (profile->start_extended != NULL));
}

/** 
 * @internal
 * 
 * Invokes the first level close handler. This handler is always defined due to a close
 * handler provided by the user or due to the default handler provided by the vortex
 * library. 
 * 
 */
axl_bool      vortex_profiles_invoke_close (char             * uri, 
					    int                channel_num,
					    VortexConnection * connection)
{
	VortexProfile * profile;
	VortexCtx     * ctx = vortex_connection_get_ctx (connection);

	v_return_val_if_fail (connection,                      axl_false);
	v_return_val_if_fail (channel_num >= 0,                axl_false);
	v_return_val_if_fail (uri,                             axl_false);
	v_return_val_if_fail (ctx && ctx->registered_profiles, axl_false);

	profile = vortex_hash_lookup (ctx->registered_profiles, uri);

	if (profile == NULL) {
		vortex_log (VORTEX_LEVEL_DEBUG, "requiring to invoke close handler on a not registered profile");
		return axl_false;
	}

	return profile->close (channel_num, connection, profile->close_user_data);
}

/** 
 * @brief Returns if the given profiles have actually defined the close handler.
 *
 * It also returns no close handler defined if <i>uri</i> is not a
 * registered profile. 
 * 
 * @param ctx The context where the operation will be performed.
 * @param uri the uri profile to check.
 * 
 * @return axl_true if close handler is defined or axl_false if not
 */
axl_bool      vortex_profiles_is_defined_close (VortexCtx * ctx,
						const char  * uri)
{
	VortexProfile * profile;

	v_return_val_if_fail (uri,                             axl_false);
	v_return_val_if_fail (ctx && ctx->registered_profiles, axl_false);

	profile = vortex_hash_lookup (ctx->registered_profiles, (axlPointer) uri);
	if (profile == NULL) {
		vortex_log (VORTEX_LEVEL_DEBUG, "looking up for a close handler on a non-registered profile");
		return axl_false;
	}

	return (profile->close != NULL);
}

typedef struct _VortexProfileReceivedData {
	VortexProfile    * profile;
	int                channel_num;
	VortexConnection * connection;
	VortexFrame      * frame;
}VortexProfileReceivedData;

axlPointer __vortex_profiles_invoke_frame_received (axlPointer __data)
{
	VortexProfileReceivedData * data         = __data;
	VortexProfile             * profile      = data->profile;
	VortexConnection          * connection   = data->connection;
	VortexFrame               * frame        = data->frame;
	VortexChannel             * channel      = NULL;
	axl_bool                    is_connected = axl_false;
#if defined(ENABLE_VORTEX_LOG)
	VortexCtx                 * ctx          = vortex_connection_get_ctx (connection);
#endif

	/* get a reference to channel number so we can check after
	 * frame received handler if the channel have been closed.
	 * Once the frame received have finished this will help us to
	 * know if application space have issued a close channel. */
	int                       channel_num  = data->channel_num;

	/* free paramters data */
	axl_free (data);

	/* record actual connection state */
	is_connected = vortex_connection_is_ok (connection, axl_false);

	if (! is_connected) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "invoking frame receive on a non-connected session");
		goto free_resources;
	}

	/* get a reference to the channel to manage and the channel number */
	channel      = vortex_connection_get_channel (connection, channel_num);

	/* sees the connection is not properly running because the
	 * channel reference returned was null. */
	if (channel == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, 
			    "unable to get the channel reference (channel num=%d) over the connection (id=%d), avoiding frame deliver",
			    channel_num, vortex_connection_get_id (connection));
		goto free_resources;
	} /* end if */

	/* check to enforce serialize */
	if (vortex_channel_check_serialize (CONN_CTX(connection), connection, channel, frame)) {
		/* if the function returns axl_true, we must return
		 * because the message was stored for later
		 * delivery */
		return NULL;
	} /* end if */

 deliver_frame:

	/* invoke frame received on this channel */
	profile->received (channel, connection, frame, profile->received_user_data);

	/* check serialize to broadcast other waiting threads */
	if (vortex_channel_check_serialize_pending (CONN_CTX(connection), channel, &frame)) {
		/* if previous function returns axl_true, a new frame
		 * reference we have to deliver */
		goto deliver_frame;
	}

	/* before frame receive handler we have to check if client
	 * have closed its connection */
	is_connected = vortex_connection_is_ok (connection, axl_false);

	if (! is_connected ) {
		/* the connection have been closed inside frame
		 * receive */
		goto free_resources;
	}

	/* The function __vortex_channel_0_frame_received_close_msg
	 * can be blocked awaiting to receive all replies
	 * expected. The following signal tries to wake up a possible
	 * thread blocked until last_reply_expected change. */
	if (vortex_connection_channel_exists (connection, channel_num))
		vortex_channel_signal_on_close_blocked (channel);

	/* log a message */
	vortex_log (VORTEX_LEVEL_DEBUG, 
	       "invocation frame received handler for channel %d finished (first level: profiles)",
	       channel_num);

 free_resources:
	/* update channel reference */
	vortex_channel_unref (channel);

	/* decrease reference counting */
	vortex_connection_unref (connection, "first level handler (frame received)");

	/* before invoke profile received frame free frame */
	vortex_frame_unref (frame);

	/* return nothing */
	return NULL;
}

/** 
 * @internal
 * 
 * Invoke the first level frame received handler for the given profile
 * into the selected channel. This function also frees the vortex
 * frame passed in.
 *
 * @param uri 
 * @param channel_num 
 * @param connection 
 * @param frame 
 * 
 * @return axl_true if frame was delivered to a handler or axl_false if
 * frame was not delivered.
 */
axl_bool      vortex_profiles_invoke_frame_received (const char       * uri,
						     int                channel_num,
						     VortexConnection * connection,
						     VortexFrame      * frame)
{
	VortexProfile             * profile;
	VortexProfileReceivedData * data;
	VortexCtx                 * ctx = vortex_connection_get_ctx (connection);

	v_return_val_if_fail (uri,                             axl_false);
	v_return_val_if_fail (channel_num >= 0,                axl_false);
	v_return_val_if_fail (connection,                      axl_false); 
	v_return_val_if_fail (frame,                           axl_false);
	v_return_val_if_fail (ctx && ctx->registered_profiles, axl_false);

	profile = vortex_hash_lookup (ctx->registered_profiles, (axlPointer) uri);

	if ((profile == NULL) || (profile->received == NULL)) {
		vortex_log (VORTEX_LEVEL_DEBUG, "invoking frame received handler on a profile which haven't been defined");
		return axl_false;
	}

	/*
	 * invoke frame received handler. Because frame must be run
	 * inside a separated thread (as defined in user doc) we have
	 * to do that. This contrast with close and start invocation
	 * which have to return immediately is channel can be close or
	 * created.
	 *
	 * Moreover, the close and start channel are invoked in a
	 * separated thread because the channel 0's frame received
	 * handler (the only one channel which can process close and
	 * start message is the 0), is run inside a separated thread. See vortex_frame_
	 
	 * invoke frame received */
	data              = axl_new (VortexProfileReceivedData, 1);
	data->profile     = profile;
	data->channel_num = channel_num;
	data->connection  = connection;
	data->frame       = frame;

	/* increase connection reference counting to avoid reference
	 * loosing if connection is closed */
	if (! vortex_connection_ref (connection, "first level handler (frame received)")) {
		/* unable to increate reference for the connection */
		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to increase connection reference, avoiding delivering data (dropping frame)..");

		/* deallocate resources */
		vortex_frame_unref (frame);
		axl_free (data);
		return axl_false;
	}

	/* update channel reference */
	vortex_channel_ref (vortex_connection_get_channel (connection, channel_num));

	/* call to deliver the frame */
	vortex_thread_pool_new_task (ctx, __vortex_profiles_invoke_frame_received, data);
	
	return axl_true;
}

/** 
 * @brief  Returns if the given profiles have actually defined the received  handler. 
 * 
 * It also returns no received handler defined if <i>uri</i> is not a
 * registered profile.
 * 
 * @param ctx The context where the operation will be performed.
 * @param uri The uri to check.
 * 
 * @return axl_true if received handler is defined or axl_false if not
 */
axl_bool      vortex_profiles_is_defined_received (VortexCtx   * ctx,
						   const char  * uri)
{
	VortexProfile * profile;

	/* check references received */
	if (ctx == NULL || uri == NULL)
		return axl_false;

	profile = vortex_hash_lookup (ctx->registered_profiles, (axlPointer) uri);
	if (profile == NULL) {
		vortex_log (VORTEX_LEVEL_DEBUG, "looking up for a received handler on a non-registered profile");
		return axl_false;
	}

	return (profile->close != NULL);
}



axl_bool  __get_actual_list (axlPointer     key,
			     axlPointer     value,
			     axlPointer     user_data)
{
	/* add the element */
	axl_list_append ((axlList *) user_data, axl_strdup ((char *) key));

	return axl_false;
}

/** 
 * @brief Returns current profiles registered, in a newly allocated list.
 * 
 * The axlList returned contains all uri that have been
 * registered by using \ref vortex_profiles_register.
 * 
 * @param ctx The context where the operation will be performed.
 *
 * You must free returned after using it. Use axl_list_free.  
 * 
 * @return the actual profile registered list.
 */
axlList * vortex_profiles_get_actual_list (VortexCtx * ctx)
{
	/* get current context */
	axlList   * result = NULL;

	v_return_val_if_fail (ctx && ctx->registered_profiles, NULL);

	/* create the list */
	result = axl_list_new (axl_list_always_return_1, axl_free);

	vortex_hash_foreach (ctx->registered_profiles, __get_actual_list, result);

	return result;
}

/** 
 * @brief Return current profiles registered, in a internally created
 * list.
 * 
 * @param ctx The context where the operation will be performed.
 * 
 * @return A reference to the profile list. Do not manipulate or
 * dealloc the list.
 */
axlList * vortex_profiles_get_actual_list_ref (VortexCtx * ctx)
{
	/* check reference context received */
	if (ctx == NULL)
		return NULL;

	/* return the reference */
	return ctx->profiles_list;
}

/** 
 * @brief Return the actual number of profiles registered. 
 * 
 * @param ctx The context where the operation will be performed.
 *
 * @return number of profiles registered.
 */
int     vortex_profiles_registered (VortexCtx * ctx)
{
	/* check reference context received */
	if (ctx == NULL)
		return 0;

	if (ctx->registered_profiles == NULL)
		return 0;
	return vortex_hash_size (ctx->registered_profiles);
}

/** 
 * @brief Return if a profile identifier is registered. 
 * 
 * @param ctx The context where the operation will be performed.
 * @param uri The unique URI identifier used to lookup.
 * 
 * @return axl_true if the profile is registered or axl_false if not. 
 */
axl_bool      vortex_profiles_is_registered (VortexCtx  * ctx, 
					     const char  * uri)
{
	/* check references */
	if (ctx == NULL || uri == NULL)
		return axl_false;

	return (vortex_hash_lookup (ctx->registered_profiles, (axlPointer) uri) != NULL);
}

/** 
 * @brief Allows to configure automatic MIME header addition handling
 * at profile level.
 * 
 * See \ref vortex_manual_using_mime for a long explanation. In sort,
 * this function allows to configure if MIME headers should be added
 * or not automatically on each message sent using the family of
 * functions vortex_channel_send_*.
 *
 * The function allows to configure at profile level automatic MIME
 * handling associated to the profile. This configuration will
 * override configuration provided at \ref vortex_conf_set and \ref
 * VORTEX_AUTOMATIC_MIME_HANDLING.
 *
 * Use the following values for "value":
 * 
 * - 1: Enable automatic MIME handling for messages send under the
 * profile provided, making the configuration process to not check
 * next levels.
 *
 * - 0: Makes automatic MIME handling configuration at profile level
 * to have no signification, making the configuration process to check
 * next levels.
 *
 * - 2: Disable automatic MIME handling, making the configuration
 * process to not check next levels.
 *
 * @param ctx The context where the operation will be performed.
 *
 * @param uri The uri profile to be configured.
 *
 * @param value The value to be configured.
 */
void      vortex_profiles_set_automatic_mime      (VortexCtx   * ctx,
						   const char  * uri, 
						   int           value)
{
	VortexProfile             * profile;

	v_return_if_fail (uri);
	v_return_if_fail (ctx);
	v_return_if_fail (value == 0 || value == 1 || value == 2);

	profile = vortex_hash_lookup (ctx->registered_profiles, (axlPointer)uri);
	if (profile == NULL) {
		vortex_log (VORTEX_LEVEL_DEBUG, 
			    "configuring automatic mime handling on a profile not registered=%s (value=%d)", 
			    uri, value);
		return;
	} /* end if */

	/* configuring automatic MIME handling */
	profile->automatic_mime = value;
	return;
}

/** 
 * @brief Allows to get current automatic MIME handling associated to
 * the profile provided. See \ref vortex_profiles_set_automatic_mime
 * function for values returned.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param uri The profile that is required to return the value
 * associated.
 * 
 * @return The value associated to the automatic MIME handling
 * configured on the profile provided. The function return 0 (not
 * configured) if the parameter is NULL or the profile wasn't
 * registered.
 */
int       vortex_profiles_get_automatic_mime      (VortexCtx   * ctx,
						   const char  * uri)
{
	VortexProfile             * profile;

	v_return_val_if_fail (uri, 0);

	profile = vortex_hash_lookup (ctx->registered_profiles, (axlPointer)uri);
	if (profile == NULL) {
		vortex_log (VORTEX_LEVEL_DEBUG, 
			    "accessing to automatic mime handling on a profile not registered=%s", 
			    uri);
		return 0;
	} /* end if */

	/* configuring automatic MIME handling */
	return profile->automatic_mime;
}

/** 
 * @internal Init profiles module.
 * 
 * @param ctx The context where the profile module state will be
 * initialized.
 */
void    vortex_profiles_init (VortexCtx * ctx) 
{
	v_return_if_fail (ctx);

	ctx->registered_profiles = 
		vortex_hash_new_full (axl_hash_string, axl_hash_equal_string,
				      NULL, /* no key destruction function */
				      (axlDestroyFunc) __vortex_profiles_destroy_profile_item); /* value destroy function */

	ctx->profiles_list = axl_list_new (axl_list_always_return_1, NULL);

	return;
}

/** 
 * @internal Cleanup the profiles module state on the provided vortex
 * context.
 * 
 * @param ctx The vortex context to cleanup.
 */
void  vortex_profiles_cleanup (VortexCtx * ctx)
{
	v_return_if_fail (ctx);

	/* destroy the hash, including profile items. It is done by
	 * configuring __vortex_profiles_destroy_profile_item function
	 * at hash time creation. */
	vortex_hash_destroy (ctx->registered_profiles);
	ctx->registered_profiles = NULL;

	/* destroy the list */
	axl_list_free (ctx->profiles_list);
	ctx->profiles_list = NULL;

	return;
}

/* @} */
