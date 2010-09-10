/*
 *  Copyright (C) 2010 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or   
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 */

#include <vortex.h>

#define LISTENER_SHUTDOWN_PROFILE  "http://aspl.es/profiles/shutdown-listener"
#define LISTENER_SHUTDOWN_PROFILE2 "http://aspl.es/profiles/shutdown-listener/version/2.0"
#define LISTENER_SHUTDOWN_PROFILE3 "http://aspl.es/profiles/shutdown-listener/version/3.0"

axlList * list;

/* listener context */
VortexCtx * ctx = NULL;

void frame_received (VortexChannel    * channel,
		     VortexConnection * connection,
		     VortexFrame      * frame,
		     axlPointer           user_data)
{
	int                iterator;
	VortexConnection * listener;

	/* reply */
	vortex_channel_send_rpy (channel, "recieved", 8, vortex_frame_get_msgno (frame));

	printf ("Received listener to close: %s\n", (char*) vortex_frame_get_payload (frame));

	/* get the port number received */
	iterator = 0;
	while (iterator < axl_list_length (list)) {

		/* get the listener */
		listener = axl_list_get_nth (list, iterator);

		printf ("  Checking listener: '%s' with '%s'\n", (char*) vortex_frame_get_payload (frame),
			vortex_connection_get_port (listener));

		/* check if the listener found is the one to be
		 * closed */
		if (axl_cmp (vortex_connection_get_port (listener),
			     vortex_frame_get_payload (frame))) {
			printf ("    listener found, shutting down..\n");

			/* remove the listener */
			axl_list_remove (list, listener);
			
			/* close the listener connection */
			vortex_connection_shutdown (listener);
			
			break;
		} /* end if */

		/* get next iterator */
		iterator++;
	} /* end while */

	printf ("checking if the profile='%s' is registered\n", 
		(char*) vortex_frame_get_payload (frame));
	if (vortex_profiles_is_registered (ctx, vortex_frame_get_payload (frame))) {
		/* free the payload */
		printf ("  Found registered profiled, unregistering it: %s\n", 
			(char*) vortex_frame_get_payload (frame));
		vortex_profiles_unregister (ctx, vortex_frame_get_payload (frame));
	} /* end if */

	return;
}

int  main (int  argc, char ** argv) 
{
	VortexConnection * listener;

	/* create the context */
	ctx = vortex_ctx_new ();

	/* init vortex library */
	if (! vortex_init_ctx (ctx)) {
		/* unable to init context */
		vortex_ctx_free (ctx);
		return -1;
	} /* end if */

	/* register a profile */
	vortex_profiles_register (ctx, LISTENER_SHUTDOWN_PROFILE,
				  /* no start channel handling */
				  NULL, NULL, 
				  /* no close channel handling */
				  NULL, NULL,
				  frame_received, NULL);

	/* register a profile */
	vortex_profiles_register (ctx, LISTENER_SHUTDOWN_PROFILE2,
				  /* no start channel handling */
				  NULL, NULL, 
				  /* no close channel handling */
				  NULL, NULL,
				  frame_received, NULL);

	/* register a profile */
	vortex_profiles_register (ctx, LISTENER_SHUTDOWN_PROFILE3,
				  /* no start channel handling */
				  NULL, NULL, 
				  /* no close channel handling */
				  NULL, NULL,
				  frame_received, NULL);

	/* create the list of listeners created */
	list = axl_list_new (axl_list_always_return_1, NULL);
				  
	/* create a vortex server */
	listener = vortex_listener_new (ctx, "0.0.0.0", "44000", NULL, NULL);
	axl_list_add (list, listener);

	listener = vortex_listener_new (ctx, "0.0.0.0", "44001", NULL, NULL);
	axl_list_add (list, listener);

	listener = vortex_listener_new (ctx, "0.0.0.0", "44002", NULL, NULL);
	axl_list_add (list, listener);

	/* wait for listeners (until vortex_exit is called) */
	vortex_listener_wait (ctx);

	/* do not call vortex_exit here if you define an on ready
	 * function which actually ends the execution */
	vortex_exit_ctx (ctx, axl_true);

	return 0;
}

