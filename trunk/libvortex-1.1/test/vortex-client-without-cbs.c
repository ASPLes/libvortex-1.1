/** 
 * A vortex client application driven without callbacks that gets
 * inside the main loop.
 * 
 * This is an example that shows how to create a simple client that
 * handles all incoming messages, without callbacks, using a frame
 * received handler the vortex_channel_queue_reply function and pipes
 * to comunicate with a main loop.
 *
 * The idea is to use the pipe to notify the loop and the queue to
 * transport the data received between the thread that received the
 * frame notification and the thread that is running the main loop.
 *
 */
#include <vortex.h>

#define PROFILE_URI "http://vortex.aspl.es/profiles/example"

/* listener context */
VortexCtx * ctx = NULL;

/** 
 * @brief Pipe used to get notifications about frame received.
 */
int             frame_pipe[2];

/** 
 * @brief Pipe used to get notifications about channel close request.
 */
int             close_pipe[2];

#define MAX(a,b) ((a > b) ? a : b)

/* prototypes to handle frame received */
void  queue_reply           (VortexChannel    * channel,
			     VortexConnection * connection,
			     VortexFrame      * frame,
			     axlPointer           user_data);

void  handle_frame_received (int                frame_pipe [], 
			     VortexAsyncQueue * queue);

/* prototypes to handler close request received */
void close_request_received (VortexChannel * channel, 
			     int             msg_no,
			     axlPointer        user_data);

void handle_close_request   (int                close_pipe[2], 
			     VortexAsyncQueue * close_queue);


int main (int argc, char ** argv) {
	/* the all frames received queue */
	VortexAsyncQueue   * queue;
	VortexAsyncQueue   * close_queue;
	VortexChannel      * channel;
	VortexConnection   * connection = NULL;

	/* loop variables */
	int                iterator   = 0;
	fd_set             rdfs;
	struct timeval     tv;
	int                retval;
	int                max_fds;
	
	/* create the context */
	ctx = vortex_ctx_new ();

	/* init vortex library */
	if (! vortex_init_ctx (ctx)) {
		/* unable to init context */
		vortex_ctx_free (ctx);
		return -1;
	} /* end if */
	
	
	/* create the queue for frame received */
	queue = vortex_async_queue_new ();

	/* create the queue for close channel requests */
	close_queue = vortex_async_queue_new ();

	/* create the pipe and calculate the max fd */
	if (pipe (frame_pipe) < 0) {
		printf ("Unable to create pipe\n");
		return -1;
	}
	max_fds    = MAX (frame_pipe[0], frame_pipe[1]);

	/* create the pipe to get close channel notifications */
	if (pipe (close_pipe) < 0) {
		printf ("Unable to create the close pipe\n");
		return -1;
	}

	/* get max fds */
	max_fds = MAX (max_fds, close_pipe[0]);
	max_fds = MAX (max_fds, close_pipe[1]);

	/* register a profile */
	vortex_profiles_register (ctx, "http://vortex.aspl.es/profiles/example", 
				  /* no start handler, accept all channels */
				  NULL, NULL,
				  /* no close channel, accept all
				   * channels to be closed */
				  NULL, NULL,
				  vortex_channel_queue_reply, queue);


	/* create a connection in a blocking manner */
	connection = vortex_connection_new (ctx, "localhost", "4400", NULL, NULL);
	if (! vortex_connection_is_ok (connection, axl_false)) {
		printf ("Unable to create the connection..\n");
		goto finish;
	}

	/* create a channel */
	channel = vortex_channel_new (connection, 
				      /* next channel available */
				      0,
				      /* profile */
				      PROFILE_URI,
				      /* no close notification */
				      NULL, NULL,
				      /* use queue_reply as frame
				       * receive handler */
				      queue_reply, queue,
				      /* no channel creation notification */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("Unable to create the channel..\n");
		goto finish;
	}

	/* configure close notification */
	vortex_channel_set_close_notify_handler (channel, close_request_received, close_queue);
				      
	/* Hey Sam, a really simple event loop K-) */
	while (axl_true) {

		/* send a frame */
		if (iterator < 10) {
			printf ("Sending message: %d..\n", iterator);
			if (! vortex_channel_send_msgv (channel, NULL, "This is a test: %d", iterator)) {
				printf ("Unable to send message\n");
				goto finish;
			}
		}

 		/* update iterator */
		iterator++;

		/* configure file descriptors */
		FD_ZERO (&rdfs);
		FD_SET (frame_pipe[0], &rdfs);
		FD_SET (close_pipe[0], &rdfs);

		/* configure timeout */
		tv.tv_sec  = 5;
		tv.tv_usec = 0;
		
		/* wait for the frame */
		retval = select (max_fds + 1, &rdfs, NULL, NULL, &tv);

		/* signal trap */
		if (retval == -1 && errno == EINTR) {
			printf ("Signal trap..retrying..\n");
			continue;
		}
		
		/* timeout */
		if (retval == 0) {
			printf ("Timeout found..retrying..\n");
			continue;
		}

		/* check for the frame pipe for a fresh frame  */
		if (FD_ISSET (frame_pipe[0], &rdfs)) {

			/* handle frame received request */
			handle_frame_received (frame_pipe, queue);

			/* next event pending */
			continue;
		} /* end if */

		/* check for a fresh close request */ 
		if (FD_ISSET (close_pipe[0], &rdfs)) {

			/* handler close request */
			handle_close_request (close_pipe, close_queue);
			
			break;
		}

	} /* end while */

	/* terminate example */
 finish:

	/* unref the queue */
	vortex_async_queue_unref (queue);

	/* first the connection */
	vortex_connection_close (connection);

	/* then the vortex engine */
	vortex_exit_ctx (ctx, axl_true);

	return 0;
}


/** 
 * A marshalling function that queues all replies received on the
 * async queue found on the last parameter, allowing the async queue
 * owner to control when read frames received. 
 *
 * The main loop, instead of getting blocked on vortex_async_queue_pop, it
 * gets notifications from the pipe that this handler writes. So, if a
 * frame is received, one byte "f" (frame) is written on the pipe,
 * waking up the main loop.
 */
void  queue_reply                    (VortexChannel    * channel,
				      VortexConnection * connection,
				      VortexFrame      * frame,
				      axlPointer user_data)
{
	VortexAsyncQueue * queue      = user_data;
	VortexFrame      * frame_copy = NULL;
	int                result;

	/* copy the frame because the first level invocation handler
	 * will deallocate the frame once terminated this handler */
	frame_copy = vortex_frame_copy (frame);

	/* queue the channel and the frame */
	vortex_async_queue_push (queue, frame_copy);

	/* write to the frame pipe */
	result = write (frame_pipe[1], "f", 1);

	/* nothing more */
	return;
}

/** 
 * Function called from inside the main loop to notify that a frame
 * was received.
 * 
 * @param frame_pipe The pipe used to get notifications from the queue_reply handler.
 *
 * @param queue The queue where fresh frames will be received.
 */
void handle_frame_received (int frame_pipe [], VortexAsyncQueue * queue)
{

	/* local variables */
	char               value[2];
	VortexFrame      * frame;

	/* frame received */
	if (read (frame_pipe[0], value, 1) < 0) {
		printf ("Failed to read from the pipe\n");
		return;
	}
	
	/* get the frame:
	 * NOTE: This call must be evolved to something with
	 * error handling, timeout and channel piggyback
	 * support, but, for the example will do.  */
	frame = vortex_async_queue_pop (queue);
	printf ("Frame received: (size: %d):\n%s\n\n",
		vortex_frame_get_payload_size (frame),
		(char*) vortex_frame_get_payload (frame));
	
	/* to know exactly the channel and the
	 * connection where the frame was received use
	 * the following:
	 * 
	 * channel = vortex_frame_get_channel_ref (frame);
	 * 
	 * and:
	 *
	 * connection = vortex_channel_get_connection (channel);
	 */
	
	/* At this point, is inserted your particular
	 * stuff to notify the frame received, the
	 * channel and the connection. */
	
	
	/* Because the frame isn't notified from the
	 * second and first level handlers (threaded
	 * invocation) you must free the frame */
	vortex_frame_free (frame);
	
	/* next event pending */
	return;
}

/** 
 * This is part of the decoupled close request notification, that
 * allows to get the notification and then manage the close action
 * later.
 *
 * This handler will be used to queue the channel that is requested to
 * be closed along with the msg no required by the \ref
 * vortex_channel_close_notify function.
 *
 * Then, a write on the close pipe is done to notify the mainloop that
 * a request to close a channel is pending to be managed.
 */
void close_request_received (VortexChannel * channel, 
			     int             msg_no,
			     axlPointer      user_data)
{
	/* get the queue */
	VortexAsyncQueue * queue      = user_data;
	int                result;

	/* queue the frame reference */
	vortex_async_queue_push (queue, channel);
	
	/* queue the msgno value */
	vortex_async_queue_push (queue, INT_TO_PTR (msg_no));

	/* notify the main loop */
	result = write (close_pipe[1], "c", 1);
	
	printf ("Received a close notify!\n");

	/* nothing more */
	return;
}

/** 
 * Function used to handle close request queue by the
 * close_request_received.
 * 
 * @param close_pipe The pipe where close request notifications are
 * received.
 *
 * @param close_queue The queue where the channel and the msgno
 * parameters are expected to be found.
 */
void handle_close_request (int close_pipe[2], VortexAsyncQueue * close_queue)
{
	char            value[2];
	VortexChannel * channel;
	int             msg_no;
			
	/* read close request */
	if (read (close_pipe[0], value, 1) < 0) {
		printf ("Failed to read close notification\n");
		return;
	}
	
	printf ("Handling close channel, accepting to close it\n");

	/* get the channel to be closed */
	channel = vortex_async_queue_pop (close_queue);
	
	/* get the msgno */
	msg_no  = PTR_TO_INT (vortex_async_queue_pop (close_queue));
	
	/* close the channel (by default on this
	 * example of course) */
	vortex_channel_notify_close (channel, 
				     /* close the channel */
				     axl_true, 
				     /* msg no, the parameter received 
					in the close notify */
				     msg_no);
	return;
}
