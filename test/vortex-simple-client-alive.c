#include <vortex.h>
#include <time.h>
#include <vortex_alive.h>

#define PLAIN_PROFILE "http://fact.aspl.es/profiles/plain_profile"

time_t start_period;
VortexAsyncQueue * queue;

void failure_handler (VortexConnection *conn, long check_period, int unreply_count)
{
	/* print failure handler */
	printf ("ALIVE: failure detected on connection id=%d\n", vortex_connection_get_id (conn));
	vortex_async_queue_push (queue, INT_TO_PTR (time (NULL)));
	return;
}

int  main (int  argc, char ** argv)
{
	VortexConnection * connection;
	VortexCtx        * ctx;
	time_t             stop_period;
	int                unreply_count = 3;
	int                check_period = 3000000;

	/* init vortex library */
	ctx = vortex_ctx_new ();
	if (! vortex_init_ctx (ctx)) {
		/* unable to init vortex */
		vortex_ctx_free (ctx);

		return -1;
	} /* end if */


	/* creates a new connection against localhost:44000 */
	printf ("ALIVE: connecting to localhost:44000...\n");
	connection = vortex_connection_new (ctx, "localhost", "44000", NULL, NULL);
	if (!vortex_connection_is_ok (connection, axl_false)) {
		printf ("Unable to connect remote server, error was: %s\n",
			 vortex_connection_get_message (connection));
		exit (-1);
	}
	printf ("ALIVE: ok (%d)\n", vortex_connection_is_ok (connection, axl_false));

	queue = vortex_async_queue_new ();
	if (! vortex_alive_enable_check (connection, check_period, unreply_count, failure_handler)) {
		printf ("ALIVE: error, unable to activate alive process..\n");
		exit (-1);
	}  /* end if */

	start_period = time (NULL);
	printf ("ALIVE: starting check at %d, wait_period=%d, unreply_count=%d, waiting for failure...\n", 
		(int) start_period, check_period, unreply_count);
	stop_period = PTR_TO_INT (vortex_async_queue_pop (queue));

	printf ("ALIVE: failure detected after %d seconds\n", (int) (stop_period - start_period));
		      
	vortex_connection_shutdown (connection);
	vortex_connection_close (connection);

	/* terminate execution context */
	vortex_exit_ctx (ctx, axl_false);

	/* free ctx */
	vortex_ctx_free (ctx);

	return 0 ;	      
}


