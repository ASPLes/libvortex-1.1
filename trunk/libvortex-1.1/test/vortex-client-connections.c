/* include main library */
#include <vortex.h>

/* include tunnel support */
#include <vortex_tunnel.h>

#define MAX_NUM_CON 1000

/* location of the vortex-regression-listener */
#define LISTENER_HOST "localhost"
#define LISTENER_PORT "44010"

/* BEEP proxy running on the following port, also provided by
 * vortex-regression-listener */
#define LISTENER_PROXY_HOST "localhost"
#define LISTENER_PROXY_PORT "44110"

/** 
 * @internal Allows to know if the connection must be created directly or
 * through the tunnel.
 */
VortexTunnelSettings * tunnel_settings = NULL;

/* listener context */
VortexCtx * ctx = NULL;

VortexConnection * connection_new (void)
{
	/* create a new connection */
	if (tunnel_settings) {
		printf ("..using proxy..\n");
		/* create a tunnel */
		return vortex_tunnel_new (tunnel_settings, NULL, NULL);
	} else {
		/* create a direct connection */
		return vortex_connection_new (ctx, LISTENER_HOST, LISTENER_PORT, NULL, NULL);
	}
}

void __pause (char * message, int seconds)
{
	struct timeval      time;
	printf ("%s",message);
	time.tv_sec  = seconds;
	time.tv_usec = 0;
	select (0, NULL, NULL, NULL, &time);

	return;
}

/* substract */
void subs (struct timeval stop, struct timeval start, struct timeval * _result)
{
	long  result;

	result = stop.tv_usec - start.tv_usec;
	if (result < 0) {
		/* update result */
		_result->tv_usec = (1000000 - start.tv_usec) + stop.tv_usec;
	} else {
		/* update result */
		_result->tv_usec = result;
	}

	/* seconds */
	_result->tv_sec = stop.tv_sec - start.tv_sec;
}

int main (int argc, char ** argv) 
{
	int                 iterator = 0;
	int                 count    = 0;
	VortexConnection  * connections[MAX_NUM_CON];
#if defined(AXL_OS_UNIX)
	struct timeval      start;
	struct timeval      stop;
#endif

/*	vortex_log_enable (axl_true);
	vortex_color_log_enable (axl_true);  */

	/* create the context */
	ctx = vortex_ctx_new ();

	/* init vortex library */
	if (! vortex_init_ctx (ctx)) {
		/* unable to init context */
		vortex_ctx_free (ctx);
		return -1;
	} /* end if */

/*	vortex_io_waiting_use (VORTEX_IO_WAIT_EPOLL);   */
	vortex_conf_set (ctx, VORTEX_HARD_SOCK_LIMIT, 4096, NULL); 
	vortex_conf_set (ctx, VORTEX_SOFT_SOCK_LIMIT, 4096, NULL);  
	
	/* create tunnel settings */
	if (argc > 1 && axl_cmp (argv[1], "--use-proxy")) {
		tunnel_settings = vortex_tunnel_settings_new (ctx);
		
		/* add end point behind it */
		vortex_tunnel_settings_add_hop (tunnel_settings,
						TUNNEL_IP4, LISTENER_HOST,
						TUNNEL_PORT, LISTENER_PORT,
						TUNNEL_END_CONF);

		/* add the tunnel hop through */
		vortex_tunnel_settings_add_hop (tunnel_settings,
						TUNNEL_IP4, LISTENER_PROXY_HOST,
						TUNNEL_PORT, LISTENER_PROXY_PORT,
						TUNNEL_END_CONF);
	} /* end if */

#if defined(AXL_OS_UNIX)	
	/* take a start measure */
	gettimeofday (&start, NULL);
#endif

	/* clear array */
	iterator = 0;
	while (iterator < MAX_NUM_CON) { 
		connections[iterator] = 0;

		iterator++;
	}

	iterator = 0;
	memset (connections, 0, sizeof (VortexConnection *) * MAX_NUM_CON);
	while (iterator < MAX_NUM_CON) { 
		/* creates a new connection against localhost:44000 */
		printf ("(iterator=%d) connecting to localhost:44000...", iterator);
		connections[iterator] =connection_new ();
		if (!vortex_connection_is_ok (connections[iterator], axl_false)) {
			printf ("..failed to create a connection on iteration=%d: %s..\n", iterator, vortex_connection_get_message (connections[iterator]));
			break;
			/* break; */
		}
		printf ("ok\n");

		/* update iterator */
		iterator++;
		
		/* check for proxy test activation */
		if (tunnel_settings != NULL && iterator == 10) {
			/* stop proxy test .. */
			break;
		} /* end if */
	} /* end while */

	/* store current count */
	count = iterator;
	
	iterator = 0;
	while (iterator < MAX_NUM_CON) {
		printf ("(iterator=%d) closing localhost:44000...\n", iterator);

		/* close the connection */
		if (connections[iterator] != NULL) 
			vortex_connection_close (connections[iterator]);
		else
			break;

		/* update iterator */
		iterator++;
	}

#if defined(AXL_OS_UNIX)
	/* take a stop measure */
	gettimeofday (&stop, NULL);
#endif

	/* free proxy settings */
	vortex_tunnel_settings_free (tunnel_settings);

	/* end vortex function */
	vortex_exit_ctx (ctx, axl_true);

#if defined(AXL_OS_UNIX)
	/* substract */
	subs (stop, start, &stop);
#endif

	switch (vortex_io_waiting_get_current (ctx)) {
	case VORTEX_IO_WAIT_SELECT:
		printf ("INFO: used select(2) system call\n");
		break;
	case VORTEX_IO_WAIT_POLL:
		printf ("INFO: used poll(2) system call\n");
		break;
	case VORTEX_IO_WAIT_EPOLL:
		printf ("INFO: used epoll(2) system call\n");
		break;
	} /* end if */

#if defined(AXL_OS_UNIX)
	printf ("INFO: Test ok, created %d connections: diff: %ld.%ld!\n", 
		count, (long) stop.tv_sec, (long) stop.tv_usec);
#endif

	return 0;
}
