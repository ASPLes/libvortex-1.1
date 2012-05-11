#include <vortex.h>

#define PLAIN_PROFILE "http://fact.aspl.es/profiles/plain_profile"

/* listener context */
VortexCtx * ctx = NULL;

void frame_received (VortexChannel    * channel,
		     VortexConnection * connection,
		     VortexFrame      * frame,
		     axlPointer           user_data)
{
	VortexAsyncQueue * queue;
	int                iterator = 0;

	
	printf ("A frame received on channl: %d\n",     vortex_channel_get_number (channel));
	printf ("Data received: '%s'\n",                (char*) vortex_frame_get_payload (frame)); 

	if (vortex_frame_get_type (frame) == VORTEX_FRAME_TYPE_MSG) {
		/* send back a series of replies */
		queue = vortex_async_queue_new ();

		while (iterator < 100) {
			/* wait during 50ms */
			vortex_async_queue_timedpop (queue, 50000);

			/* send a reply */
			vortex_channel_send_ans_rpy (channel, "This is a reply to the message", 30, vortex_frame_get_msgno (frame));

			/* next message */
			iterator++;
		}

		/* send NUL */
		vortex_channel_finalize_ans_rpy (channel, vortex_frame_get_msgno (frame));
		
	} 

	printf ("VORTEX_LISTENER: end task (pid: %d)\n", getpid ());


	return;
}

int      start_channel (int                channel_num, 
			VortexConnection * connection, 
			axlPointer           user_data)
{
	/* implement profile requirement for allowing starting a new
	 * channel */

	/* to return axl_false denies channel creation to return axl_true
	 * allows create the channel */
	return axl_true;
}

int      close_channel (int                channel_num, 
			VortexConnection * connection, 
			axlPointer           user_data)
{
	/* implement profile requirement for allowing to closeing a
	 * the channel to return axl_false denies channel closing to
	 * return axl_true allows to close the channel */
	return axl_true;
}

int      on_accepted (VortexConnection * connection, axlPointer data)
{
	printf ("New connection accepted from: %s:%s\n", 
		 vortex_connection_get_host (connection),
		 vortex_connection_get_port (connection));

	/* return axl_true to accept the connection to be created */
	return axl_true;
}

int  main (int  argc, char ** argv) 
{

	/* create the context */
	ctx = vortex_ctx_new ();

	vortex_thread_pool_set_num (1);

	/* init vortex library */
	if (! vortex_init_ctx (ctx)) {
		/* unable to init context */
		vortex_ctx_free (ctx);

		return -1;
	} /* end if */

	vortex_thread_pool_setup2 (ctx, 40, 1, 1, 1, 4, axl_true, axl_true);

	/* register a profile */
	vortex_profiles_register (ctx, PLAIN_PROFILE, 
				  start_channel, NULL, 
				  close_channel, NULL,
				  frame_received, NULL);

	/* create a vortex server */
	vortex_listener_new (ctx, "0.0.0.0", "44000", NULL, NULL);

	/* configure connection notification */
	vortex_listener_set_on_connection_accepted (ctx, on_accepted, NULL);

	/* wait for listeners (until vortex_exit is called) */
	vortex_listener_wait (ctx);
	
	/* end vortex function */
	vortex_exit_ctx (ctx, axl_true);

	return 0;
}

