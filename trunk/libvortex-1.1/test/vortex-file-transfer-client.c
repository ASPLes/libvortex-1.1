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

/* the file name to save */
#define FILE_TO_TRANSFER "linux-2.6.20.1.tar.bz2" 

/* server hosting the file */
#define SERVER_HOST "localhost"

FILE      * file;

/* listener context */
VortexCtx * ctx = NULL;

void frame_received (VortexChannel    * channel,
		     VortexConnection * connection,
		     VortexFrame      * frame,
		     axlPointer         user_data)
{
	int    bytes_written;
/*	char * md5; */

	/* check for the last nul frame */
	if (vortex_frame_get_ansno (frame) == 0 && vortex_frame_get_type (frame) == VORTEX_FRAME_TYPE_NUL) {
		if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_NUL) {
			printf ("Expected to find NUL terminator message, but found: %d frame type\n",
				vortex_frame_get_type (frame));
			return;
		} /* end if */
		
		/* check size */
		if (vortex_frame_get_payload_size (frame) != 0) {
			printf ("Expected to find NUL terminator message, with empty content, but found: %d frame size\n",
				vortex_frame_get_payload_size (frame));
			return;
		} /* end if */
		
		printf ("LAST frame received:   Operation completed, close the file..Ok!\n");
		vortex_async_queue_push ((axlPointer) user_data, INT_TO_PTR (1));
		return;
	} /* end if */

	/* dump content to the file */
/*	md5           = vortex_tls_get_digest_sized (VORTEX_MD5, vortex_frame_get_payload (frame), vortex_frame_get_payload_size (frame)); */
	bytes_written = fwrite (vortex_frame_get_payload (frame), 1, vortex_frame_get_payload_size (frame), file); 
/*	printf ("Writing frame content: asno=%d, size=%d, md5=%s\n", vortex_frame_get_ansno (frame), bytes_written, md5);  
	axl_free (md5); */

	if (bytes_written != vortex_frame_get_payload_size (frame)) {
		printf ("ERROR: error while writing to the file: %d != %d\n", 
			bytes_written, vortex_frame_get_payload_size (frame));
	} /* end if */
	
	return;
}

void frame_received_with_msg (VortexChannel    * channel,
			      VortexConnection * connection,
			      VortexFrame      * frame,
			      axlPointer         user_data)
{
	int bytes_written;

	/* received big file */
	printf ("Received big file, size: %d\n", vortex_frame_get_payload_size (frame));

	/* dump content to the file */
	bytes_written = fwrite (vortex_frame_get_payload (frame),
				1, vortex_frame_get_payload_size (frame), file);
	if (bytes_written != vortex_frame_get_payload_size (frame)) {
		printf ("ERROR: error while writing to the file: %d != %d\n", 
			bytes_written, vortex_frame_get_payload_size (frame));
	} /* end if */

	printf ("Looking caller..\n");
	vortex_async_queue_push ((axlPointer) user_data, INT_TO_PTR (1));
	
	return;
}



int  main (int  argc, char ** argv)
{
	VortexConnection * connection;
	VortexChannel    * channel;
	VortexAsyncQueue * queue;



	/* create the context */
	ctx = vortex_ctx_new ();

	/* init vortex library */
	if (! vortex_init_ctx (ctx)) {
		/* unable to init context */
		vortex_ctx_free (ctx);
		return -1;
	} /* end if */

	/* creates a new connection against localhost:44017 */
	printf ("connecting to %s:44017...\n", SERVER_HOST);
	connection = vortex_connection_new (ctx, SERVER_HOST, "44017", NULL, NULL);
	if (!vortex_connection_is_ok (connection, axl_false)) {
		printf ("Unable to connect remote server, error was: %s\n",
			 vortex_connection_get_message (connection));
		goto end;
	}
	printf ("ok\n");

	/* create a new channel (by chosing 0 as channel number the
	 * Vortex Library will automatically assign the new channel
	 * number free. */
	queue   = vortex_async_queue_new ();
	if (argv != NULL && argv[1] != NULL && axl_cmp (argv[1], "bigmsg")) {
		printf ("Creating a channel with big message profile\n");
		channel = vortex_channel_new (connection, 0,
					      FILE_TRANSFER_URI_WITH_MSG,
					      /* no close handling */
					      NULL, NULL,
					      /* no frame receive async
					       * handling */
					      frame_received_with_msg, queue,
					      /* no async channel creation */
					      NULL, NULL);
	} else {
		printf ("Creating channel with ANS/NUL pattern..\n");
		channel = vortex_channel_new (connection, 0,
					      FILE_TRANSFER_URI,
					      /* no close handling */
					      NULL, NULL,
					      /* no frame receive async
					       * handling */
					      frame_received, queue,
					      /* no async channel creation */
					      NULL, NULL);
	} /* end if */

	if (channel == NULL) {
		printf ("Unable to create the channel..\n");
		goto end;
	}

	/* serialize channel */
	vortex_channel_set_serialize (channel, axl_true);
	
	/* open the file */
#if defined(AXL_OS_UNIX)
	file = fopen (FILE_TO_TRANSFER, "w");
#elif defined(AXL_OS_WIN32)
	file = fopen (FILE_TO_TRANSFER, "wb");
#endif
	if (file == NULL) {
		printf ("Unable to create the file (%s) to hold content: %s\n", FILE_TO_TRANSFER, strerror (errno));
		goto end;
	}

	/* send the message */
	if (! vortex_channel_send_msg (channel, 
				       "send the message, please",
				       24, 
				       NULL)) {
		printf ("Failed to send message that request the file to transfer..\n");
		return -1;
	}

	/* wait for the file */
	vortex_async_queue_pop (queue);
	vortex_async_queue_unref (queue);

	printf ("closing file...\n");
	fclose (file);

	/* close the channel */
	vortex_channel_close (channel, NULL);

 end:				      
	vortex_connection_close (connection);
	vortex_exit_ctx (ctx, axl_true);
	return 0 ;	      
}

