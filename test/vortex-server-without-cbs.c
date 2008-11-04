/** 
 * This is an example that shows how to create a simple server that
 * handles all incoming messages, without callbacks, using a frame
 * received handler the vortex_channel_queue_reply function.
 */
#include <vortex.h>

#define PROFILE_URI "http://vortex.aspl.es/profiles/example"

/* listener context */
VortexCtx * ctx = NULL;

/** 
 * A marshalling function that queues all replies received on the
 * async queue found on the last parameter, allowing the async queue
 * owner to control when read frames received.
 */
void  queue_reply                    (VortexChannel    * channel,
				      VortexConnection * connection,
				      VortexFrame      * frame,
				      axlPointer         user_data)
{
	VortexAsyncQueue * queue      = user_data;
	VortexFrame      * frame_copy = NULL;

	/* copy the frame because the first level invocation handler
	 * will deallocate the frame once terminated this handler */
	frame_copy = vortex_frame_copy (frame);

	/* queue the channel and the frame */
	vortex_async_queue_push (queue, frame_copy);

	/* nothing more */
	return;
}

int main (int argc, char ** argv) {

	/* the all frames received queue */
	VortexAsyncQueue * queue;
	VortexFrame      * frame;
	VortexChannel    * channel;

	/* count */
	int             iterator = 0;
	
	/* create the context */
	ctx = vortex_ctx_new ();

	/* init vortex library */
	if (! vortex_init_ctx (ctx)) {
		/* unable to init context */
		vortex_ctx_free (ctx);
		return -1;
	} /* end if */
	
	/* create the queue */
	queue = vortex_async_queue_new ();

	/* register a profile */
	vortex_profiles_register (ctx, PROFILE_URI,
				  /* no start handler, accept all channels */
				  NULL, NULL,
				  /* no close channel, accept all
				   * channels to be closed */
				  NULL, NULL,
				  vortex_channel_queue_reply, queue);
	
	/* now create a vortex server listening on several ports */
	vortex_listener_new (ctx, "0.0.0.0", "4400", NULL, NULL);


	/* and handle all frames received */
	while (axl_true) {

		/* get the next message, blocking at this call. 
		 *
		 * NOTE: This call must be evolved to something with
		 * error handling, timeout and channel piggyback
		 * support, but, for the example will do.  */
		frame = vortex_async_queue_pop (queue);

		if (frame == NULL) {
			/* for our example, the default action is:
			 *     keep on reading!.
			 */
			continue;
		}
		printf ("Frame received, content: %s\n",
			(char*) vortex_frame_get_payload (frame));

		/* get the channel */
		channel = vortex_frame_get_channel_ref (frame);
		
		/* reply */
		vortex_channel_send_rpy (channel, 
					 vortex_frame_get_payload (frame),
					 vortex_frame_get_payload_size (frame),
					 vortex_frame_get_msgno (frame));

		/* deallocate the frame received */
		vortex_frame_free (frame);

		iterator++;

		/* close the channel */
		if (iterator == 10) {
			/* close the channel */
			if (! vortex_channel_close (channel, NULL)) {
				printf ("Unable to close the channel!\n");
			}

			printf ("Ok, channel closed\n");

			/* reset */
			iterator = 0;
			
		} /* end if */
	} /* end while */
	
	
	/* end vortex internal subsystem (if no one have done it yet!) */
	vortex_exit_ctx (ctx, axl_true);
 
	/* that's all to start BEEPing! */
	return 0;     
 }  
