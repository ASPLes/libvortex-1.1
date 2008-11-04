/*  LibVortex:  A BEEP implementation
 *  Copyright (C) 2007 Advanced Software Production Line, S.L.
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

/* profile that sends the big file using ANS/NUL reply */
#define FILE_TRANSFER_URI "http://www.aspl.es/vortex/profiles/file-transfer"

/* profile that sends the big file using a single MSG */
#define FILE_TRANSFER_URI_WITH_MSG "http://www.aspl.es/vortex/profiles/file-transfer/bigmessage"

/* the file to send: 43378171 bytes = 42M */
#define FILE_TO_TRANSFER "/usr/src/linux-2.6.20.1.tar.bz2"

/* file size, in bytes */
#define FILE_SIZE (43378171)

/* listener context */
VortexCtx * ctx = NULL;

void frame_received (VortexChannel    * channel,
		     VortexConnection * connection,
		     VortexFrame      * frame,
		     axlPointer           user_data)
{
	FILE * file;
	char   buffer[12288];
	int    bytes_read;
	long   total_bytes;

	/* open file */
	file = fopen (FILE_TO_TRANSFER, "r");
	if (file == NULL) {
		printf ("FAILED to open file..\n");
		vortex_channel_send_err (channel, 
					 "Unable to open file requested",
					 29, 
					 vortex_frame_get_msgno (frame));
		return;
	}
	
	/* reply the peer client with the same content 10 times */
	total_bytes = 0;
	do {
		/* read content */
		bytes_read = fread (buffer, 1, 4096, file);

		/* update total count */
		total_bytes += bytes_read;
		
		/* break if eof is found */
		if (bytes_read == 0) 
			break;

/*		printf ("Sending the reply: bytes %d..\n", bytes_read); */
		if (! vortex_channel_send_ans_rpy (channel,
						   buffer, 
						   bytes_read, 
						   vortex_frame_get_msgno (frame))) {
			fprintf (stderr, "ERROR: There was an error while sending the reply message");
			break;
		}

	} while (axl_true);
	
	/* send the last reply. */
	if (!vortex_channel_finalize_ans_rpy (channel, vortex_frame_get_msgno (frame))) {
		fprintf (stderr, "There was an error while sending the NUL reply message");
	}
				
	printf ("VORTEX_LISTENER: end task (pid: %d), bytes transferred: %ld (ans/nul pattern)\n", getpid (), total_bytes);
	fclose (file);


	return;
}

void frame_received_with_msg (VortexChannel    * channel,
			      VortexConnection * connection,
			      VortexFrame      * frame,
			      axlPointer           user_data)
{
	FILE * file;
	char * buffer;
	int    bytes_read;
	long   total_bytes;

	/* open file */
	file = fopen (FILE_TO_TRANSFER, "r");
	if (file == NULL) {
		printf ("FAILED to open file..\n");
		vortex_channel_send_err (channel, 
					 "Unable to open file requested",
					 29, 
					 vortex_frame_get_msgno (frame));
		return;
	}

	/* allow */
	buffer = axl_new (char, FILE_SIZE + 1);
	
	/* read all the content */
	bytes_read = fread (buffer, 1, FILE_SIZE, file);
	
	if (bytes_read != FILE_SIZE) {
		axl_free (buffer);

		printf ("failed to load the hole file, expected to read %d, but found: %d\n",
			bytes_read, FILE_SIZE);
		return;
	} /* end if */
	total_bytes = bytes_read;
	
	/* send the last reply. */
	if (! vortex_channel_send_rpy (channel, buffer, FILE_SIZE, vortex_frame_get_msgno (frame))) {
		fprintf (stderr, "There was an error while sending the NUL reply message");
	}

	axl_free (buffer);
				
	printf ("VORTEX_LISTENER: end task (pid: %d), bytes transferred: %ld (one big message)\n", getpid (), total_bytes);
	fclose (file);


	return;
}

axl_bool      start_channel (int                channel_num, 
			     VortexConnection * connection, 
			     axlPointer           user_data)
{
	/* implement profile requirement for allowing starting a new
	 * channel to return false denies channel creation to return
	 * true allows create the channel */
	return axl_true;
}

axl_bool      close_channel (int                channel_num, 
			     VortexConnection * connection, 
			     axlPointer         user_data)
{
	/* implement profile requirement for allowing to closeing a
	 * the channel to return false denies channel closing to
	 * return true allows to close the channel */
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
	vortex_profiles_register (ctx, FILE_TRANSFER_URI,
				  start_channel, NULL, 
				  close_channel, NULL,
				  frame_received, NULL);

	/* register a profile */
	vortex_profiles_register (ctx, FILE_TRANSFER_URI_WITH_MSG,
				  start_channel, NULL, 
				  close_channel, NULL,
				  frame_received_with_msg, NULL);
       
	/* create a vortex server */
	vortex_listener_new (ctx, "0.0.0.0", "44017", NULL, NULL);

	/* wait for listeners (until vortex_exit is called) */
	vortex_listener_wait (ctx);
	
	/* end vortex function */
	vortex_exit_ctx (ctx, axl_true);

	return 0;
}



