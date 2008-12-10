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

/* local include */
#include <vortex_pull.h>

#define VORTEX_PULL_QUEUE_KEY   "vo:pu:qu"
#define VORTEX_PULL_EVENT_MASKS "vo:pu:ev:ma"

/**
 * \defgroup vortex_pull Vortex PULL API: A pull API suited for singled threaded or event driven environments
 */

/**
 * \addtogroup vortex_pull
 * @{
 */

struct _VortexEvent {
	/* context where the event happened */
	VortexCtx        * ctx;

	/* @internal reference counting */
	int                ref_count;
	VortexMutex        ref_count_mutex;

	/* @internal Event type reported. */
	VortexEventType    type;

	/* @internal Optional channel reference reported */
	VortexChannel    * channel;

	/* @interanl Optional connection reference reported */
	VortexConnection * conn;

	/* @internal Optional frame reference reported */
	VortexFrame      * frame;

	/* @internal Reference to the msgno reported */
	int                msgno;

	/* @internal Reference to the serverName defined */
	char             * serverName;

	/* @internal Reference to the profile content defined */
	char             * profile_content;

	/* @internal Profile content encoding */
	VortexEncoding     encoding;
};

struct _VortexEventMask {
	/* @internal Mask string identifier. */
	char             * mask_id;
	/* @internal Mask defined */
	int                mask;
	/* @internal Mask activation state */
	axl_bool           is_enabled;
};

/**
 * @internal Function used to create empty events.
 */
VortexEvent * __vortex_event_new_empty (VortexEventType    type,
					VortexCtx        * ctx, 
					VortexConnection * conn,
					axl_bool           checked_conn_ref,
					VortexChannel    * channel,
					VortexFrame      * frame)
{
	/* create event to report */
	VortexEvent * event   = axl_new (VortexEvent, 1);
	event->ref_count      = 1;
	vortex_mutex_create (&event->ref_count_mutex);

	/* configure default values */
	event->msgno          = -1;

	/* update reference connection */
	if (conn) {
		/* ref and set */
		if (checked_conn_ref && vortex_connection_ref (conn, "__vortex_event_new_empty")) 
			event->conn = conn;
		else if (! checked_conn_ref && vortex_connection_uncheck_ref (conn)) 
			event->conn = conn;
	} /* end if */
	
	/* now update channel reference counting */
	if (channel) {
		/* ref and set */
		if (vortex_channel_ref (channel))
			event->channel = channel;
	} /* end if */

	/* increase frame reference counting to make the VortexEvent
	 * to own its reference */
	if (frame) {
		if (vortex_frame_ref (frame))
			event->frame = frame;
	} /* end if */

	/* context */
	event->ctx        = ctx;

	/* type */
	event->type       = type;

	/* return event created */
	return event;
}

/**
 * @brief Activates the pull based event notification. This interface
 * allows single threaded applications to better interface with vortex
 * API. Due to is threading nature, this pull API allows single
 * threaded deesign to "fetch" or "pull" pending events rather
 * receiving a notification through a handler.
 *
 * Once the PULL API is enabled, no async handler will be called and,
 * it is required by the application programmer do not configure new
 * handlers since that would produce an undefined behavior.
 *
 * A proper activation sequence is described in the following example:
 *
 * \code
 * VortexCtx * client_ctx;
 *
 * // create an indepenent client context 
 * client_ctx = vortex_ctx_new ();
 *
 * // init vortex on this context 
 * if (! vortex_init_ctx (client_ctx)) {
 *      printf ("ERROR: failed to init client vortex context for PULL API..\n");
 *      return axl_false;
 * }
 *
 * // now activate PULL api on this context 
 * if (! vortex_pull_init (ctx)) {
 *      printf ("ERROR: failed to activate PULL API after vortex initialization..\n");
 * }
 *
 * // do some work and fetch events with vortex_pull_next_event
 * \endcode
 * 
 * To stop library function, the usual function must be used: \ref
 * vortex_exit_ctx
 */
axl_bool           vortex_pull_init               (VortexCtx * ctx)
{
	VortexAsyncQueue * pull_pending_events;
	axlList          * event_masks;

	if (ctx == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Failed to activate PULL API because no vortex context is defined");
		return axl_false; 
	} /* end if */

	/* get the reference to the pull queue */
	pull_pending_events = vortex_ctx_get_data (ctx, VORTEX_PULL_QUEUE_KEY);
	if (ctx != NULL && pull_pending_events != NULL) {
		vortex_log (VORTEX_LEVEL_DEBUG, "Pull API is already activated (found defined pending events queue)");
		return axl_true;
	}
	
	/* configure event pending queue into the context */
	pull_pending_events = vortex_async_queue_new ();
	vortex_ctx_set_data (ctx, VORTEX_PULL_QUEUE_KEY, pull_pending_events);

	/* create the list of event masks */
	event_masks = axl_list_new (axl_list_always_return_1, (axlDestroyFunc) vortex_event_mask_free);
	vortex_ctx_set_data (ctx, VORTEX_PULL_EVENT_MASKS, event_masks);
	
	/* configure internal handlers to receive notifications */

	/* VORTEX_EVENT_FRAME_RECEIVED */
	vortex_ctx_set_frame_received          (ctx, vortex_pull_frame_received, ctx);

	/* VORTEX_EVENT_CLOSE_REQUEST */
	vortex_ctx_set_close_notify_handler    (ctx, vortex_pull_close_notify, ctx);

	/* VORTEX_EVENT_CHANNEL_ADDED */
	vortex_ctx_set_channel_added_handler   (ctx, vortex_pull_channel_added, ctx);

	/* VORTEX_EVENT_CHANNEL_REMOVED */
	vortex_ctx_set_channel_removed_handler (ctx, vortex_pull_channel_removed, ctx);

	/* VORTEX_EVENT_CONNECTION_CLOSED */
	vortex_connection_set_connection_actions (ctx, CONNECTION_STAGE_POST_CREATED, 
						  vortex_pull_register_close_connection, ctx);

	/* VORTEX_EVENT_CONNECTION_ACCEPTED */
	vortex_listener_set_on_connection_accepted (ctx, vortex_pull_connection_accepted, ctx);

	/* VORTEX_EVENT_CHANNEL_START */
	vortex_ctx_set_channel_start_handler (ctx, vortex_pull_start_handler, ctx);

	/* install auto-cleanup on ctx termimation */
	vortex_ctx_install_cleanup (ctx, (axlDestroyFunc) vortex_pull_cleanup);
	
	/* return pull api configured */
	return axl_true;
}

/**
 * @brief Allows to uninstall pull API and to cleanup all allocated
 * resources.
 *
 * @param ctx The context where the operation will take place.
 */
void           vortex_pull_cleanup            (VortexCtx * ctx)
{
	/* get the reference to the pull queue */
	VortexAsyncQueue * pull_pending_events = vortex_ctx_get_data (ctx, VORTEX_PULL_QUEUE_KEY);
	axlList          * list                = vortex_ctx_get_data (ctx, VORTEX_PULL_EVENT_MASKS);
	VortexEvent      * event;

	if (ctx == NULL || pull_pending_events == NULL) {
		return;
	} /* end if */

	/* uninstall handlers */
	vortex_ctx_set_frame_received (ctx, NULL, NULL);
	vortex_ctx_set_close_notify_handler (ctx, NULL, NULL);
	vortex_ctx_set_channel_start_handler (ctx, NULL, NULL);

	/* remove all pending items */
	while (vortex_async_queue_items (pull_pending_events) > 0) {
		/* next event */
		event = vortex_async_queue_pop (pull_pending_events);

		/* unref */
		vortex_event_unref (event);
	} /* end while */

	/* terminate queue */
	vortex_async_queue_unref (pull_pending_events);

	/* terminate lists */
	axl_list_free (list);

	return;
}

/**
 * @brief Allows to check if there are pending events in the
 * queue. This function is useful in the case the caller want to be
 * not blocked on the next call to vortex_pull_next_event. 
 *
 * @param ctx The context with PULL API activated, where pending
 * events will be checked.
 *
 * @return axl_true in the case pending events are waiting to be read;
 * otherwise axl_false is returned.
 */
axl_bool           vortex_pull_pending_events     (VortexCtx * ctx)
{
	VortexAsyncQueue * pull_pending_events = vortex_ctx_get_data (ctx, VORTEX_PULL_QUEUE_KEY);

	/* return if there are pending elements stored */
	return (vortex_async_queue_items (pull_pending_events) > 0) ? axl_true : axl_false;
}

/**
 * @brief Allows to get the number of pending events to be read. 
 *
 * @param ctx The context with PULL API activated, where pending
 * events will be checked.
 *
 * @return The number of pending events to be read. The function
 * return -1 if it fails to get number of items stored.
 */
int                vortex_pull_pending_events_num (VortexCtx * ctx)
{
	VortexAsyncQueue * pull_pending_events = vortex_ctx_get_data (ctx, VORTEX_PULL_QUEUE_KEY);

	/* return if there are pending elements stored */
	return vortex_async_queue_items (pull_pending_events);
}


/**
 * @brief Allows to get next pending event. The function will block
 * the caller in the case no event is available. 
 *
 * @param ctx The context with PULL API activated, where pending
 * events will be checked.
 *
 * @param milliseconds_to_wait How many milliseconds to wait for an
 * event before returning NULL. In the case 0 is used, the function
 * will wait with no limit until next event.
 *
 * @return A reference to the \ref VortexEvent representing an async
 * event received. According to the type returned by \ref
 * vortex_event_get_type, the event has particular data
 * associated. See \ref VortexEventType for more information. In the
 * case a limited wait (milliseconds_to_wait) is configured, the
 * function can return NULL, which means timeout reached.
 */
VortexEvent      * vortex_pull_next_event         (VortexCtx * ctx,
						   int         milliseconds_to_wait)
{
	VortexAsyncQueue * pull_pending_events = vortex_ctx_get_data (ctx, VORTEX_PULL_QUEUE_KEY);

	if (milliseconds_to_wait > 0) {
		/* limited wait */
		return vortex_async_queue_timedpop (pull_pending_events, milliseconds_to_wait);
	} /* end if */

	/* return if there are pending elements stored */
	return vortex_async_queue_pop (pull_pending_events);
}

/**
 * @brief Allows to increase the reference counting associated to the
 * event.
 *
 * @param event \ref VortexEvent event reference to increase its
 * reference counting by one unit.
 *
 * @return axl_true if the reference counting was properly
 * updated. Otherwise axl_false is returned.
 */
axl_bool           vortex_event_ref                (VortexEvent * event)
{
	if (event == NULL)
		return axl_false;

	/* lock */
	vortex_mutex_lock (&event->ref_count_mutex);

	/* increase reference */
	event->ref_count++;

	/* unlock */
	vortex_mutex_unlock (&event->ref_count_mutex);
	
	return axl_true;
} /* end if */

/**
 * @brief Allows to reduce the reference counting on the provided \ref
 * VortexEvent instance. In the case 0 is reached the object is
 * terminated.
 *
 * @param event The VortexEvent to be unreferenced.
 */ 
void               vortex_event_unref              (VortexEvent * event)
{
	if (event == NULL)
		return;

	/* lock */
	vortex_mutex_lock (&event->ref_count_mutex);

	/* decrease reference */
	event->ref_count--;
	if (event->ref_count != 0) {
		/* unlock */
		vortex_mutex_unlock (&event->ref_count_mutex);
		return;
	} /* end if */

	/* terminate the reference */
	if (event->frame) {
		vortex_frame_unref (event->frame);
		event->frame = NULL;
	} /* end if */
	
	if (event->conn) {
		vortex_connection_unref (event->conn, "vortex_event_unref");
		event->conn = NULL;
	} /* end if */

	if (event->channel) {
		vortex_channel_unref (event->channel);
		event->channel = NULL;
	} /* end if */

	/* clear serverName and profile content */
	axl_free (event->serverName);
	event->serverName = NULL;
	axl_free (event->profile_content);
	event->profile_content = NULL;

	/* unlock */
	vortex_mutex_unlock (&event->ref_count_mutex);
	vortex_mutex_destroy (&event->ref_count_mutex);

	/* free the node itself */
	axl_free (event);
	
	return;
}

/**
 * @brief Allows to get event type from the provided \ref VortexEvent
 * reference.
 *
 * @return The type configured or \ref VORTEX_EVENT_UNKNOWN.
 */
VortexEventType    vortex_event_get_type           (VortexEvent * event)
{
	/* check NULL reference */
	if (event == NULL)
		return VORTEX_EVENT_UNKNOWN;

	/* return type configured */
	return event->type;
}

/**
 * @brief Allows to get the context reference where the event was
 * reported.
 *
 * @param event The event reference to get the context from.
 *
 * @return A reference to the context or NULL if it fails.
 */
VortexCtx        * vortex_event_get_ctx            (VortexEvent * event)
{
	if (event == NULL)
		return NULL;
	/* return context configured */
	return event->ctx;
}

/**
 * @brief Allows to get the connection reference associated to the
 * event.
 *
 * @param event The event reference to get the connection from.
 *
 * @return A reference to the connection or NULL if it is not defined.
 */
VortexConnection * vortex_event_get_conn           (VortexEvent * event)
{
	if (event == NULL)
		return NULL;

	/* return connection configured */
	return event->conn;
}

/**
 * @brief Allows to get the channel reference or NULL if it fails.
 *
 * @param event The event reference to get the channel from.
 *
 * @return A reference to the channel or NULL if it is not defined.
 */
VortexChannel    * vortex_event_get_channel        (VortexEvent * event)
{
	if (event == NULL)
		return NULL;
	/* return channel configured */
	return event->channel;
}

/**
 * @brief Allows to get the frame reference or NULL if it fails.
 *
 * @param event The event reference to get the frame from.
 *
 * @return A reference to the frame or NULL if it is not defined.
 */
VortexFrame      * vortex_event_get_frame          (VortexEvent * event)
{
	if (event == NULL)
		return NULL;
	/* return frame configured */
	return event->frame;
}

/**
 * @brief Allows to get the msgno value configured on the provided
 * event.
 *
 * @param event The event reference to get the msgno value from.
 *
 * @return The msgno configured or -1 if it fails.
 */
int                vortex_event_get_msgno         (VortexEvent * event)
{
	if (event == NULL)
		return -1;
	/* return msgno configured */
	return event->msgno;
}

/**
 * @brief Allows to get the serverName value defined on the channel
 * start request received (\ref VORTEX_EVENT_CHANNEL_START).
 *
 * @param event The event reference to get the serverName value from.
 *
 * @return The serverName value defined, otherwise NULL if nothing was
 * defined.
 */
const char       * vortex_event_get_server_name            (VortexEvent * event)
{
	if (event == NULL)
		return NULL;
	/* return serverName configured */
	return event->serverName;
}

/**
 * @brief Allows to get the profile content defined on the
 * channel start request received (\ref VORTEX_EVENT_CHANNEL_START).
 *
 * @param event The event reference to get the profile content value from.
 *
 * @return The profile content value defined, otherwise NULL if nothing was
 * defined.
 */
const char       * vortex_event_get_profile_content        (VortexEvent * event)
{
	if (event == NULL)
		return NULL;

	/* return profile_content configured */
	return event->profile_content;
}

/**
 * @brief Allows to get the profile encoding defined on the channel
 * start request received (\ref VORTEX_EVENT_CHANNEL_START).
 *
 * @param event The event reference to get the profile encoding value
 * from.
 *
 * @return The profile encoding value defined.
 */
VortexEncoding     vortex_event_get_encoding               (VortexEvent * event)
{
	if (event == NULL)
		return -1;
	/* return encoding configured */
	return event->encoding;
}


/**
 * @brief Allows to create a mask with the provided identifier,
 * initial event mask and an initial activation state.
 *
 * @param identifier Unique string that will be used find the mask
 * installed on the context.
 *
 * @param initial_mask Initial mask configuration.
 *
 * @param initial_state if the event mask is enabled or disabled. The
 * mask must be enabled to take effect (\ref
 * vortex_event_mask_enable).
 *
 * @return A newly created reference to a \ref VortexEventMask with
 * the provided data. You must use \ref vortex_event_mask_add to add
 * new events to mask (block). Once you are done, install the mask by
 * using \ref vortex_pull_set_event_mask. Remember to activate the
 * mask (if you did not provide an axl_true for <b>initial_state</b>)
 * by using \ref vortex_event_mask_enable. 
 */
VortexEventMask  * vortex_event_mask_new          (const char  * identifier,
						   int           initial_mask,
						   axl_bool      initial_state)
{
	VortexEventMask * mask;

	/* check value provided */
	if (identifier == NULL)
		return NULL;

	mask             = axl_new (VortexEventMask, 1);
	mask->mask_id    = axl_strdup (identifier);
	mask->mask       = initial_mask;
	mask->is_enabled = initial_state;
	
	/* return mask created */
	return mask;
} 

/**
 * @brief Allows to configure a new event to blocked by the mask
 * reference provided.
 *
 * @param mask The mask to configure.
 *
 * @param events The list of events to configure. In the case several
 * events are to be configured separate them with "|".
 *
 * \code
 * // how to set several events 
 * vortex_event_mask_add (mask, VORTEX_EVENT_CHANNEL_ADDED | VORTEX_EVENT_CHANNEL_REMOVED);
 * \endcode
 */
void               vortex_event_mask_add          (VortexEventMask * mask,
						   int               events)
{
	if (mask == NULL)
		return;
	/* add events to the mask */
	mask->mask |= events;
	return;
}

/**
 * @brief Allows to remove an event from the mask provided.
 *
 * @param mask The mask to configure.
 *
 * @param events The list of events to remove. In the case several
 * events are to be removed separate them with "|".
 *
 * \code
 * // how to remove several events 
 * vortex_event_mask_remove (mask, VORTEX_EVENT_CHANNEL_ADDED | VORTEX_EVENT_CHANNEL_REMOVED);
 * \endcode
 */
void               vortex_event_mask_remove       (VortexEventMask * mask,
						   int               events)
{
	if (mask == NULL)
		return;
	/* remove events to the mask */
	mask->mask = (mask->mask & ~events);
	return;
}

/**
 * @brief Allows to check if an event is configred in the provided
 * mask.
 *
 * @param mask The mask to check.
 *
 * @param event The event to check.
 *
 * @return axl_true in the case the event is configured in the mask,
 * otherwise axl_false is returned.
 */
axl_bool           vortex_event_mask_is_set       (VortexEventMask * mask,
						   VortexEventType   event)
{
	if (mask == NULL)
		return axl_false;
	/* check is enabled */
	if (! mask->is_enabled)
		return axl_false;
	/* check the event in the mask */
	return (mask->mask & event) > 0;
}

/**
 * @brief Allows to enable/disable the provided mask.
 * @param mask The mask to enable/disable.
 * @param enable Value used to enable/disable the mask.
 */
void               vortex_event_mask_enable       (VortexEventMask * mask,
						   axl_bool          enable)
{
	if (mask == NULL)
		return;  
	mask->is_enabled = enable;
	return;
}

/**
 * @brief Allows to install the provided event mask (\ref
 * VortexEventMask) on the selected context (\ref VortexCtx).
 *
 * @param ctx The context where the mask will be installed. 
 *
 * @param mask The mask to install. Its effect will be cumulative with
 * other masks installed. It is not required to unref (\ref
 * vortex_event_mask_free) the mask before exit vortex. This is
 * already done by \ref vortex_exit_ctx. 
 * 
 * @param error Optional axlError reference to report textual
 * diagnostic errors.
 *
 * @return axl_true if the event mask was installed, otherwise
 * axl_false is returned and error is updated with a textual
 * diagnostic along with an especific error code.
 */
axl_bool           vortex_pull_set_event_mask     (VortexCtx        * ctx,
						   VortexEventMask  * mask,
						   axlError        ** error)
{
	axlList         * list;
	int               iterator;
	VortexEventMask * auxMask;

	/* check reference */
	if (ctx == NULL || mask == NULL) {
		axl_error_report (error, -1, "Failed to set event mask because context or mask reference received is NULL");
		return axl_false;
	} /* end if */

	/* check that the context has the pull API enabled */
	if (vortex_ctx_get_data (ctx, VORTEX_PULL_QUEUE_KEY) == NULL) {
		axl_error_report (error, -2, "Failed to set event mask because context provided do not have PULL API enabled");
		return axl_false;
	} /* end if */
		
	/* get the mask list */
	list  = vortex_ctx_get_data (ctx, VORTEX_PULL_EVENT_MASKS);
	if (list == NULL) {
		axl_error_report (error, -3, "Failed to set event mask because event mask list reference is NULL");
		return axl_false;
	} /* end if */

	/* check the event mask identifier is not already installed */
	iterator = 0;
	while (iterator < axl_list_length (list)) {
		/* get mask installed */
		auxMask = axl_list_get_nth (list, iterator);
		
		/* check the installed mask is not using a mask id
		 * already in use. */
		if (axl_cmp (auxMask->mask_id, mask->mask_id)) {
			axl_error_report (error, -4, "Failed to set event mask because the mask id is already in use");
			return axl_false;
		} /* end if */

		/* next iterator */
		iterator++;
	} /* end while */

	/* install the mask */
	axl_list_append (list, mask);
	
	return axl_true;
}

/**
 * @internal Function that allows to check if the provided even type
 * is filtered by context event masks installed.
 *
 * @param ctx The context where the PULL API, and optionally event
 * masks are installed.
 *
 * @param type The event type to check if it is filtered.
 *
 * @return axl_true if the event is filtered, otherwise axl_false is returned.
 */
axl_bool vortex_pull_event_is_filtered (VortexCtx        * ctx,
					VortexEventType    type)
{
	axlList         * list     = vortex_ctx_get_data (ctx, VORTEX_PULL_EVENT_MASKS);
	int               iterator = 0;
	VortexEventMask * mask;

	/* no mask installed, no event filtered */
	if (axl_list_length (list) < 1)
		return axl_false;
	
	/* for each mask installed, check for the first filtering the
	 * event */
	while (iterator < axl_list_length (list)) {

		/* get mask and check */
		mask = axl_list_get_nth (list, iterator);
		
		/* check if it's filtered */
		if (vortex_event_mask_is_set (mask, type)) {
			vortex_log (VORTEX_LEVEL_DEBUG, "event type=%d filtered by event mask %s",
				    type, mask->mask_id);
			return axl_true;
		} /* end if */

		/* next iterator */
		iterator++;
	} /* end while */

	/* event type not filtered */
	return axl_false;
}

/**
 * @brief Release resources allocated pointed by the mask reference.
 * @param mask The reference to terminate.
 */
void               vortex_event_mask_free         (VortexEventMask * mask)
{
	if (mask == NULL)
		return;
	axl_free (mask->mask_id);
	axl_free (mask);
	return;
}

/* @internal generic marshaller */
VortexEvent *      vortex_pull_event_marshaller   (VortexCtx        * ctx,
						   const char       * event_description,
						   VortexEventType    type,
						   VortexChannel    * channel,
						   VortexConnection * conn,
						   axl_bool           checked_conn_ref,
						   VortexFrame      * frame,
						   int                msg_no)
{
	VortexEvent      * event;
	VortexAsyncQueue * pull_pending_events;

	/* check if the event is fileted */
	if (vortex_pull_event_is_filtered (ctx, type))
		return NULL;

	/* get the reference to the queue and return if it is found to
	 * be not defined */
	pull_pending_events = vortex_ctx_get_data (ctx, VORTEX_PULL_QUEUE_KEY);
	if (pull_pending_events == NULL) 
		return NULL;

	/* create an empty event */
	event = __vortex_event_new_empty (
		/* type */
		type,
		/* context */
		ctx,
		/* connection */
		conn,
		/* should check connection ref */
		checked_conn_ref, 
		/* channel */
		channel,
		/* frame */
		frame);

	/* configure msgno */
	event->msgno = msg_no;
					  
	/* queue pending event to process */
	vortex_async_queue_push (pull_pending_events, event);

	/* drop a log */
	vortex_log (VORTEX_LEVEL_DEBUG, "(PULL API) event: %s, connection-id=%d, channel-num=%d",
		    event_description,
		    /* connection */
		    conn ? vortex_connection_get_id (conn) : -1, 
		    channel ? vortex_channel_get_number (channel) : -1);
	
	/* return a reference to the event created */
	return event;
}
						   
						   

/**
 * @internal Frame received handler for PULL API.
 */
void               vortex_pull_frame_received     (VortexChannel    * channel,
						   VortexConnection * connection,
						   VortexFrame      * frame,
						   axlPointer         user_data)
{
	/* call to generic implementation to marshall async
	 * notification into a pulled event */
	vortex_pull_event_marshaller (
		(VortexCtx *) user_data,
		"frame received",
		VORTEX_EVENT_FRAME_RECEIVED,
		channel,
		/* connection ref and signal checked ref */
		connection, axl_true,
		frame,
		/* msg no */
		-1);

	return;
}

/**
 * @internal Close channel request pull API marshaller.
 */
void               vortex_pull_close_notify       (VortexChannel * channel,
						   int             msg_no,
						   axlPointer      user_data)
{
	VortexEvent * event;
	VortexCtx   * ctx = user_data;

	/* call to generic implementation to marshall async
	 * notification into a pulled event */
	event = vortex_pull_event_marshaller (
		(VortexCtx *) user_data,
		"channel close request",
		VORTEX_EVENT_CHANNEL_CLOSE,
		channel,
		/* connection ref and signal checked ref */
		vortex_channel_get_connection (channel), axl_true,
		NULL,
		msg_no);

	if (event == NULL) {
		/* because the CHANNEL_CLOSE has been filtered, we
		 * have to accept channel close */
		vortex_log (VORTEX_LEVEL_DEBUG, "because the VORTEX_EVENT_CHANNEL_CLOSE has been filtered, we have to accept channel=%d close",
			    vortex_channel_get_number (channel));
		vortex_channel_notify_close (channel, msg_no, axl_true);
	} /* end if */


	return;
}

/**
 * @internal Channel added pull API marshaller.
 */
void               vortex_pull_channel_added      (VortexChannel * channel,
						   axlPointer      user_data)
{
	/* call to generic implementation to marshall async
	 * notification into a pulled event */
	vortex_pull_event_marshaller (
		(VortexCtx *) user_data,
		"channel added",
		VORTEX_EVENT_CHANNEL_ADDED,
		channel,
		/* connection ref and signal checked ref */
		vortex_channel_get_connection (channel), axl_true,
		NULL,
		-1);

	return;
}

/**
 * @internal Channel removed pull API marshaller.
 */
void               vortex_pull_channel_removed    (VortexChannel * channel,
						   axlPointer      user_data)
{
	/* call to generic implementation to marshall async
	 * notification into a pulled event */
	vortex_pull_event_marshaller (
		(VortexCtx *) user_data,
		"channel removed",
		VORTEX_EVENT_CHANNEL_REMOVED,
		channel,
		/* connection ref and signal checked ref */
		vortex_channel_get_connection (channel), axl_true,
		NULL,
		-1);
	return;
}

void vortex_pull_connection_closed (VortexConnection * connection, axlPointer user_data)
{
	/* call to generic implementation to marshall async
	 * notification into a pulled event */
	vortex_pull_event_marshaller (
		(VortexCtx *) user_data,
		"connection closed",
		VORTEX_EVENT_CONNECTION_CLOSED,
		/* null channel */
		NULL,
		/* connection ref and signal unchecked ref */
		connection, axl_false,
		/* null frame and msgno = -1 */
		NULL, -1);
	return;
}

/* @internal handler to install connection closed handler: VORTEX_EVENT_CONNECTION_CLOSED */
int                vortex_pull_register_close_connection   (VortexCtx               * ctx,
							    VortexConnection        * conn,
							    VortexConnection       ** new_conn,
							    VortexConnectionStage     state,
							    axlPointer                user_data)
{
	/* configure connection closed, and set VortexCtx (user_data)
	 * as optional user defined data*/
	vortex_connection_set_on_close_full (conn, vortex_pull_connection_closed, user_data);
	return 1;
}

/* @intenal handler for VORTEX_EVENT_CONNECTION_ACCEPTED */
axl_bool vortex_pull_connection_accepted (VortexConnection * connection, axlPointer user_data)
{
	/* call to generic implementation to marshall async
	 * notification into a pulled event */
	vortex_pull_event_marshaller (
		(VortexCtx *) user_data,
		"connection accepted",
		VORTEX_EVENT_CONNECTION_ACCEPTED,
		/* null channel */
		NULL,
		/* connection ref and signal checked ref */
		connection, axl_true,
		/* null frame and msgno = -1 */
		NULL, -1);

	/* accept the connection here and let the user to close it
	 * once received the CONNECTION_ACCEPTED notification. */
	return axl_true;
}

/* @internal handler for VORTEX_EVENT_CHANNEL_START */
axl_bool           vortex_pull_start_handler               (char              * profile,
							    int                 channel_num,
							    VortexConnection  * connection,
							    char              * serverName,
							    char              * profile_content,
							    char             ** profile_content_reply,
							    VortexEncoding      encoding,
							    axlPointer          user_data)
{
	/* get a reference to the channel */
	VortexChannel * channel = vortex_connection_get_channel (connection, channel_num);
	VortexEvent   * event;
	VortexCtx     * ctx     = user_data;
	
	vortex_log (VORTEX_LEVEL_DEBUG, "pull API channel start received (%d: %p)",
		    vortex_channel_get_number (channel), channel);

	/* call to generic implementation to marshall async
	 * notification into a pulled event */
	event = vortex_pull_event_marshaller (
		(VortexCtx *) user_data,
		"channel start",
		VORTEX_EVENT_CHANNEL_START,
		/* null channel */
		channel,
		/* connection ref and signal checked ref */
		connection, axl_true,
		/* null frame and msgno = -1 */
		NULL, -1);
	
	/* check for filtered event, in that case, return true */
	if (event == NULL)
		return axl_true;

	vortex_log (VORTEX_LEVEL_DEBUG, "configuring channel=%d start defer..", vortex_channel_get_number (channel));

	/* set channel start defer */
	channel = vortex_connection_get_channel (connection, channel_num);
	vortex_channel_defer_start (channel);

	/* configure serverName, profile_content and encoding */
	event->serverName      = axl_strdup (serverName);
	event->profile_content = axl_strdup (profile_content);
	event->encoding        = encoding;

	/* return axl_false but this is not the definitive decision */
	return axl_false;
}

/**
 * @}
 */
