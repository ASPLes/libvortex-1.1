/*  LibVortex:  A BEEP implementation for af-arch
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <vortex.h>

#define PLAIN_PROFILE "http://fact.aspl.es/profiles/plain_profile"

/* listener context */
VortexCtx * ctx = NULL;

void frame_received (VortexChannel    * channel,
		     VortexConnection * connection,
		     VortexFrame      * frame,
		     axlPointer           user_data)
{
	int iterator = 10;
	printf ("A frame received on channl: %d\n",     vortex_channel_get_number (channel));
	printf ("Data received: '%s'\n",                (char*) vortex_frame_get_payload (frame));

	/* reply the peer client with the same content 10 times */
	while (iterator >= 0) {
		printf ("Sending the reply..\n");
		if (! vortex_channel_send_ans_rpyv (channel,
						    vortex_frame_get_msgno (frame),
						    "Received Ok(%d): %s",
						    iterator,
						    vortex_frame_get_payload (frame))) {
			fprintf (stderr, "There was an error while sending the reply message");
		}
		printf ("Reply: %d sent..\n", iterator);
		iterator--;
	}
	
	/* send the last reply. */
	if (!vortex_channel_finalize_ans_rpy (channel, vortex_frame_get_msgno (frame))) {
		fprintf (stderr, "There was an error while sending the NUL reply message");
	}
				
	printf ("VORTEX_LISTENER: end task (pid: %d)\n", getpid ());


	return;
}

int      start_channel (int                channel_num, 
			VortexConnection * connection, 
			axlPointer           user_data)
{
	/* implement profile requirement for allowing starting a new
	 * channel to return false denies channel creation to return
	 * axl_true allows create the channel */
	return axl_true;
}

int      close_channel (int                channel_num, 
			VortexConnection * connection, 
			axlPointer         user_data)
{
	/* implement profile requirement for allowing to closeing a
	 * the channel to return false denies channel closing to
	 * return axl_true allows to close the channel */
	return axl_true;
}

int  main (int  argc, char ** argv) 
{

	/* create the context */
	ctx = vortex_ctx_new ();

	/* init vortex library */
	if (! vortex_init_ctx (ctx)) {
		/* unable to init context */
		vortex_ctx_free (ctx);
		return -1;
	} /* end if */

	/* register a profile */
	vortex_profiles_register (ctx, PLAIN_PROFILE, 
				  start_channel, NULL, 
				  close_channel, NULL,
				  frame_received, NULL);
	
	vortex_greetings_set_features (ctx, "enable-tls");
	vortex_greetings_set_localize (ctx, "es-ES");

	/* create a vortex server */
	vortex_listener_new (ctx, "0.0.0.0", "44000", NULL, NULL);

	/* wait for listeners (until vortex_exit is called) */
	vortex_listener_wait (ctx);
	
	/* end vortex function */
	vortex_exit_ctx (ctx, axl_true);

	return 0;
}

