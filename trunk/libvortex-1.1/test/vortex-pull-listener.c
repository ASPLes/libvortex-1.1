/*
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2010 Advanced Software Production Line, S.L.
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
 *         C/ Antonio Suarez Nº 10, 
 *         Edificio Alius A, Despacho 102
 *         Alcalá de Henares 28802 (Madrid)
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.aspl.es/vortex
 */

/* include base vortex library */
#include <vortex.h>

/* include pull API */
#include <vortex_pull.h>

void pull_handle_frame_received       (VortexEvent * event);
void pull_handle_channel_close        (VortexEvent * event);
void pull_handle_connection_accepted  (VortexEvent * event);
void pull_handle_connection_closed    (VortexEvent * event);

int main (int argc, char ** argv)
{
	VortexCtx         * ctx;
	VortexEvent       * event;
	VortexEventMask   * mask;
	axlError          * error = NULL;
	VortexConnection  * listener;

	/* create a context */
	printf ("LISTENER: Creating context..\n");
	ctx = vortex_ctx_new ();

	/* init vortex library */
	printf ("LISTENER: Starting vortex..\n");
	if (! vortex_init_ctx (ctx)) {
		/* unable to init context */
		vortex_ctx_free (ctx);
		exit (-1);
	} /* end if */

	/* now activate PULL api on this context */
	printf ("LISTENER: Activating pull API..\n");
	if (! vortex_pull_init (ctx)) {
		printf ("ERROR: failed to activate PULL API..\n");
		exit (-1);
	} /* end if */

	/* now mask some events to avoid handling it. Masking an event
	 * causes to use default behaviour defined */
	mask = vortex_event_mask_new ("listener mask", 
				      VORTEX_EVENT_CHANNEL_REMOVED | 
				      VORTEX_EVENT_CHANNEL_ADDED | 
				      VORTEX_EVENT_CHANNEL_START,
				      axl_true);
	if (! vortex_pull_set_event_mask (ctx, mask, &error)) {
		printf ("ERROR: failed to install listener event mask, error reported (code: %d): %s\n",
			axl_error_get_code (error), axl_error_get (error));
		exit (-1);
	} /* end if */

	/* start a listener */
	listener = vortex_listener_new (ctx, "0.0.0.0", "44005", NULL, NULL);
	if (vortex_connection_is_ok (listener, axl_false))
		printf ("LISTENER: started at 0.0.0.0:44005..\n");
	else {
		printf ("LISTENER: failed to start listener at 0.0.0.0:44005..\n");
		exit (-1);
	}

	/* register a profile */
	vortex_profiles_register (ctx, 
				  "http://www.aspl.es/vortex/pull-listener",
				  /* do not provide handlers here:
				   * they are ignored due to pull api
				   * activation */
				  NULL, NULL,
				  NULL, NULL,
				  NULL, NULL);

	/* process events */
	while (1) {

		/* get next event */
		event = vortex_pull_next_event (ctx, 0);
		switch (vortex_event_get_type (event)) {
		case VORTEX_EVENT_FRAME_RECEIVED:
			/* reply with the same content in the case it is MSG */
			pull_handle_frame_received (event);
			break;
		case VORTEX_EVENT_CHANNEL_CLOSE:
			pull_handle_channel_close (event);
			break;
		case VORTEX_EVENT_CONNECTION_ACCEPTED:
			pull_handle_connection_accepted (event);
			break;
		case VORTEX_EVENT_CONNECTION_CLOSED:
			pull_handle_connection_closed (event);
			break;
		default:
			/* unhandled event */
			printf ("Unhandled event type=%d..\n",
				vortex_event_get_type (event));
			break;
		} /* end if */

		/* terminate event reference */
		vortex_event_unref (event);

	} /* end while */

	printf ("LISTENER: finishing listener..\n");
	return 0;
}

void pull_handle_frame_received (VortexEvent * event)
{
	VortexFrame   * frame   = vortex_event_get_frame (event);
	VortexChannel * channel = vortex_event_get_channel (event);

	/* echo frame received */
	printf ("LISTENER: frame received (type %d): %s\n",
		vortex_frame_get_type (frame), (char*) vortex_frame_get_payload (frame));
	
	/* check if we have a MSG frame to reply */
	if (vortex_frame_get_type (frame) == VORTEX_FRAME_TYPE_MSG) {
		/* do echo */
		vortex_channel_send_rpy (channel, 
					 vortex_frame_get_payload (frame),
					 vortex_frame_get_payload_size (frame), vortex_frame_get_msgno (frame));
	} /* end if */
	return;
}

void pull_handle_channel_close (VortexEvent * event)
{
	VortexChannel * channel = vortex_event_get_channel (event);
	int             msg_no  = vortex_event_get_msgno   (event);

	/* check if the channel number if equal to 5 to deny closing
	 * it. Otherwise, accept closing the channel */
	if (vortex_channel_get_number (channel) == 5) {
		printf ("LISTENER: received channel=%d close request..denying..\n", 
			vortex_channel_get_number (channel));
		vortex_channel_notify_close (channel, msg_no, axl_false);
	} else {
		printf ("LISTENER: received channel=%d close request..accepting..\n", 
			vortex_channel_get_number (channel));
		vortex_channel_notify_close (channel, msg_no, axl_true);
	} /* end if */
	
	return;
}

void pull_handle_connection_accepted  (VortexEvent * event)
{
	VortexConnection * conn = vortex_event_get_conn (event);

	printf ("LISTENER: received new connection from %s:%s..\n",
		vortex_connection_get_host (conn), vortex_connection_get_port (conn));
	return;
}

void pull_handle_connection_closed  (VortexEvent * event)
{
	VortexConnection * conn = vortex_event_get_conn (event);

	printf ("LISTENER: connection from %s:%s was closed..\n",
		vortex_connection_get_host (conn), vortex_connection_get_port (conn));
	return;
}
