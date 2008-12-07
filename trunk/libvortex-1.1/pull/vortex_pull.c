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

#define LOG_DOMAIN "vortex-pull"
#define VORTEX_PULL_QUEUE_KEY "vo:pu:qu"

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
};

/**
 * @internal Function used to create empty events.
 */
VortexEvent * __vortex_event_new_empty (void)
{
	/* create event to report */
	VortexEvent * event   = axl_new (VortexEvent, 1);
	event->ref_count      = 1;
	vortex_mutex_create (&event->ref_count_mutex);

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
	
	/* activate queue */
	pull_pending_events = vortex_async_queue_new ();

	/* configure into the context */
	vortex_ctx_set_data (ctx, VORTEX_PULL_QUEUE_KEY, pull_pending_events);
	
	/* configure internal handlers to receive notifications */
	vortex_reader_set_frame_received (ctx, vortex_pull_frame_received, ctx);

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
	VortexEvent      * event;

	if (ctx == NULL || pull_pending_events == NULL) {
		return;
	} /* end if */

	/* uninstall handlers */
	vortex_reader_set_frame_received (ctx, NULL, NULL);

	/* remove all pending items */
	while (vortex_async_queue_items (pull_pending_events) > 0) {
		/* next event */
		event = vortex_async_queue_pop (pull_pending_events);

		/* unref */
		vortex_event_unref (event);
	} /* end while */

	/* terminate queue */
	vortex_async_queue_unref (pull_pending_events);

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
 * @internal Frame received handler for PULL API.
 */
void               vortex_pull_frame_received     (VortexChannel    * channel,
						   VortexConnection * connection,
						   VortexFrame      * frame,
						   axlPointer         user_data)
{
	VortexEvent      * event;
	VortexAsyncQueue * pull_pending_events;
	VortexCtx        * ctx = user_data;

	/* get the reference to the queue and return if it is found to
	 * be not defined */
	pull_pending_events = vortex_ctx_get_data (ctx, VORTEX_PULL_QUEUE_KEY);
	if (pull_pending_events == NULL) 
		return;

	/* update reference connection */
	if (! vortex_connection_ref (connection, "vortex_pull_frame-received")) 
		return;
	
	/* now update channel reference counting */
	vortex_channel_ref (channel);

	/* increase frame reference counting to make the VortexEvent
	 * to own its reference */
	vortex_frame_ref (frame);

	/* create an empty event */
	event = __vortex_event_new_empty ();
	
	/* references */
	event->ctx        = user_data;
	event->frame      = frame;
	event->channel    = channel;
	event->conn       = connection;

	/* configure event type */
	event->type       = VORTEX_EVENT_FRAME_RECEIVED;

	/* queue pending event to process */
	vortex_async_queue_push (pull_pending_events, event);

	/* drop a log */
	vortex_log (VORTEX_LEVEL_DEBUG, "(PULL API) frame received connection-id=%d, channel-num=%d",
		    vortex_connection_get_id (connection), vortex_channel_get_number (channel));
	return;
}

/**
 * @}
 */
