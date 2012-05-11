#include <vortex.h>


#define PLAIN_PROFILE "http://fact.aspl.es/profiles/plain_profile"

int total_nul_received = 0;
VortexMutex mutex;
VortexAsyncQueue * queue;

void frame_received (VortexChannel    * channel,
		     VortexConnection * connection,
		     VortexFrame      * frame,
		     axlPointer         user_data)
{
	int              msg_no;
	char           * msg_no_str;
	struct timeval * start;
	struct timeval   now;
	struct timeval   diff;

	/* get message we are getting reply to */
	msg_no = vortex_frame_get_msgno (frame);

	/* get now reference */
	msg_no_str = axl_strdup_printf ("%d", msg_no);
	start      = vortex_channel_get_data (channel, msg_no_str);
	axl_free (msg_no_str);

	/* get current stamp */
	gettimeofday (&now, NULL);

	if (start) {
		/* get difference */
		vortex_timeval_substract (&now, start, &diff);
	}

	if (start && diff.tv_sec >= 1) {
		printf ("ERR: timeout expired found during wait, timeout found was: %d for message: %d (%p)\n", (int) diff.tv_sec, msg_no, start);
		exit (-1);
	}

	if (start && vortex_frame_get_type (frame) == VORTEX_FRAME_TYPE_ANS) {
		/* update timeout */
		start->tv_sec  = now.tv_sec;
		start->tv_usec = now.tv_usec;
		printf ("INF: timeout reseted, timeout found was: %d for message: %d (%p), %d.%d\n", 
			(int) diff.tv_sec, msg_no, start, (int) start->tv_sec, (int) start->tv_usec);
	} else if (vortex_frame_get_type (frame) == VORTEX_FRAME_TYPE_NUL) {
		vortex_mutex_lock (&mutex);
		total_nul_received++;
		vortex_mutex_unlock (&mutex);

		if (total_nul_received == 10) {
			/* unlock caller */
			vortex_async_queue_push (queue, INT_TO_PTR (axl_true));
		}
	} /* end if */

	return;
}

int  main (int  argc, char ** argv)
{
	VortexConnection * connection;
	VortexChannel    * channel;
	VortexCtx        * ctx;
	int                msg_no;
	struct timeval   * now;
	int                iterator;

	/* init mutex */
	vortex_mutex_create (&mutex);
	queue = vortex_async_queue_new ();

	/* init vortex library */
	ctx = vortex_ctx_new ();
	if (! vortex_init_ctx (ctx)) {
		/* unable to init vortex */
		vortex_ctx_free (ctx);

		return -1;
	} /* end if */


	/* creates a new connection against localhost:44000 */
	printf ("connecting to localhost:44000...\n");
	connection = vortex_connection_new (ctx, "localhost", "44000", NULL, NULL);
	if (!vortex_connection_is_ok (connection, axl_false)) {
		printf ("Unable to connect remote server, error was: %s\n",
			 vortex_connection_get_message (connection));
		exit (-1);
	}
	printf ("ok\n");

	/* send 10 messages */
	iterator = 0;
	while (iterator < 10) {

		/* create a new channel (by chosing 0 as channel number the
		 * Vortex Library will automatically assign the new channel
		 * number free. */
		channel = vortex_channel_new (connection, 0,
					      PLAIN_PROFILE,
					      /* no close handling */
					      NULL, NULL,
					      /* no frame receive async
					       * handling */
					      frame_received, NULL,
					      /* no async channel creation */
					      NULL, NULL);
		if (channel == NULL) {
			printf ("Unable to create the channel..\n");
			exit (-1);
		}

		/* store message time stamp */
		now = axl_new (struct timeval, 1);
		gettimeofday (now, NULL);

		/* send a message */
		if (! vortex_channel_send_msg (channel, "This is a test message to be sent to the central server to test timeout while creating threads that handles content received", 124, &msg_no)) {
			printf ("ERROR: failed to send message to the server..\n");
			exit (-1);
		} /* end if */

		/* stored reply */
		vortex_channel_set_data_full (channel, (axlPointer) axl_strdup_printf ("%d", msg_no), now, axl_free, axl_free);

		/* next iterator */
		iterator++;
	}

	/* waiting replies */
	printf ("Waiting replies from the server..\n");
	vortex_async_queue_pop (queue);
	printf ("All messages recieved: %d\n", total_nul_received);
     
	vortex_connection_close (connection);

	/* terminate execution context */
	vortex_exit_ctx (ctx, axl_false);

	/* free ctx */
	vortex_ctx_free (ctx);

	return 0 ;	      
}


