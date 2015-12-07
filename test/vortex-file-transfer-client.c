/*  LibVortex:  A BEEP implementation
 *  Copyright (C) 2015 Advanced Software Production Line, S.L.
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

#if defined(ENABLE_TLS_SUPPORT)
#include <vortex_tls.h>
#endif

/* profile that sends the big file using ANS/NUL reply */
#define FILE_TRANSFER_URI "http://www.aspl.es/vortex/profiles/file-transfer"

/* profile that sends the big file using a single MSG */
#define FILE_TRANSFER_URI_WITH_MSG "http://www.aspl.es/vortex/profiles/file-transfer/bigmessage"

/* profile that sends the big file using a single MSG */
#define FILE_TRANSFER_URI_WITH_FEEDER "http://www.aspl.es/vortex/profiles/file-transfer/feeder"

/* profile that sends the big file using a ANS/NUL sent with a feeder */
#define FILE_TRANSFER_URI_WITH_ANS_FEEDER "http://www.aspl.es/vortex/profiles/file-transfer/ans-feeder"

/* the file name to save */
#define FILE_TO_TRANSFER "/tmp/file.2" 

/* server hosting the file */
#define SERVER_HOST "192.168.0.153"

FILE      * file;

/* listener context */
VortexCtx * ctx = NULL;

axl_bool ansnulfeeder = axl_false;
axl_bool ansnulfeeder_count = 0;

void frame_received (VortexChannel    * channel,
		     VortexConnection * connection,
		     VortexFrame      * frame,
		     axlPointer         user_data)
{
	int    bytes_written;
#if defined(ENABLE_TLS_SUPPORT)
	char * md5; 
#endif

	/* check for the last nul frame */
	if (vortex_frame_get_type (frame) == VORTEX_FRAME_TYPE_NUL) {
			
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

	if (vortex_frame_get_type (frame) == VORTEX_FRAME_TYPE_ANS) {
		printf ("ANS RECEIVED (%d)\n", vortex_frame_get_ansno (frame));
	}

	/* dump content to the file */
#if defined(ENABLE_TLS_SUPPORT)
	md5           = vortex_tls_get_digest_sized (VORTEX_MD5, vortex_frame_get_payload (frame), vortex_frame_get_payload_size (frame)); 
	printf ("MD5 CONTENT: %s (from total bytes %d)\n", md5, vortex_frame_get_payload_size (frame));
#endif
	bytes_written = fwrite (vortex_frame_get_payload (frame), 1, vortex_frame_get_payload_size (frame), file); 
/*	printf ("Written frame content: ansno=%d, size=%d, seqno=%u\n", vortex_frame_get_ansno (frame), bytes_written, vortex_frame_get_seqno (frame));   
	printf ("        Is nul frame: %d\n", vortex_frame_get_type (frame) == VORTEX_FRAME_TYPE_NUL); */
	
#if defined(ENABLE_TLS_SUPPORT)
	axl_free (md5); 
#endif

/*	if (vortex_frame_get_seqno (frame) == 1240) {
		vortex_log_enable (CONN_CTX (connection), axl_true);
		vortex_color_log_enable (CONN_CTX (connection), axl_true);
	} 
*/

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
	/* printf ("Received frame with size: %d, content is: (%d, %d) '%s'\n", vortex_frame_get_payload_size (frame), 
	   vortex_frame_get_content (frame)[0], vortex_frame_get_content (frame)[1], vortex_frame_get_content (frame));  */

	/* dump content to the file */
	bytes_written = fwrite (vortex_frame_get_payload (frame),
				1, vortex_frame_get_payload_size (frame), file);
	if (bytes_written != vortex_frame_get_payload_size (frame)) {
		printf ("ERROR: error while writing to the file: %d != %d\n", 
			bytes_written, vortex_frame_get_payload_size (frame));
	} /* end if */

	if (! vortex_frame_get_more_flag (frame)) {
		printf ("Looking caller..\n");
		vortex_async_queue_push ((axlPointer) user_data, INT_TO_PTR (1));
	} /* end if */
	
	return;
}

int  main (int  argc, char ** argv)
{
	VortexConnection * connection;
	VortexChannel    * channel = NULL;
	VortexAsyncQueue * queue;
	int                window_size    = 0;
	int                transfer_count = 1;
	const char       * server_host    = NULL;
	const char       * format         = NULL;
	int                iterator;

	/* file size */
	struct stat status;

	/* timing tracking */
	struct timeval    start;
	struct timeval    stop;
	struct timeval    result;

	printf ("Usage:    ./vortex-file-transfer-client [--server=hostname] [bigmsg|feeder|ansnul|ansnulfeeder]\n");
	printf ("Usage:                                  [--transfer-count=transfer_count]\n");
	printf ("Usage:                                  [--window-size=window_size]\n");
	printf ("Examples: \n");
	printf ("  -- transfer one copy using feeder method..\n");
	printf ("  >> ./vortex-file-transfer-client --server=localhost feeder\n");
	printf ("  -- transfer 10 copies using bigmsg method..\n");
	printf ("  >> ./vortex-file-transfer-client --server=localhost bigmsg --transfer-count=10\n");


	printf ("\n");
	printf ("Running test..\n");

	/* create the context */
	ctx = vortex_ctx_new ();

	/* init vortex library */
	if (! vortex_init_ctx (ctx)) {
		/* unable to init context */
		vortex_ctx_free (ctx);
		return -1;
	} /* end if */

	/* creates a new connection against localhost:44017 */
	server_host = SERVER_HOST;

	/* find server */
	iterator = 1;
	while (iterator < argc) {
		if (axl_memcmp (argv[iterator], "--server=", 9)) {
			/* configuring host */
			server_host = argv[iterator] + 9;
		} else if (axl_cmp (argv[iterator], "bigmsg")) {
			format = "bigmsg";
		} else if (axl_cmp (argv[iterator], "feeder")) {
			format = "feeder";
		} else if (axl_cmp (argv[iterator], "ansnul")) {
			format = "ansnul";
		} else if (axl_cmp (argv[iterator], "ansnulfeeder")) {
			format = "ansnulfeeder";
		} else if (axl_memcmp (argv[iterator], "--transfer-count=", 17)) {
			/* configure transfer count */
			transfer_count = atoi (argv[iterator] + 17);
		} else if (axl_memcmp (argv[iterator], "--window-size=", 14)) {
			/* configure host */
			window_size = atoi (argv[iterator] + 14);
		}

		/* iterator next */
		iterator++;
	} /* end while */

	printf ("connecting to %s:44017...\n", server_host);
	
	/* create the connection */
	connection = vortex_connection_new (ctx, server_host, "44017", NULL, NULL);
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
	if (axl_cmp (format, "bigmsg")) {
		printf ("Creating a channel with big message profile\n");
		channel = vortex_channel_new (connection, 0,
					      FILE_TRANSFER_URI_WITH_MSG,
					      /* no close handling */
					      NULL, NULL,
					      /* frame received */
					      frame_received_with_msg, queue,
					      /* no async channel creation */
					      NULL, NULL);
	} else 	if (axl_cmp (format, "feeder")) {
		printf ("Creating a channel with payload feeder\n");
		channel = vortex_channel_new (connection, 0,
					      FILE_TRANSFER_URI_WITH_FEEDER,
					      /* no close handling */
					      NULL, NULL,
					      /* frame received */
					      frame_received_with_msg, queue,
					      /* no async channel creation */
					      NULL, NULL);		

		/* set complete frame */
		vortex_channel_set_complete_flag (channel, axl_false);
	} else 	if (axl_cmp (format, "ansnul")) {
		printf ("Creating channel with ANS/NUL pattern..\n");
		channel = vortex_channel_new (connection, 0,
					      FILE_TRANSFER_URI,
					      /* no close handling */
					      NULL, NULL,
					      /* frame received */
					      frame_received, queue,
					      /* no async channel creation */
					      NULL, NULL);
	} else 	if (axl_cmp (format, "ansnulfeeder")) {
		printf ("Creating channel with ANS/NUL FEEDER pattern..\n");
		ansnulfeeder = axl_true;
		channel = vortex_channel_new (connection, 0,
					      FILE_TRANSFER_URI_WITH_ANS_FEEDER,
					      /* no close handling */
					      NULL, NULL,
					      /* frame received */
					      frame_received, queue,
					      /* no async channel creation */
					      NULL, NULL);
	} /* end if */

	if (channel == NULL) {
		printf ("Unable to create the channel..\n");
		goto end;
	}

	/* get number of messages to be sent */
	printf ("Requested to download %s, %d times..\n", FILE_TO_TRANSFER, transfer_count);

	/* get channel window size */
	if (window_size > 0) {
		printf ("Requested to change window size to: %d..\n", window_size);
		vortex_channel_set_window_size (channel, window_size);
	}

	/* serialize channel */
	printf ("Configuring set serialize..\n");
	vortex_channel_set_serialize (channel, axl_true);
	
	/* open the file */
transfer_again:
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
	gettimeofday (&start, NULL);
	if (! vortex_channel_send_msg (channel, 
				       "send the message, please",
				       24, 
				       NULL)) {
		printf ("Failed to send message that request the file to transfer..\n");
		return -1;
	}

	/* wait for the file */
	vortex_async_queue_pop (queue);
	transfer_count--;
	if (transfer_count > 0) {
/*		if (transfer_count == 2) {
			vortex_log_enable (ctx, axl_true);
			vortex_color_log_enable (ctx, axl_true);
		} 
 */
		printf ("Transfer done, pending count: %d\n", transfer_count); 
		goto transfer_again;
	}

	printf ("closing file...\n");
	fclose (file);

	gettimeofday (&stop, NULL);

	vortex_async_queue_unref (queue);

	/* close the channel */
	vortex_channel_close (channel, NULL);

 end:				      
	vortex_connection_close (connection);
	vortex_exit_ctx (ctx, axl_true);

	/* get result */
	vortex_timeval_substract (&stop, &start, &result);

	memset (&status, 0, sizeof (struct stat));
	if (stat (FILE_TO_TRANSFER, &status) != 0) {
		printf ("ERROR: unable to get file size from %s\n", FILE_TO_TRANSFER);
		return 0;
	}
	printf ("Time to transfer %d bytes (%d Kbytes): %ld.%ld secs\n", 
		(int) status.st_size, (int) status.st_size / 1024, (long) result.tv_sec, (long) result.tv_usec / 1000);

	return 0;	      
}


