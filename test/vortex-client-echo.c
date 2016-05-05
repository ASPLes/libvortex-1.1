/*  LibVortex:  A BEEP implementation
 *  Copyright (C) 2016 Advanced Software Production Line, S.L.
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

int  main (int  argc, char ** argv) 
{

	VortexConnection   * conn;
	VortexCtx          * ctx;
	struct timeval       start;
	struct timeval       stop;
	struct timeval       diff;
	VortexChannel      * channel;
	int                  iterator;
	VortexAsyncQueue   * queue;
	VortexChannelPool  * pool;
	VortexFrame        * frame;

	/* create the context */
	ctx = vortex_ctx_new ();

	/* init vortex library */
	if (! vortex_init_ctx (ctx)) {
		/* unable to init context */
		vortex_ctx_free (ctx);
		return -1;
	} /* end if */

	if (argc < 3) {
		printf ("You must provide host and port to connect to: \n");
		printf (">> %s [host] [port]\n", argv[0]);
		exit (-1);
	}

	/* get current start */
	gettimeofday (&start, NULL);

	/* create connection */
	printf ("CONNECTION: Connecting to %s:%s..\n", argv[1], argv[2]);
	conn = vortex_connection_new (ctx, argv[1], argv[2], NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR: failed to connect to host %s:%s, error was: %s", 
			argv[1], argv[2], vortex_connection_get_message (conn));
		exit (-1);
	}

	/* get current stop and diff them */
	gettimeofday (&stop, NULL);
	gettimeofday (&diff, NULL);
	vortex_timeval_substract (&stop, &start, &diff);

	printf ("CONNECTION: OK, connected in %ld secs and %ld microsecs\n", (long) diff.tv_sec, (long) diff.tv_usec);

	/* create the queue */
	queue = vortex_async_queue_new ();

	/* create a channel */
	gettimeofday (&start, NULL);
	pool = vortex_channel_pool_new (conn, 
					"urn:aspl.es:beep:profiles:echo",
					1, /* one channel */
					/* no close handling */
					NULL, NULL,
					/* frame receive async handling */
					NULL, NULL,
					/* no async channel creation */
					NULL, NULL);
	if (pool == NULL) {
		printf ("ERROR: failed to create channel pool..\n");
		exit (-1);
	} /* end if */


	gettimeofday (&stop, NULL);
	gettimeofday (&diff, NULL);
	vortex_timeval_substract (&stop, &start, &diff);
	
	printf ("POOL: OK, channel pool with 1 channel created in in %ld secs and %ld microsecs\n", (long) diff.tv_sec, (long) diff.tv_usec);
	printf ("POOL: Now testing..\n");
	iterator = 0;
	while (iterator < 3) {

		/* create a channel */
		gettimeofday (&start, NULL);
		channel = vortex_channel_pool_get_next_ready (pool, axl_true);
		
		if (channel == NULL) {
			printf ("ERROR: failed to get channel from the pool..\n");
			exit (-1);
		} /* end if */
		gettimeofday (&stop, NULL);
		gettimeofday (&diff, NULL);
		vortex_timeval_substract (&stop, &start, &diff);

		/* set serialize */
		vortex_channel_set_serialize (channel, axl_true);
		vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);

		printf ("CHANNEL: OK, channel created in %ld secs and %ld microsecs\n", (long) diff.tv_sec, (long) diff.tv_usec);

		/* now send message */
		gettimeofday (&start, NULL);

		/* send message */
		printf ("DATA: Ok, now sending data..\n");
		if (! vortex_channel_send_msg (channel, "This is a test..", 16, NULL)) {
			printf ("ERROR: failed send content..\n");
			exit (-1);
		}

		/* get frame reply */
		frame = vortex_channel_get_reply (channel, queue);
		if (frame == NULL) {
			printf ("ERROR: expected to receive frame but found NULL reply..\n");
			exit (-1);
		}

		/* check content */
		if (! axl_cmp (vortex_frame_get_payload (frame), "This is a test..")) {
			printf ("ERROR: expected to receive frame content...\n");
			exit (-1);
		} /* end if */

		/* get diference */
		gettimeofday (&stop, NULL);
		gettimeofday (&diff, NULL);
		vortex_timeval_substract (&stop, &start, &diff);

		printf ("DATA: OK, message reply received in %ld secs and %ld microsecs\n", (long) diff.tv_sec, (long) diff.tv_usec);

		/* release channel */
		vortex_channel_pool_release_channel (pool, channel);

		vortex_frame_unref (frame);

		/* next */
		iterator++;
	}

	/* close connection */
	vortex_connection_close (conn);

	/* end vortex function */
	vortex_exit_ctx (ctx, axl_true);

	return 0;
}
