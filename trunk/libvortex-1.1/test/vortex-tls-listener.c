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

/* include base library */
#include <vortex.h>

/* include tls library */
#include <vortex_tls.h>

#define PLAIN_PROFILE "http://fact.aspl.es/profiles/plain_profile"

/* listener context */
VortexCtx * ctx = NULL;

void on_ready (char  * host, int  port, VortexStatus status, char  * message, axlPointer user_data)
{
	
	if (status == VortexError) {
		printf ("error at: %s\n", message);

		/* exit from vortex */
		vortex_exit_ctx (ctx, axl_false);
		return;
	}

	printf ("ready on: %s:%d, message: %s\n", host, port, message);
	return;
}

void frame_received (VortexChannel    * channel,
		     VortexConnection * connection,
		     VortexFrame      * frame,
		     axlPointer           user_data)
{
	printf ("VORTEX_LISTENER: STARTED (pid: %d)\n", getpid ());
	printf ("A frame received on channl: %d\n",     vortex_channel_get_number (channel));
	printf ("Data received: '%s'\n",                (char*) vortex_frame_get_payload (frame));

	/* reply */
	vortex_channel_send_rpy (channel,
				 "I have received you message, thanks..",
				 37, vortex_frame_get_msgno (frame));
	printf ("VORTEX_LISTENER: CLOSE CHANNEL (pid: %d)\n", getpid ());
	
	/* close the channel */
	vortex_channel_close (channel, NULL);
	
	printf ("VORTEX_LISTENER: FINSHED (pid: %d)\n",       getpid ());
	return;
}

int      start_channel (int  channel_num, VortexConnection * connection, axlPointer user_data)
{
	printf ("A new channel to be created..");

	if (channel_num == 4) {
		printf ("channel 4 can not be created\n");
		return axl_false;
	}

	printf ("create the channel..\n");
	return axl_true;
}

int  main (int  argc, char  ** argv) 
{

	/* create the context */
	ctx = vortex_ctx_new ();

	/* init vortex library */
	if (! vortex_init_ctx (ctx)) {
		/* unable to init context */
		vortex_ctx_free (ctx);
		return -1;
	} /* end if */

#if defined(ENABLE_TLS_SUPPORT)
	/* check for TLS initialization */
	if (! vortex_tls_init (ctx)) {
		printf ("Current Vortex Library is not prepared for TLS profile");
		return -1;
	}
#else
	printf ("Current build does not have TLS support.\n");
	return -1;
#endif

	/* enable accepting incoming tls connections, this step could
	 * also be read as register the TLS profile */
	if (! vortex_tls_accept_negotiation (ctx, NULL, NULL, NULL)) {
		printf ("Unable to start accepting TLS profile requests");
	}

	/* register the plain profile to test data sending before
	 * TLS-fication */
	vortex_profiles_register (ctx, PLAIN_PROFILE, 
				  NULL, NULL, 
				  NULL, NULL,
				  frame_received, NULL);
	
	/* create a vortex server */
	vortex_listener_new (ctx, "0.0.0.0", "44001", on_ready, NULL);

	/* wait for listeners (until vortex_exit is called) */
	vortex_listener_wait (ctx);

	/* do not call vortex_exit here if you define an on ready
	 * function which actually ends the execution */
	

	return 0;
}

