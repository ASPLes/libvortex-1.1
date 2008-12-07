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
#ifndef __VORTEX_PULL_H__
#define __VORTEX_PULL_H__

#include <vortex.h>

/**
 * \addtogroup vortex_pull
 * @{
 */

/**
 * @brief Type representation for an event notification from the PULL
 * API.
 */
typedef struct _VortexEvent      VortexEvent;

/**
 * @brief List of events that can happen/pulled from the vortex pull
 * API. This list not only describes the event but also provides a
 * reference of all references that are associated to a particular
 * event.
 *
 * For example, the \ref VORTEX_EVENT_FRAME_RECEIVED signals that a
 * frame has arrive and a call to \ref vortex_event_get_frame will
 * return a reference to the frame. Not all events has all references
 * defined.
 * 
 * There are a list of references that is always defined. They are assumed implicitly:
 * - \ref vortex_event_get_ctx : the context where the event was received.
 * - \ref vortex_event_get_conn : the connection where the event happend.
 */
typedef enum {
	/**
	 * @brief Undefined event. This type is used to report errors
	 * found while using pull API.
	 */
	VORTEX_EVENT_UNKNOWN        = 1,
	/**
	 * @brief Even type that represents a frame received. You must
	 * call to vortex_pull_get_frame to get the reference to the
	 * frame received. 
	 *
	 * This event has the following references defined:
	 * - \ref vortex_event_get_frame : frame received due to this event.
	 * - \ref vortex_event_get_channel : the channel where the frame was received.
	 */
	VORTEX_EVENT_FRAME_RECEIVED = 2,
} VortexEventType;

axl_bool           vortex_pull_init               (VortexCtx * ctx);

void               vortex_pull_cleanup            (VortexCtx * ctx);

axl_bool           vortex_pull_pending_events     (VortexCtx * ctx);

int                vortex_pull_pending_events_num (VortexCtx * ctx);

VortexEvent      * vortex_pull_next_event         (VortexCtx * ctx, 
						   int         milliseconds_to_wait);

axl_bool           vortex_event_ref               (VortexEvent * event);

void               vortex_event_unref             (VortexEvent * event);

VortexEventType    vortex_event_get_type          (VortexEvent * event);

VortexCtx        * vortex_event_get_ctx           (VortexEvent * event);

VortexConnection * vortex_event_get_conn          (VortexEvent * event);

VortexChannel    * vortex_event_get_channel       (VortexEvent * event);

VortexFrame      * vortex_event_get_frame         (VortexEvent * event);

/* internal API */
void               vortex_pull_frame_received     (VortexChannel    * channel,
						   VortexConnection * connection,
						   VortexFrame      * frame,
						   axlPointer         user_data);

#endif


/**
 * @}
 */
