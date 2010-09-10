/* Hey emacs, show me this like 'c': -*- c -*-
 *
 * Support functions for test performed.
 * Copyright (C) 2010 Advanced Software Production Line, S.L.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 */
#include <_support.h>

/** 
 * @brief Initializes the vortex engine, creates a connection and a
 * XML-RPC channel.
 *
 * @return A newly created VortexChannel or NULL if fails.
 */
VortexChannel * start_and_create_channel ()
{
	VortexConnection * connection;
	VortexChannel    * channel;

	/* initialize the vortex */
	vortex_init ();

	/* create a connection to a local server */
	connection = vortex_connection_new ("localhost", "44000", NULL, NULL);
	if (! vortex_connection_is_ok (connection, FALSE)) {
		/* drop an error log if not connected */
		printf ("ERROR: unable to start a BEEP session. Is the BEEP XML-RPC server up?\n");

		/* terminate vortex session */
		vortex_exit ();
		
		/* report an error */
		return NULL;
	}

	/* create the xml-rpc channel */
	channel = BOOT_CHANNEL (connection, NULL);

	if (channel == NULL) {
		printf ("ERROR: unable to start channel. Is the BEEP XML-RPC server up?\n");
	}

	/* return the channel created */
	return channel;
}

/** 
 * @brief Closes the channel provided, and its connection and stops
 * the vortex engine.
 * 
 * @param channel The channel to close.
 */
void            close_channel_and_stop (VortexChannel * channel)
{
	VortexConnection * connection;

	/* get the connection */
	connection = vortex_channel_get_connection (channel);

	/* close the channel */
	vortex_channel_close (channel, NULL);

	/* close the connection */
	vortex_connection_close (connection);

	/* terminate vortex */
	vortex_exit ();	

	return;
}

