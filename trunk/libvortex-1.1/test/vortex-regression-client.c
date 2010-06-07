/*
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2008 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1 of
 *  the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 *  
 *  You may find a copy of the license under this software is released
 *  at COPYING file. This is LGPL software: you are welcome to
 *  develop proprietary applications using this library without any
 *  royalty or fee but returning back any change, improvement or
 *  addition in the form of source code, project image, documentation
 *  patches, etc. 
 *
 *  For commercial support on build BEEP enabled solutions contact us:
 *          
 *      Postal address:
 *         Advanced Software Production Line, S.L.
 *         C/ Antonio Suarez Nº 10, 
 *         Edificio Alius A, Despacho 102
 *         Alcalá de Henares 28802 (Madrid)
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.aspl.es/vortex
 */

/* include vortex library */
#include <vortex.h>

/* include vortex tunnel support */
#include <vortex_tunnel.h>

/* include sasl support */
#include <vortex_sasl.h>

/* include tls support */
#include <vortex_tls.h> 

/* include local headers produced by xml-rpc-gen */
#include <test_xml_rpc.h>

/* include pull API header */
#include <vortex_pull.h>

/* include http connect header */
#include <vortex_http.h>

/* include alive header */
#include <vortex_alive.h>

/* include local support */
#include <vortex-regression-common.h>

#ifdef AXL_OS_UNIX
#include <signal.h>
#endif

/* disable time checks */
axl_bool disable_time_checks = axl_false;

/* listener location */
char   * listener_host = NULL;
#define LISTENER_PORT              "44010"
#define LISTENER_UNIFIED_SASL_PORT "44011"

/* listener proxy location */
char   * listener_tunnel_host = NULL;
#define LISTENER_PROXY_PORT "44110"

/* HTTP proxy support */
char   * http_proxy_host = NULL;
char   * http_proxy_port = NULL;

/* substract */
void subs (struct timeval stop, struct timeval start, struct timeval * _result)
{
	long result;

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

/** 
 * @internal Allows to know if the connection must be created directly or
 * through the tunnel.
 */
axl_bool               tunnel_tested   = axl_false;
VortexTunnelSettings * tunnel_settings = NULL;

/* listener context */
VortexCtx * ctx = NULL;

/** 
 * @internal Reference to signal connection_new to do a connection
 * through vortex HTTP CONNECT implementation.
 */
axl_bool enable_http_proxy = axl_false;

VortexConnection * connection_new (void)
{
	VortexHttpSetup  * setup;
	VortexConnection * conn;

	/* create a new connection */
	if (tunnel_tested) {

		/* create a tunnel */
		return vortex_tunnel_new (tunnel_settings, NULL, NULL);

	} else if (enable_http_proxy) {

		/* configure proxy options */
		setup = vortex_http_setup_new (ctx);
	
		/* configure connection: assume squid running at
		 * localhost:3128 (FIXME, add general support to configure a
		 * different proxy location) */
		vortex_http_setup_conf (setup, VORTEX_HTTP_CONF_ITEM_PROXY_HOST, http_proxy_host);
		vortex_http_setup_conf (setup, VORTEX_HTTP_CONF_ITEM_PROXY_PORT, http_proxy_port);
		
		/* create a connection */
		conn = vortex_http_connection_new (listener_host, "443", setup, NULL, NULL);
		
		/* terminate setup */
		vortex_http_setup_unref (setup);

		/* return connection created */
		return conn;

	} else {
		/* create a direct connection */
		return vortex_connection_new (ctx, listener_host, LISTENER_PORT, NULL, NULL);
	}
}

/** 
 * @brief Checks current implementation for async queues.
 *
 * @return axl_true if checks runs ok, otherwise axl_false is returned.
 */
axl_bool  test_00 (void) 
{
	VortexAsyncQueue * queue;

	/* check that the queue starts and ends properly */
	queue = vortex_async_queue_new ();

	vortex_async_queue_unref (queue);

	/* check queue data */
	queue = vortex_async_queue_new ();

	vortex_async_queue_push (queue, INT_TO_PTR (1));
	vortex_async_queue_push (queue, INT_TO_PTR (2));
	vortex_async_queue_push (queue, INT_TO_PTR (3));

	if (vortex_async_queue_length (queue) != 3) {
		fprintf (stderr, "found different queue length, expected 3");
		return axl_false;
	}

	/* check data poping */
	if (PTR_TO_INT (vortex_async_queue_pop (queue)) != 1) {
		fprintf (stderr, "expected to find value=1");
		return axl_false;
	}

	/* check data poping */
	if (PTR_TO_INT (vortex_async_queue_pop (queue)) != 2) {
		fprintf (stderr, "expected to find value=1");
		return axl_false;
	}

	/* check data poping */
	if (PTR_TO_INT (vortex_async_queue_pop (queue)) != 3) {
		fprintf (stderr, "expected to find value=1");
		return axl_false;
	}

	if (vortex_async_queue_length (queue) != 0) {
		fprintf (stderr, "found different queue length, expected 0");
		return axl_false;
	}

	/* free the queue */
	vortex_async_queue_unref (queue);

	/* check reference counting */
	queue = vortex_async_queue_new ();

	vortex_async_queue_ref (queue);
	vortex_async_queue_ref (queue);
	vortex_async_queue_ref (queue);

	/* unref */
	vortex_async_queue_unref (queue);
	vortex_async_queue_unref (queue);
	vortex_async_queue_unref (queue);
	vortex_async_queue_unref (queue);

	return axl_true;
}

void test_00a_block (VortexAsyncQueue * queue)
{
	/* wait master thread */
	printf ("Test 00-a: blocking on queue: %p\n", queue);
	vortex_async_queue_pop (queue);
	printf ("Test 00-a: unblocked on queue: %p\n", queue);
	return;
}

void test_00a_new_task (int * value_to_update)
{
	/* printf ("Test 00-a: running blocked task\n"); */
	/* sending data */
	*value_to_update = 17;
	
	/* printf ("Test 00-a: finished blocked task\n"); */
	return;
}

/** 
 * @brief Checks current implementation for async queues.
 *
 * @return axl_true if checks runs ok, otherwise axl_false is returned.
 */
axl_bool  test_00a (void) 
{
	int                started_threads;
	int                waiting_threads;
	int                pending_tasks;
	VortexAsyncQueue * queue;
	VortexAsyncQueue * temp;
	int                iterator;
	VortexCtx        * ctx;
	int                value_to_update = 0;

	/* init vortex here */
	ctx = vortex_ctx_new ();
	if (! vortex_init_ctx (ctx)) {
		printf ("Test 00-a: failed to init VortexCtx reference..\n");
		return axl_false;
	}

	/* create a queue */
	queue      = vortex_async_queue_new ();

	/* get stats from the master ctx */
	iterator = 0;
	while (iterator < 10) {
		vortex_thread_pool_stats (ctx, &started_threads, &waiting_threads, &pending_tasks);
		if (waiting_threads != 5)
			vortex_async_queue_timedpop (queue, 10000);
		iterator++;
	}

	/* check at this point that the pool is available */
	printf ("Test 00-a: values started_threads=%d, waiting_threads=%d, pending_tasks=%d\n",
		started_threads, waiting_threads, pending_tasks);
	if (started_threads != 5 || waiting_threads != 5 || pending_tasks != 0) {
		printf ("ERROR(1): expected to find different values at pool on init...\n");
		return axl_false;
	}

	/* TASK */
	vortex_thread_pool_new_task (ctx, (VortexThreadFunc) test_00a_block, queue);

	vortex_thread_pool_stats (ctx, &started_threads, &waiting_threads, &pending_tasks);
	printf ("Test 00-a: values started_threads=%d, waiting_threads=%d, pending_tasks=%d\n",
		started_threads, waiting_threads, pending_tasks);

	/* TASK */
	vortex_thread_pool_new_task (ctx, (VortexThreadFunc) test_00a_block, queue);

	vortex_thread_pool_stats (ctx, &started_threads, &waiting_threads, &pending_tasks);
	printf ("Test 00-a: values started_threads=%d, waiting_threads=%d, pending_tasks=%d\n",
		started_threads, waiting_threads, pending_tasks);

	/* TASK */
	vortex_thread_pool_new_task (ctx, (VortexThreadFunc) test_00a_block, queue);

	vortex_thread_pool_stats (ctx, &started_threads, &waiting_threads, &pending_tasks);
	printf ("Test 00-a: values started_threads=%d, waiting_threads=%d, pending_tasks=%d\n",
		started_threads, waiting_threads, pending_tasks);

	/* TASK */
	vortex_thread_pool_new_task (ctx, (VortexThreadFunc) test_00a_block, queue);

	vortex_thread_pool_stats (ctx, &started_threads, &waiting_threads, &pending_tasks);
	printf ("Test 00-a: values started_threads=%d, waiting_threads=%d, pending_tasks=%d\n",
		started_threads, waiting_threads, pending_tasks);

	/* TASK */
	vortex_thread_pool_new_task (ctx, (VortexThreadFunc) test_00a_block, queue);

	vortex_thread_pool_stats (ctx, &started_threads, &waiting_threads, &pending_tasks);
	printf ("Test 00-a: values started_threads=%d, waiting_threads=%d, pending_tasks=%d\n",
		started_threads, waiting_threads, pending_tasks);

	/* wait until there are 5 thread running a blocked (simulating
	 * they are doing jobs) */
	iterator = 0;
	while (iterator < 10) {
		if (vortex_async_queue_waiters (queue) == 5)
			break;
		printf ("Test 00-a waiting for all threads, still not enough waiters: %d\n", 
			vortex_async_queue_waiters (queue));
		/* get stats from the master ctx */
		vortex_thread_pool_stats (ctx, &started_threads, &waiting_threads, &pending_tasks);
		printf ("Test 00-a:   values started_threads=%d, waiting_threads=%d, pending_tasks=%d\n",
			started_threads, waiting_threads, pending_tasks);
		vortex_async_queue_timedpop (queue, 1000);
		iterator++;
	}

	vortex_thread_pool_stats (ctx, &started_threads, &waiting_threads, &pending_tasks);
	printf ("Test 00-a: all threads working started_threads=%d, waiting_threads=%d, pending_tasks=%d\n",
		started_threads, waiting_threads, pending_tasks);

	/* at this point no more work is possible to handle, queue a new task */
	vortex_thread_pool_new_task (ctx, (VortexThreadFunc) test_00a_new_task, &value_to_update);

	/* check where stats */
	vortex_thread_pool_stats (ctx, &started_threads, &waiting_threads, &pending_tasks);
	printf ("Test 00-a: all threads working started_threads=%d, waiting_threads=%d, pending_tasks=%d\n",
		started_threads, waiting_threads, pending_tasks);
	
	/* check pending tasks */
	if (pending_tasks != 1) {
		printf ("Test 00-a: expected to find 1 pending tasks but found %d\n", pending_tasks);
		return axl_false;
	}

	/* add a new thread */
	vortex_thread_pool_add (ctx, 1);
	
	/* get the queue value */
	printf ("Test 00-a: Waiting for task blocked to start..\n");
	iterator = 0;
	while (iterator < 10) {
		if (value_to_update == 17)
			break;
		vortex_async_queue_timedpop (queue, 1000);
		iterator++;
	} /* end while */

	/* check where stats */
	vortex_thread_pool_stats (ctx, &started_threads, &waiting_threads, &pending_tasks);
	if (waiting_threads != 1) {
		printf ("ERROR (6): expected to find waiting threads 1 but found %d\n", waiting_threads);
		return axl_false;
	}

	/* now synchronize with threads */
	printf ("Test 00-a: sending stop signals..\n");
	vortex_async_queue_push (queue, INT_TO_PTR (1));
	vortex_async_queue_push (queue, INT_TO_PTR (1));
	vortex_async_queue_push (queue, INT_TO_PTR (1));
	vortex_async_queue_push (queue, INT_TO_PTR (1));
	vortex_async_queue_push (queue, INT_TO_PTR (1));

	temp = vortex_async_queue_new ();

	/* wait until there are 5 waitiers */
	iterator = 0;
	while (iterator < 10) {
		if (vortex_async_queue_waiters (queue) == 0)
			break;
		printf ("Test 00-a waiting, still not enough waiters: %d\n", 
			vortex_async_queue_waiters (queue));
		/* get stats from the master ctx */
		vortex_thread_pool_stats (ctx, &started_threads, &waiting_threads, &pending_tasks);
		printf ("Test 00-a:   values started_threads=%d, waiting_threads=%d, pending_tasks=%d\n",
			started_threads, waiting_threads, pending_tasks);
		vortex_async_queue_timedpop (temp, 1000);
		iterator++;
	}

	vortex_thread_pool_stats (ctx, &started_threads, &waiting_threads, &pending_tasks);
	printf ("Test 00-a: all threads stopped started_threads=%d, waiting_threads=%d, pending_tasks=%d\n",
		started_threads, waiting_threads, pending_tasks);

	/* call to stop 5 threads check results */
	vortex_thread_pool_remove (ctx, 4);
	iterator = 0;
	while (iterator < 10) {
		/* get started threads */
		vortex_thread_pool_stats (ctx, &started_threads, &waiting_threads, &pending_tasks);

		if (started_threads == 2) {
			printf ("Test 00-a: stoped 2 threads from the pool.., current status is..\n");
			printf ("Test 00-a: all threads stopped started_threads=%d, waiting_threads=%d, pending_tasks=%d\n",
				started_threads, waiting_threads, pending_tasks);
			break;
		} /* end if */
		
		/* wait for some time */
		vortex_async_queue_timedpop (temp, 10000);
		iterator++;
	}

	/* get started threads */
	vortex_thread_pool_stats (ctx, &started_threads, &waiting_threads, &pending_tasks);

	if (started_threads != 2) {
		printf ("ERROR: expected to find only 2 running threads on the pool, but found %d..\n",
			started_threads);
		return axl_false;
	} /* end if */

	/* now create 50 threads and send some tasks */
	vortex_thread_pool_add (ctx, 50);

	printf ("Test 00-a: added 50 threads to the pool..\n");

	/* now tasks some content */
	iterator = 0;
	while (iterator < 29) {
		vortex_thread_pool_new_task (ctx, (VortexThreadFunc) test_00a_new_task, &value_to_update);
		iterator++;
	} /* end if */

	/* now request to stop 17 threads */
	vortex_thread_pool_remove (ctx, 17);

	/* now tasks some content */
	iterator = 0;
	while (iterator < 29) {
		vortex_thread_pool_new_task (ctx, (VortexThreadFunc) test_00a_new_task, &value_to_update);

		vortex_thread_pool_remove (ctx, 1);
		iterator++;
	} /* end if */

	iterator = 0;
	while (iterator < 900) {
		vortex_thread_pool_new_task (ctx, (VortexThreadFunc) test_00a_new_task, &value_to_update);
		iterator++;
	} /* end if */

	iterator = 0;
	while (iterator < 10) {
		/* get started threads */
		vortex_thread_pool_stats (ctx, &started_threads, &waiting_threads, &pending_tasks);

		if (started_threads == 6) 
			break;

		/* wait for some time */
		vortex_async_queue_timedpop (temp, 10000);
		iterator++;
	}

	vortex_thread_pool_stats (ctx, &started_threads, &waiting_threads, &pending_tasks);
	printf ("Test 00-a: 29 threads stopped started_threads=%d, waiting_threads=%d, pending_tasks=%d\n",
		started_threads, waiting_threads, pending_tasks);

	/* get started threads */
	vortex_thread_pool_stats (ctx, &started_threads, &waiting_threads, &pending_tasks);
	if (started_threads != 6) {
		printf ("ERROR (2): expected to find 26 thread running but found: %d..\n", 
			started_threads);
		return axl_false;
	} /* end if */

	/* remove more threads */
	vortex_thread_pool_remove (ctx, 6);

	iterator = 0;
	while (iterator < 10) {
		/* get started threads */
		vortex_thread_pool_stats (ctx, &started_threads, &waiting_threads, &pending_tasks);

		if (started_threads == 1) 
			break;

		/* wait for some time */
		vortex_async_queue_timedpop (temp, 10000);
		iterator++;
	}

	vortex_thread_pool_stats (ctx, &started_threads, &waiting_threads, &pending_tasks);
	printf ("Test 00-a: after stopping all threads started_threads=%d, waiting_threads=%d, pending_tasks=%d\n",
		started_threads, waiting_threads, pending_tasks);

	/* check that there is at least one thread running */
	if (started_threads != 1) {
		printf ("ERROR (3): expected to find one thread running but found %d..\n", started_threads);
		return axl_false;
	} /* end if */

	/* lock until no waiter is reading */
	vortex_async_queue_unref (queue);
	vortex_async_queue_unref (temp);

	/* finish context */
	vortex_exit_ctx (ctx, axl_true);

	return axl_true;
}

axl_bool test_00b (void) {
	int                running_threads;
	int                waiting_threads;
	VortexCtx        * test_ctx;
	int                tries = 10;
	VortexAsyncQueue * queue;

	test_ctx = vortex_ctx_new ();

	/* set thread pool */
	printf ("Test 00-b: Calling to set thread pool num..\n");
	vortex_thread_pool_set_num (10);
	printf ("Test 00-b: configured thread pool initial num..\n");

	/* call to init vortex */
	if (! vortex_init_ctx (test_ctx)) {
		return axl_false;
	}

	printf ("Test 00-b: vortex ctx started..doing test..\n");

	queue = vortex_async_queue_new ();
	while (tries > 0) {
		/* get running threads */
		vortex_thread_pool_stats (test_ctx, &running_threads, &waiting_threads, NULL);

		if (running_threads == 10 && waiting_threads == 10)
			break;
		vortex_async_queue_timedpop (queue, 1000);
		tries--;
	} /* end if */
	vortex_async_queue_unref (queue);
	
	if (running_threads != 10) {
		printf ("ERROR (1): expected to find running threads equal 10 but found %d\n", 
			running_threads);
		return axl_false;
	}

	if (waiting_threads != 10) {
		printf ("ERROR (2): expected to find waiting threads equal 10 but found %d\n", 
			waiting_threads);
		return axl_false;
	}
	/* terminate context */
	vortex_exit_ctx (test_ctx, axl_true);
	return axl_true;
}

axl_bool test_00c_event (VortexCtx * ctx, axlPointer data, axlPointer data2)
{
	int              * count = (int *) data;
	VortexAsyncQueue * queue = data2;
	struct timeval     now;

	/* get timeofday value */
	gettimeofday (&now, NULL);
	(*count)++;
	printf ("Test 00-c: Time now (%p): %ld.%ld, count=%d\n", data, now.tv_sec, now.tv_usec, *count); 
	if ((*count) == 10)  {
		/* notify waiting thread we have finished */
		vortex_async_queue_push (queue, INT_TO_PTR (1723123));
		/* notify to remove event */
		return axl_true;
	}
		
	/* notify to update keep event */
	return axl_false;
}

axl_bool test_00c_event_2 (VortexCtx * ctx, axlPointer data, axlPointer data2)
{
	int              * count = (int *) data;
	VortexAsyncQueue * queue = data2;
	struct timeval     now;

	/* get timeofday value */
	gettimeofday (&now, NULL);
	(*count)++;
	printf ("Test 00-c: Time now (%p): %ld.%ld, count=%d\n", data, now.tv_sec, now.tv_usec, *count); 
	if ((*count) == 3)  {
		/* notify waiting thread we have finished */
		vortex_async_queue_push (queue, INT_TO_PTR (5231412));
		/* notify to remove event */
		return axl_true;
	}
		
	/* notify to update keep event */
	return axl_false;
}

axl_bool test_00c (void) {
	VortexCtx        * test_ctx;
	VortexAsyncQueue * queue;
	int                count, count2, count3;
	int                iterator;

	/* create a test context */
	test_ctx = vortex_ctx_new ();

	/* call to init vortex */
	if (! vortex_init_ctx (test_ctx)) {
		return axl_false;
	}

	printf ("Test 00-c: vortex ctx started..doing test..\n");

	queue = vortex_async_queue_new ();
	
	/* queue one event */
	count = 0;
	vortex_thread_pool_new_event (test_ctx, 30000, 
				      test_00c_event, &count, queue);
	/* wait for the event to finish */
	if (PTR_TO_INT (vortex_async_queue_pop (queue)) != 1723123) {
		printf ("Test 00-c: Value returned by queue pop, differs from value expected..\n");
		return axl_false;
	} /* end if */

	/* now check ount */
	if (count != 10) {
		printf ("Test 00-c: Value expected from event execute differs from exepcted: %d != 10\n", count);
		return axl_false;
	} /* end if */

	/* wait a bit and check that the event is finished */
	vortex_async_queue_timedpop (queue, 30000);
	
	/* get events installed */
	vortex_thread_pool_event_stats (test_ctx, &count);
	if (count != 0) {
		printf ("Test 00-c: expected to find 0 events installed, but found: %d\n", count);
		return axl_false;
	}

	/* now queue tree events */
	printf ("Test 00-c: activating tree events at the same time...\n");
	count = 0;
	vortex_thread_pool_new_event (test_ctx, 30000, test_00c_event, &count, queue);
	count2 = 0;
	vortex_thread_pool_new_event (test_ctx, 30000, test_00c_event, &count2, queue);
	count3 = 0;
	vortex_thread_pool_new_event (test_ctx, 30000, test_00c_event, &count3, queue);

	iterator = 0;
	while (iterator < 3) {
		/* wait for the event to finish */
		if (PTR_TO_INT (vortex_async_queue_pop (queue)) != 1723123) {
			printf ("Test 00-c: Value returned by queue pop, differs from value expected..\n");
			return axl_false;
		} /* end if */

		/* next iterator */
		iterator++;
	} /* end while */

	/* wait a bit and check that the event is finished */
	vortex_async_queue_timedpop (queue, 30000);
	
	/* get events installed */
	vortex_thread_pool_event_stats (test_ctx, &count);
	if (count != 0) {
		printf ("Test 00-c: expected to find 0 events installed, but found: %d\n", count);
		return axl_false;
	}

	/* check timeout for more than seconds period */
	printf ("Test 00-c: activating tree events at the same time (activation beyond 1 second, wait 6 seconds)\n");
	count = 0;
	vortex_thread_pool_new_event (test_ctx, 2000000, test_00c_event_2, &count, queue);
	count2 = 0;
	vortex_thread_pool_new_event (test_ctx, 300000,   test_00c_event_2, &count2, queue);
	count3 = 0;
	vortex_thread_pool_new_event (test_ctx, 1600000, test_00c_event_2, &count3, queue);

	iterator = 0;
	while (iterator < 3) {
		/* wait for the event to finish */
		if (PTR_TO_INT (vortex_async_queue_pop (queue)) != 5231412) {
			printf ("Test 00-c: Value returned by queue pop, differs from value expected..\n");
			return axl_false;
		} /* end if */

		/* next iterator */
		iterator++;
	} /* end while */

	/* wait a bit and check that the event is finished */
	vortex_async_queue_timedpop (queue, 30000);
	
	/* get events installed */
	vortex_thread_pool_event_stats (test_ctx, &count);
	if (count != 0) {
		printf ("Test 00-c: expected to find 0 events installed, but found: %d\n", count);
		return axl_false;
	}
	
	/* terminate context */
	vortex_async_queue_unref (queue);
	vortex_exit_ctx (test_ctx, axl_true);
	return axl_true;
}

axl_bool  test_01 (void) {
	VortexConnection  * connection;
	VortexChannel     * channel;
	VortexAsyncQueue  * queue;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		printf ("Test 01: failed to create connection..");
		vortex_connection_close (connection);
		return axl_false;
	}

	/* create the queue */
	queue   = vortex_async_queue_new ();

	/* create a channel */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("Unable to create the channel..");
		return axl_false;
	}

	/* ok, close the channel */
	vortex_channel_close (channel, NULL);

	/* ok, close the connection */
	vortex_connection_close (connection);

	/* free queue */
	vortex_async_queue_unref (queue);

	/* return axl_true */
	return axl_true;
}

axl_bool  test_01a (void) {
	VortexConnection  * connection;
	VortexChannel     * channel;
	VortexAsyncQueue  * queue;
	unsigned char     * content;
	int                 iterator;
	int                 iterator2;
	VortexFrame       * frame;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
	}

	/* create the queue */
	queue   = vortex_async_queue_new ();

	/* create a channel */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI_ZERO,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("Unable to create the channel..");
		return axl_false;
	}

	/* large zero binary frames */
	content = axl_new (unsigned char, 64 * 1024);
	memset (content, 0, 64 * 1024);

	iterator = 0;
	while (iterator < 10) {
		
		/* send the content */
		if (! vortex_channel_send_msg (channel, content, 
					       64 * 1024,
					       NULL)) {
			printf ("Failed to send binary zeroed content..\n");
			return axl_false;
		} /* end if */

		/* update iterator */
		iterator++;
	} /* end while */

	axl_free (content);

	/* now read frame replies */
	iterator = 0;
	while (iterator < 10) {
		
		/* get pending content */
		frame = vortex_channel_get_reply (channel, queue);
		
		if (frame == NULL) {
			printf ("Expected to find binary zeroed frame.. but null reference was found..\n");
			return axl_false;
		} /* end if */
		

		/* check the content */
		if (vortex_frame_get_payload_size (frame) != 1024 * 64) {
			printf ("Expected to find a frame of size %d, but found %d..\n",
				1024 * 64, vortex_frame_get_payload_size (frame));
			return axl_false;
		} /* end if */

		/* get the content and check all of its positions */
		content = (unsigned char *) vortex_frame_get_payload (frame);

		iterator2 = 0;
		while (iterator2 < (1024 * 64)) {
			if (content[iterator2] != 0) {
				printf ("Expected to find zeroed content at %d, but found content[%d] = '%c'..\n",
					iterator2, iterator2, (char) content[iterator2]);
			}

			/* next iterator */
			iterator2++;
		} /* end while */

		/* free frame */
		vortex_frame_unref (frame);

		/* update iterator */
		iterator++;
	} /* end while */
	

	/* ok, close the channel */
	vortex_channel_close (channel, NULL);

	/* ok, close the connection */
	vortex_connection_close (connection);

	/* free queue */
	vortex_async_queue_unref (queue);

	/* return axl_true */
	return axl_true;
}

void test_01b_created (int                channel_num, 
		       VortexChannel    * channel, 
		       VortexConnection * conn,
		       axlPointer         user_data)
{
	VortexAsyncQueue * queue = user_data;

	/* check piggy back */
	if (vortex_channel_have_piggyback (channel)) {
		printf ("Found piggy back on the channel, this is not expected..\n");

		/* push a notification */
		vortex_async_queue_push (queue, INT_TO_PTR(0));
		return;
	}
	
	/* configure a piggy back */
	vortex_channel_set_piggyback (channel, ">>>dummy content<<<");

	/* close the channel here */
	if (! vortex_channel_close (channel, NULL)) {
		printf ("Failed to close the channel...\n");

		/* push a notification */
		vortex_async_queue_push (queue, INT_TO_PTR(0));

		return;
	}

	/* push a notification */
	vortex_async_queue_push (queue, INT_TO_PTR(3));

	return;
}

axl_bool  test_01b (void) {
	VortexConnection  * connection;
	VortexAsyncQueue  * queue;
	int                 iterator;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
	}

	/* create the queue */
	queue   = vortex_async_queue_new ();

	iterator = 0;
	while (iterator < 10) {

		/* create a channel */
		vortex_channel_new (connection, 0,
				    REGRESSION_URI,
				    /* no close handling */
				    NULL, NULL,
				    /* no frame received */
				    NULL, NULL,
				    /* no async channel creation */
				    test_01b_created, queue);
		
		/* do not check NULL reference for channel here since the
		 * threaded mode was activated causing to always return
		 * NULL. */
		
		/* unref the queue */
		if (PTR_TO_INT (vortex_async_queue_pop (queue)) != 3) {
			printf ("Failed to close the channel..\n");
			return axl_false;
		} /* end if */

		iterator++;
	} /* end while */

	/* close the connection */
	vortex_connection_close (connection);

	vortex_async_queue_unref (queue);

	return axl_true;

} /* end test_01b */

axl_bool  test_01c (void) {
	VortexConnection  * connection;
	VortexAsyncQueue  * queue;
	VortexChannel     * channel;
	VortexFrame       * frame;
	int                 iterator;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
	}

	/* create the queue */
	queue   = vortex_async_queue_new ();

	iterator = 0;
	while (iterator < 1) {

		/* create a channel */
		channel = vortex_channel_new (connection, 0,
					      REGRESSION_URI_FAST_SEND,
					      /* no close handling */
					      NULL, NULL,
					      /* no frame received */
					      vortex_channel_queue_reply, queue,
					      /* no async channel creation */
					      NULL, NULL);

		/* check connection here */
		if (! vortex_connection_is_ok (connection, axl_false)) {
			printf ("Test 01-c: (1) Failed to check connection, it should be running..\n");
			return axl_false;
		} /* end if */

		if (channel == NULL) {
			printf ("Unable to create channel for fast send..\n");
			return axl_false;
		} /* end if */

		/* receive both messages */
		frame = vortex_channel_get_reply (channel, queue);
		if (! axl_cmp (vortex_frame_get_payload (frame), "message 1")) {
			printf ("Test 01-c: (1.1) Expected to find content %s but found %s..\n",
				"message 1", (char*)vortex_frame_get_payload (frame));
		} /* end if */

		/* send reply */
		if (! vortex_channel_send_rpy (channel, "", 0, vortex_frame_get_msgno (frame))) {
			printf ("Test 01-c: (1.1.1) Expected to be able to reply to message received..\n");
			return axl_false;
		} /* end if */

		vortex_frame_unref (frame);

		/* get next reply */
		frame = vortex_channel_get_reply (channel, queue);
		if (! axl_cmp (vortex_frame_get_payload (frame), "message 2")) {
			printf ("Test 01-c: (1.2) Expected to find content %s but found %s..\n",
				"message 2", (char*)vortex_frame_get_payload (frame));
		} /* end if */

		if (! vortex_channel_send_rpy (channel, "", 0, vortex_frame_get_msgno (frame))) {
			printf ("Test 01-c: (1.1.1) Expected to be able to reply to message received..\n");
			return axl_false;
		} /* end if */

		vortex_frame_unref (frame);
		
		/* close channel */
		if (! vortex_channel_close (channel, NULL)) {
			printf ("Unable to close the channel..\n");
			return axl_false;
		}

		/* check connection here */
		if (! vortex_connection_is_ok (connection, axl_false)) {
			printf ("Test 01-c: (2) Failed to check connection, it should be running..\n");
			return axl_false;
		} /* end if */
		
		iterator++;
	} /* end while */

	/* check connection here */
	if (! vortex_connection_is_ok (connection, axl_false)) {
		printf ("Test 01-c: (3) Failed to check connection, it should be running..\n");
		return axl_false;
	} /* end if */

	/* close the connection */
	vortex_connection_close (connection);

	vortex_async_queue_unref (queue);
	
	return axl_true;

} /* end test_01c */

axl_bool  test_01d_01 (void)
{
	char             * mime_message;
	int                mime_message_size;
	char             * mime_body;
	VortexFrame      * frame;
	VortexMimeHeader * header;

	printf ("Test 01-d: checking MIME support for wrong UNIX MIME (no CR-LF but LF)..\n");
	mime_message = vortex_regression_common_read_file ("mime.example.1.txt", NULL);
	if (mime_message == NULL) {
		printf ("ERROR: failed to load mime message: %s", "mime.example.1.txt");
		return axl_false;
	} /* end if */

	/* create an artificial frame */
	mime_message_size = strlen (mime_message);
	frame             = vortex_frame_create (ctx, VORTEX_FRAME_TYPE_MSG,
						 0, 0, axl_false, 0, mime_message_size, 0, mime_message);
	if (frame == NULL) {
		printf ("ERROR: expected to create a frame but NULL reference was found..\n");
		return axl_false;
	}

	/* activate mime support on the frame */
	if (! vortex_frame_mime_process (frame)) {
		printf ("ERROR: expected to find proper MIME process, but a failure was found..\n");
		return axl_false;
	} /* end if */

	/* check mime header */
	if (vortex_frame_get_mime_header_size (frame) != 1418) {
		printf ("ERROR: expected to find MIME Headers %d but found %d..\n",
			vortex_frame_get_mime_header_size (frame), 1418);
		return axl_false;
	}

	/* check mime body */
	if (vortex_frame_get_payload_size (frame) != 4943) {
		printf ("ERROR: expected to find MIME BODY %d but found %d (%s): '%s..\n",
			vortex_frame_get_payload_size (frame), 4943, "mime.example.1.txt", 
			(char*) vortex_frame_get_payload (frame));
		return axl_false;
	}

	/* check content */
	mime_body = vortex_regression_common_read_file ("mime.example.body.1.txt", NULL);
	if (mime_body == NULL) {
		printf ("ERROR: failed to load mime message: %s", "mime.example.body.1.txt");
		return axl_false;
	} /* end if */

	/* check content */
	if (! axl_cmp (mime_body, vortex_frame_get_payload (frame))) {
		printf ("ERROR: expected to find same mime body (%s) content (size %d != %d)..\n", "mime.example.body.1.txt",
			(int) strlen (mime_body), (int) strlen (vortex_frame_get_payload (frame)));
		return axl_false;
	}

	/* check headers */
	if (! axl_cmp ("8bit", vortex_frame_get_transfer_encoding (frame))) {
		printf ("ERROR: expected Content-Transfer-Encoding with 8bit, but found %s",
			vortex_frame_get_transfer_encoding (frame));
		return axl_false;
	}

	if (! axl_cmp ("text/plain; charset=\"ISO-8859-1\"", vortex_frame_get_content_type (frame))) {
		printf ("ERROR: expected Content-Type with %s, but found %s",
			"text/plain; charset=\"ISO-8859-1\"", vortex_frame_get_content_type (frame));
		return axl_false;
	}

	/* check all headers */
	printf ("Test 01-d: checking Return-Path..\n");
	header = vortex_frame_get_mime_header (frame, "Return-Path");
	if (! axl_cmp ("<cyrus@dolphin>", vortex_frame_mime_header_content (header))) {
		printf ("ERROR: expected to find %s, but found %s..\n",
			"<cyrus@dolphin>", vortex_frame_mime_header_content (header));
		return axl_false;
	}
	
	/* check header count */
	if (vortex_frame_mime_header_count (header) != 2) {
		printf ("ERROR: expected to find HEADER count equal to 1, but found %d..\n",
			vortex_frame_mime_header_count (header));
		return axl_false;
	}

	/* get next */
	header = vortex_frame_mime_header_next (header);
	if (header == NULL) {
		printf ("ERROR: expected to find second Return-Path value but it wasn't found..\n");
		return axl_false;
	}

	if (! axl_cmp ("<bounce-zdnetuk-1909492@newsletters.zdnetuk.cneteu.net>", vortex_frame_mime_header_content (header))) {
		printf ("ERROR: expected to find %s, but found %s..\n",
			"<bounce-zdnetuk-1909492@newsletters.zdnetuk.cneteu.net>", vortex_frame_mime_header_content (header));
		return axl_false;
	}

	printf ("Test 01-d: checking Received..\n");
	header = vortex_frame_get_mime_header (frame, "received");
	if (! axl_cmp ("from dolphin ([unix socket]) by dolphin (Cyrus\n	v2.1.18-IPv6-Debian-2.1.18-5.1) with LMTP; Thu, 15 May 2008 15:23:27 +0200", 
		       vortex_frame_mime_header_content (header))) {
		printf ("ERROR: expected Received header to have value %s but found %s..\n",
			"from dolphin ([unix socket]) by dolphin (Cyrus\n	v2.1.18-IPv6-Debian-2.1.18-5.1) with LMTP; Thu, 15 May 2008 15:23:27 +0200", 
			vortex_frame_mime_header_content (header));
		return axl_false;
	} /* end if */

	/* check header count */
	if (vortex_frame_mime_header_count (header) != 3) {
		printf ("ERROR: expected to find HEADER count equal to 1, but found %d..\n",
			vortex_frame_mime_header_count (header));
		return axl_false;
	}

	/* get next */
	header = vortex_frame_mime_header_next (header);
	if (header == NULL) {
		printf ("ERROR: expected to find second Return-Path value but it wasn't found..\n");
		return axl_false;
	}

	if (! axl_cmp ("from mail by dolphin.aspl.es with spam-scanned (ASPL Mail Server\n	XP#1) id 1JwdOA-00050S-00 for <acinom@aspl.es>; Thu, 15 May 2008 15:20:58\n	+0200", vortex_frame_mime_header_content (header))) {
		printf ("ERROR: expected Received header to have value %s but found %s..\n",
			"from mail by dolphin.aspl.es with spam-scanned (ASPL Mail Server\n	XP#1) id 1JwdOA-00050S-00 for <acinom@aspl.es>; Thu, 15 May 2008 15:20:58\n	+0200", vortex_frame_mime_header_content (header));
		return axl_false;
	} /* end if */

	/* get next */
	header = vortex_frame_mime_header_next (header);
	if (header == NULL) {
		printf ("ERROR: expected to find second Return-Path value but it wasn't found..\n");
		return axl_false;
	}

	if (! axl_cmp ("from newsletters.cneteu.net ([62.108.136.190]\n	helo=newsletters.zdnetuk.cneteu.net) by dolphin.aspl.es with smtp (ASPL\n	Mail Server XP#1) id 1Jwd76-00047G-00 for <francis@aspl.es>; Thu, 15 May\n	2008 15:03:16 +0200", vortex_frame_mime_header_content (header))) {
		printf ("ERROR: expected Received header to have value %s but found %s..\n",
			"from newsletters.cneteu.net ([62.108.136.190]\n	helo=newsletters.zdnetuk.cneteu.net) by dolphin.aspl.es with smtp (ASPL\n	Mail Server XP#1) id 1Jwd76-00047G-00 for <francis@aspl.es>; Thu, 15 May\n	2008 15:03:16 +0200", vortex_frame_mime_header_content (header));
		return axl_false;
	} /* end if */

	/* get next */
	header = vortex_frame_mime_header_next (header);
	if (header != NULL) {
		printf ("ERROR: expected to NOT find any Received value but it was found..\n");
		return axl_false;
	}

	/* check mime version */
	if (! axl_cmp (VORTEX_FRAME_GET_MIME_HEADER (frame, "MIME-version"), "1.0")) {
		printf ("ERROR: Expected to find MIME header version 1.0 but found %s\n",
			VORTEX_FRAME_GET_MIME_HEADER (frame, "MIME-version"));
		return axl_false;
	}

	/* check mime version */
	if (! axl_cmp (VORTEX_FRAME_GET_MIME_HEADER (frame, "List-Unsubscribe"), "<mailto:leave-zdnetuk-1909492M@newsletters.zdnetuk.cneteu.net>")) {
		printf ("ERROR: Expected to find List-Unsusbcribe '%s' != '%s'\n",
			"<mailto:leave-zdnetuk-1909492M@newsletters.zdnetuk.cneteu.net>",
			VORTEX_FRAME_GET_MIME_HEADER (frame, "List-Unsubscribe"));
		return axl_false;
	}

	vortex_frame_unref (frame);
	axl_free (mime_message);
	axl_free (mime_body);

	return axl_true;
}

axl_bool  test_01d_02 (void)
{
	char        * mime_message;
	int           mime_message_size;
	char        * mime_body;
	VortexFrame * frame;

	printf ("Test 01-d: checking MIME support (right CR-LF terminated)..\n");
	mime_message = vortex_regression_common_read_file ("mime.example.2.txt", NULL);
	if (mime_message == NULL) {
		printf ("ERROR: failed to load mime message: %s", "mime.example.2.txt");
		return axl_false;
	} /* end if */

	/* create an artificial frame */
	mime_message_size = strlen (mime_message);
	frame             = vortex_frame_create (ctx, VORTEX_FRAME_TYPE_MSG,
						 0, 0, axl_false, 0, mime_message_size, 0, mime_message);
	if (frame == NULL) {
		printf ("ERROR: expected to create a frame but NULL reference was found..\n");
		return axl_false;
	}

	/* activate mime support on the frame */
	if (! vortex_frame_mime_process (frame)) {
		printf ("ERROR: expected to find proper MIME process, but a failure was found..\n");
		return axl_false;
	} /* end if */

	/* check mime header */
	if (vortex_frame_get_mime_header_size (frame) != 1123) {
		printf ("ERROR: expected to find MIME Headers %d but found %d..\n",
			vortex_frame_get_mime_header_size (frame), 1123);
		return axl_false;
	}

	/* check mime body */
	if (vortex_frame_get_payload_size (frame) != 4) {
		printf ("ERROR: expected to find MIME BODY %d but found %d..\n",
			vortex_frame_get_payload_size (frame), 4);
		return axl_false;
	}

	/* check content */
	mime_body = vortex_regression_common_read_file ("mime.example.body.2.txt", NULL);
	if (mime_body == NULL) {
		printf ("ERROR: failed to load mime message: %s", "mime.example.body.2.txt");
		return axl_false;
	} /* end if */

	/* check content */
	if (! axl_cmp (mime_body, vortex_frame_get_payload (frame))) {
		printf ("ERROR: expected to find same mime body content..\n");
		return axl_false;
	}

	vortex_frame_unref (frame);
	axl_free (mime_message);
	axl_free (mime_body);

	return axl_true;
}

axl_bool  test_01d_03 (void)
{
	char        * mime_message;
	char        * mime_body;
	int           mime_message_size;
	VortexFrame * frame;

	printf ("Test 01-d: checking MIME support (no headers)..\n");
	mime_message = vortex_regression_common_read_file ("mime.example.3.txt", NULL);
	if (mime_message == NULL) {
		printf ("ERROR: failed to load mime message: %s", "mime.example.3.txt");
		return axl_false;
	} /* end if */

	/* create an artificial frame */
	mime_message_size = strlen (mime_message);
	frame             = vortex_frame_create (ctx, VORTEX_FRAME_TYPE_MSG,
						 0, 0, axl_false, 0, mime_message_size, 0, mime_message);
	if (frame == NULL) {
		printf ("ERROR: expected to create a frame but NULL reference was found..\n");
		return axl_false;
	}

	/* activate mime support on the frame */
	if (! vortex_frame_mime_process (frame)) {
		printf ("ERROR: expected to find proper MIME process, but a failure was found..\n");
		return axl_false;
	} /* end if */

	/* check mime header */
	if (vortex_frame_get_mime_header_size (frame) != 2) {
		printf ("ERROR: expected to find MIME Headers %d but found %d..\n",
			vortex_frame_get_mime_header_size (frame), 2);
		return axl_false;
	}

	/* check mime body */
	if (vortex_frame_get_payload_size (frame) != 2) {
		printf ("ERROR: expected to find MIME BODY %d but found %d..\n",
			vortex_frame_get_payload_size (frame), 2);
		return axl_false;
	}

	/* check content */
	mime_body = (char*) vortex_frame_get_payload (frame);
	if (mime_body[0] != '\x0D' || mime_body[1] != '\x0A') {
		printf ("ERROR: expected to find same mime body content..\n");
		return axl_false;
	}

	vortex_frame_unref (frame);
	axl_free (mime_message);

	return axl_true;
}

axl_bool  test_01d_04 (void)
{
	char        * mime_message;
	int           mime_message_size;
	VortexFrame * frame;

	printf ("Test 01-d: checking MIME support (no content, CR-LF)..\n");
	mime_message = axl_new (char, 2);
	if (mime_message == NULL) {
		printf ("ERROR: failed to allocate mime message memory..\n");
		return axl_false;
	} /* end if */

	/* configure the message */
	mime_message[0] = '\x0D';
	mime_message[1] = '\x0A';

	/* create an artificial frame */
	mime_message_size = 2;
	frame             = vortex_frame_create (ctx, VORTEX_FRAME_TYPE_MSG,
						 0, 0, axl_false, 0, mime_message_size, 0, mime_message);
	if (frame == NULL) {
		printf ("ERROR: expected to create a frame but NULL reference was found..\n");
		return axl_false;
	}

	/* activate mime support on the frame */
	if (! vortex_frame_mime_process (frame)) {
		printf ("ERROR: expected to find proper MIME process, but a failure was found..\n");
		return axl_false;
	} /* end if */

	/* check mime header */
	if (vortex_frame_get_mime_header_size (frame) != 2) {
		printf ("ERROR: expected to find MIME Headers %d but found %d..\n",
			vortex_frame_get_mime_header_size (frame), 2);
		return axl_false;
	}

	/* check mime body */
	if (vortex_frame_get_payload_size (frame) != 0) {
		printf ("ERROR: expected to find MIME BODY %d but found %d..\n",
			vortex_frame_get_payload_size (frame), 0);
		return axl_false;
	}

	vortex_frame_unref (frame);
	axl_free (mime_message);

	return axl_true;
}

axl_bool  test_01d_05 (void)
{
	char        * mime_message;
	int           mime_message_size;
	VortexFrame * frame;

	printf ("Test 01-d: checking MIME support (no content, LF, unix)..\n");
	mime_message = axl_new (char, 1);
	if (mime_message == NULL) {
		printf ("ERROR: failed to allocate mime message memory..\n");
		return axl_false;
	} /* end if */

	/* configure the message */
	mime_message[0] = '\x0A';

	/* create an artificial frame */
	mime_message_size = 1;
	frame             = vortex_frame_create (ctx, VORTEX_FRAME_TYPE_MSG,
						 0, 0, axl_false, 0, mime_message_size, 0, mime_message);
	if (frame == NULL) {
		printf ("ERROR: expected to create a frame but NULL reference was found..\n");
		return axl_false;
	}

	/* activate mime support on the frame */
	if (! vortex_frame_mime_process (frame)) {
		printf ("ERROR: expected to find proper MIME process, but a failure was found..\n");
		return axl_false;
	} /* end if */

	/* check mime header */
	if (vortex_frame_get_mime_header_size (frame) != 1) {
		printf ("ERROR: expected to find MIME Headers %d but found %d..\n",
			vortex_frame_get_mime_header_size (frame), 2);
		return axl_false;
	}

	/* check mime body */
	if (vortex_frame_get_payload_size (frame) != 0) {
		printf ("ERROR: expected to find MIME BODY %d but found %d..\n",
			vortex_frame_get_payload_size (frame), 0);
		return axl_false;
	}

	vortex_frame_unref (frame);
	axl_free (mime_message);

	return axl_true;
}

axl_bool  test_01d_06 (void)
{
	char        * mime_message;
	int           mime_message_size;
	VortexFrame * frame;

	printf ("Test 01-d: checking MIME support (no header, but content CR-LF)..\n");
	mime_message = axl_new (char, 375);
	if (mime_message == NULL) {
		printf ("ERROR: failed to allocate mime message memory..\n");
		return axl_false;
	} /* end if */

	/* configure the message */
	mime_message[0] = '\x0D';
	mime_message[1] = '\x0A';

	/* create an artificial frame */
	mime_message_size = 375;
	frame             = vortex_frame_create (ctx, VORTEX_FRAME_TYPE_MSG,
						 0, 0, axl_false, 0, mime_message_size, 0, mime_message);
	if (frame == NULL) {
		printf ("ERROR: expected to create a frame but NULL reference was found..\n");
		return axl_false;
	}

	/* activate mime support on the frame */
	if (! vortex_frame_mime_process (frame)) {
		printf ("ERROR: expected to find proper MIME process, but a failure was found..\n");
		return axl_false;
	} /* end if */

	/* check mime header */
	if (vortex_frame_get_mime_header_size (frame) != 2) {
		printf ("ERROR: expected to find MIME Headers %d but found %d..\n",
			vortex_frame_get_mime_header_size (frame), 2);
		return axl_false;
	}

	/* check mime body */
	if (vortex_frame_get_payload_size (frame) != 373) {
		printf ("ERROR: expected to find MIME BODY %d but found %d..\n",
			vortex_frame_get_payload_size (frame), 373);
		return axl_false;
	}

	/* check default headers: Content-Type */
	if (! axl_cmp (vortex_frame_get_content_type (frame), "application/octet-stream")) {
		printf ("ERROR: (1) expected to find MIME header \"Content-Type\" equal to: %s, but found %s\n",
			"application/octet-stream", vortex_frame_get_content_type (frame));
		return axl_false;
	}

	/* check defaul header: Content-Transfer-Encoding */
	if (! axl_cmp (vortex_frame_get_transfer_encoding (frame), "binary")) {
		printf ("ERROR: (2) expected to find MIME header \"Content-Transfer-Encoding\" equal to: %s, but found %s\n",
			"binary", vortex_frame_get_transfer_encoding (frame));
		return axl_false;
	}

	vortex_frame_unref (frame);
	axl_free (mime_message);

	return axl_true;
}

axl_bool  test_01d_07 (void)
{
	char        * mime_message;
	int           mime_message_size;
	VortexFrame * frame;

	printf ("Test 01-d: checking MIME support (no header, not content (CR-LF)..\n");
	mime_message = axl_new (char, 2);
	if (mime_message == NULL) {
		printf ("ERROR: failed to allocate mime message memory..\n");
		return axl_false;
	} /* end if */

	/* configure the message */
	mime_message[0] = '\x0D';
	mime_message[1] = '\x0A';

	/* create an artificial frame */
	mime_message_size = 2;
	frame             = vortex_frame_create (ctx, VORTEX_FRAME_TYPE_MSG,
						 0, 0, axl_false, 0, mime_message_size, 0, mime_message);
	if (frame == NULL) {
		printf ("ERROR: expected to create a frame but NULL reference was found..\n");
		return axl_false;
	}

	/* activate mime support on the frame */
	if (! vortex_frame_mime_process (frame)) {
		printf ("ERROR: expected to find proper MIME process, but a failure was found..\n");
		return axl_false;
	} /* end if */

	/* check mime header */
	if (vortex_frame_get_mime_header_size (frame) != 2) {
		printf ("ERROR: expected to find MIME Headers %d but found %d..\n",
			vortex_frame_get_mime_header_size (frame), 2);
		return axl_false;
	}

	/* check mime body */
	if (vortex_frame_get_payload_size (frame) != 0) {
		printf ("ERROR: expected to find MIME BODY %d but found %d..\n",
			vortex_frame_get_payload_size (frame), 0);
		return axl_false;
	}

	/* check default headers: Content-Type */
	if (! axl_cmp (vortex_frame_get_content_type (frame), "application/octet-stream")) {
		printf ("ERROR: (1) expected to find MIME header \"Content-Type\" equal to: %s, but found %s\n",
			"application/octet-stream", vortex_frame_get_content_type (frame));
		return axl_false;
	}

	/* check defaul header: Content-Transfer-Encoding */
	if (! axl_cmp (vortex_frame_get_transfer_encoding (frame), "binary")) {
		printf ("ERROR: (2) expected to find MIME header \"Content-Transfer-Encoding\" equal to: %s, but found %s\n",
			"binary", vortex_frame_get_transfer_encoding (frame));
		return axl_false;
	}

	vortex_frame_unref (frame);
	axl_free (mime_message);

	return axl_true;
}

axl_bool  test_01d (void) {
	VortexConnection  * connection;
	VortexAsyncQueue  * queue;
	VortexChannel     * channel;
	VortexFrame       * frame;
	char              * mime_message;
	char              * mime_body;
	VortexMimeHeader  * mime_header;

	/* check mime support first */
	if (! test_01d_01 ())
		return axl_false;

	if (! test_01d_02 ())
		return axl_false;

	if (! test_01d_03 ())
		return axl_false;

	if (! test_01d_04 ())
		return axl_false; 

	if (! test_01d_05 ())
		return axl_false;

	if (! test_01d_06 ())
		return axl_false;

	if (! test_01d_07 ())
		return axl_false;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
	}

	/* create the queue */
	queue   = vortex_async_queue_new ();

	/* create a channel */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI_MIME,
				      /* no close handling */
				      NULL, NULL,
				      /* no frame received */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);

	if (channel == NULL) {
		printf ("Unable to create channel to test MIME support..\n");
		return axl_false;
	} /* end if */

	printf ("Test 01-d: disabling automatic MIME handling for outgoing messages..\n");
	vortex_channel_set_automatic_mime (channel, 2);

	/* open first test: mime.example.1.txt */
	printf ("Test 01-d: opening MIME message..\n");
	mime_message = vortex_regression_common_read_file ("mime.example.1.txt", NULL);
	if (mime_message == NULL) {
		printf ("ERROR: failed to load mime message: %s", "mime.example.1.txt");
		return axl_false;
	} /* end if */

	/* send mime message */
	if (! vortex_channel_send_msg (channel, mime_message, strlen (mime_message), NULL)) {
		printf ("Unable to send MIME message over channel=%d\n", vortex_channel_get_number (channel));
		return axl_false;
	} /* end if */

	/* get mime reply and check headers */
	printf ("Test 01-d: waiting MIME message reply..\n");
	frame = vortex_channel_get_reply (channel, queue);
	if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY) {
		printf ("Expected to find rpy reply after requesting to change remote step..\n");
		return axl_false;
	} /* end if */

	/* check MIME headers */
	printf ("Test 01-d: Content-Type: %s..\n", vortex_frame_get_content_type (frame));
	if (! axl_cmp (vortex_frame_get_content_type (frame), "text/plain; charset=\"ISO-8859-1\"")) {
		printf ("Expected to find Content-Type: %s but found %s..\n", 
			"text/plain; charset=\"ISO-8859-1\"", vortex_frame_get_content_type (frame));
		return axl_false;
	}

	/* check MIME header: Return-path */
	printf ("Test 01-d: Return-path: %s..\n", VORTEX_FRAME_GET_MIME_HEADER (frame, "Return-path"));
	if (! axl_cmp (VORTEX_FRAME_GET_MIME_HEADER (frame, "Return-path"), 
		       "<cyrus@dolphin>")) {
		printf ("Expected to find Return-path: %s but found %s..\n", 
			"<cyrus@dolphin>",
			VORTEX_FRAME_GET_MIME_HEADER (frame, "Return-path"));
		return axl_false;
	} 

	/* check MIME header: Received */
	printf ("Test 01-d: Received: %s..\n", VORTEX_FRAME_GET_MIME_HEADER (frame, "Received"));
	if (! axl_cmp (VORTEX_FRAME_GET_MIME_HEADER (frame, "received"), 
		       "from dolphin ([unix socket]) by dolphin (Cyrus\n\
	v2.1.18-IPv6-Debian-2.1.18-5.1) with LMTP; Thu, 15 May 2008 15:23:27 +0200")) {
		printf ("Expected to find Return-path: %s but found %s..\n", 
			"from dolphin ([unix socket]) by dolphin (Cyrus\n\
	v2.1.18-IPv6-Debian-2.1.18-5.1) with LMTP; Thu, 15 May 2008 15:23:27 +0200",
			VORTEX_FRAME_GET_MIME_HEADER (frame, "received"));
		return axl_false;
	} 

	/* check MIME header: Message-Id */
	printf ("Test 01-d: Message-Id: %s..\n", VORTEX_FRAME_GET_MIME_HEADER (frame, "message-Id"));
	if (! axl_cmp (VORTEX_FRAME_GET_MIME_HEADER (frame, "Message-Id"), 
		       "<LYRIS-1909492-337994-2008.05.15-13.59.43--francis#aspl.es@newsletters.zdnetuk.cneteu.net>")) {
		printf ("Expected to find Return-path: %s but found %s..\n", 
			"<LYRIS-1909492-337994-2008.05.15-13.59.43--francis#aspl.es@newsletters.zdnetuk.cneteu.net>",
			VORTEX_FRAME_GET_MIME_HEADER (frame, "message-id"));
		return axl_false;
	} 

	printf ("Test 01-d: MIME message received %d size, with body %d..\n",
		vortex_frame_get_content_size (frame), vortex_frame_get_payload_size (frame));

	/* check all content size */
	if (vortex_frame_get_content_size (frame) != 6361) {  
		printf ("ERROR: expected to find full content size of %d but found %d..\n",
			6361, vortex_frame_get_content_size (frame));
		return axl_false;
	} /* end if */

	/* check payload (MIME message body) size */
	if (vortex_frame_get_payload_size (frame) != 4943) {  
		printf ("ERROR: expected to find MIME message body size of %d but found %d..\n",
			4943, vortex_frame_get_payload_size (frame));
		return axl_false;
	} /* end if */

	/* check mime message content */
	if (! axl_cmp (vortex_frame_get_content (frame), mime_message)) {
		printf ("ERROR: expected to find same MIME message as sent..\n");
		return axl_false;
	}

	/* check MIME message body */
	mime_body = vortex_regression_common_read_file ("mime.example.body.1.txt", NULL);
	if (! axl_cmp (vortex_frame_get_payload (frame), mime_body)) {
		printf ("ERROR: expected to find same MIME message as sent..\n");
		return axl_false;
	}

	/* check extended MIME support (several mime headers defined) */
	mime_header = vortex_frame_get_mime_header (frame, "received");
	if (vortex_frame_mime_header_count (mime_header) != 3) {
		printf ("ERROR: expected to find %d times the MIME header %s but found %d times..\n",
			3, "Received", vortex_frame_mime_header_count (mime_header));
		return axl_false;
	}
	
	/* check first ocurrence of "Received" */
	if (! axl_cmp (vortex_frame_mime_header_content (mime_header), 
		       "from dolphin ([unix socket]) by dolphin (Cyrus\n\
	v2.1.18-IPv6-Debian-2.1.18-5.1) with LMTP; Thu, 15 May 2008 15:23:27 +0200")) {
		printf ("ERROR: Expected to find MIME header content %s, but found %s..\n",
			"from dolphin ([unix socket]) by dolphin (Cyrus\n\
	v2.1.18-IPv6-Debian-2.1.18-5.1) with LMTP; Thu, 15 May 2008 15:23:27 +0200",
			vortex_frame_mime_header_content (mime_header));
		return axl_false;
	}

	/* next MIME header */
	mime_header = vortex_frame_mime_header_next (mime_header);
	
	if (! axl_cmp (vortex_frame_mime_header_content (mime_header), 
		       "from mail by dolphin.aspl.es with spam-scanned (ASPL Mail Server\n\
	XP#1) id 1JwdOA-00050S-00 for <acinom@aspl.es>; Thu, 15 May 2008 15:20:58\n\
	+0200")) {
		printf ("ERROR: Expected to find MIME header content %s, but found %s..\n",
			"from mail by dolphin.aspl.es with spam-scanned (ASPL Mail Server\n\
	XP#1) id 1JwdOA-00050S-00 for <acinom@aspl.es>; Thu, 15 May 2008 15:20:58\n\
	+0200",
			vortex_frame_mime_header_content (mime_header));
		return axl_false;
	}

	/* next MIME header */
	mime_header = vortex_frame_mime_header_next (mime_header);
	
	if (! axl_cmp (vortex_frame_mime_header_content (mime_header), 
		       "from newsletters.cneteu.net ([62.108.136.190]\n\
	helo=newsletters.zdnetuk.cneteu.net) by dolphin.aspl.es with smtp (ASPL\n\
	Mail Server XP#1) id 1Jwd76-00047G-00 for <francis@aspl.es>; Thu, 15 May\n\
	2008 15:03:16 +0200")) {
		printf ("ERROR: Expected to find MIME header content %s, but found %s..\n",
			"from newsletters.cneteu.net ([62.108.136.190]\n\
	helo=newsletters.zdnetuk.cneteu.net) by dolphin.aspl.es with smtp (ASPL\n\
	Mail Server XP#1) id 1Jwd76-00047G-00 for <francis@aspl.es>; Thu, 15 May\n\
	2008 15:03:16 +0200",
			vortex_frame_mime_header_content (mime_header));
		return axl_false;
	} /* end if */

	/* get next should return NULL */
	mime_header = vortex_frame_mime_header_next (mime_header);
	if (mime_header != NULL) {
		printf ("ERROR: expected to find NULL reference after call to vortex_frame_mime_header_next..\n");
		return axl_false;
	}
	printf ("Test 01-d: multiple MIME header instances ok..\n");

	/* unref frame */
	vortex_frame_unref (frame);

	/* now send non-MIME content */
	if (! vortex_channel_send_msg (channel, "this is a non-MIME message", 26, NULL)) {
		printf ("Unable to send MIME message over channel=%d\n", vortex_channel_get_number (channel));
		return axl_false;
	} /* end if */

	/* get mime reply and check headers */
	printf ("Test 01-d: waiting non-MIME message reply..\n");
	frame = vortex_channel_get_reply (channel, queue);
	if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY) {
		printf ("Expected to find rpy reply after requesting to change remote step..\n");
		return axl_false;
	} /* end if */
	
	printf ("Test 01-d: message received: %s..\n", 
		(char*)vortex_frame_get_payload (frame));

	/* check content */
	if (vortex_frame_get_payload_size (frame) != vortex_frame_get_content_size (frame)) {
		printf ("Test 01-d: expected same payload and content size for non-MIME message received..\n");
		return axl_false;
	}

	if (vortex_frame_get_payload_size (frame) != 26) {
		printf ("Test 01-d: expected to find %d as frame size but found %d..\n",
			vortex_frame_get_payload_size (frame), 26);
		return axl_false;
	}

	if (! axl_cmp (vortex_frame_get_content (frame), 
		       vortex_frame_get_payload (frame))) {
		printf ("Test 01-d: expected to find the same content on a non-MIME message received..\n");
		return axl_false;
	}

	/* check content type */
	if (vortex_frame_get_content_type (frame) != NULL) {
		printf ("Test 01-d: expected to find NULL content type for a non-MIME message received..\n");
		return axl_false;
	}

	/* check content transfer encoding */
	if (vortex_frame_get_content_type (frame) != NULL) {
		printf ("Test 01-d: expected to find NULL content transfer encoding for a non-MIME message received..\n");
		return axl_false;
	}

	/* check mime headers size */
	if (vortex_frame_get_mime_header_size (frame) != 0) {
		printf ("Test 01-d: expected to find empty MIME headers (size) non-MIME message received..\n");
		return axl_false;
	}

	/* unref frame */
	vortex_frame_unref (frame);

	/* check to unconfigure channel level and access to the
	 * library level */
	vortex_channel_set_automatic_mime (channel, 0);

	/* now configure library level */
	vortex_conf_set (ctx, VORTEX_AUTOMATIC_MIME_HANDLING, 1, NULL);

	/* now send non-MIME content, but will be arrive as MIME content at the remote side */
	printf ("Test 01-d: sending content with empty MIME headers...\n");
	if (! vortex_channel_send_msg (channel, "this is a non-MIME message", 26, NULL)) {
		printf ("Unable to send MIME message over channel=%d\n", vortex_channel_get_number (channel));
		return axl_false;
	} /* end if */

	/* get mime reply and check headers */
	printf ("Test 01-d: waiting non-MIME message reply..\n");
	frame = vortex_channel_get_reply (channel, queue);
	if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY) {
		printf ("Expected to find rpy reply after requesting to change remote step..\n");
		return axl_false;
	} /* end if */

	/* check MIME header size */
	if (vortex_frame_get_content_size (frame) != 28) {
		printf ("Test 01-d: expected to find frame content size %d but found %d..\n",
			28, vortex_frame_get_content_size (frame));
		return axl_false;
	} /* end if */

	/* check mime headers size */
	if (vortex_frame_get_mime_header_size (frame) != 2) {
		printf ("Test 01-d: expected to find frame content size %d but found %d..\n",
			2, vortex_frame_get_mime_header_size (frame));
		return axl_false;
	}
	
	/* check content transfer encoding */
	if (vortex_frame_get_content_type (frame) == NULL) {
		printf ("Test 01-d: expected to NOT find NULL content type for a non-MIME message received..\n");
		return axl_false;
	}

	/* check content transfer encoding */
	if (vortex_frame_get_transfer_encoding (frame) == NULL) {
		printf ("Test 01-d: expected to NOT find NULL content transfer encoding for a non-MIME message received..\n");
		return axl_false;
	}

	/* check content type */
	if (! axl_cmp (vortex_frame_get_content_type (frame),
		       "application/octet-stream")) {
		printf ("Test 01-d: expected to find content type %s but found %s..\n",
			vortex_frame_get_content_type (frame),
			"application/octet-stream");
		return axl_false;
	}

	/* check content type */
	if (! axl_cmp (vortex_frame_get_transfer_encoding (frame),
		       "binary")) {
		printf ("Test 01-d: expected to find content transfer encoding %s but found %s..\n",
			vortex_frame_get_content_type (frame),
			"binary");
		return axl_false;
	}

	/* unref frame */
	vortex_frame_unref (frame);
	
	
	/* check connection here */
	if (! vortex_connection_is_ok (connection, axl_false)) {
		printf ("Test 01-d: (1) Failed to check connection, it should be running..\n");
		return axl_false;
	} /* end if */

	/* free mime */
	axl_free (mime_message);
	axl_free (mime_body);

	/* close the connection */
	vortex_connection_close (connection);

	vortex_async_queue_unref (queue);
	
	return axl_true;

} /* end test_01c */

axl_bool test_01e (void) {

	VortexConnection * listener;
	VortexConnection * listener2;
	int                times = 4;
	

	/* create two listeners using the same port and check the second tries fails */
 test_01e_do_test:
	listener = vortex_listener_new (ctx, "0.0.0.0", "0", NULL, NULL);
	if (! vortex_connection_is_ok (listener, axl_false)) {
		printf ("ERROR: Expected to find proper listener creation, but failure was found..\n");
		return axl_false;
	} /* end if */
	
	/* now create a second listener using previous listener port */
	printf ("Test 01-e: listener created, now creating second listener reusing port: %s\n", 
		vortex_connection_get_port (listener));
	listener2 = vortex_listener_new (ctx, "0.0.0.0", vortex_connection_get_port (listener), NULL, NULL);
	if (vortex_connection_is_ok (listener2, axl_false)) {
		printf ("ERROR: expected to find listener failure, but found proper status code..\n");
		return axl_false;
	}

	/* check connection role (even having it not connected) */
	if (vortex_connection_get_role (listener2) != VortexRoleMasterListener) {
		printf ("ERROR: expected to find role %d, but found %d..\n", 
			VortexRoleMasterListener, vortex_connection_get_role (listener2));
		return axl_false;
	} /* end if */

	/* close the listener */
	vortex_connection_close (listener2);

	/* check listener here */
	if (! vortex_connection_is_ok (listener, axl_false)) {
		printf ("ERROR: expected to find proper listener status (first one created), but found a failure..\n");
		return axl_false;
	} /* end if */

	/* shutdown listener */
	vortex_connection_shutdown (listener);

	/* now, now do the same operation but several times */
	times--;
	if (times > 0)
		goto test_01e_do_test;
	
	return axl_true;
}

axl_bool test_01f (void) {

	VortexConnection * listener;
	VortexConnection * connection; 
	VortexCtx        * vortex_ctx;

	/* create a new vortex context */
	vortex_ctx = vortex_ctx_new ();

	/* now create a listener */
	printf ("Test 01-f: expected error: ");
	listener = vortex_listener_new (vortex_ctx, "0.0.0.0", "0", NULL, NULL);

	if (vortex_connection_is_ok (listener, axl_false)) {
		printf ("ERROR: failed to create local random listener..");
		return axl_false;
	} /* end if */

	vortex_ctx_free (vortex_ctx);
	vortex_ctx = vortex_ctx_new ();

	/* connect to local listener without registering */
	printf ("Test 01-f: expected error: ");
	connection = vortex_connection_new (vortex_ctx, 
					    vortex_connection_get_host (listener), 
					    vortex_connection_get_port (listener),
					    NULL, NULL);
	if (vortex_connection_is_ok (connection, axl_false)) {
		printf ("ERROR: failed to connect to local random listener located at: %s:%s..\n",
			vortex_connection_get_host (listener),
			vortex_connection_get_port (listener));
		return axl_false;
	}

	/* exit vortex_ctx: this should not segfault...it should do
	   anything because the context was not initialized */
	printf ("Test 01-f: expected error: ");
	vortex_exit_ctx (vortex_ctx, axl_true);

	/* now terminate context context used */
	vortex_ctx_free (vortex_ctx);
	
	return axl_true;
}


axl_bool test_01g (void) {

	VortexConnection * connection; 
	VortexChannel    * channel;
	VortexAsyncQueue * queue;
	VortexFrame      * frame;
	int                code;
	char             * msg;

	/* create a connection with serverName by default */
	connection = connection_new ();

	/* now check servername configured at serverSide */
	queue   = vortex_async_queue_new ();
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);

	if (channel == NULL) {
		printf ("ERROR (1): expected to find proper channel creation but a NULL reference was found..\n");
		return axl_false;
	} /* end if */

	/* check servername here */
	if (! axl_cmp (vortex_connection_get_server_name (connection), listener_host)) {
		printf ("ERROR (2): expected to find connection serverName %s but found %s\n",
			listener_host, vortex_connection_get_server_name (connection));
		return axl_false;
	} /* end if */

	/* ask for remote serverName */
	vortex_channel_send_msg (channel, 
				 "GET serverName",
				 14, 
				 NULL);

	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("ERROR (3): Failed to get the reply from the server..\n");
		return axl_false;
	}
	/* check servername received from remote server */
	if (! axl_cmp (vortex_frame_get_payload (frame), vortex_connection_get_server_name (connection))) {
		printf ("ERROR (4): Received a different server name configured than value received: %s..\n",
			(char*) vortex_frame_get_payload (frame));
		return axl_false;
	}
	if (! axl_cmp (vortex_frame_get_payload (frame), listener_host)) {
		printf ("ERROR (5): Received a different server name configured than value received: %s..\n",
			(char*) vortex_frame_get_payload (frame));
		return axl_false;
	}
	vortex_frame_unref (frame);
	
	/* close the connection */
	vortex_connection_close (connection);

	/* now open connection without accepting automatic serverName
	 * configuration */
	vortex_ctx_server_name_acquire (ctx, axl_false);

	/* do a connection */
	connection = connection_new ();

	/* check serverName feature */
	if (vortex_connection_get_server_name (connection) != NULL) {
		printf ("ERROR (6): expected to not find serverName configured, but found value: %s\n",
			vortex_connection_get_server_name (connection));
		return axl_false;
	}

	/* check connection status */
	if (! vortex_connection_is_ok (connection, axl_false)) {
		printf ("ERROR (7): expected to find proper connection status..but not found..\n");
		return axl_false;
	}

	/* close the connection */
	vortex_connection_close (connection);

	/* reenable server name acquire */
	vortex_ctx_server_name_acquire (ctx, axl_true);

	/* now connect asking for a particular servername */
	connection = vortex_connection_new_full (ctx, listener_host, LISTENER_PORT, 
						 CONN_OPTS (VORTEX_SERVERNAME_FEATURE, "reg-test.local"),
						 NULL, NULL);
	
	/* check connection status */
	if (! vortex_connection_is_ok (connection, axl_false)) {
		printf ("ERROR (8): expected to find proper connection status..but not found..\n");
		return axl_false;
	}

	/* now check servername configured at serverSide */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);

	/* check servername here */
	if (! axl_cmp (vortex_connection_get_server_name (connection), "reg-test.local")) {
		printf ("ERROR (9): expected to find connection serverName %s but found %s\n",
			"reg-test.local", vortex_connection_get_server_name (connection));
		return axl_false;
	} /* end if */

	/* ask for remote serverName */
	vortex_channel_send_msg (channel, 
				 "GET serverName",
				 14, 
				 NULL);

	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("ERROR (10): Failed to get the reply from the server..\n");
		return axl_false;
	}
	/* check servername received from remote server */
	if (! axl_cmp (vortex_frame_get_payload (frame), vortex_connection_get_server_name (connection))) {
		printf ("ERROR (11): Received a different server name configured than value received: %s..\n",
			(char*) vortex_frame_get_payload (frame));
		return axl_false;
	}
	if (! axl_cmp (vortex_frame_get_payload (frame), "reg-test.local")) {
		printf ("ERROR (12): Received a different server name configured than value received: %s..\n",
			(char*) vortex_frame_get_payload (frame));
		return axl_false;
	}
	vortex_frame_unref (frame);

	/* close the connection */
	vortex_connection_close (connection);

	/* now connect asking for a particular servername not allowed */
	connection = vortex_connection_new_full (ctx, listener_host, LISTENER_PORT, 
						 CONN_OPTS (VORTEX_SERVERNAME_FEATURE, "reg-test.wrong.local"),
						 NULL, NULL); 

	/* now check servername configured at serverSide */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);

	/* check connection status */
	if (channel != NULL) {
		printf ("ERROR (13): expected to NOT find proper connection status..but found..\n");
		return axl_false;
	}

	/* check here returned values */
	vortex_connection_pop_channel_error (connection, &code, &msg);

	if (code != 554 && ! axl_cmp (msg, "Unable to provide services under such serverName: reg-test.wrong.local")) {
		printf ("ERROR (14): expected error code=554 and error msg='Unable to provide services under such serverName: reg-test.wrong.local' after connection denied but found code='%d' and msg='%s'..\n", code, msg);
		return axl_false;
	} /* end if */
	axl_free (msg);

	/* close connection */
	vortex_connection_close (connection);

	/* unref the queue */
	vortex_async_queue_unref (queue);

	
	return axl_true;
}

#define TEST_01H_CHECK(string) do {                             \
                                                                \
	/* do a socket connect with wrong header content */     \
	socket = vortex_connection_sock_connect (ctx,           \
						 listener_host, \
						 LISTENER_PORT, \
						 NULL, NULL);   \
	if (socket == -1)                                       \
		return axl_false;                               \
	                                                        \
	/* send wrong content */                                \
	result = write (socket, string, strlen (string));       \
	                                                        \
	vortex_close_socket (socket);                           \
} while (0);


axl_bool test_01h (void) {

	VORTEX_SOCKET socket;
	int result;

	/* check different wrong headers */
	TEST_01H_CHECK ("RPY\n");

	TEST_01H_CHECK ("RPY\r\n");

	TEST_01H_CHECK ("RPY \0 \0 \r\n");

	TEST_01H_CHECK ("\n");

	TEST_01H_CHECK ("\0");

	TEST_01H_CHECK ("RPY 1234123\0\r\n");

	return axl_true;
}

axl_bool test_01i (void) {
	VortexConnection * conn;
	int                stamp;
	long               cur_timeout;

	/* check it with a timeout */
	cur_timeout = vortex_connection_get_connect_timeout (ctx);
	vortex_connection_connect_timeout (ctx, 500000);

	/* check connection to unreachable address */
	stamp = time (NULL);
	conn  = vortex_connection_new (ctx,
				       "172.26.7.3", "3200", NULL, NULL);
	if (vortex_connection_is_ok (conn, axl_false)) {
		printf ("Test 01-i (1): found connection ok where it was expected a failure..\n");
		return axl_false;
	} /* end if */

	/* check stamp before continue */
	if ((time (NULL) - stamp) > 2) {
		printf ("Test 01-i (2): expected to find faster error reporting for an unreachable address, but delayed %d seconds!!..\n",
			(int) (time (NULL) - stamp));
		return axl_false;
	}
	vortex_connection_close (conn);

	/* define again connect timeout */
	printf ("Test 01-i (3): restoring default timeout %ld\n", cur_timeout);
	vortex_connection_connect_timeout (ctx, cur_timeout);
	printf ("Test 01-i (4): after configuring it: %ld\n", vortex_connection_get_connect_timeout (ctx));

	return axl_true;
}

axl_bool test_01j_handler_value = axl_true;

void test_01j_handler (const char        * file,
		       int                 line,
		       VortexDebugLevel    log_level,
		       const char        * log_string,
		       va_list             args)
{
	int iterator = 0;

	/* check for % values */
	while (log_string[iterator] != 0) {
		if (log_string[iterator] == '%') {
			printf ("Found %% inside message (iterator=%d): %s\n",
				iterator, log_string);
			test_01j_handler_value = axl_false;
		}
		iterator++;
	}

	/* printf ("%s\n", log_string); */
	return;
}

/** 
 * @brief Check log handling with string preparation works.
 */
axl_bool test_01j (void) {

	VortexCtx * client_ctx = vortex_ctx_new ();

	/* enable log */
	vortex_log_enable (client_ctx, axl_true);
	test_01j_handler_value = axl_true;

	/* set log handler */
	vortex_log_set_handler (client_ctx, test_01j_handler);
	vortex_log_set_prepare_log (client_ctx, axl_true);
	
	/* init vortex library */
	if (! vortex_init_ctx (client_ctx)) {
		/* unable to init context */
		vortex_ctx_free (client_ctx);
		return axl_false;
	} /* end if */

	/* free context */
	vortex_exit_ctx (client_ctx, axl_true);

	/* return current handler status */
	return test_01j_handler_value;
}

/** 
 * @brief Checks memory consuption while sending a huge amount of
 * messages without wanting for replies.
 */
axl_bool test_01k (void) {
	VortexConnection * conn;
	int                iterator;
	VortexChannel    * channel;
	VortexAsyncQueue * queue;
	VortexFrame      * frame;

	/* do a connection */
	conn = connection_new ();
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (1): expected proper connection..\n");
		return axl_false;
	}

	/* create the queue */
	queue   = vortex_async_queue_new ();

	/* create a channel */
	channel = vortex_channel_new (conn, 0,
				      REGRESSION_URI,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("Unable to create the channel..");
		return axl_false;
	}

	/* set channel serialize */
	vortex_channel_set_serialize (channel, axl_true);

	/* set a pending outstanding message limit */
	vortex_channel_set_outstanding_limit (channel, 10, axl_false);  

	/* now send 10000 messages */
	iterator = 0;
	while (iterator < 10000) {
		/* send message */
		if (! vortex_channel_send_msg (channel, "This is a test", 14, NULL)) {
			printf ("ERROR (2): expected proper channel send operation for iterator=%d..\n", iterator);
			return axl_false;
		} /* end if */
		
		/* next position */
		iterator++;
	}

	/* now get all replies */
	iterator = 0;
	while (iterator < 10000) {
		/* next frame */
		frame = vortex_channel_get_reply (channel, queue);

		/* unref frame */
		vortex_frame_unref (frame);

		/* next iterator */
		iterator++;
	}

	/* remove queue */
	vortex_async_queue_unref (queue);

	vortex_connection_close (conn);
	return axl_true;
}

/** 
 * @brief Checks memory consuption for channel serialize replies and
 * with ANS/NUL.
 */
axl_bool test_01l (void) {
	VortexConnection * conn;
	int                iterator;
	int                iterator2;
	VortexChannel    * channel;
	VortexAsyncQueue * queue;
	VortexFrame      * frame;

	/* do a connection */
	conn = connection_new ();
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (1): expected proper connection..\n");
		return axl_false;
	}

	/* create the queue */
	queue   = vortex_async_queue_new ();

	/* create a channel */
	channel = vortex_channel_new (conn, 0,
				      REGRESSION_URI_SIMPLE_ANS_NUL,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("Unable to create the channel..");
		return axl_false;
	}

	/* set channel serialize */
	vortex_channel_set_serialize (channel, axl_true);

	/* now send 10 messages */
	iterator = 0;
	while (iterator < 10) {
		/* send message */
		if (! vortex_channel_send_msg (channel, "This is a test", 14, NULL)) {
			printf ("ERROR (2): expected proper channel send operation for iterator=%d..\n", iterator);
			return axl_false;
		} /* end if */

		/* wait for reply */
		iterator2 = 0;
		while (iterator2 < 30) {
			/* next frame */
			frame = vortex_channel_get_reply (channel, queue);

			if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_ANS) {
				printf ("ERROR (3): expected to find ANS frame but found frame type: %d..\n",
					vortex_frame_get_type (frame));
				return axl_false;
			} /* end if */

			/* unref frame */
			vortex_frame_unref (frame);
			
			/* next iterator */
			iterator2++;
		} /* end if */

		/* next frame */
		frame = vortex_channel_get_reply (channel, queue);
		if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_NUL) {
			printf ("ERROR (4): expected to find NUL frame but found frame type: %d..\n",
				vortex_frame_get_type (frame));
			return axl_false;
		} /* end if */

		/* get the frame */
		vortex_frame_unref (frame);
		
		/* next position */
		iterator++;
	}

	/* remove queue */
	vortex_async_queue_unref (queue);

	vortex_connection_close (conn);
	return axl_true;
}

/** 
 * @brief Checks memory consuption for channel pool
 */
axl_bool test_01o (void) {

	VortexConnection   * conn;
	int                  iterator;
	VortexChannel      * channel;
	VortexChannelPool  * pool;
	int                  channel_num = -1;

	/* do a connection */
	conn = connection_new ();
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (1): expected proper connection..\n");
		return axl_false;
	}

	/* create the channel pool */
	pool = vortex_channel_pool_new (conn,
					REGRESSION_URI,
					1,
					/* no close handling */
					NULL, NULL,
					/* frame receive async handling */
					NULL, NULL,
					/* no async channel creation */
					NULL, NULL);
	
	iterator = 0;
	while (iterator < 100000) {
		/* get a channel from the pool */
		channel = vortex_channel_pool_get_next_ready (pool, axl_true);
		if (channel == NULL) {
			printf ("ERROR (3): expected to find proper channel reference but found NULL..\n");
			return axl_false;
		} /* end if */

		if (channel_num == -1) 
			channel_num = vortex_channel_get_number (channel);
		else {
			if (channel_num != vortex_channel_get_number (channel)) {
				printf ("ERROR (4): expected to find different channel number (%d != %d)\n",
					channel_num, vortex_channel_get_number (channel));
				return axl_false; 
			} /* end if */
		}

		vortex_channel_pool_release_channel (pool, channel);

		/* check reference after releasing it */

		/* check connections channel */
		if (vortex_connection_channels_count (conn) != 2) {
			printf ("ERROR (2): expected to find 2 channels on the connection but found %d..\n", 
				vortex_connection_channels_count (conn));
			return axl_false;
		} /* end if */
		
		/* next iterator */
		iterator++;
	} /* end while */

	vortex_connection_close (conn);
	return axl_true;
}

/** 
 * @brief Checks window sizes upper limits.
 */
axl_bool test_01p (void) {

	VortexConnection   * conn;
	VortexConnection   * conn2;
	int                  iterator;
	VORTEX_SOCKET        socket;
	VortexChannel      * channel;
	char               * content;
	int                  bytes_written;
	VortexAsyncQueue   * wait_queue = NULL;
	VortexFrame        * frame;

	/* init queue */
	wait_queue = vortex_async_queue_new ();

	printf ("Test 01-p: Checking first part, ensuring remote size don't close by connection after notifying a bigger SEQ frame..\n");
	/* do a connection */
	conn = connection_new ();
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR (1): expected proper connection..\n");
		return axl_false;
	}

	/* create a channel */
	channel = vortex_channel_new (conn, 0,
				      REGRESSION_URI,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      NULL, NULL,
				      /* no async channel creation */
				      NULL, NULL);

	/* set window size for next exchanges = 256k to allow remote
	 * side to send without limits until consuming that window
	 * size */
	vortex_channel_set_window_size (channel, 262144);

	/* send content to check fix introduced */
	iterator = 0;
	while (iterator < 10) {
		/* send content */
		if (! vortex_channel_send_msg (channel, TEST_REGRESION_URI_4_MESSAGE, 4096, NULL)) {
			printf ("ERROR: found error at send_msg operation when that error was not expected..\n");
			return axl_false;
		} /* end if */

		/* next iterator */
		iterator++;
	} /* end while */

	/* check connection status */
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR: expected proper connection status, but found error..\n");
		return axl_false;
	} /* end if */

	vortex_connection_close (conn);

	printf ("Test 01-p: OK");


	/* socket close after incomplete frame */
	printf ("Test 01-p: Now check close after incomplete frame..\n");
	socket = vortex_connection_sock_connect (ctx, listener_host, LISTENER_PORT, NULL, NULL);
	printf ("Test 01-p: Socket created: %d\n", socket);

	/* injet content */
	if (send (socket, "RPY 0 0 . 0 20\r\n", 16, 0) != 16) {
		printf ("ERROR: expected to be able to send 16 bytes..\n");
		return axl_false;
	} /* end if */
	printf ("Test 01-p: injected wrong header..sending content\n");
	content = axl_new (char, 20);
	bytes_written = send (socket, content, 20, 0);
	vortex_async_queue_timedpop (wait_queue, 200000);

	shutdown (socket, SHUT_WR);
	vortex_close_socket (socket);

	/* wait */
	axl_free (content);
	printf ("Test 01-p: OK, (is alive server ;-) ??\n");

	/* window size under flow */
	printf ("Test 01-p: Now check sending frame fragments..\n");
	socket = vortex_connection_sock_connect (ctx, listener_host, LISTENER_PORT, NULL, NULL);
	printf ("Test 01-p: Socket created: %d\n", socket);

	/* injet content */
	if (send (socket, "RPY 0 0 . 0 10\r\n", 16, 0) != 16) {
		printf ("ERROR: expected to be able to send 16 bytes..\n");
		return axl_false;
	} /* end if */

	iterator = 0;
	content = axl_new (char, 20);
	while (iterator < 100) {
		bytes_written = send (socket, content, 20, 0);
		if (bytes_written == -1)
			break;

		/* next position */
		iterator++;
	} /* end while */

	/* wait */
	recv (socket, content, 20, 0);
	axl_free (content);

	shutdown (socket, SHUT_WR);
	vortex_close_socket (socket);	

	/* window size overflow */
	printf ("Test 01-p: Now checking I can't push more content that the window size expected at the remote BEEP peer\n");
	socket = vortex_connection_sock_connect (ctx, listener_host, LISTENER_PORT, NULL, NULL);
	printf ("Test 01-p: Socket created: %d\n", socket);

	/* injet content */
	if (send (socket, "RPY 0 0 . 0 65535\r\n", 19, 0) != 19) {
		printf ("ERROR: expected to be able to send 19 bytes..\n");
		return axl_false;
	} /* end if */
	printf ("Test 01-p: injected wrong header..sending content\n");
	content = axl_new (char, 65536);
	bytes_written = send (socket, content, 65536, 0);

	/* wait */
	recv (socket, content, 65536, 0);
	axl_free (content);

	vortex_close_socket (socket);

	/* request remote server to install idle handling */
	conn = connection_new ();
	if (!vortex_connection_is_ok (conn, axl_false)) {
		vortex_connection_close (conn);
		return axl_false;
	} /* end if */

	/* create a channel */
	channel = vortex_channel_new (conn, 0,
				      REGRESSION_URI,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      vortex_channel_queue_reply, wait_queue,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("ERROR: expected to be able to create a connection to set idle handler but channel creation failed\n");
		return axl_false;
	} /* end if */

	printf ("Test 01-p: Requesting to activate idle handling..\n");
	vortex_channel_send_msg (channel, "enable-idle-handling", 20, NULL);

	/* wait for reply */
	frame = vortex_channel_get_reply (channel, wait_queue);
	vortex_frame_unref (frame);

	/* window size overflow with frame fragmentation */
	printf ("Test 01-p: Now check if we can keep a half opened connection...\n");
	socket = vortex_connection_sock_connect (ctx, listener_host, LISTENER_PORT, NULL, NULL);
	printf ("Test 01-p: Socket created: %d\n", socket);

	/* injet content */
	if (send (socket, "RPY 0 0 . 0 353\r\n", 19, 0) != 19) {
		printf ("ERROR: expected to be able to send 19 bytes..\n");
		return axl_false;
	} /* end if */
	printf ("Test 01-p: injected wrong header..sending content\n");
	content = axl_new (char, 353);
	bytes_written = send (socket, content, 100, 0);

	/* wait */
	printf ("Test 01-p: configured idle handler, waiting to unlock..\n");
	recv (socket, content, 100, 0); 
	axl_free (content);

	/* before closing, check again the idle handling do not avoid creating new connections */
	conn2 = connection_new ();
	if (! vortex_connection_is_ok (conn2, axl_false)) {
		printf ("Test 01-p: expected to be able to create second connection having idle handler installed..\n");
		return axl_false;
	} /* end if */
	vortex_connection_close (conn2);

	/* close connection */
	vortex_connection_close (conn);

	/* finish wait queue */
	vortex_async_queue_unref (wait_queue);

	return axl_true;
}




#define TEST_02_MAX_CHANNELS 24

void test_02_channel_created (int                channel_num, 
			      VortexChannel    * channel, 
			      VortexConnection * conn, 
			      axlPointer         user_data)
{
	
	/* check error code received */
	if (channel_num != -1) {
		printf ("ERROR: expected to received -1 on channel creation failure, inside threaded mode, but found: %d\n", 
			channel_num);
		vortex_async_queue_push ((VortexAsyncQueue *) user_data, INT_TO_PTR(2));
		return;
	} /* end if */

	/* push data received (in the case a null reference is
	 * received push 1 because we can't push NULL or 0. in the
	 * case a valid reference is received push it as is to break
	 * the other side */
	vortex_async_queue_push ((VortexAsyncQueue *) user_data, channel == NULL ? INT_TO_PTR(1) : channel);

	return;
}

axl_bool  test_02_common (VortexConnection * connection)
{

	VortexChannel    * channel[TEST_02_MAX_CHANNELS];
	VortexAsyncQueue * queue;
	VortexFrame      * frame;
	int                iterator;
	char             * message;
	char             * msg;
	int                code;

	/* create the queue */
	queue   = vortex_async_queue_new ();

	/* create all channels */
	iterator = 0;
	while (iterator < TEST_02_MAX_CHANNELS) {
	
		/* create a channel */
		channel[iterator] = vortex_channel_new (connection, 0,
							REGRESSION_URI,
							/* no close handling */
							NULL, NULL,
							/* frame receive async handling */
							vortex_channel_queue_reply, queue,
							/* no async channel creation */
							NULL, NULL);
		/* check channel returned */
		if (channel[iterator] == NULL) {
			printf ("Unable to create the channel, failed to create channel=%d..", iterator);
			return axl_false;
		}

		/* update reference counting */
		vortex_channel_ref (channel[iterator]);

		/* enable serialize */
		vortex_channel_set_serialize (channel[iterator], axl_true);

		/* update the iterator */
		iterator++;

	} /* end while */

	/* now send data */
	iterator = 0;
	while (iterator < TEST_02_MAX_CHANNELS) {
		
		/* build the message */
		message = axl_strdup_printf ("Message: %d\n", iterator);
		
		/* send message */
		if (! vortex_channel_send_msg (channel[iterator], message, strlen (message), NULL)) {
			printf ("Unable to send message over channel=%d\n", vortex_channel_get_number (channel[iterator]));
			return axl_false;
		}

		/* free message */
		axl_free (message);

		/* update iterator */
		iterator++;

	} /* end while */

	/* get all replies */
	iterator = 0;
	while (iterator < TEST_02_MAX_CHANNELS) {

		/* get message */
		frame    = vortex_async_queue_pop (queue);

		/* get frame */
		vortex_frame_free (frame);
		
		/* update iterator */
		iterator++;

	} /* end while */

	/* check queue state */
	if (vortex_async_queue_length (queue) != 0) {
		/* expected to find an empty queue */
		printf ("Expected to find an empty queue\n");
		return axl_false;
	}

	/* now send data */
	iterator = 0;
	while (iterator < TEST_02_MAX_CHANNELS) {
		
		/* build the message */
		message = axl_strdup_printf ("Message: %d\n", iterator);
		
		/* send message */
		if (! vortex_channel_send_msg (channel[0], message, strlen (message), NULL)) {
			printf ("Unable to send message over channel=%d\n", vortex_channel_get_number (channel[0]));
			return axl_false;
		}

		/* free message */
		axl_free (message);

		/* update iterator */
		iterator++;
		
	} /* end while */

	/* get all replies */
	iterator = 0;
	while (iterator < TEST_02_MAX_CHANNELS) {

		/* get message */
		frame    = vortex_async_queue_pop (queue);

		/* build the message */
		message = axl_strdup_printf ("Message: %d\n", iterator);

		if (! axl_cmp (message, vortex_frame_get_payload (frame))) {
			printf ("Message received (on the same channel) which isn't the expected one: %s != %s (iterator=%d)\n", 
				message, (char*) vortex_frame_get_payload (frame), iterator);
			return axl_false;
		}

		/* get frame and the expected message */
		vortex_frame_free (frame);
		axl_free (message);
		
		/* update iterator */
		iterator++;

	} /* end while */

	/* check queue state */
	if (vortex_async_queue_length (queue) != 0) {
		/* expected to find an empty queue */
		printf ("Expected to find an empty queue\n");
		return axl_false;
	}

	/* close all channels */
	iterator = 0;
	while (iterator < TEST_02_MAX_CHANNELS) {

		/* close the channel */
		if (! vortex_channel_close (channel[iterator], NULL)) {
			printf ("failed to close channel=%d\n", vortex_channel_get_number (channel[iterator]));
			return axl_false;
		}

		/* unref the channel */
		vortex_channel_unref (channel[iterator]);

		/* update the iterator */
		iterator++;

	} /* end while */

	/* now check for channels denied to be opened */
	channel[0] = vortex_channel_new (connection, 0, 
					 REGRESSION_URI_DENY,
					 /* no close handling */
					 NULL, NULL,
					 /* no frame receive handling */
					 NULL, NULL,
					 /* no async channel create notification */
					 NULL, NULL);
	if (channel[0] != NULL) {
		printf ("Expected to find an error while trying to create a channel under the profile: %s\n",
			REGRESSION_URI_DENY);
		return axl_false;
	}

	/* now check channel creation in threaded mode */
	vortex_channel_new (connection, 0, 
			    REGRESSION_URI_DENY,
			    /* no close handling */
			    NULL, NULL,
			    /* no frame receive handling */
			    NULL, NULL,
			    /* no async channel create notification */
			    test_02_channel_created, queue);

	/* get channel created */
	channel[0] = vortex_async_queue_pop (queue);
					 
	if (PTR_TO_INT (channel[0]) != 1) {
		printf ("Expected to find an error while trying to create a channel under the profile: %s\n",
			REGRESSION_URI_DENY);
		return axl_false;
	}
	
	/* check error code here */
	if (! vortex_connection_pop_channel_error (connection, &code, &msg)) {
		printf ("Expected to find error message after channel creation failure..\n");
		return axl_false;
	}

	/* check profile not supported error code */
	if (code != 554) {
		printf ("Expected to find error code reported as profile not supported.\n");
		return axl_false;
	}

	axl_free (msg);

	/* now check channel creation in threaded mode for a remote supported profile */
	vortex_channel_new (connection, 0, 
			    REGRESSION_URI_DENY_SUPPORTED,
			    /* no close handling */
			    NULL, NULL,
			    /* no frame receive handling */
			    NULL, NULL,
			    /* no async channel create notification */
			    test_02_channel_created, queue);

	/* get channel created */
	channel[0] = vortex_async_queue_pop (queue);
					 
	if (PTR_TO_INT (channel[0]) != 1) {
		printf ("Expected to find an error while trying to create a channel under the profile: %s\n",
			REGRESSION_URI_DENY);
		return axl_false;
	}
	
	/* check error code here */
	if (! vortex_connection_pop_channel_error (connection, &code, &msg)) {
		printf ("Expected to find error message after channel creation failure..\n");
		return axl_false;
	}

	/* check profile not supported error code */
	if (code != 421) {
		printf ("Expected to find error code reported as profile supported by denied to create a channel.\n");
		return axl_false;
	} 

	axl_free (msg);

	/* free queue */
	vortex_async_queue_unref (queue);

	return axl_true;
}

/* message size variable used by test_03 and test_02f */
#define TEST_03_MSGSIZE (65536 * 8)

void test_03_reply (VortexChannel    * channel,
		    VortexConnection * connection,
		    VortexFrame      * frame, 
		    axlPointer         user_data)
{
	VortexAsyncQueue * queue = user_data;
	
	/* push the frame received */
	frame = vortex_frame_copy (frame);
	vortex_async_queue_push (queue, frame);
	
	return;
}

axl_bool  test_03_common (VortexConnection * connection) {

	VortexChannel    * channel;
	VortexFrame      * frame;
	VortexAsyncQueue * queue;
	char             * message;
	int                iterator;

	/* create the queue */
	queue = vortex_async_queue_new ();
	
	/* create a channel */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      test_03_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("Unable to create the channel..");
		return axl_false;
	}

	/* build the message */
	message = axl_new (char, TEST_03_MSGSIZE);
	for (iterator = 0; iterator < TEST_03_MSGSIZE; iterator++)
		message [iterator] = (iterator % 3);

	iterator = 0;
	while (iterator < 20) {

		/* send the hug message */
		if (! vortex_channel_send_msg (channel, message, TEST_03_MSGSIZE, NULL)) {
			printf ("Test 03: Unable to send large message");
			axl_free (message);
			return axl_false;
		}

		/* update the iterator */
		iterator++;
		
	} /* end while */

	iterator = 0;
	while (iterator < 20) {

		/* wait for the message */
		frame = vortex_async_queue_pop (queue);
		
		/* check payload size */
		if (vortex_frame_get_payload_size (frame) != TEST_03_MSGSIZE) {
			printf ("Test 03: found that payload received isn't the value expected: %d != %d\n",
				vortex_frame_get_payload_size (frame), TEST_03_MSGSIZE);
			/* free frame */
			vortex_frame_free (frame);
			
			return axl_false;
		}

		/* check result */
		if (memcmp (message, vortex_frame_get_payload (frame), TEST_03_MSGSIZE)) {
			printf ("Test 03: Messages aren't equal\n");
			return axl_false;
		}

		/* check the reference of the channel associated */
		if (vortex_frame_get_channel_ref (frame) == NULL) {
			printf ("Test 03: Frame received doesn't have a valid channel reference configured\n");
			return axl_false;
		} /* end if */

		/* check channel reference */
		if (! vortex_channel_are_equal (vortex_frame_get_channel_ref (frame),
						channel)) {
			printf ("Test 03: Frame received doesn't have the spected channel reference configured\n");
			return axl_false;
		} /* end if */
		
		/* free frame received */
		vortex_frame_free (frame);

		/* update iterator */
		iterator++;
	}

	/* free the message */
	axl_free (message);

	/* ok, close the channel */
	vortex_channel_close (channel, NULL);

	/* free queue */
	vortex_async_queue_unref (queue);

	/* return axl_true */
	return axl_true;
}

char * test_04_read_file (const char * file, int * size)
{
	char * result;
	FILE * handle;
	struct stat status;

	/* check parameter received */
	if (file == NULL)
		return NULL;

	/* open the file */
	handle = fopen (file, "r");
	if (handle == NULL) {
		printf ("Failed to open file: %s\n", file);
		return NULL;
	}

	/* get the file size */
	memset (&status, 0, sizeof (struct stat));
	if (stat (file, &status) != 0) {
		/* failed to get file size */
		fprintf (stderr, "Failed to get file size for %s..\n", file);
		fclose (handle);
		return NULL;
	} /* end if */
	
	result = axl_new (char, status.st_size + 1);
	if (fread (result, 1, status.st_size, handle) != status.st_size) {
		/* failed to read content */
		fprintf (stdout, "Unable to properly read the file, size expected to read %d, wasn't fulfilled",
			 (int) status.st_size);
		axl_free (result);
		fclose (handle);
		return NULL;
	} /* end if */
	
	/* close the file and return the content */
	fclose (handle);

	/* update size */
	if (size)
		*size = status.st_size;

	return result;
}

char * test_04_ab_gen_md5 (const char * file)
{
	char * result;
	char * resultAux = NULL;
	int    size      = 0;
	
	/* read file */
	result = test_04_read_file (file, &size);

#if defined(ENABLE_TLS_SUPPORT)
	/* now create the md5 representation */
	resultAux = vortex_tls_get_digest_sized (VORTEX_MD5, result, size);
#else
	printf ("Current build does not have TLS support.\n");
#endif
	axl_free (result);

	return resultAux;
}


/** 
 * Common implementation for test_04_ab test
 */
axl_bool  test_04_ab_common (VortexConnection * connection, int window_size, const char * prefix, int * amount_transferred, int times, axl_bool  change_mss) {

	VortexChannel    * channel;
	VortexAsyncQueue * queue;
	VortexFrame      * frame;
	char             * file_name;
	char             * md5;
	char             * md5Aux;
	FILE             * file;
	int                bytes_written;
	int                iterator = 0;
	axl_bool           disable_log = (times == 4);

	if (amount_transferred)
		(*amount_transferred) = 0;

#if ! defined(ENABLE_TLS_SUPPORT)	
	printf ("--- WARNING: Current build does not have TLS support (UNABLE TO TRANSFER).\n");
	return axl_true;
#endif

	/* create the queue */
	queue = vortex_async_queue_new ();
	
	/* create a channel */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI_5,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("Unable to create the channel..");
		return axl_false;
	}

	/* configure serialize */
	vortex_channel_set_serialize (channel, axl_true);

	/* check and change default window size */
	if (window_size != -1) 
		vortex_channel_set_window_size (channel, window_size);

	if (change_mss) {
		/* request remote peer to change its step to the
		 * current mss */
		if (! vortex_channel_send_msg (channel, "change-mss", 10, NULL)) {
			printf ("failed to request mss change..\n");
			return axl_false;
		}
		frame = vortex_channel_get_reply (channel, queue);
		if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY) {
			printf ("Expected to find rpy reply after requesting to change remote step..\n");
			return axl_false;
		} /* end if */
		if (! axl_cmp (vortex_frame_get_payload (frame), "change-mss")) {
			printf ("Expected to find 'change-mss' message in frame payload, but it wasn't found..\n");
			return axl_false;
		}
		vortex_frame_unref (frame);
		printf ("%sTest 04-ab: changed remote segmentation, limiting it to MSS\n", prefix ? prefix : "");
	} /* end if */

 transfer_again:

	/* configure the file requested: first file */
	file_name = "vortex-regression-client.c";

 transfer_file:
	/* create the md5 to track content transfered */
	md5       = test_04_ab_gen_md5 (file_name);
	if (! disable_log)
		printf ("%sTest 04-ab:   request files: %s (md5: %s)\n", prefix ? prefix : "", file_name, md5);
	if (! vortex_channel_send_msg (channel, file_name, strlen (file_name), NULL)) {
		printf ("Failed to send message request to retrieve file: %s..\n", file_name);
		return axl_false;
	} /* end if */

	/* open the file */
#if defined(AXL_OS_UNIX)
	file = fopen ("vortex-regression-client-test-04-ab.txt", "w");
#elif defined(AXL_OS_WIN32)
	file = fopen ("vortex-regression-client-test-04-ab.txt", "wb");
#endif

	/* check result */
	if (file == NULL) {
		printf ("Unable to create the file (%s) to hold content: %s\n", "vortex-regression-client-test-04-ab.txt", strerror (errno));
		return axl_false;
	}

	/* wait for all replies */
	if (! disable_log)
		printf ("%sTest 04-ab:   waiting replies having file: %s\n", prefix ? prefix : "", file_name);
	while (axl_true) {
		/* get the next message, blocking at this call. */
		frame = vortex_channel_get_reply (channel, queue);
		if (frame == NULL) {
			printf ("Timeout received for regression test: %s\n", REGRESSION_URI_4);
			continue;
		}

		/* check frame type */
		if (vortex_frame_get_type (frame) == VORTEX_FRAME_TYPE_NUL) {
			/* unref the frame and finish the loop */
			vortex_frame_unref (frame);
			break;
		} /* endif */

		/* write content */
		bytes_written = fwrite (vortex_frame_get_payload (frame),
					1, vortex_frame_get_payload_size (frame), file);

		if (bytes_written != vortex_frame_get_payload_size (frame)) {
			printf ("ERROR: error while writing to the file: %d != %d\n", 
				bytes_written, vortex_frame_get_payload_size (frame));
			return axl_false;
		} /* end if */

		/* update amount transferred */
		if (amount_transferred)
			(*amount_transferred) += vortex_frame_get_payload_size (frame);
			
		/* deallocate the frame received */
		vortex_frame_unref (frame);
	} /* end while */

	/* close the file */
	fclose (file);

	/* now check md5 sum */
	md5Aux = test_04_ab_gen_md5 ("vortex-regression-client-test-04-ab.txt");
	if (! axl_cmp (md5, md5Aux)) {
		printf ("Content transfered is not the expected, md5 sum differs: %s != %s",
			md5, md5Aux);
		axl_free (md5);
		axl_free (md5Aux);
		return axl_false;
	}
	
	if (! disable_log)
		printf ("%sTest 04-ab:   content transfered for %s ok\n", prefix ? prefix : "", file_name);

	axl_free (md5);
	axl_free (md5Aux);

	/* check to transfer more files */
	iterator++;
	switch (iterator) {
	case 1:
		file_name = "vortex-client.c";
		goto transfer_file;
	case 2:
		file_name = "vortex-regression-listener.c";
		goto transfer_file;
	case 3:
		file_name = "vortex-sasl-listener.c";
		goto transfer_file;
	default:
		/* more files to transfer */
		break;
	}

	times--;
	if (times > 0) {
		iterator  = 0;
		goto transfer_again;
	}

	/* free the queue */
	vortex_async_queue_unref (queue);

	/* ok, close the channel */
	if (! vortex_channel_close (channel, NULL))
		return axl_false;

	/* close connection */
	return axl_true;
}


axl_bool  test_02 (void) {

	VortexConnection * connection;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
		
	} /* end if */

	/* call common implementation */
	if (! test_02_common (connection))
		return axl_false;
	
	/* ok, close the connection */
	if (! vortex_connection_close (connection)) {
		printf ("failed to close the BEEP session\n");
		return axl_false;
	} /* end if */
	
	/* return axl_true */
	return axl_true;
}

VortexAsyncQueue * test_02a_queue;

void test_02a_handler1 (VortexConnection * connection)
{
	/* store data on the queue */
	vortex_async_queue_push (test_02a_queue, INT_TO_PTR (1));
}

void test_02a_handler2 (VortexConnection * connection)
{
	/* store data on the queue */
	vortex_async_queue_push (test_02a_queue, INT_TO_PTR (1));
}

void test_02a_handler3 (VortexConnection * connection)
{
	/* store data on the queue */
	vortex_async_queue_push (test_02a_queue, INT_TO_PTR (1));
}

void test_02a_handler1_full (VortexConnection * connection, axlPointer data)
{
	/* store data on the queue */
	vortex_async_queue_push ((VortexAsyncQueue *) data, INT_TO_PTR (1));
}

void test_02a_handler2_full (VortexConnection * connection, axlPointer data)
{
	/* store data on the queue */
	vortex_async_queue_push ((VortexAsyncQueue *) data, INT_TO_PTR (1));
}

void test_02a_handler3_full (VortexConnection * connection, axlPointer data)
{
	/* store data on the queue */
	vortex_async_queue_push ((VortexAsyncQueue *) data, INT_TO_PTR (1));
}

/** 
 * @brief Check current connection close notification
 * 
 * 
 * @return axl_true if ok, otherwise axl_false is returned.
 */
axl_bool  test_02a (void) {

	VortexConnection * connection;
	int                count;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
		
	} /* end if */

	/* create a queue to store data from handlers */
	test_02a_queue = vortex_async_queue_new ();

	/* install on close handlers */
	vortex_connection_set_on_close (connection, test_02a_handler1);

	/* install on close handlers */
	vortex_connection_set_on_close (connection, test_02a_handler2);

	/* install on close handlers */
	vortex_connection_set_on_close (connection, test_02a_handler3);

	/* close the connection */
	vortex_connection_close (connection);

	/* wait for all handlers */
	vortex_async_queue_pop (test_02a_queue);
	vortex_async_queue_pop (test_02a_queue);
	vortex_async_queue_pop (test_02a_queue);


	/* check the queue */
	count = vortex_async_queue_length (test_02a_queue);
	if (vortex_async_queue_length (test_02a_queue) != 0) {
		fprintf (stderr, "expected to find 0 items on the queue, but found (%d), on close handler is not working\n",
			 count);
		return axl_false;
	}

	/* unref the queue */
	vortex_async_queue_unref (test_02a_queue);

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
		
	} /* end if */

	/* create a queue to store data from handlers */
	test_02a_queue = vortex_async_queue_new ();

	/* install on close handlers */
	vortex_connection_set_on_close_full (connection, test_02a_handler1_full, test_02a_queue);

	/* install on close handlers */
	vortex_connection_set_on_close_full (connection, test_02a_handler2_full, test_02a_queue);

	/* install on close handlers */
	vortex_connection_set_on_close_full (connection, test_02a_handler3_full, test_02a_queue);

	/* close the connection */
	vortex_connection_close (connection);

	/* wait for all handlers */
	vortex_async_queue_pop (test_02a_queue);
	vortex_async_queue_pop (test_02a_queue);
	vortex_async_queue_pop (test_02a_queue);

	/* check the queue */
	count = vortex_async_queue_length (test_02a_queue);
	if (vortex_async_queue_length (test_02a_queue) != 0) {
		fprintf (stderr, "expected to find 0 items on the queue, but found (%d), on close full handler is not working\n",
			 count);
		return axl_false;
	}

	/* unref the queue */
	vortex_async_queue_unref (test_02a_queue);
	

	return axl_true;
}

void test_02a1_remove (VortexConnection * conn, axlPointer data)
{
	/* remove my handler */
	vortex_connection_remove_on_close_full (conn, test_02a1_remove, data);
	return;
}

void test_02a1_data (VortexConnection * conn, axlPointer data)
{
	/* install a value */
	vortex_async_queue_push ((VortexAsyncQueue *) data, INT_TO_PTR (37));
	return;
}

axl_bool  test_02a1 (void) {
	VortexConnection * connection;
	VortexAsyncQueue * queue;
	int                value;

	/* create a queue to store data from handlers */
	queue = vortex_async_queue_new ();

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
		
	} /* end if */

	/* install on close handlers */
	vortex_connection_set_on_close_full (connection, test_02a1_remove, queue);

	/* install on close handlers */
	vortex_connection_set_on_close_full (connection, test_02a1_data, queue);

	/* close the connection */
	vortex_connection_close (connection);

	/* wait for all handlers */
	value = PTR_TO_INT (vortex_async_queue_pop (queue));

	if (value != 37) {
		printf ("ERROR: expected to find 37 but found: %d..\n", value);
		return axl_false;
	}

	/* unref the queue */
	vortex_async_queue_unref (queue);

	return axl_true;
}

axl_bool  test_02b (void) {
	VortexConnection  * connection;
	VortexChannel     * channel;
	VortexAsyncQueue  * queue;
	VortexFrame       * frame;


	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
	}

	/* create the queue */
	queue   = vortex_async_queue_new ();

	/* create a channel */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("Unable to create the channel..");
		return axl_false;
	}

	/* send a message the */
	if (! vortex_channel_send_msg (channel, "a", 1, NULL)) {
		printf ("failed to send a small message\n");
		return axl_false;
	}

	/* ok, close the channel */
	vortex_channel_close (channel, NULL);

	/* ok, close the connection */
	vortex_connection_close (connection);

	/* free queue and frames inside */
	while (vortex_async_queue_length (queue) != 0) {

		/* pop and free */
		frame = vortex_async_queue_pop (queue);
		vortex_frame_free (frame);
	} /* end if */
	
	/* dealloc the queue */
	vortex_async_queue_unref (queue);

	/* return axl_true */
	return axl_true;
}

axl_bool  test_02c (void) {
	VortexConnection  * connection;
	VortexChannel     * channel;
	VortexAsyncQueue  * queue;
	VortexFrame       * frame;
	int                 iterator;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
	}

	/* create the queue */
	queue   = vortex_async_queue_new ();

	/* create a channel */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("Unable to create the channel..");
		return axl_false;
	}

	/* send a message the */
	iterator = 0;
	while (iterator < 1000) {
	
		if (! vortex_channel_send_msg (channel, "a", 1, NULL)) {
			printf ("failed to send a small message\n");
			return axl_false;
		}

		iterator++;
	} /* end while */

	/* ok, close the channel */
	vortex_channel_close (channel, NULL);

	/* ok, close the connection */
	vortex_connection_close (connection);

	/* free queue and frames inside */
	while (vortex_async_queue_length (queue) != 0) {

		/* pop and free */
		frame = vortex_async_queue_pop (queue);
		vortex_frame_free (frame);
	} /* end if */
	
	/* dealloc the queue */
	vortex_async_queue_unref (queue);

	/* return axl_true */
	return axl_true;
}

axl_bool  test_02d (void) {
	VortexConnection  * connection;
	VortexChannel     * channel;
	VortexAsyncQueue  * queue;
	VortexFrame       * frame;
	char              * message;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
	}

	/* create the queue */
	queue   = vortex_async_queue_new ();

	/* create a channel */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI_CLOSE_AFTER_LARGE_REPLY,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("Unable to create the channel..");
		return axl_false;
	}

	/* send a message the */
	printf ("Test 02-d: Sending message..\n");
	if (! vortex_channel_send_msg (channel, "a", 1, NULL)) {
		printf ("failed to send a small message\n");
		return axl_false;
	}

	/* pop and free */
	printf ("Test 02-d: message sent, waiting reply....\n");
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("Found null frame when expecting for content..\n");
		return axl_false;
	}
	if (vortex_frame_get_payload_size (frame) != 32767) {
		printf ("Expected to find payload content size %d but found %d\n",
			32767, vortex_frame_get_payload_size (frame));
		return axl_false;
	}
	vortex_frame_unref (frame);

	printf ("Test 02-d: message received ok..\n");
	
	
	/* ok, close the connection */
	if (! vortex_connection_close (connection)) {
		printf ("Failed to close connection: %s\n", vortex_connection_get_message (connection));
		return axl_false;
	}


	/*** SECOND TEST PART ***/
	printf ("Test 02-d: now check local close..\n");

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
	}

	/* create a channel */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI_CLOSE_AFTER_LARGE_REPLY,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("Unable to create the channel..");
		return axl_false;
	}

	/* send a message the */
	printf ("Test 02-d: Sending message..\n");
	if (! vortex_channel_send_msg (channel, "send-message", 12, NULL)) {
		printf ("failed to send a small message\n");
		return axl_false;
	}

	/* block until reply is received */
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("Found null frame when expecting for content..\n");
		return axl_false;
	}

	/* check reply for our first message and send rply for next
	 * message */
	printf ("Test 02-d: received message reply, checking its type...\n");
	if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY) {
		printf ("Expected to find RPY type but found: %d..\n", vortex_frame_get_type (frame));
		return axl_false;
	}
	vortex_frame_unref (frame);

	/* block until reply is received */
	printf ("Test 02-d: received message to be replied..\n");
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("Found null frame when expecting for content..\n");
		return axl_false;
	}

	/* check reply for our first message and send rply for next
	 * message */
	printf ("Test 02-d: Received message...checking..\n");
	if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_MSG) {
		printf ("Expected to find RPY type but found: %d..\n", vortex_frame_get_type (frame));
		return axl_false;
	}

	/* now reply with a huge message */
	message = axl_new (char, 65536);
	if (! vortex_channel_send_rpy (channel, message, 65536, vortex_frame_get_msgno (frame))) {
		printf ("Expected to be able to send reply..\n");
		return axl_false;
	}
	axl_free (message);
	vortex_frame_unref (frame);

	/* now close the connection */
	printf ("Test 02-d: now close connection..\n");
	if (! vortex_connection_close (connection)) {
		printf ("Failed to close connection: %s\n", vortex_connection_get_message (connection));
		return axl_false;
	}

	/* dealloc the queue */
	vortex_async_queue_unref (queue);

	/* return axl_true */
	return axl_true;
}

axl_bool  test_02e (void) {

	VortexConnection * connection;
	VortexChannel    * channel;
	WaitReplyData    * wait_reply;
	VortexFrame      * frame;
	int                iterator;
	char             * message;
	int                msg_no;
	VortexAsyncQueue * queue;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
		
	} /* end if */

	/* create the queue */
	queue = vortex_async_queue_new ();
	
	/* create the channel */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      NULL, NULL,
				      /* no async channel creation */
				      NULL, NULL);
	/* check channel returned */
	if (channel == NULL) {
		printf ("Unable to create the channel, failed to create channel..\n");
		return axl_false;
	}

	/* now send data */
	iterator = 0;
	while (iterator < 10) {

		/* create wait reply object */
		wait_reply = vortex_channel_create_wait_reply ();
		
		/* build the message */
		message    = axl_strdup_printf ("Message: %d\n", iterator);

		/* send message */
		if (! vortex_channel_send_msg_and_wait (channel, message, strlen (message), &msg_no, wait_reply)) {
			printf ("Unable to send message over channel=%d\n", vortex_channel_get_number (channel));
			return axl_false;
		}

		/* get message */
		frame = vortex_channel_wait_reply (channel, msg_no, wait_reply);
		if (frame == NULL) {
			printf ("there was an error while receiving the reply or a timeout have occur\n");
			return axl_false;
		}

		/* check reference counting for frame returned */
		if (vortex_frame_ref_count (frame) != 1) {
			printf ("Expected to find ref counting equal to == 1, but found %d..\n",
				vortex_frame_ref_count (frame));
			return axl_false;
		}

		/* check data */
		if (! axl_cmp (vortex_frame_get_payload (frame), message)) {
			printf ("Found different content at message..\n");
			return axl_false;
		}

		/* unref frame */
		vortex_frame_unref (frame);

		/* free message */
		axl_free (message);

		/* update iterator */
		iterator++;

	} /* end while */

	vortex_async_queue_unref (queue);

	/* ok, close the connection */
	if (! vortex_connection_close (connection)) {
		printf ("failed to close the BEEP session\n");
		return axl_false;
	} /* end if */
	

	/* return axl_true */
	return axl_true;
}

axl_bool  test_02f_send_data (VortexChannel * channel, const char * message, VortexAsyncQueue * queue, 
			 long max_tv_sec, long max_tv_usec)
{
	int               iterator;
	VortexFrame     * frame;
	struct timeval    start;
	struct timeval    stop;
	struct timeval    result;

	/* start */
	gettimeofday (&start, NULL);

	iterator = 0;
	while (iterator < 10) {

		/* send the hug message */
		if (! vortex_channel_send_msg (channel, message, TEST_03_MSGSIZE, NULL)) {
			printf ("Test 03: Unable to send large message");
			return axl_false;
		}

		/* wait for the message */
		frame = vortex_async_queue_pop (queue);
		
		/* check payload size */
		if (vortex_frame_get_payload_size (frame) != TEST_03_MSGSIZE) {
			printf ("Test 03: found that payload received isn't the value expected: %d != %d\n",
				vortex_frame_get_payload_size (frame), TEST_03_MSGSIZE);
			/* free frame */
			vortex_frame_free (frame);
			
			return axl_false;
		}

		/* check result */
		if (memcmp (message, vortex_frame_get_payload (frame), TEST_03_MSGSIZE)) {
			printf ("Test 03: Messages aren't equal\n");
			return axl_false;
		}

		/* check the reference of the channel associated */
		if (vortex_frame_get_channel_ref (frame) == NULL) {
			printf ("Test 03: Frame received doesn't have a valid channel reference configured\n");
			return axl_false;
		} /* end if */

		/* check channel reference */
		if (! vortex_channel_are_equal (vortex_frame_get_channel_ref (frame),
						channel)) {
			printf ("Test 03: Frame received doesn't have the spected channel reference configured\n");
			return axl_false;
		} /* end if */
		
		/* free frame received */
		vortex_frame_free (frame);

		/* update the iterator */
		iterator++;
		
	} /* end while */

	/* stop */
	gettimeofday (&stop, NULL);

	/* get result */
	vortex_timeval_substract (&stop, &start, &result);
	
	printf ("Test 02-f:    ellapsed time to transfer %d bytes (%d Kbytes): %ld secs +  %ld microseconds\n", 
		TEST_03_MSGSIZE * 10, (TEST_03_MSGSIZE * 10) / 1024 , result.tv_sec, result.tv_usec);

	/* check time results if not enabled */
/*	if (! disable_time_checks) {
		if (result.tv_sec >= max_tv_sec && result.tv_usec > max_tv_usec) {
			printf ("Test 02-f:    ERROR: transfer limit delay reached..test failed\n");
			return axl_false;
		} 
		}  */

	return axl_true;
}
	

/** 
 * @brief Check vortex under packet delay conditions.
 * 
 * @return axl_true if tests are passed.
 */
axl_bool  test_02f (void) {

	VortexConnection * connection;
	VortexChannel    * channel;
	int                iterator;
	long               mss;
	char             * message;
	VortexAsyncQueue * queue;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
		
	} /* end if */

	/* create the queue */
	queue = vortex_async_queue_new ();

	mss   = vortex_connection_get_mss (connection);
	printf ("Test 02-f: mss found %ld..\n", mss);
	
	/* create the channel */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);
	/* check channel returned */
	if (channel == NULL) {
		printf ("Unable to create the channel, failed to create channel..\n");
		return axl_false;
	}

	/* build the message to be used */
	message = axl_new (char, TEST_03_MSGSIZE);
	for (iterator = 0; iterator < TEST_03_MSGSIZE; iterator++)
		message [iterator] = (iterator % 3);

	/* check no delay escenario */
	printf ("Test 02-f: checking no delay scenario..\n");
	if (! test_02f_send_data (channel, message, queue, 1, 270000))
		return axl_false;

	/* free queue and message */
	vortex_async_queue_unref (queue);
	axl_free (message);

	/* ok, close the connection, for that, restore previous
	handler */
	printf ("Test 02-f: restoring default handler to close connection..\n");
	if (! vortex_connection_close (connection)) {
		printf ("failed to close the BEEP session\n");
		return axl_false;
	} /* end if */
	

	/* return axl_true */
	return axl_true;
}

int test_02g_frame_size_64 (VortexChannel *channel, int next_seq_no, int message_size, int max_seq_no, axlPointer user_data) 
{
	int result = VORTEX_MIN (PTR_TO_INT(user_data), VORTEX_MIN (message_size, max_seq_no - next_seq_no + 1));
	if (result > 64) {
		printf ("ERROR: test is failing, supposed to segment frames into, at maximum, 64 bytes of payload..\n");
	}
	return result;
}

int test_02g_frame_size (VortexChannel *channel, int next_seq_no, int message_size, int max_seq_no, axlPointer user_data) 
{
	return VORTEX_MIN (PTR_TO_INT(user_data), VORTEX_MIN (message_size, max_seq_no - next_seq_no + 1));
}

double test_02g_rate (int bytes, struct timeval result)
{
	double seconds = result.tv_sec + (result.tv_usec / (double) 1000000);
	double kbytes  = bytes / (double) 1024;
	
	return kbytes / seconds;
}

axl_bool  test_02g (void) {

	VortexConnection * connection;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
		
	} /* end if */

	/* configure 512 frame size */
	printf ("Test 02-g: testing 512 frame size (slow)..\n");
	vortex_connection_set_next_frame_size_handler (connection, test_02g_frame_size, INT_TO_PTR(512));

	/* call common implementation */
	if (! test_02_common (connection))
		return axl_false;

	/* configure 2048 frame size */
	printf ("Test 02-g: testing 2048 frame size (faster, but more content sent)..\n");
	vortex_connection_set_next_frame_size_handler (connection, test_02g_frame_size, INT_TO_PTR(2048));

	/* call common implementation */
	if (! test_02_common (connection))
		return axl_false;

	/* call to common implementation */
	if (! test_03_common (connection)) 
		return axl_false;
	
	/* ok, close the connection */
	if (! vortex_connection_close (connection)) {
		printf ("failed to close the BEEP session\n");
		return axl_false;
	} /* end if */

	/* now configure global frame segmentation function */
	printf ("Test 02-g: checking globally configured frame segmentator (including greetings..)\n");
	printf ("Test 02-g: doing 64 frame size segmentation..\n");
	vortex_connection_set_default_next_frame_size_handler (ctx, test_02g_frame_size_64, INT_TO_PTR (64));
	
	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
		
	} /* end if */

	printf ("Test 02-g: connection ok, running tests..\n");

	/* call common implementation */
	if (! test_02_common (connection))
		return axl_false;

	/* ok, close the connection */
	if (! vortex_connection_close (connection)) {
		printf ("failed to close the BEEP session\n");
		return axl_false;
	} /* end if */

	/* now configure global frame segmentation function */
	vortex_connection_set_default_next_frame_size_handler (ctx, NULL, NULL);

	/* return axl_true */
	return axl_true;
}

#define TEST_02I_MESSAGES (100)

axl_bool  test_02i (void) {

	VortexConnection * connection;
	VortexChannel    * channel;
	int                iterator;
	VortexAsyncQueue * queue;
	VortexFrame      * frame;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
		
	} /* end if */

	/* create a channel */
	queue   = vortex_async_queue_new ();
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI_ORDERED_DELIVERY,
				      /* no close handling */
				      NULL, NULL,
				      /* no frame received */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);

	if (channel == NULL) {
		printf ("ERROR: unable to create channel to check enforced server side ordered delivery..\n");
		return axl_false;
	}

	/* send one thousand messages */
	iterator = 0;
	while (iterator < TEST_02I_MESSAGES) {
		/* send the message */
		if (! vortex_channel_send_msg (channel, "this is a test", 14, NULL)) {
			printf ("ERROR: failed to send message to check ordered delivery (iterator=%d)..\n", iterator);
			return axl_false;
		} /* end if */

		if (! vortex_connection_is_ok (connection, axl_false)) {
			printf ("ERROR: expected to find connection status = ok, but failure was found during ordered delivery checking..\n");
			return axl_false;
		} /* end if */

		/* next iterator */
		iterator++;
	}

	/* get all messages */
	iterator = 0;
	while (iterator < TEST_02I_MESSAGES) {

		frame = vortex_async_queue_pop (queue);
		iterator++;

		/* free frame */
		if (frame != NULL) {
			vortex_frame_unref (frame);
		}

		if (! vortex_connection_is_ok (connection, axl_false)) {
			printf ("ERROR (2): expected to find connection status = ok, but failure was found during ordered delivery checking..\n");
			return axl_false;
		} /* end if */

	} /* end if */
	
	/* check messages replies received */
	if (iterator != TEST_02I_MESSAGES) {
		printf ("ERROR (3): failed to check number of items that should be expected (%d != 1000)\n", iterator);
		return axl_false;
	}

	if (! vortex_connection_is_ok (connection, axl_false)) {
		printf ("ERROR (4): expected to find connection status = ok, but failure was found during ordered delivery checking..\n");
		return axl_false;
	} /* end if */

	/* unref the queue */
	vortex_async_queue_unref (queue);
	
	/* ok, close the connection */
	if (! vortex_connection_close (connection)) {
		printf ("failed to close the BEEP session\n");
		return axl_false;
	} /* end if */
	
	/* return axl_true */
	return axl_true;
}


axl_bool  test_02j (void) {

	VortexConnection * connection;
	VortexChannel    * channel;
	VortexFrame      * frame;
	VortexAsyncQueue * queue;

	/****** CONNECTION BROKEN DURING CLOSE OPERATION ******/
	printf ("Test 02-j: checking connection broken during close operation..\n");
	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
		
	} /* end if */

	/* create a channel */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI_SUDDENTLY_CLOSE,
				      /* no close handling */
				      NULL, NULL,
				      /* no frame received */
				      NULL, NULL,
				      /* no async channel creation */
				      NULL, NULL);

	if (channel == NULL) {
		printf ("ERROR: unable to create channel to check enforced server side ordered delivery..\n");
		return axl_false;
	}


	/* close the channel */
	vortex_channel_close (channel, NULL);

	/* ok, close the connection */
	if (! vortex_connection_close (connection)) {
		printf ("failed to close the BEEP session\n");
		return axl_false;
	} /* end if */

	/****** CONNECTION BROKEN DURING CLOSE OPERATION WITH DELAY ******/
	printf ("Test 02-j: checking connection broken during close operation with delay..\n");
	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
		
	} /* end if */

	/* create a channel */
	channel = vortex_channel_new_full (connection, 0,
					   /* serverName */
					   NULL,
					   REGRESSION_URI_SUDDENTLY_CLOSE,
					   /* profile content encoding and profile content */
					   EncodingNone, "1", 1,
					   /* no close handling */
					   NULL, NULL,
					   /* no frame received */
					   NULL, NULL,
					   /* no async channel creation */
					   NULL, NULL);

	if (channel == NULL) {
		printf ("ERROR: unable to create channel to check enforced server side ordered delivery..\n");
		return axl_false;
	}


	/* close the channel */
	vortex_channel_close (channel, NULL);

	/* ok, close the connection */
	if (! vortex_connection_close (connection)) {
		printf ("failed to close the BEEP session\n");
		return axl_false;
	} /* end if */

	/****** CONNECTION BROKEN DURING START OPERATION ******/
	printf ("Test 02-j: checking connection broken during start operation..\n");
	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
		
	} /* end if */

	/* create a channel (support for broken channel start) */
	channel = vortex_channel_new_full (connection, 0,
					   /* serverName */
					   NULL,
					   REGRESSION_URI_SUDDENTLY_CLOSE,
					   /* profile content encoding and profile content */
					   EncodingNone, "2", 1,
					   /* no close handling */
					   NULL, NULL,
					   /* no frame received */
					   NULL, NULL,
					   /* no async channel creation */
					   NULL, NULL);

	if (channel != NULL) {
		printf ("ERROR: expected NULL channel reference..\n");
		return axl_false;
	}

	/* ok, close the connection */
	if (! vortex_connection_close (connection)) {
		printf ("failed to close the BEEP session\n");
		return axl_false;
	} /* end if */

	/****** CONNECTION BROKEN DURING FRAME RECEPTION OPERATION ******/
	printf ("Test 02-j: checking connection broken during frame reception operation..\n");
	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
		
	} /* end if */

	/* create a channel (support for broken channel start) */
	queue   = vortex_async_queue_new ();
	channel = vortex_channel_new_full (connection, 0,
					   /* serverName */
					   NULL,
					   REGRESSION_URI_SUDDENTLY_CLOSE,
					   /* profile content encoding and profile content */
					   EncodingNone, "3", 1,
					   /* no close handling */
					   NULL, NULL,
					   /* no frame received */
					   NULL, NULL,
					   /* no async channel creation */
					   NULL, NULL);
	if (channel == NULL) {
		printf ("ERROR: expected NULL channel reference..\n");
		return axl_false;
	}
	
	/* send a message */
	if (! vortex_channel_send_msg (channel, "this is a test", 14, NULL)) {
		printf ("ERROR: expected to be able to send a message ..\n");
		return axl_false;
	}

	/* get the reply (it should not come and it should not block
	 * us) */
	printf ("Test 02-j: getting reply..");
	frame = vortex_channel_get_reply (channel, queue);
	if (frame != NULL) {
		printf ("ERROR: expected to receive a NULL reply but received content..\n");
		return axl_false;
	}
	printf ("ok\n");

	/* unref queue */
	vortex_async_queue_unref (queue);

	/* ok, close the connection */
	if (! vortex_connection_close (connection)) {
		printf ("failed to close the BEEP session\n");
		return axl_false;
	} /* end if */

	/* operation completed */
	return axl_true;
}

axl_bool  test_02k (void) {

	VortexConnection * connection;
	VortexChannel    * channel;
	VortexFrame      * frame; 
	VortexAsyncQueue * queue;
	int                iterator;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
		
	} /* end if */

	/* create a channel */
	queue   = vortex_async_queue_new ();
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI_MIXING_REPLIES,
				      /* no close handling */
				      NULL, NULL,
				      /* no frame received */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);

	if (channel == NULL) {
		printf ("ERROR: unable to create channel to check mixed replies types (RPY + ANS..NUL..\n");
		return axl_false;
	}

	/* set serialize */
	vortex_channel_set_serialize (channel, axl_true);

	/* send frame */
	iterator = 0;
	while (iterator < 10) {
		/* send a message */
		vortex_channel_send_msg (channel, "this is a test", 14, NULL);

		/* get a reply */
		frame = vortex_channel_get_reply (channel, queue);
		if (frame == NULL) {
			printf ("ERROR: expected reply from remote side while running mixed replies tests..\n");
		}
		
		/* check connection status */
		if (! vortex_connection_is_ok (connection, axl_false)) {
			printf ("ERROR: found connection broken while running mixed replies test\n");
			return axl_false;
		} /* end if */

		/* check reply content */
		switch (vortex_frame_get_type (frame)) {
		case VORTEX_FRAME_TYPE_RPY:
			if (! axl_cmp (vortex_frame_get_payload (frame), "a reply")) {
				printf ("ERROR: failed to check content expected inside frame (RPY type)..\n");
				return axl_false;
			}
			break;
		case VORTEX_FRAME_TYPE_ANS:
			if (! axl_cmp (vortex_frame_get_payload (frame), "a reply 1")) {
				printf ("ERROR: failed to check content expected inside frame (ANS type)..\n");
				return axl_false;
			}

			/* release frame */
			vortex_frame_unref (frame);

			/* get next reply received */
			frame = vortex_channel_get_reply (channel, queue);
			if (! axl_cmp (vortex_frame_get_payload (frame), "a reply 2")) {
				printf ("ERROR: failed to check content expected inside frame (ANS type, second reply)..\n");
				return axl_false;
			} /* end if */

			/* release frame received */
			vortex_frame_unref (frame);

			/* get next reply */
			frame = vortex_channel_get_reply (channel, queue);
			if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_NUL) {
				printf ("ERROR: failed to check content expected inside frame (NUL type)..\n");
				return axl_false;
			}

			break;
		default:
			/* not handled case */
			break;
		} /* end switch */

		/* release frame received */
		vortex_frame_unref (frame);

		/* check connection status */
		if (! vortex_connection_is_ok (connection, axl_false)) {
			printf ("ERROR: found connection broken while running mixed replies test (second check)\n");
			return axl_false;
		} /* end if */

		/* next step */
		iterator++;

	} /* end while */

	/* close the channel */
	vortex_channel_close (channel, NULL);

	/* ok, close the connection */
	if (! vortex_connection_close (connection)) {
		printf ("failed to close the BEEP session\n");
		return axl_false;
	} /* end if */

	/* terminate queue */
	vortex_async_queue_unref (queue);

	/* operation completed */
	return axl_true;
}

axl_bool  test_02m (void) {

	VortexConnection * connection;
	VortexChannel    * channel;
	VortexFrame      * frame; 
	VortexAsyncQueue * queue;
	int                iterator;
	int                code;
	char             * msg;
	int                count = 0;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
		
	} /* end if */

	/* create a channel */
	queue   = vortex_async_queue_new ();
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI_CLOSE_AFTER_ANS_NUL_REPLIES,
				      /* no close handling */
				      NULL, NULL,
				      /* no frame received */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);

	if (channel == NULL) {
		printf ("ERROR: unable to create channel to check connection close after ANS..NUL reply\n");
		return axl_false;
	} /* end if */

	/* get a reference to the channel to avoid close conditions
	 * because remote side will send a close operation */
	vortex_channel_ref (channel);

	/* send a message to start retrieval */
	if (! vortex_channel_send_msg (channel, "get-content", 11, NULL)) {
		printf ("ERROR: failed to send first message to get list of ans-nul replies\n");
		return axl_false;
	}

	/* get all replies */
	iterator = 0;
	while (iterator < 10000) {
		/* get a reply */
		frame = vortex_channel_get_reply (channel, queue);
		if (frame == NULL) {
			printf ("ERROR: expected reply from remote side while running mixed replies tests..\n");
			return axl_false;
		}

		/* check reply type */
		if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_ANS) {
			printf ("ERROR: expected to receive a ANS frame (iterator=%d), type found: %d..\n", 
				iterator, vortex_frame_get_type (frame));
			return axl_false;
		}

		/* check content received */
		if (! axl_cmp (vortex_frame_get_payload (frame), TEST_REGRESION_URI_4_MESSAGE)) {
			printf ("ERROR: expected to find different content than received..\n");
			return axl_false;
		}

		/* update count */
		count += vortex_frame_get_payload_size (frame);
	
		/* release frame */
		vortex_frame_unref (frame);

		/* check channel */
		if (! vortex_channel_is_opened (channel)) {
			printf ("ERROR: expected to find channel opened..\n");
			return axl_false;
		}

		/* check connection */
		if (! vortex_connection_is_ok (connection, axl_false)) {
			printf ("ERROR: expected to find connection opened..\n");
			return axl_false;
		} 
	
		/* update iterator */
		iterator++;
	} /* end while */

	printf ("Test 02m: bytes transferred %d..\n", count);

	/* get a reply */
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("ERROR: expected reply from remote side while running mixed replies tests..\n");
		return axl_false;
	}
	
	/* check reply type */
	if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_NUL) {
		printf ("ERROR: expected to receive a MSG frame..\n");
		return axl_false;
	}

	/* unref nul reply */
	vortex_frame_unref (frame);
	
	/* try to close the channel */
	/* vortex_log_enable (axl_true);
	   vortex_color_log_enable (axl_true); */
	if (! vortex_channel_close (channel, NULL)) {
		printf ("ERROR: failed to close channel: ..\n");

		/* close operation have failed */
		while (vortex_connection_pop_channel_error (connection, &code, &msg)) {
			/* drop a error message */
			printf ("Close failed, error was: code=%d, %s\n",
				code, msg);
			/* dealloc resources */
			axl_free (msg);
		} /* end while */

		return axl_false;
	} /* end if */

	/* ok, close the connection */
	if (! vortex_connection_close (connection)) {
		printf ("failed to close the BEEP session\n");
		return axl_false;
	} /* end if */

	/* terminate queue */
	vortex_async_queue_unref (queue);

	/* ..and channel reference */
	vortex_channel_unref (channel);

	/* operation completed */
	return axl_true;
}

int test_02m1_frame_sizer (VortexChannel *channel, 
			   int            next_seq_no, 
			   int            message_size, 
			   int            max_seq_no, 
			   axlPointer     user_data) 
{
	/* use default implementation */
	if ((next_seq_no + message_size) > max_seq_no)
		return VORTEX_MIN (max_seq_no - next_seq_no + 1, 32768);
	return VORTEX_MIN (message_size, 32768);
}

axl_bool  test_02m1 (void) {

	VortexConnection * connection;
	VortexChannel    * channel;
	struct timeval     start, stop, result;
	char             * content;
	int                size;
	VortexAsyncQueue * queue;
	VortexFrame      * frame;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
	} /* end if */

	/****** test 4096 ******/
	printf ("Test 02-m1: check transfer with big frames\n");

	/* CREATE A CHANNEL */
	queue   = vortex_async_queue_new ();
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI, 
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("Test 02-m1: failed to create channel to perform transfer\n");
		return axl_false;
	}

	/* ASK TO CHANGE WINDOW SIZE */
	if (! vortex_channel_send_msg (channel, "window_size=65536", 17, NULL)) {
		printf ("Test 02-m1: failed to ask for window size change..\n");
		return axl_false;
	} /* end if */

	/* get next reply */
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("Test 02-m1: expected to find frame reply..\n");
		return axl_false;
	}
	/* check reply */
	if (! axl_cmp (vortex_frame_get_payload (frame), "ok")) {
		printf ("Test 02-m1: expected a positive reply to change window size request..\n");
		return axl_false;
	}

	vortex_frame_unref (frame);

	/* change sending step */
	vortex_channel_set_next_frame_size_handler (channel, test_02m1_frame_sizer, NULL);

	/* LOAD FILE CONTENT */
	size    = 0;
	content = vortex_regression_common_read_file ("vortex-regression-client.c", &size);
	if (content == NULL) {
		printf ("ERROR: failed to open file %s to perform test..\n", "vortex-regression-client.c");
		return axl_false;
	} /* end if */

	gettimeofday (&start, NULL);

	/* SEND THE CONTENT */
	if (! vortex_channel_send_msg (channel, content, size, NULL)) {
		printf ("Test 02-m1: failed to send file..\n");
		return axl_false;
	} /* end if */


	gettimeofday (&stop, NULL);
	vortex_timeval_substract (&stop, &start, &result);
	printf ("Test 02-m1: ..transfer %d bytes done in %ld segs + %ld microsegs (window size 65536, step 32768). Waiting reply..\n", 
		size, result.tv_sec, result.tv_usec);

	/* GET NEXT REPLY */
	gettimeofday (&start, NULL);
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("Test 02-m1: expected to find frame reply..\n");
		return axl_false;
	}
	gettimeofday (&stop, NULL);
	vortex_timeval_substract (&stop, &start, &result);
	printf ("Test 02-m1: ..transfer reply %d bytes done in %ld segs + %ld microsegs (window size 65536, step 32768). \n", 
		vortex_frame_get_payload_size (frame), result.tv_sec, result.tv_usec);

	if (! axl_cmp (content, vortex_frame_get_payload (frame))) {
		printf ("Test 02-m1: expected to find different content..\n");
		return axl_false;
	}

	axl_free (content);
	vortex_frame_unref (frame);

	if (! vortex_connection_is_ok (connection, axl_false)) {
		printf ("Test 02-h: ERROR, connection status is not ok before test..\n");
		return axl_false;
	}

	/* ok, close the connection */
	if (! vortex_connection_close (connection)) {
		printf ("failed to close the BEEP session\n");
		return axl_false;
	} /* end if */

	/* terminate queue */
	vortex_async_queue_unref (queue);
	
	/* return axl_true */
	return axl_true;
}

axl_bool  test_02m2 (void) {

	VortexConnection * connection;
	VortexChannel    * channel;
	VortexAsyncQueue * queue;
	int                iterator;
	VortexFrame      * frame;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
	} /* end if */

	printf ("Test 02-m2: checking pending content during connection close\n");

	/* CREATE A CHANNEL */
	queue   = vortex_async_queue_new ();
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI, 
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("Test 02-m1: failed to create channel to perform transfer\n");
		return axl_false;
	}

	/* perform 1000 sendings */
	iterator = 0;
	while (iterator < 3000) {

		/* SEND THE CONTENT */
		if (! vortex_channel_send_msg (channel, TEST_REGRESION_URI_4_MESSAGE, 4096, NULL)) {
			printf ("Test 02-m2: failed to send content..\n");
			return axl_false;
		} /* end if */

		iterator++;

	} /* end while */

	printf ("Test 02-m2: channel ref count=%d..\n", vortex_channel_ref_count (channel));

	/* shutdown the connection */
	vortex_connection_shutdown (connection);

	/* get replies received */
	while (vortex_async_queue_items (queue) > 0) {
		/* get frame reference */
		frame = vortex_channel_get_reply (channel, queue);
		
		/* dealloc frame */
		vortex_frame_unref (frame);
	} /* end while */

	/* ok, close the connection */
	if (! vortex_connection_close (connection)) {
		printf ("failed to close the BEEP session\n");
		return axl_false;
	} /* end if */

	/* terminate queue */
	vortex_async_queue_unref (queue);


	/* return axl_true */
	return axl_true;
}

axl_bool test_02n_check_sequence (VortexChannel * channel, VortexAsyncQueue * queue, 
				  int first, int second, int third, int fourth)
{
	int                msg_no   = -1;
	int                msg_no_used;
	int                iterator = 0;
	VortexFrame      * frame;
	VortexAsyncQueue * sleep_queue = vortex_async_queue_new ();

	/* do sends operations */
	while (iterator < 4) {
		/* select next message number to use */
		msg_no = -1;
		switch (iterator) {
		case 0:
			if (first >= 0)
				msg_no = first;
			break;
		case 1:
			if (second >= 0)
				msg_no = second;
			break;
		case 2:
			if (third >= 0)
				msg_no = third;
			break;
		case 3:
			if (fourth >= 0)
				msg_no = fourth;
			break;
		}

		/* no next send operation */
		if (msg_no == -1)
			break;

		/* perform send operation */
		if (! vortex_channel_send_msg_common (channel, "this is a test", 14, msg_no, &msg_no_used, NULL)) {
			printf ("Failed to send message..\n");
			return axl_false;
		} /* end if */
	
		if (msg_no_used != msg_no) {
			printf ("Requested to perform send operation with %d but found %d..\n", msg_no, msg_no_used);
			return axl_false;
		}
		printf ("Test 02-n: sent message with msgno %d..\n", msg_no);

		/* after each send, wait a bit */
		vortex_async_queue_timedpop (sleep_queue, 2000);

		/* next iterator */
		iterator++;
	} /* end if */
	
	/* receive replies */
	iterator = 0;
	while (iterator < 4) {
		/* select next message number to use */
		msg_no = -1;
		switch (iterator) {
		case 0:
			if (first >= 0)
				msg_no = first;
			break;
		case 1:
			if (second >= 0)
				msg_no = second;
			break;
		case 2:
			if (third >= 0)
				msg_no = third;
			break;
		case 3:
			if (fourth >= 0)
				msg_no = fourth;
			break;
		}

		/* no next receive operation */
		if (msg_no == -1)
			break;

		frame = vortex_channel_get_reply (channel, queue);
		if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY) {
			printf ("Expected to receive RPY type to message no 0..\n");
			return axl_false;
		}
		if (vortex_frame_get_msgno (frame) != msg_no) {
			printf ("Expected to receive RPY message with msgno equal to %d, but received: %d..\n",
				msg_no, vortex_frame_get_msgno (frame));
			return axl_false;
		}
		vortex_frame_unref (frame);
		printf ("Test 02-n: received message with msgno %d..\n", msg_no);

		/* next iterator */
		iterator++;
	} /* end if */

	vortex_async_queue_unref (sleep_queue);
	
	return axl_true;
}

axl_bool  test_02n (void) {
	VortexConnection  * connection;
	VortexChannel     * channel;
	VortexAsyncQueue  * queue;
	VortexFrame       * frame;
	int                 msg_no = 0;
	int                 iterator;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
	}

	/* create the queue */
	queue   = vortex_async_queue_new ();

	/* create a channel */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("Unable to create the channel..");
		return axl_false;
	}

	/* now perform tree sends operations */
	if (! vortex_channel_send_msg_common (channel, "this is a test", 14, 5, &msg_no, NULL)) {
		printf ("Failed to send message..\n");
		return axl_false;
	} /* end if */

	/* check msg no returned */
	if (msg_no != 5) {
		printf ("Expected to recevied another value for msg_no: %d != 5\n",
			msg_no);
		return axl_false;
	}

	/* now perform tree sends operations */
	if (! vortex_channel_send_msg_common (channel, "this is a test", 14, 659, &msg_no, NULL)) {
		printf ("Failed to send message..\n");
		return axl_false;
	} /* end if */

	/* check msg no returned */
	if (msg_no != 659) {
		printf ("Expected to recevied another value for msg_no: %d != 659\n",
			msg_no);
		return axl_false;
	}

	/* now perform tree sends operations */
	if (! vortex_channel_send_msg_common (channel, "this is a test", 14, 3, &msg_no, NULL)) {
		printf ("Failed to send message..\n");
		return axl_false;
	} /* end if */

	/* check msg no returned */
	if (msg_no != 3) {
		printf ("Expected to recevied another value for msg_no: %d != 3\n",
			msg_no);
		return axl_false;
	}

	/* wait for frames received */
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("Expected to receive reply to frame msgno 5..\n");
		return axl_false;
	} /* end if */
	if (vortex_frame_get_msgno (frame) != 5) {
		printf ("Expected to receive reply to frame msgno 5 but received: %d..\n",
			vortex_frame_get_msgno (frame));
		return axl_false;
	} /* end if */
	vortex_frame_unref (frame);

	/* wait for frames received */
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("Expected to receive reply to frame msgno 659..\n");
		return axl_false;
	} /* end if */
	if (vortex_frame_get_msgno (frame) != 659) {
		printf ("Expected to receive reply to frame msgno 659 but received: %d..\n",
			vortex_frame_get_msgno (frame));
		return axl_false;
	} /* end if */
	vortex_frame_unref (frame);

	/* wait for frames received */
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("Expected to receive reply to frame msgno 3..\n");
		return axl_false;
	} /* end if */
	if (vortex_frame_get_msgno (frame) != 3) {
		printf ("Expected to receive reply to frame msgno 3 but received: %d..\n",
			vortex_frame_get_msgno (frame));
		return axl_false;
	} /* end if */
	vortex_frame_unref (frame);

	/* also check with large message with complete flag
	 * disabled */
	iterator = 0;

	while (iterator < 10) {
		/* send content with message number 0 */
		if (! vortex_channel_send_msg_common (channel, TEST_REGRESION_URI_4_MESSAGE, strlen (TEST_REGRESION_URI_4_MESSAGE), 0, &msg_no, NULL)) {
			printf ("Failed to send message..\n");
			return axl_false;
		} /* end if */

		if (msg_no != 0) {
			printf ("Requested to perform send operation with 0 but found %d..\n", msg_no);
			return axl_false;
		}

		/* wait for frames received */
		frame = vortex_channel_get_reply (channel, queue);
		if (frame == NULL) {
			printf ("Expected to receive reply to frame msgno 3..\n");
			return axl_false;
		} /* end if */
		if (vortex_frame_get_msgno (frame) != 0) {
			printf ("Expected to receive reply to frame msgno 0 but received: %d..\n",
				vortex_frame_get_msgno (frame));
			return axl_false;
		} /* end if */

		/* check content */
		if (! axl_cmp (vortex_frame_get_payload (frame), TEST_REGRESION_URI_4_MESSAGE)) {
			printf ("Expected to find test content but found something different..\n");
			return axl_false;
		} /* end if */

		vortex_frame_unref (frame);

		/* next iterator */
		iterator++;

	} /* end while */

	/**** now test message numbers sequence 0,1 ****/
	printf ("Test 02-n: check msg no sequence 0,1,2...\n");
	if (! test_02n_check_sequence (channel, queue, 0, 1, 2, -1))
		return axl_false;

	printf ("Test 02-n: check msg no sequence 1,2,3...\n");
	if (! test_02n_check_sequence (channel, queue, 1, 2, 3, -1))
		return axl_false;
	
	printf ("Test 02-n: check msg no sequence 2147483646, 2147483647, 0, 1...\n");
	if (! test_02n_check_sequence (channel, queue, 2147483646, 2147483647, 0, 1))
		return axl_false;

	printf ("Test 02-n: check msg no sequence 7,5,3,1...\n");
	if (! test_02n_check_sequence (channel, queue, 7,5,3,1))
		return axl_false;

	printf ("Test 02-n: check msg no sequence 1, 0, 2147483647, 2147483646...\n");
	if (! test_02n_check_sequence (channel, queue, 1, 0, 2147483647, 2147483646))
		return axl_false;

	/* ok, close the channel */
	vortex_channel_close (channel, NULL);

	/* ok, close the connection */
	vortex_connection_close (connection);

	/* free queue */
	vortex_async_queue_unref (queue);

	/* now open a channel and check connection broken if reused a
	 * msg no that wasn't replied */
	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
	}

	/* create a channel */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI_NOTHING,
				      /* no close handler */
				      NULL, NULL,
				      /* no frame received */
				      NULL, NULL,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("Failed to create nothing channel..\n");
		return axl_false;
	} /* end if */

	/* perform two send operations to force connection broken */
	if (! vortex_channel_send_msg_common (channel, TEST_REGRESION_URI_4_MESSAGE, strlen (TEST_REGRESION_URI_4_MESSAGE), 0, &msg_no, NULL)) {
		printf ("Failed to send message..\n");
		return axl_false;
	} /* end if */

	/* perform two send operations to force connection broken */
	if (! vortex_channel_send_msg_common (channel, TEST_REGRESION_URI_4_MESSAGE, strlen (TEST_REGRESION_URI_4_MESSAGE), 0, &msg_no, NULL)) {
		printf ("Failed to send message..\n");
		return axl_false;
	} /* end if */

	/* wait a bit */
	queue = vortex_async_queue_new ();
	vortex_async_queue_timedpop (queue, 20000);
	vortex_async_queue_unref (queue);

	/* check connection */
	if (vortex_connection_is_ok (connection, axl_false)) {
		printf ("Expected to find a failure in the connection after reunsing MSG numbers not replied..\n");
		return axl_false;
	} /* end if */

	vortex_connection_close (connection);

	/* return axl_true */
	return axl_true;
}

axl_bool  test_02o (void) {
	VortexConnection  * connection;
	VortexChannel     * channel;
	VortexAsyncQueue  * queue;
	VortexFrame       * frame;
	unsigned int        seq_no_sent;
	char              * file_content;
	int                 file_size;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
	}

	/* create the queue */
	queue   = vortex_async_queue_new ();

	/* create a channel */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI_SEQNO_EXCEEDED,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("Unable to create the channel..");
		return axl_false;
	}

	/*******************************
	 ***  FIRST PHASE: 2GB LIMIT ***
	 *******************************/
	/* send a message  */
	printf ("Test 02-o: simulating send operation of 2GB - 4096 bytes..\n");
	if (! vortex_channel_send_msg (channel, "first message", 13, NULL)) {
		printf ("Failed to send first message..\n");
		return axl_false;
	} /* end if */

	/* wait for the reply */
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL || vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY || ! axl_cmp (vortex_frame_get_payload (frame), "first message")) {
		printf (" (0) Expected to find a particular reply for first message but received something different..\n");
		printf (" frame == NULL: %d\n", frame == NULL);
		printf (" vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY: %d\n", vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY);
		printf (" ! axl_cmp (vortex_frame_get_payload (frame), \"first message\"): %d\n", ! axl_cmp (vortex_frame_get_payload (frame), "first message"));

		return axl_false;
	}
	vortex_frame_unref (frame);

	/* now simulate we have sent until now 2GB - 4096 bytes = 2147479552 */ 
	printf ("Test 02-o: updating internal counters for: 2GB - 4096 bytes = 2147479552..\n");
	/* the following function requires to also take into
	 * consideration previous send. Because we have sent 13 + 2
	 * bytes (due to mime headers) we provide the value:
	 * 2147479552 - 15 = 2147479537 to simulate 2GB - 4096 sent. */
	vortex_channel_update_status (channel, 2147479537, 0, UPDATE_SEQ_NO);
	vortex_channel_update_remote_incoming_buffer (channel, 2147479552, 4096);
	vortex_channel_set_next_seq_no (channel, 2147479552);

	/* send a new message with 4096 and then a new one  */
	printf ("Test 02-o: sending 4k additional content..\n");
	if (! vortex_channel_send_msg (channel, TEST_REGRESION_URI_4_MESSAGE, 4096, NULL)) {
		printf ("Failed to send 4k message after first message..\n");
		return axl_false;
	} /* end if */

	/* check connection */
	printf ("Test 02-o: checking connection status..\n");
	if (! vortex_connection_is_ok (connection, axl_false)) {
		printf ("Expected to find connection status ok, but found an error..\n");
		return axl_false;
	}

	/* wait for the reply */
	printf ("Test 02-o: getting first reply..\n");
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL || vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY || ! axl_cmp (vortex_frame_get_payload (frame), TEST_REGRESION_URI_4_MESSAGE)) {
		printf (" (1) Expected to find a particular reply for first message but received something different..\n");
		printf (" frame == NULL: %d\n", frame == NULL);
		printf (" vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY: %d\n", vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY);
		printf (" ! axl_cmp (vortex_frame_get_payload (frame), TEST_REGRESSION_URI_4_MESSAGE): %d\n", ! axl_cmp (vortex_frame_get_payload (frame), TEST_REGRESION_URI_4_MESSAGE));
		return axl_false;
	}
	vortex_frame_unref (frame);

	/* check next_seq_no */
	/* NOTE: 4096 + 2 (due to MIME headers) */
	if (vortex_channel_get_next_seq_no (channel) != ((unsigned int) 2147479552 + 4096 + 2)) {
		printf ("(1) Expected to find next sequence number to use, but something different was found: %u != %u\n",
			vortex_channel_get_next_seq_no (channel) , ((unsigned int) 2147479552 + 4096 + 2));
		return axl_false;
	}

	/* ...and the second one  */
	printf ("Test 02-o: sending 4k additional content..\n");
	if (! vortex_channel_send_msg (channel, TEST_REGRESION_URI_4_MESSAGE, 4096, NULL)) {
		printf ("Failed to send 4k message after first message..\n");
		return axl_false;
	} /* end if */

	/* wait for the reply */
	printf ("Test 02-o: second reply..\n");
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL || vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY || ! axl_cmp (vortex_frame_get_payload (frame), TEST_REGRESION_URI_4_MESSAGE)) {
		printf (" (2) Expected to find a particular reply for first message but received something different..\n");
		printf (" frame == NULL: %d\n", frame == NULL);
		printf (" vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY: %d\n", vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY);
		printf (" ! axl_cmp (vortex_frame_get_payload (frame), TEST_REGRESSION_URI_4_MESSAGE): %d\n", ! axl_cmp (vortex_frame_get_payload (frame), TEST_REGRESION_URI_4_MESSAGE));
		return axl_false;
	}
	vortex_frame_unref (frame);

	/* check next_seq_no */
	/* NOTE: 4096 + 4 (due to MIME headers) */
	if (vortex_channel_get_next_seq_no (channel) != ((unsigned int) 2147479552 + 4096 + 4096 + 4)) {
		printf ("(2) Expected to find next sequence number to use, but something different was found: %u != %u\n",
			vortex_channel_get_next_seq_no (channel) , ((unsigned int) 2147479552 + 4096 + 4096 + 4));
		return axl_false;
	}

	/* check connection */
	printf ("Test 02-o: checking connection status..\n");
	if (! vortex_connection_is_ok (connection, axl_false)) {
		printf ("Expected to find connection status ok, but found an error..\n");
		return axl_false;
	}

	/*******************************
	 *** SECOND PHASE: 4GB LIMIT ***
	 *******************************/
	/* send a message  */
	printf ("Test 02-o: simulating send operation of 4GB - 4096 bytes..\n");
	if (! vortex_channel_send_msg (channel, "second message", 14, NULL)) {
		printf ("Failed to send first message..\n");
		return axl_false;
	} /* end if */

	/* wait for the reply */
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL || vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY || ! axl_cmp (vortex_frame_get_payload (frame), "second message")) {
		printf (" (3) Expected to find a particular reply for first message but received something different..\n");
		printf (" frame == NULL: %d\n", frame == NULL);
		printf (" vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY: %d\n", vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY);
		printf ("! axl_cmp (vortex_frame_get_payload (frame), \"second message\"): %d", ! axl_cmp (vortex_frame_get_payload (frame), "second message"));
		return axl_false;
	}
	vortex_frame_unref (frame);

	/* now simulate we have sent until now 4GB - 4096 bytes = 4294963200 */ 
	/* To simulate this we have sent until now: 2147479552 + 4096 + 4096 = 2147487744 
	   So, to simulate we have sent 4294963200 we need to provide the following value: 
	   4294963200 - 2147487744 - 20 = 2147475436 
	   The value 20 comes from: 14 + 2 ("second message" + mime headers) and 2 mime headers added to previous messages (2 +2) */
	printf ("Test 02-o: updating internal counters to similate content sent: 4GB - 4096 bytes = 4294963200\n");
	seq_no_sent = ((unsigned int) 1024 * 1024 * 1024 * 4) - 4096;
	vortex_channel_update_status (channel, (unsigned int) 2147475436, 0, UPDATE_SEQ_NO);
	vortex_channel_update_remote_incoming_buffer (channel, seq_no_sent , 4096);
	vortex_channel_set_next_seq_no (channel, seq_no_sent);
	

	/* send a new message with 4096 and then a new one  */
	printf ("Test 02-o: sending 4k additional content..\n");
	if (! vortex_channel_send_msg (channel, TEST_REGRESION_URI_4_MESSAGE, 4096, NULL)) {
		printf ("Failed to send 4k message after first message..\n");
		return axl_false;
	} /* end if */

	/* ...and the second one  */
	printf ("Test 02-o: sending 4k additional content..\n");
	if (! vortex_channel_send_msg (channel, TEST_REGRESION_URI_4_MESSAGE, 4096, NULL)) {
		printf ("Failed to send 4k message after first message..\n");
		return axl_false;
	} /* end if */

	/* check connection */
	printf ("Test 02-o: checking connection status..\n");
	if (! vortex_connection_is_ok (connection, axl_false)) {
		printf ("Expected to find connection status ok, but found an error..\n");
		return axl_false;
	}

	/* wait for the reply */
	printf ("Test 02-o: getting first reply..\n");
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL || vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY || ! axl_cmp (vortex_frame_get_payload (frame), TEST_REGRESION_URI_4_MESSAGE)) {
		printf ("Expected to find a particular reply for first message but received something different..\n");
		return axl_false;
	}
	vortex_frame_unref (frame);

	/* wait for the reply */
	printf ("Test 02-o: second reply..\n");
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL || vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY || ! axl_cmp (vortex_frame_get_payload (frame), TEST_REGRESION_URI_4_MESSAGE)) {
		printf ("Expected to find a particular reply for first message but received something different..\n");
		return axl_false;
	}
	vortex_frame_unref (frame);

	/* check connection */
	printf ("Test 02-o: checking connection status..\n");
	if (! vortex_connection_is_ok (connection, axl_false)) {
		printf ("Expected to find connection status ok, but found an error..\n");
		return axl_false;
	}

	/* ok, close the channel */
	vortex_channel_close (channel, NULL);

	/* ok, close the connection */
	vortex_connection_close (connection);

	printf ("Test 02-o: now check transfer..\n");
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
	}

	/* create a channel */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI_SEQNO_EXCEEDED,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("Unable to create the channel..");
		return axl_false;
	}

	/* now request remote side to set its seqno received to (2^32-1) - 100000 */
	printf ("Test 02-o: sending request to change..\n");
	if (! vortex_channel_send_msg (channel, "set-to=4294867295", 17, NULL)) {
		printf ("Failed to send 4k message after first message..\n");
		return axl_false;
	} /* end if */

	/* check connection */
	printf ("Test 02-o: checking connection status after requesting change..\n");
	if (! vortex_connection_is_ok (connection, axl_false)) {
		printf ("Expected to find connection status ok, but found an error..\n");
		return axl_false;
	}

	/* wait for the reply */
	printf ("Test 02-o: getting first reply..\n");
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL || vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY || ! axl_cmp (vortex_frame_get_payload (frame), "set-to=4294867295")) {
		printf ("Expected to find a particular reply for first message but received something different..\n");
		return axl_false;
	}
	vortex_frame_unref (frame);

	/* read file content */
	file_content = test_04_read_file ("vortex-regression-client.o", &file_size);

	/* now update internal counters to simulate we have
	   transferred until now 4294867295 */
	vortex_channel_update_status (channel, 4294867295, 0, UPDATE_SEQ_NO);
	vortex_channel_update_remote_incoming_buffer (channel, 4294867295, 4096);
	vortex_channel_set_next_seq_no (channel, 4294867295);
	
	/* send content and wait reply */
	printf ("Test 02-o: Sending file content (vortex-regression-client.o) with size: %d\n", file_size);
	if (! vortex_channel_send_msg (channel, file_content, file_size, NULL)) {
		printf ("Failed to send file..\n");
		return axl_false;
	}
	
	/* now wait for the reply */
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL || vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY || ! axl_cmp (vortex_frame_get_payload (frame), file_content)) {
		printf ("Expected to find a particular reply for first message but received something different..\n");
		return axl_false;
	}
	vortex_frame_unref (frame);

	/* free file content */
	axl_free (file_content);

	/* free queue */
	vortex_async_queue_unref (queue);

	return axl_true;
}

/** 
 * @brief Checks BEEP support to send large messages that goes beyond
 * default window size advertised.
 * 
 * 
 * @return axl_true if the test is ok, otherwise axl_false is returned.
 */
axl_bool  test_03 (void) {
	VortexConnection * connection;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
	}

	/* call to common implementation */
	if (! test_03_common (connection)) 
		return axl_false;

	/* ok, close the connection */
	vortex_connection_close (connection);

	/* return axl_true */
	return axl_true;
}

axl_bool  test_02h (void) {

	VortexConnection * connection;
	struct timeval     start, stop, result;
	int                amount;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
	} /* end if */

	/****** test 4096 ******/
	printf ("Test 02-h: check to perform a transfer updated the default window size to 4096, step 4096\n");
	gettimeofday (&start, NULL);
	/* call to base implementation */
	if (! test_04_ab_common (connection, -1, "Test 02-h::", &amount, 4, axl_false))
		return axl_false;
	gettimeofday (&stop, NULL);
	vortex_timeval_substract (&stop, &start, &result);
	printf ("Test 02-h: ..transfer %d bytes done in %ld segs + %ld microsegs (window size 4096, step 4096.\n", amount, result.tv_sec, result.tv_usec);
	if (! vortex_connection_is_ok (connection, axl_false)) {
		printf ("Test 02-h: ERROR, connection status is not ok before test..\n");
		return axl_false;
	}
	printf ("Test 02-h:    download rate at %.2f KBytes/segs..\n", test_02g_rate (amount, result));

	/****** test 8192 *******/
	printf ("Test 02-h: check to perform a transfer updated the default window size to 8192, step 4096\n");
	gettimeofday (&start, NULL);
	/* call to base implementation */
	if (! test_04_ab_common (connection, 8192, "Test 02-h::", &amount, 4, axl_false))
		return axl_false;
	gettimeofday (&stop, NULL);
	vortex_timeval_substract (&stop, &start, &result);
	printf ("Test 02-h: ..transfer %d bytes done in %ld segs + %ld microsegs (window size 8192, step 4096.\n", amount, result.tv_sec, result.tv_usec);
	if (! vortex_connection_is_ok (connection, axl_false)) {
		printf ("Test 02-h: ERROR, connection status is not ok before test..\n");
		return axl_false;
	}
	printf ("Test 02-h:    download rate at %.2f KBytes/segs..\n", test_02g_rate (amount, result));
	
	/****** test 16384 ******/
	printf ("Test 02-h: check to perform a transfer updated the default window size to 16384, step 4096\n");
	gettimeofday (&start, NULL);
	/* call to base implementation */
	if (! test_04_ab_common (connection, 16384, "Test 02-h::", &amount, 4, axl_false))
		return axl_false;
	gettimeofday (&stop, NULL);
	vortex_timeval_substract (&stop, &start, &result);
	printf ("Test 02-h: ..transfer %d bytes done in %ld segs + %ld microsegs (window size 16384, step 4096.\n", amount, result.tv_sec, result.tv_usec);
	if (! vortex_connection_is_ok (connection, axl_false)) {
		printf ("Test 02-h: ERROR, connection status is not ok before test..\n");
		return axl_false;
	}
	printf ("Test 02-h:    download rate at %.2f KBytes/segs..\n", test_02g_rate (amount, result));

	/****** test 32768 ******/
	printf ("Test 02-h: check to perform a transfer updated the default window size to 32768, step 4096\n");
	gettimeofday (&start, NULL);
	/* call to base implementation */
	if (! test_04_ab_common (connection, 32768, "Test 02-h::", &amount, 4, axl_false))
		return axl_false;
	gettimeofday (&stop, NULL);
	vortex_timeval_substract (&stop, &start, &result);
	printf ("Test 02-h: ..transfer %d bytes done in %ld segs + %ld microsegs (window size 32768, step 4096.\n", amount, result.tv_sec, result.tv_usec);
	if (! vortex_connection_is_ok (connection, axl_false)) {
		printf ("Test 02-h: ERROR, connection status is not ok before test..\n");
		return axl_false;
	}
	printf ("Test 02-h:    download rate at %.2f KBytes/segs..\n", test_02g_rate (amount, result));

	/****** test 65536 ******/
	printf ("Test 02-h: check to perform a transfer updated the default window size to 65536, step 4096\n");
	gettimeofday (&start, NULL);
	/* call to base implementation */
	if (! test_04_ab_common (connection, 65536, "Test 02-h::", &amount, 4, axl_false))
		return axl_false;
	gettimeofday (&stop, NULL);
	vortex_timeval_substract (&stop, &start, &result);
	printf ("Test 02-h: ..transfer %d bytes done in %ld segs + %ld microsegs (window size 65536, step 4096.\n", amount, result.tv_sec, result.tv_usec);
	if (! vortex_connection_is_ok (connection, axl_false)) {
		printf ("Test 02-h: ERROR, connection status is not ok before test..\n");
		return axl_false;
	}
	printf ("Test 02-h:    download rate at %.2f KBytes/segs..\n", test_02g_rate (amount, result));

	/******* check if current mss is less than 4096 bytes ********/
	printf ("Test 02-h: checking TCP MSS (%d) to be less than 4096..\n", vortex_connection_get_mss (connection));
	if (vortex_connection_get_mss (connection) < 4096) {
		/* configure the segmentator to limit transfer to tcp
		 * maximum segment size configured */
		vortex_connection_set_next_frame_size_handler (connection, test_02g_frame_size, INT_TO_PTR (vortex_connection_get_mss (connection) - 60));

		/* test 32768 */
		printf ("Test 02-h: check to perform a transfer updated the default window size to 32768, step %d (TCP MSS(%d) - BEEP HEADERS(60)\n",
			vortex_connection_get_mss (connection) - 60, vortex_connection_get_mss (connection));
		gettimeofday (&start, NULL);
		/* call to base implementation */
		if (! test_04_ab_common (connection, 32768, "Test 02-h::", &amount, 4, axl_true))
			return axl_false;
		gettimeofday (&stop, NULL);
		vortex_timeval_substract (&stop, &start, &result);
		printf ("Test 02-h: ..transfer %d bytes done in %ld segs + %ld microsegs (window size 32768, step 4096.\n", amount, result.tv_sec, result.tv_usec);
		if (! vortex_connection_is_ok (connection, axl_false)) {
			printf ("Test 02-h: ERROR, connection status is not ok before test..\n");
			return axl_false;
		}
		printf ("Test 02-h:    download rate at %.2f KBytes/segs..\n", test_02g_rate (amount, result));
	}

	/* ok, close the connection */
	if (! vortex_connection_close (connection)) {
		printf ("failed to close the BEEP session\n");
		return axl_false;
	} /* end if */
	
	/* return axl_true */
	return axl_true;
}

axl_bool  test_03a (void) {
	
	VortexConnection   * connection;
	VortexChannelPool  * pool;
	axlList            * channels;
	VortexChannel      * channel;
	

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
	}

	/* create the channel pool */
	pool = vortex_channel_pool_new (connection,
					REGRESSION_URI,
					1,
					/* no close handling */
					NULL, NULL,
					/* frame receive async handling */
					NULL, NULL,
					/* no async channel creation */
					NULL, NULL);
	/* close the channel pool */
	vortex_channel_pool_close (pool);
	
	/* create the channel pool */
	pool = vortex_channel_pool_new (connection,
					REGRESSION_URI,
					1,
					/* no close handling */
					NULL, NULL,
					/* frame receive async handling */
					NULL, NULL,
					/* no async channel creation */
					NULL, NULL);
	

	/* ok, close the connection */
	vortex_connection_close (connection);

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
	}

	/* create the channel pool */
	pool = vortex_channel_pool_new (connection,
					REGRESSION_URI,
					1,
					/* no close handling */
					NULL, NULL,
					/* frame receive async handling */
					NULL, NULL,
					/* no async channel creation */
					NULL, NULL);
	/* add tree channels */
	vortex_channel_pool_add (pool, 3);
	
	/* check channel number */
	if (vortex_channel_pool_get_num (pool) != 4) {
		fprintf (stderr, "number of channels expected 4 != %d doesn't match\n", vortex_channel_pool_get_num (pool));
		return axl_false;
	}

	/* use every channel */
	channels = axl_list_new (axl_list_always_return_1, NULL);

	/* get next channel */
	channel = vortex_channel_pool_get_next_ready (pool, axl_false);
	if (channel == NULL) 
		return axl_false;
	axl_list_add (channels, channel);

	/* get next channel */
	channel = vortex_channel_pool_get_next_ready (pool, axl_false);
	if (channel == NULL) 
		return axl_false;
	axl_list_add (channels, channel);

	/* get next channel */
	channel = vortex_channel_pool_get_next_ready (pool, axl_false);
	if (channel == NULL) 
		return axl_false;
	axl_list_add (channels, channel);


	/* get next channel */
	channel = vortex_channel_pool_get_next_ready (pool, axl_false);
	if (channel == NULL) 
		return axl_false;
	axl_list_add (channels, channel);

	/* get next channel */
	channel = vortex_channel_pool_get_next_ready (pool, axl_false);
	if (channel != NULL) 
		return axl_false;

	/* free the list */
	axl_list_free (channels);

	/* ok, close the connection */
	vortex_connection_close (connection);

	/* return axl_true */
	return axl_true;
}

axl_bool  test_03b (void) {
	
	VortexConnection   * connection;
	VortexChannelPool  * pool;
	VortexChannel      * channel;
	VortexChannel      * channel2;
	VortexAsyncQueue   * queue;
	VortexFrame        * frame;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
	}

	/* create the channel pool */
	pool = vortex_channel_pool_new (connection,
					REGRESSION_URI_ANS_NUL_WAIT,
					1,
					/* no close handling */
					NULL, NULL,
					/* frame receive async handling */
					NULL, NULL,
					/* no async channel creation */
					NULL, NULL);

	/* ask for a channel */
	channel = vortex_channel_pool_get_next_ready (pool, axl_true);
	
	/* check channel is ready at this point */
	if (! vortex_channel_is_ready (channel)) {
		printf ("ERROR: expected to find channel to be ready, but found it isn't..\n");
		return axl_false;
	} /* end if */

	/* set receive handler */
	queue = vortex_async_queue_new ();
	vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);

	/* send a message */
	vortex_channel_send_msg (channel, "this is a test", 14, NULL);
	
	/* wait for replies: first ANS */
	frame = vortex_channel_get_reply (channel, queue);
	if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_ANS) {
		printf ("ERROR: expected to find ANS reply but it was found..\n");
		return axl_false;
	}
	vortex_frame_unref (frame);

	/* check channel is ready at this point */
	if (vortex_channel_is_ready (channel)) {
		printf ("ERROR: expected to find channel to be not ready, but found it is..\n");
		return axl_false;
	} /* end if */

	/* wait for replies: second ANS */
	frame = vortex_channel_get_reply (channel, queue);
	if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_ANS) {
		printf ("ERROR: expected to find ANS reply but it was found..\n");
		return axl_false;
	}
	vortex_frame_unref (frame);

	/* check channel is ready at this point */
	if (vortex_channel_is_ready (channel)) {
		printf ("ERROR(4): expected to find channel to be not ready, but found it is..\n");
		return axl_false;
	} /* end if */

	/* wait for replies: third ANS */
	frame = vortex_channel_get_reply (channel, queue);
	if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_ANS) {
		printf ("ERROR: expected to find ANS reply but it was found..\n");
		return axl_false;
	}
	vortex_frame_unref (frame);

	/* check channel is ready at this point */
	if (vortex_channel_is_ready (channel)) {
		printf ("ERROR(6): expected to find channel to be not ready, but found it is..\n");
		return axl_false;
	} /* end if */

	/* wait for replies: fourth ANS */
	frame = vortex_channel_get_reply (channel, queue);
	if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_ANS) {
		printf ("ERROR: expected to find ANS reply but it was found..\n");
		return axl_false;
	}
	vortex_frame_unref (frame);

	/* check channel is ready at this point */
	if (vortex_channel_is_ready (channel)) {
		printf ("ERROR (8): expected to find channel to be not ready, but found it is..\n");
		return axl_false;
	} /* end if */

	/* wait for replies: last NUL frame */
	frame = vortex_channel_get_reply (channel, queue);
	if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_NUL) {
		printf ("ERROR: expected to find NUL reply but it was found..\n");
		return axl_false;
	}
	vortex_frame_unref (frame);

	/* check channel is ready at this point */
	if (! vortex_channel_is_ready (channel)) {
		printf ("ERROR (10): expected to find channel to be ready, but found it isn't..\n");
		return axl_false;
	} /* end if */

	/* second part of the test */
	vortex_channel_pool_release_channel (pool, channel);

	/* get channel from the pool */
	channel2 = vortex_channel_pool_get_next_ready (pool, axl_true);
	if (vortex_channel_get_number (channel2) != vortex_channel_get_number (channel)) {
		printf ("ERROR (11): expected to find equal channels but different values were found. This means vortex_channel_pool_get_next_ready is returning a newly created channel..\n");
		return axl_false;
	}

	/* send a new message */
	vortex_channel_send_msg (channel, "this is another content", 23, NULL);

	/* now release */
	vortex_channel_pool_release_channel (pool, channel);

	/* now get another channel from the pool */
	channel2 = vortex_channel_pool_get_next_ready (pool, axl_true);

	if (channel2 == NULL) {
		printf ("ERROR (12): expected a newly created channel reference but found NULL value..\n");
		return axl_false;
	} /* end if */
	
	/* check channel numbers */
	if (vortex_channel_get_number (channel2) == vortex_channel_get_number (channel)) {
		printf ("ERROR (13): expected to find different channels but equal values were found. This means vortex_channel_pool_get_next_ready is returning the same channel..\n");
		return axl_false;
	}

	/* first ANS frame */
	frame = vortex_channel_get_reply (channel, queue);
	vortex_frame_unref (frame);
	/* second ANS frame */
	frame = vortex_channel_get_reply (channel, queue);
	vortex_frame_unref (frame);
	/* third ANS frame */
	frame = vortex_channel_get_reply (channel, queue);
	vortex_frame_unref (frame);
	/* fourth ANS frame */
	frame = vortex_channel_get_reply (channel, queue);
	vortex_frame_unref (frame);
	/* last NUL frame */
	frame = vortex_channel_get_reply (channel, queue);
	vortex_frame_unref (frame);

	/* ok, close the connection */
	vortex_connection_close (connection);

	/* unref queue */
	vortex_async_queue_unref (queue);

	/* return axl_true */
	return axl_true;
}

void test_03c_received (VortexChannel    * channel,
			VortexConnection * conn,
			VortexFrame      * frame,
			axlPointer         user_data)
{
	VortexAsyncQueue * queue   = vortex_connection_get_data (conn, "test_03c_queue");
	VortexAsyncQueue * replies = vortex_connection_get_data (conn, "test_03c_replies");

	/* wait 2 second */
	if (queue != NULL) {
		vortex_connection_set_data (conn, "test_03c_queue", NULL);
		printf ("Test 03-c: Waiting 2 seconds to accumulate ANS/NUL replies..\n");
		vortex_async_queue_timedpop (queue, 2000000);
	}

	/* ok, now queue replies */
	vortex_frame_ref (frame);
	vortex_async_queue_push (replies, frame);

	return;
}

axl_bool  test_03c (void) {
	
	VortexConnection   * connection;
	VortexChannel      * channel;
	VortexAsyncQueue   * replies;
	VortexAsyncQueue   * queue;
	VortexFrame        * frame;
	int                  iterator;
	int                  msgno;
	int                  ansno;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
	}

	/* ok, now create a channel */
	queue   = vortex_async_queue_new ();
	replies = vortex_async_queue_new ();
	vortex_connection_set_data (connection, "test_03c_queue",   queue);
	vortex_connection_set_data (connection, "test_03c_replies", replies);
				    
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI_SIMPLE_ANS_NUL,
				      /* no close handling */
				      NULL, NULL,
				      test_03c_received, NULL,
				      NULL, NULL);

	/* check channel reference */
	if (channel == NULL) {
		printf ("ERROR (1): expected to find proper channel reference..\n");
		return axl_false;
	} /* end if */

	/* set serialize */
	vortex_channel_set_serialize (channel, axl_true);

	/* send a couple of messages and wait for replies */
	vortex_channel_send_msg (channel, "this is a test message 1", 24, NULL);
	vortex_channel_send_msg (channel, "this is a test message 2", 24, NULL);
	
	/* now get frames .. */
	iterator = 0;
	msgno    = 0;
	ansno    = 0;
	while (iterator < 62) {
		frame = vortex_async_queue_pop (replies);

		/* printf ("Received reply: %d (type: %s) msgno:%d, ansno:%d, frame id:%d\n",
			iterator, vortex_frame_get_type (frame) == VORTEX_FRAME_TYPE_ANS ? "ANS" : "NUL",
			vortex_frame_get_msgno (frame), vortex_frame_get_ansno (frame),
			vortex_frame_get_id (frame)); */
		if ((iterator == 30 || iterator == 61) && 
		    vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_NUL) {
			printf ("\n\n**** ERROR: expected to find NUL frame but found ANS iterator=%d..\n", iterator);
			return axl_false;
		}

		if (vortex_frame_get_msgno (frame) != msgno) {
			printf ("\n\n**** ERROR: expected to find msgno %d but found %d (iterator=%d)\n", 
				msgno, vortex_frame_get_msgno (frame), iterator);
			return axl_false;
		}

		if ((vortex_frame_get_type (frame) == VORTEX_FRAME_TYPE_ANS) && vortex_frame_get_ansno (frame) != ansno) {
			printf ("\n\n**** ERROR: expected to find ansno %d but found %d (iterator=%d)\n", 
				ansno, vortex_frame_get_ansno (frame), iterator);
			return axl_false;
		} 

		/* update msgno */
		if (iterator == 30) {
			msgno++;
			ansno = 0;
		} else {
			ansno++;
		} /* end if */

		vortex_frame_unref (frame);
		iterator++;

	} /* end if */

	vortex_async_queue_unref (queue);
	vortex_async_queue_unref (replies);
	
	vortex_connection_close (connection);
	return axl_true;
}

VortexChannel * test_03d_create_handler (VortexConnection     * conn,
					 int                    channel_num,
					 const char           * profile,
					 VortexOnCloseChannel   on_close, 
					 axlPointer             on_close_user_data,
					 VortexOnFrameReceived  on_received, 
					 axlPointer             on_received_user_data,
					 /* additional pointers */
					 axlPointer             create_channel_user_data,
					 axlPointer             get_next_data)
{
	/* check references received */
	printf ("Test 03-d: beacons received: %d - %d\n", 
		PTR_TO_INT (create_channel_user_data), PTR_TO_INT (get_next_data));
	if (PTR_TO_INT (create_channel_user_data) != 3 || PTR_TO_INT (get_next_data) != 4)
		return NULL;

	/* create a channel */
	return vortex_channel_new (conn, channel_num, REGRESSION_URI, NULL, NULL, NULL, NULL, NULL, NULL);
}

axl_bool  test_03d (void) {
	
	VortexConnection   * connection;
	VortexChannelPool  * pool;
	VortexChannel      * channel;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
	}

	/* create the channel pool */
	pool = vortex_channel_pool_new_full (connection,
					     REGRESSION_URI,
					     1,
					     /* create handler */
					     test_03d_create_handler, 
					     /* pointer */
					     INT_TO_PTR (3),
					     /* no close handling */
					     NULL, NULL,
					     /* frame receive async handling */
					     NULL, NULL,
					     /* no async channel creation */
					     NULL, NULL);

	/* ask for a channel */
	channel = vortex_channel_pool_get_next_ready_full (pool, axl_true, INT_TO_PTR (4));

	if (channel == NULL) {
		printf ("ERROR: expected to find proper references..\n");
		return axl_false;
	} /* end if */
	
	/* check channel is ready at this point */
	if (! vortex_channel_is_ready (channel)) {
		printf ("ERROR: expected to find channel to be ready, but found it isn't..\n");
		return axl_false;
	} /* end if */

	/* ok, close the connection */
	vortex_connection_close (connection);

	/* return axl_true */
	return axl_true;
}

/* constant for test_04 */
#define MAX_NUM_CON 1000

axl_bool  test_04 (void)
{
	int                 iterator = 0;
	VortexConnection  * connections[MAX_NUM_CON];

	/* clear array */
	iterator = 0;
	while (iterator < MAX_NUM_CON) { 
		connections[iterator] = 0;

		iterator++;
	} /* end while */

	iterator = 0;
	while (iterator < MAX_NUM_CON) { 
		/* creates a new connection against localhost:44000 */
		connections[iterator] = connection_new ();
		if (!vortex_connection_is_ok (connections[iterator], axl_false)) {
			printf ("Test 04: Unable to connect remote server, error was: %s",
				vortex_connection_get_message (connections[iterator]));
			return axl_false;
		}

		/* update iterator */
		iterator++;
	} /* end while */


	iterator = 0;
	while (iterator < MAX_NUM_CON) {

		/* close the connection */
		if (connections[iterator] != NULL) 
			vortex_connection_close (connections[iterator]);
		else
			return axl_false;

		/* update iterator */
		iterator++;
	}

	/* return axl_true */
	return axl_true;
}

#define TEST_05_MSGSIZE (65536 * 8)

/** 
 * @brief Checking TLS profile support.
 * 
 * @return axl_true if ok, otherwise, axl_false is returned.
 */
axl_bool  test_05 (void)
{
	/* TLS status notification */
	VortexStatus       status;
	char             * status_message = NULL;
	VortexChannel    * channel;
	VortexAsyncQueue * queue;
	char             * message;
	int                iterator;
	VortexFrame      * frame;

	/* vortex connection */
	VortexConnection * connection;

#if defined(ENABLE_TLS_SUPPORT)
	/* initialize and check if current vortex library supports TLS */
	if (! vortex_tls_init (ctx)) {
		printf ("--- WARNING: Unable to activate TLS, current vortex library has not TLS support activated. \n");
		return axl_true;
	}
#else
	printf ("--- WARNING: Current build does not have TLS support.\n");
	return axl_true;
#endif

	/* create a new connection */
	connection = connection_new ();

	/* enable TLS negotiation */
	connection = vortex_tls_start_negotiation_sync (connection, NULL, 
							&status,
							&status_message);
	if (vortex_connection_get_data (connection, "being_closed")) {
		printf ("Found TLS connection flagges a being_closed\n");
		return axl_false;
	}

	if (status != VortexOk) {
		printf ("Test 05: Failed to activate TLS support: %s\n", status_message);
		/* return axl_false */
		return axl_false;
	}

	/* check connection status at this point */
	if (! vortex_connection_is_ok (connection, axl_false)) {
		printf ("Test 05: Connection status after TLS activation is not ok, message: %s", vortex_connection_get_message (connection));
		return axl_false;
	}

	printf ("Test 05: TLS connection activated..\n");

	/* create the queue */
	queue = vortex_async_queue_new ();
	
	/* create a channel */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      test_03_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("Test 05: .... Unable to create the channel..\n");
		return axl_false;
	}

	/* build the message */
	message = axl_new (char, TEST_05_MSGSIZE);
	for (iterator = 0; iterator < TEST_05_MSGSIZE; iterator++)
		message [iterator] = (iterator % 3); 

	iterator = 0;
	while (iterator < 20) {

		/* send the hug message */
		if (! vortex_channel_send_msg (channel, message, TEST_05_MSGSIZE, NULL)) {
			printf ("Test 05: Unable to send large message");
			axl_free (message);
			return axl_false;
		}

		/* update the iterator */
		iterator++;
		
	} /* end while */
	
	printf ("Test 05: receiving replies (20)..");
	iterator = 0;
	while (iterator < 20) {

		/* wait for the message */
		frame = vortex_async_queue_pop (queue);

		/* check payload size */
		if (vortex_frame_get_payload_size (frame) != TEST_05_MSGSIZE) {
			printf ("Test 05: found that payload received isn't the value expected: %d != %d\n",
				vortex_frame_get_payload_size (frame), TEST_05_MSGSIZE);
			/* free frame */
			vortex_frame_free (frame);
			
			return axl_false;
		}

		/* check result */
		if (memcmp (message, vortex_frame_get_payload (frame), TEST_05_MSGSIZE)) {
			printf ("Test 05: Messages aren't equal\n");
			return axl_false;
		}
		
		/* free frame received */
		vortex_frame_free (frame);

		/* update iterator */
		iterator++;
	}

	/* free the message */
	axl_free (message);

	/* ok, close the channel */
	if (! vortex_channel_close (channel, NULL))
		return axl_false;

	/* ok, close the connection */
	if (! vortex_connection_close (connection))
		return axl_false;

	/* free the queue */
	vortex_async_queue_unref (queue);

	/* close connection */
	return axl_true;
}

axl_bool  test_02l (void) {

	VortexConnection * connection;
	VortexChannel    * channel;
	VortexFrame      * frame; 
	VortexAsyncQueue * queue;
	int                msg_no;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
		
	} /* end if */

	/* create a channel */
	queue   = vortex_async_queue_new ();
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI_ANS_NUL_REPLY_CLOSE,
				      /* no close handling */
				      NULL, NULL,
				      /* no frame received */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);

	if (channel == NULL) {
		printf ("ERROR: unable to create channel to check connection close after ANS..NUL reply\n");
		return axl_false;
	} /* end if */


	/* get a reply */
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("ERROR: expected reply from remote side while running mixed replies tests..\n");
		return axl_false;
	}

	/* check reply type */
	if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_MSG) {
		printf ("ERROR: expected to receive a MSG frame..\n");
		return axl_false;
	}

	/* check content received */
	if (! axl_cmp (vortex_frame_get_payload (frame), "message 1")) {
		printf ("ERROR: expected to find different content than received..\n");
		return axl_false;
	}
	
	/* get msgno number */
	msg_no = vortex_frame_get_msgno (frame);

	/* release frame */
	vortex_frame_unref (frame);
	
	/* now reply with two ANS frames with NUL frame */
	if (! vortex_channel_send_ans_rpy (channel, "this is a reply 1", 17, msg_no)) {
		printf ("ERROR: failed to send first ANS reply ..\n");
		return axl_false;
	}
	
	/* second reply */
	if (! vortex_channel_send_ans_rpy (channel, "this is a reply 2", 17, msg_no)) {
		printf ("ERROR: failed to send second ANS reply ..\n");
		return axl_false;
	}

	/* finalize reply */
	if (!vortex_channel_finalize_ans_rpy (channel, msg_no)) {
		printf ("ERROR: failed to finalize ANS .. NUL reply series..\n");
		return axl_false;
	} /* end if */
	    
	/* ok, close the connection */
	if (! vortex_connection_close (connection)) {
		printf ("failed to close the BEEP session\n");
		return axl_false;
	} /* end if */

	/* terminate queue */
	vortex_async_queue_unref (queue);

	/* operation completed */
	return axl_true;
}

/** 
 * @brief Checking TLS profile support.
 * 
 * @return axl_true if ok, otherwise, axl_false is returned.
 */
axl_bool  test_05_a (void)
{
	/* TLS status notification */
	VortexStatus       status;
	char             * status_message = NULL;
	VortexChannel    * channel;
	int                connection_id;

	/* vortex connection */
	VortexConnection * connection;
	VortexConnection * connection2;

#if defined(ENABLE_TLS_SUPPORT)
	/* initialize and check if current vortex library supports TLS */
	if (! vortex_tls_init (ctx)) {
		printf ("--- WARNING: Unable to activate TLS, current vortex library has not TLS support activated. \n");
		return axl_true;
	}
#else
	printf ("--- WARNING: Current build does not have TLS support.\n");
	return axl_true;
#endif

	/* create a new connection */
	connection = connection_new ();

	/* create a channel to block tls negotiation */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI_BLOCK_TLS,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      NULL, NULL,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("Unable to create the channel..");
		return axl_false;
	}

	/* enable TLS negotiation but get the connection id first */
	connection_id = vortex_connection_get_id (connection);
	connection    = vortex_tls_start_negotiation_sync (connection, NULL, 
							   &status,
							   &status_message);

	if (connection == NULL) {
		printf ("Test 05-a: expected an error but not NULL reference..\n");
		return axl_false;
	} /* end if */

	if (vortex_connection_get_id (connection) != connection_id) {
		printf ("Test 05-a: expected an error but not a connection change..\n");
		return axl_false;
	} /* end if */

	if (status == VortexOk) {
		printf ("Test 05-a: Failed to activate TLS support: %s\n", status_message);
		/* return axl_false */
		return axl_false;
	}

	/* check that the connection is ok */
	if (! vortex_connection_is_ok (connection, axl_false)) {
		printf ("Test 05-a: expected an error but not a connection closed..\n");
		return axl_false;
	} /* end if */

	/* now, do the same text to force a TLS error at the remote
	 * side (activated due to previous exchange) */
	connection_id = vortex_connection_get_id (connection);
	connection    = vortex_tls_start_negotiation_sync (connection, NULL, 
							   &status,
							   &status_message);

	if (connection == NULL) {
		printf ("Test 05-a: expected an error but not NULL reference..\n");
		return axl_false;
	} /* end if */

	if (status == VortexOk) {
		printf ("Test 05-a: Failed to activate TLS support: %s\n", status_message);
		/* return axl_false */
		return axl_false;
	}

	/* check that the connection is ok */
	if (vortex_connection_is_ok (connection, axl_false)) {
		printf ("Test 05-a: expected an error but  a connection properly running..\n");
		return axl_false;
	} /* end if */

	vortex_connection_close (connection);

	/* now check autotls */
	connection = connection_new ();

	/* create a channel to block tls negotiation */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI_BLOCK_TLS,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      NULL, NULL,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("Unable to create the channel..");
		return axl_false;
	}

	/* enable autotls */
	vortex_tls_set_auto_tls (ctx, axl_true, axl_false, NULL);

	/* now check autotls */
	connection2 = connection_new ();

	if (connection2 == NULL) {
		printf ("Test 05-a: expected a failure using auto-tls but not a null reference..\n");
		return axl_false;
	}

	if (vortex_connection_is_ok (connection2, axl_false)) {
		printf ("Test 05-a: expected a failure using auto-tls but a proper connection status was found...\n");
		return axl_false;
	}

	/* close the connection */
	vortex_connection_close (connection2);
	vortex_connection_close (connection);

	/* now create a connection with auto tls activated */
	connection2 = connection_new ();

	if (connection2 == NULL) {
		printf ("Test 05-a: expected a proper connection using auto-tls but not a null reference (1)..\n");
		return axl_false;
	}

	/* check connection status */
	if (! vortex_connection_is_ok (connection2, axl_false)) {
		printf ("Test 05-a: expected a proper connection using auto-tls but a failure was found (2)...\n");
		return axl_false;
	}

	/* check tls fixate status */
	if (! vortex_connection_is_tlsficated (connection2)) {
		printf ("Test 05-a: expected proper TLS fixate status..\n");
		return axl_false;
	}
	
	/* close the connection */
	vortex_connection_close (connection2);
	
	/* restore auto-tls */
	vortex_tls_set_auto_tls (ctx, axl_false, axl_false, NULL);

	/* now create a connection with auto tls activated */
	connection2 = connection_new ();

	if (connection2 == NULL) {
		printf ("Test 05-a: expected a proper connection using auto-tls but not a null reference(3)..\n");
		return axl_false;
	}

	/* check connection status */
	if (! vortex_connection_is_ok (connection2, axl_false)) {
		printf ("Test 05-a: expected a proper connection using auto-tls but a failure was found(4)...\n");
		return axl_false;
	}

	/* check tls fixate status */
	if (vortex_connection_is_tlsficated (connection2)) {
		printf ("Test 05-a: expected to find disable automatic TLS negotiation, but it found enabled..\n");
		return axl_false;
	}
	
	/* close the connection */
	vortex_connection_close (connection2);


	return axl_true;
}

axl_bool test_05_b (void)
{
	/* TLS status notification */
	VortexStatus       status;
	char             * status_message = NULL;

	/* vortex connection */
	VortexConnection * connection;

#if defined(ENABLE_TLS_SUPPORT)
	/* initialize and check if current vortex library supports TLS */
	if (! vortex_tls_init (ctx)) {
		printf ("--- WARNING: Unable to activate TLS, current vortex library has not TLS support activated. \n");
		return axl_true;
	}
#else
	printf ("--- WARNING: Current build does not have TLS support.\n");
	return axl_true;
#endif

	
	/* create a new connection */
	connection = connection_new ();

	/* then close */
	vortex_connection_shutdown (connection);
	
	/* now start tls */
	connection = vortex_tls_start_negotiation_sync (connection, "test", &status, &status_message);

	vortex_connection_close (connection);

	return axl_true;
}


/* message size: 4096 */
#define TEST_REGRESION_URI_4_MESSAGE "This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary ."


/**
 * @brief Implements frame received for nul/ans replies.
 */
void test_04_a_frame_received (VortexChannel    * channel,
			       VortexConnection * connection,
			       VortexFrame      * frame,
			       axlPointer         user_data)
{
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
		
		printf ("Test 04-a:     Operation completed..Ok!\n");
		vortex_async_queue_push ((axlPointer) user_data, INT_TO_PTR (1));
		return;
	} /* end if */
	
	/* check content */
	if (!axl_memcmp ((char *) vortex_frame_get_payload (frame), 
			 TEST_REGRESION_URI_4_MESSAGE,
			 vortex_frame_get_payload_size (frame))) {
		printf ("Expected to find different message at test: %s != '%s'..\n",
			(char *) vortex_frame_get_payload (frame), TEST_REGRESION_URI_4_MESSAGE);
		return;
	} /* end if */
	
	return;
}

axl_bool  test_04_a_common (int block_size, int num_blocks, int num_times) {

	VortexConnection * connection;
	VortexChannel    * channel;
	VortexAsyncQueue * queue;
	int                iterator;
	int                times;
	VortexFrame      * frame;
	char             * message;
	int                total_bytes = 0;
	int                blocks_received;
	
#if defined(AXL_OS_UNIX)
	struct timeval      start;
	struct timeval      stop;
#endif
	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
	}

	/* create the queue */
	queue = vortex_async_queue_new ();
	
	/* create a channel */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI_4,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("Unable to create the channel..");
		return axl_false;
	}

	times           = 0;
	blocks_received = 0;
	while (times < num_times) {
		/* request for the file */
#if defined(AXL_OS_UNIX)	
		/* take a start measure */
		gettimeofday (&start, NULL);
#endif
		printf ("Test 04-a:   sending initial request block unit size=%d, block num=%d\n", block_size, num_blocks);
		message = axl_strdup_printf ("return large message,%d,%d", block_size, num_blocks);
		if (! vortex_channel_send_msg (channel, message, strlen (message), NULL)) {
			printf ("Failed to send message requesting for large file..\n");
			return axl_false;
		} /* end if */
		axl_free (message);
		
		/* wait for all replies */
		iterator    = 0;
		total_bytes = 0;
		printf ("Test 04-a:     waiting replies\n");
		while (axl_true) {
			/* get the next message, blocking at this call. */
			frame = vortex_channel_get_reply (channel, queue);
			
			if (frame == NULL) {
				printf ("Timeout received for regression test: %s\n", REGRESSION_URI_4);
				continue;
			}
			
			/* get payload size */
			total_bytes += vortex_frame_get_payload_size (frame);
			blocks_received ++;
			
			if (vortex_frame_get_type (frame) == VORTEX_FRAME_TYPE_NUL) {

				/* check blocks received (-1 because
				 * last NUL frame do not contain
				 * data) */
				if ((blocks_received - 1) != num_blocks) {
					printf ("ERROR: Expected to find %d blocks but received %d..\n",
						num_blocks, (blocks_received - 1));
					return axl_false;
				}

				/* check size */
				if (vortex_frame_get_payload_size (frame) != 0) {
					printf ("Expected to find NUL terminator message, with empty content, but found: %d frame size\n",
						vortex_frame_get_payload_size (frame));
					return axl_false;
				} /* end if */
				
				/* deallocate the frame received */
				vortex_frame_unref (frame);
				
				printf ("Test 04-a:     operation completed, ok\n");
				break;
				
			} /* end if */
			
			/* check content */
			if (!axl_memcmp ((char *) vortex_frame_get_payload (frame),
					 TEST_REGRESION_URI_4_MESSAGE,
					 block_size)) {
				printf ("Expected to find different message(%d) at test: %s != '%s'..\n",
					iterator, (char *) vortex_frame_get_payload (frame), TEST_REGRESION_URI_4_MESSAGE);
				return axl_false;
			} /* end if */
			
			
			/* check size */
			if (vortex_frame_get_payload_size (frame) != block_size) {
				printf ("Expected to find different message(%d) size %d at test, but found: %d..\n",
					iterator, block_size, vortex_frame_get_payload_size (frame));
				return axl_false;
			}
			
			/* deallocate the frame received */
			vortex_frame_unref (frame);
			
			/* update iterator */
			iterator++;
		} /* end while */
		
		/* free the queue */
		vortex_async_queue_unref (queue);
		
#if defined(AXL_OS_UNIX)
		/* take a stop measure */
		gettimeofday (&stop, NULL);
		
		/* substract */
		subs (stop, start, &stop);
		
		printf ("Test 04-a:     Test ok, operation completed in: %ld.%ld seconds!  (bytes transfered: %d)!\n", 
			stop.tv_sec, stop.tv_usec, total_bytes);
#endif
		
		printf ("Test 04-a:     now, perform the same operation without queue/reply, using frame received handler\n");

		/* request for the file */
#if defined(AXL_OS_UNIX)	
		/* take a start measure */
		gettimeofday (&start, NULL);
#endif
		
		/* create a new queue to wait */
		queue = vortex_async_queue_new ();
		
		/* reconfigure frame received */
		vortex_channel_set_received_handler (channel, test_04_a_frame_received, queue);
		
		/* request for the file */
		message = axl_strdup_printf ("return large message,%d,%d", block_size, num_blocks);
		if (! vortex_channel_send_msg (channel, message, strlen (message), NULL)) {
			printf ("Failed to send message requesting for large file..\n");
			return axl_false;
		} /* end if */
		axl_free (message);
		
		/* wait until the file is received */
		printf ("Test 04-a:     waiting for file to be received\n");
		vortex_async_queue_pop (queue);
		printf ("Test 04-a:     file received, ok\n");
		
#if defined(AXL_OS_UNIX)
		/* take a stop measure */
		gettimeofday (&stop, NULL);
		
		/* substract */
		subs (stop, start, &stop);
		
		printf ("Test 04-a:     Test ok, operation completed in: %ld.%ld seconds! (bytes transfered: %d)!\n", 
			stop.tv_sec, stop.tv_usec, total_bytes);
#endif
		
		/* free the queue */
		vortex_async_queue_unref (queue);
		
		/* release the queue */
		queue = vortex_async_queue_new ();
		
		/* reconfigure frame received */
		vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);

		/* next position */
		times++;
	
	} /* end while */

	/* unref queue */
	vortex_async_queue_unref (queue);

	/* ok, close the channel */
	if (! vortex_channel_close (channel, NULL))
		return axl_false;

	/* ok, close the connection */
	if (! vortex_connection_close (connection))
		return axl_false;

	/* close connection */
	return axl_true;
}

/** 
 * @brief Checks BEEP support for ANS/NUL while sending large messages
 * that goes beyond default window size advertised.
 * 
 * 
 * @return axl_true if the test is ok, otherwise axl_false is returned.
 */
axl_bool  test_04_a (void) {
	/* call to run default test: block=4096, block-num=4096 */
	if (! test_04_a_common (4096, 4096, 1))
		return axl_false;

	/* call to run test: block=4094, block-num=4096 */
	if (! test_04_a_common (4094, 4096, 1))
		return axl_false;

	/* call to run test: block=4094, block-num=8192 */
	if (! test_04_a_common (4094, 8192, 1))
		return axl_false;


	/* all tests ok */
	return axl_true;
}

/** 
 * @brief Checks BEEP support for ANS/NUL while sending different
 * files in the same channel.
 *
 * @return axl_true if the test is ok, otherwise axl_false is returned.
 */
axl_bool  test_04_ab (void) {

	VortexConnection * connection;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
	}

	/* call to base implementation */
	if (! test_04_ab_common (connection, -1, NULL, NULL, 1, axl_false))
		return axl_false;

	/* ok, close the connection */
	if (! vortex_connection_close (connection))
		return axl_false;

	/* close connection */
	return axl_true;
}

/** 
 * @brief Checks client adviced profiles.
 *
 * @return axl_true if the test is ok, otherwise axl_false is returned.
 */
axl_bool  test_04_c (void) {

	VortexConnection * connection;
	VortexChannel    * channel;

	/* register two profiles for this session */
	vortex_profiles_register (ctx, "urn:vortex:regression-test:uri:1", 
				  /* start channel */
				  NULL, NULL, 
				  /* stop channel */
				  NULL, NULL, 
				  /* frame received */
				  NULL, NULL);

	/* register two profiles for this session */
	vortex_profiles_register (ctx, "urn:vortex:regression-test:uri:2", 
				  /* start channel */
				  NULL, NULL, 
				  /* stop channel */
				  NULL, NULL, 
				  /* frame received */
				  NULL, NULL);

	/* register two profiles for this session */
	vortex_profiles_register (ctx, "urn:vortex:regression-test:uri:3", 
				  /* start channel */
				  NULL, NULL, 
				  /* stop channel */
				  NULL, NULL, 
				  /* frame received */
				  NULL, NULL);

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
	}

	/* create a channel */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI_6,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      NULL, NULL,
				      /* no async channel creation */
				      NULL, NULL);

	if (channel == NULL) {
		printf ("Unable to create the channel, failed to advice client profiles..\n");
		return axl_false;
	}

	/* ok, close the channel */
	if (! vortex_channel_close (channel, NULL))
		return axl_false;

	/* ok, close the connection */
	if (! vortex_connection_close (connection))
		return axl_false;

	/* register two profiles for this session */
	vortex_profiles_unregister (ctx, "urn:vortex:regression-test:uri:1");
	vortex_profiles_unregister (ctx, "urn:vortex:regression-test:uri:2");
	vortex_profiles_unregister (ctx, "urn:vortex:regression-test:uri:3");

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
	}

	/* create a channel */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI_6bis,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      NULL, NULL,
				      /* no async channel creation */
				      NULL, NULL);
				
	if (channel == NULL) {
		printf ("Unable to create the channel, failed to advice client profiles..\n");
		return axl_false;
	}

	/* ok, close the channel */
	if (! vortex_channel_close (channel, NULL))
		return axl_false;

	/* ok, close the connection */
	if (! vortex_connection_close (connection))
		return axl_false;

	/* close connection */
	return axl_true;
}

axl_bool test_04_d_send_content (VortexConnection * connection, VortexChannel * channel, VortexAsyncQueue * queue)
{
	int           iterator;
	VortexFrame * frame;

	printf ("Test 04-d: ok, now check again transfer..\n");
	iterator = 0;
	while (iterator < 10) {
		/* send message */
		if (! vortex_channel_send_msg (channel,
					       TEST_REGRESION_URI_4_MESSAGE, 4096, NULL)) {
			printf ("ERROR (6): failed to send message: %s..\n", vortex_connection_get_message (connection));
			return axl_false;
		}

		/* receive both messages */
		frame = vortex_channel_get_reply (channel, queue);
		
		if (frame == NULL) {
			printf ("ERROR (6.1): expected to find frame reply but found NULL frame..\n");
			return axl_false;
		} /* end if */

		/* check connection status */
		if (! vortex_connection_is_ok (connection, axl_false)) {
			printf ("ERROR (6.2): expected to find proper connection status but found failure..\n");
			return axl_false;
		}

		/* check message size and payload */
		if (! axl_cmp (vortex_frame_get_payload (frame), TEST_REGRESION_URI_4_MESSAGE)) {
			printf ("ERROR (7): expected to find a message, but found something different: sizes differs %d != %d...\n",
				vortex_frame_get_payload_size (frame), 4096);
			return axl_false;
		} /* end if */

		if (vortex_frame_get_payload_size (frame) != 4096) {
			printf ("ERROR (8): expected to find different payload size..\n");
			return axl_false;
		}

		/* unref frame */
		vortex_frame_unref (frame);

		/* next position */
		iterator++;
	} /* end while */

	return axl_true;
}

axl_bool test_04_d_send_content_2 (VortexConnection * connection, VortexChannel * channel, VortexAsyncQueue * queue)
{
	int           iterator;
	VortexFrame * frame;

	printf ("Test 04-d: ok, now check again transfer..\n");
	iterator = 0;
	while (iterator < 10) {
		/* send message */
		if (! vortex_channel_send_msg (channel,
					       TEST_REGRESION_URI_4_MESSAGE, 243, NULL)) {
			printf ("ERROR (6): failed to send message: %s..\n", vortex_connection_get_message (connection));
			return axl_false;
		}

		/* receive both messages */
		frame = vortex_channel_get_reply (channel, queue);
		
		if (frame == NULL) {
			printf ("ERROR (6.1): expected to find frame reply but found NULL frame..\n");
			return axl_false;
		} /* end if */

		/* check connection status */
		if (! vortex_connection_is_ok (connection, axl_false)) {
			printf ("ERROR (6.2): expected to find proper connection status but found failure..\n");
			return axl_false;
		}

		/* check message size and payload */
		if (! axl_memcmp (vortex_frame_get_payload (frame), TEST_REGRESION_URI_4_MESSAGE, 243)) {
			printf ("ERROR (7): expected to find a message, but found something different...\n");
			return axl_false;
		} /* end if */

		if (vortex_frame_get_payload_size (frame) != 243) {
			printf ("ERROR (8): expected to find different payload size..\n");
			return axl_false;
		}

		/* unref frame */
		vortex_frame_unref (frame);

		/* next position */
		iterator++;
	} /* end while */

	return axl_true;
}

axl_bool test_04_d_send_content_ans (VortexConnection * connection, VortexChannel * channel, VortexAsyncQueue * queue)
{
	int           iterator;
	VortexFrame * frame;

	/* send message */
	if (! vortex_channel_send_msg (channel,
				       TEST_REGRESION_URI_4_MESSAGE, 4096, NULL)) {
		printf ("ERROR (6): failed to send message: %s..\n", vortex_connection_get_message (connection));
		return axl_false;
	}

	printf ("Test 04-d: ok, now check for replies..\n");
	iterator = 0;
	while (iterator < 30) {

		/* receive both messages */
		frame = vortex_channel_get_reply (channel, queue);

		if (frame == NULL) {
			printf ("ERROR (6.1): expected to find frame reply but found NULL frame..\n");
			return axl_false;
		} /* end if */

		/* check connection status */
		if (! vortex_connection_is_ok (connection, axl_false)) {
			printf ("ERROR (6.2): expected to find proper connection status but found failure..\n");
			return axl_false;
		}

		/* check message size and payload */
		if (! axl_cmp (vortex_frame_get_payload (frame), TEST_REGRESION_URI_4_MESSAGE)) {
			printf ("ERROR (7): expected to find a message, but found something different...\n");
			return axl_false;
		} /* end if */

		if (vortex_frame_get_payload_size (frame) != 4096) {
			printf ("ERROR (8): expected to find different payload size..\n");
			return axl_false;
		}

		/* unref frame */
		vortex_frame_unref (frame);

		/* next position */
		iterator++;
	} /* end while */

	/* now check for nul frame */
	frame = vortex_channel_get_reply (channel, queue);
	if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_NUL) {
		printf ("ERROR (9): expected to find NUL frame but found: %d type\n", 
			vortex_frame_get_type (frame));
		return axl_false;
	} /* end if */

	vortex_frame_unref (frame);

	return axl_true;
}

axl_bool test_04_d_send_content_ans_2 (VortexConnection * connection, VortexChannel * channel, VortexAsyncQueue * queue, int check_size)
{
	int           iterator;
	VortexFrame * frame;

	/* send message */
	if (! vortex_channel_send_msg (channel,
				       TEST_REGRESION_URI_4_MESSAGE, check_size, NULL)) {
		printf ("ERROR (6): failed to send message: %s..\n", vortex_connection_get_message (connection));
		return axl_false;
	}

	printf ("Test 04-d: ok, now check for replies..\n");
	iterator = 0;
	while (iterator < 30) {

		/* receive both messages */
		frame = vortex_channel_get_reply (channel, queue);

		if (frame == NULL) {
			printf ("ERROR (6.1): expected to find frame reply but found NULL frame..\n");
			return axl_false;
		} /* end if */

		/* check connection status */
		if (! vortex_connection_is_ok (connection, axl_false)) {
			printf ("ERROR (6.2): expected to find proper connection status but found failure..\n");
			return axl_false;
		}

		/* check message size and payload */
		if (! axl_memcmp (vortex_frame_get_payload (frame), TEST_REGRESION_URI_4_MESSAGE, check_size)) {
			printf ("ERROR (7): expected to find a message, but found something different...\n");
			return axl_false;
		} /* end if */

		if (vortex_frame_get_payload_size (frame) != check_size) {
			printf ("ERROR (8): expected to find different payload size..\n");
			return axl_false;
		}

		/* unref frame */
		vortex_frame_unref (frame);

		/* next position */
		iterator++;
	} /* end while */

	/* now check for nul frame */
	frame = vortex_channel_get_reply (channel, queue);
	if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_NUL) {
		printf ("ERROR (9): expected to find NUL frame but found: %d type\n", 
			vortex_frame_get_type (frame));
		return axl_false;
	} /* end if */

	vortex_frame_unref (frame);

	return axl_true;
}


axl_bool test_04_d (void)
{
	VortexConnection * connection;
	VortexChannel    * channel;
	VortexAsyncQueue * queue;
	VortexFrame      * frame;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		vortex_connection_close (connection);
		return axl_false;
	}

	/* create the queue */
	queue   = vortex_async_queue_new ();

	/* create a channel */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("ERROR (1): Unable to create the channel..");
		return axl_false;
	}

	printf ("Test 04-d: checking reducing window size (1024) at the receiving size..\n");
	vortex_channel_set_window_size (channel, 1024);

	/* send 10 large messages */
	printf ("Test 04-d: first send operation..\n");
	if (! test_04_d_send_content (connection, channel, queue))
		return axl_false;

	/* now request remote side to change its window size */
	printf ("Test 04-d: Requesting to change remote window size to 1024..\n");
	vortex_channel_send_msg (channel, "window_size=1024", 16, NULL);
	/* receive both messages */
	frame = vortex_channel_get_reply (channel, queue);

	/* check message size and payload */
	if (! axl_cmp (vortex_frame_get_payload (frame), "ok")) {
		printf ("ERROR (5): expected to find a message, failed to change remote window size...\n");
		return axl_false;
	} /* end if */
	vortex_frame_unref (frame);

	/* send 10 large messages */
	printf ("Test 04-d: second send operation..\n");
	if (! test_04_d_send_content (connection, channel, queue))
		return axl_false;

	/* now check changing window size of the remote buffer just at
	 * the begining */

	/* create a channel */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("ERROR (9): Unable to create the channel..");
		return axl_false;
	} /* end if */

	printf ("Test 04-d: requesting to change remote buffer..\n");
	vortex_channel_send_msg (channel, "window_size=1024", 16, NULL);
	vortex_channel_set_window_size (channel, 1024);
	/* receive both messages */
	frame = vortex_channel_get_reply (channel, queue);

	/* check message size and payload */
	if (! axl_cmp (vortex_frame_get_payload (frame), "ok")) {
		printf ("ERROR (5): expected to find a message, failed to change remote window size...\n");
		return axl_false;
	} /* end if */
	vortex_frame_unref (frame);

	/* send 10 large messages */
	printf ("Test 04-d: third send operation..\n");
	if (! test_04_d_send_content (connection, channel, queue))
		return axl_false;

	/* now try something similar with ANS/NUL frames.. */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI_SIMPLE_ANS_NUL,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("ERROR (10): Unable to create the channel..");
		return axl_false;
	} /* end if */

	printf ("Test 04-d: requesting to change remote buffer (4)..\n");
	vortex_channel_send_msg (channel, "window_size=1024", 16, NULL);
	vortex_channel_set_window_size (channel, 1024);
	/* receive both messages */
	frame = vortex_channel_get_reply (channel, queue);

	/* check message size and payload */
	if (! axl_cmp (vortex_frame_get_payload (frame), "ok")) {
		printf ("ERROR (5): expected to find a message, failed to change remote window size...\n");
		return axl_false;
	} /* end if */
	vortex_frame_unref (frame);

	/* send 10 large messages */
	printf ("Test 04-d: fourth send operation..\n");
	if (! test_04_d_send_content_ans (connection, channel, queue))
		return axl_false;

	/* now try something similar with ANS/NUL frames.. */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("ERROR (10): Unable to create the channel..");
		return axl_false;
	} /* end if */

	printf ("Test 04-d: requesting to change remote buffer (5)..\n");
	vortex_channel_set_window_size (channel, 1024);

	/* send 10 large messages */
	printf ("Test 04-d: fifth send operation..\n");
	if (! test_04_d_send_content_2 (connection, channel, queue))
		return axl_false;

	/* now try something similar with ANS/NUL frames.. */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI_SIMPLE_ANS_NUL,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("ERROR (11): Unable to create the channel..");
		return axl_false;
	} /* end if */

	printf ("Test 04-d: requesting to change remote buffer (5)..\n");
	vortex_channel_set_window_size (channel, 1024);

	/* send 10 large messages */
	printf ("Test 04-d: sixth send operation..\n");
	if (! test_04_d_send_content_ans_2 (connection, channel, queue, 517))
		return axl_false;

	/* now try something similar with ANS/NUL frames.. */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI_SIMPLE_ANS_NUL,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("ERROR (11): Unable to create the channel..");
		return axl_false;
	} /* end if */

	printf ("Test 04-d: requesting to change remote buffer (5)..\n");
	vortex_channel_set_window_size (channel, 1024);

	/* send 10 large messages */
	printf ("Test 04-d: seven send operation (%d)..\n", 1020);
	if (! test_04_d_send_content_ans_2 (connection, channel, queue, 1020))
		return axl_false;

	/* close connection */
	vortex_connection_close (connection);

	/* clear queue */
	vortex_async_queue_unref (queue);

	return axl_true;
}

VortexConnection * test_06_enable_unified_api (void)
{
	return vortex_connection_new (ctx, listener_host, LISTENER_UNIFIED_SASL_PORT, NULL, NULL);
}


/** 
 * @brief Checking SASL profile support.
 * 
 * @return axl_true if ok, otherwise, axl_false is returned.
 */
axl_bool  test_06 (void)
{
#if defined(ENABLE_SASL_SUPPORT)
	VortexStatus       status;
	char             * status_message = NULL;
	VortexConnection * connection;
	axl_bool           use_unified_api = axl_false;

	/* check and initialize  SASL support */
	if (! vortex_sasl_init (ctx)) {
		printf ("--- WARNING: Unable to begin SASL negotiation. Current Vortex Library doesn't support SASL");
		return axl_true;
	}

test_06_run_test:
	
	/* check to enable listener unified handling */
	if (use_unified_api) {
		printf ("Test 06: Testing unified API..\n");
		connection = test_06_enable_unified_api ();
	} else 
		connection = connection_new ();

	/* check connection created */
	if (! vortex_connection_is_ok (connection, axl_false)) {
		return axl_false;
	}

	/**** CHECK SASL ANONYMOUS MECHANISM ****/
	printf ("Test 06: SASL ANONYMOUS profile support ");

	/* begin SASL ANONYMOUS negotiation */ 
	vortex_sasl_set_propertie (connection, VORTEX_SASL_ANONYMOUS_TOKEN,
				   "test-fail@aspl.es", NULL);
	
	vortex_sasl_start_auth_sync (connection, VORTEX_SASL_ANONYMOUS, &status, &status_message);

	if (status != VortexError) {
		printf ("Expected failed anonymous SASL login..\n");
		return axl_false;
	} /* end if */

	/* begin SASL ANONYMOUS negotiation */ 
	vortex_sasl_set_propertie (connection, VORTEX_SASL_ANONYMOUS_TOKEN,
				   "test@aspl.es", NULL);
	
	vortex_sasl_start_auth_sync (connection, VORTEX_SASL_ANONYMOUS, &status, &status_message);

	if (status != VortexOk) {
		printf ("Failed to authenticate expected anonymous to work..\n");
		return axl_false;
	} /* end if */


	/* close the connection */
	vortex_connection_close (connection);

	printf ("[   OK   ]\n");

	/**** CHECK SASL EXTERNAL MECHANISM ****/

	/* create a new connection */
	connection = connection_new ();

	/* check connection created */
	if (! vortex_connection_is_ok (connection, axl_false)) {
		return axl_false;
	}

	printf ("Test 06: SASL EXTERNAL profile support ");

	/* set external properties */
	vortex_sasl_set_propertie (connection, VORTEX_SASL_AUTHORIZATION_ID,
				   "acinom1", NULL);

	/* begin external auth */
	vortex_sasl_start_auth_sync (connection, VORTEX_SASL_EXTERNAL, &status, &status_message);

	if (status != VortexError) {
		printf ("Expected to find an authentication failed for the EXTERNAL mechanism..\n");
		return axl_false;
	} /* end if */

	/* check SASL channels opened at this point */
	if (vortex_connection_channels_count (connection) != 1) {
		printf ("Expected to find only one channel but found: %d..\n", 
			vortex_connection_channels_count (connection));
		return axl_false;
	} /* end if */

	/* set external properties */
	vortex_sasl_set_propertie (connection, VORTEX_SASL_AUTHORIZATION_ID,
				   "acinom", NULL);

	/* begin external auth */
	vortex_sasl_start_auth_sync (connection, VORTEX_SASL_EXTERNAL, &status, &status_message);

	if (status != VortexOk) {
		printf ("Failed to authenticate expected anonymous to work..\n");
		return axl_false;
	} /* end if */

	/* close the connection */
	vortex_connection_close (connection);

	printf ("[   OK   ]\n");

	/**** CHECK PLAIN MECHANISM ****/

	printf ("Test 06: SASL PLAIN profile support ");
	
	/* create a new connection */
	connection = connection_new ();

	/* check connection created */
	if (! vortex_connection_is_ok (connection, axl_false)) {
		return axl_false;
	}

	/* set plain properties */
	vortex_sasl_set_propertie (connection, VORTEX_SASL_AUTH_ID,
				   "bob", NULL);

	/* set plain properties (BAD password) */
	vortex_sasl_set_propertie (connection,  VORTEX_SASL_PASSWORD,
				   "secret1", NULL);

	/* begin plain auth */
	vortex_sasl_start_auth_sync (connection, VORTEX_SASL_PLAIN, &status, &status_message);

	if (status != VortexError) {
		printf ("Expected to find a PLAIN mechanism failure but it wasn't found.\n");
		return axl_false;
	} /* end if */

	/* check SASL channels opened at this point */
	if (vortex_connection_channels_count (connection) != 1) {
		printf ("Expected to find only one channel but found: %d..\n", 
			vortex_connection_channels_count (connection));
		return axl_false;
	} /* end if */

	/* set plain properties */
	vortex_sasl_set_propertie (connection, VORTEX_SASL_AUTH_ID,
				   "bob", NULL);

	/* set plain properties (GOOD password) */
	vortex_sasl_set_propertie (connection,  VORTEX_SASL_PASSWORD,
				   "secret", NULL);

	/* begin plain auth */
	vortex_sasl_start_auth_sync (connection, VORTEX_SASL_PLAIN, &status, &status_message);

	if (status != VortexOk) {
		printf ("Expected to find a success PLAIN mechanism but it wasn't found.\n");
		return axl_false;
	} /* end if */

	/* close the connection */
	vortex_connection_close (connection);

	printf ("[   OK   ]\n");

	/**** CHECK CRAM-MD5 MECHANISM ****/

	printf ("Test 06: SASL CRAM-MD5 profile support ");

	/* create a new connection */
	connection = connection_new ();

	/* check connection created */
	if (! vortex_connection_is_ok (connection, axl_false)) {
		return axl_false;
	}

	/* set cram-md5 properties */
	vortex_sasl_set_propertie (connection, VORTEX_SASL_AUTH_ID,
				   "bob", NULL);

	/* set cram-md5 properties (BAD password) */
	vortex_sasl_set_propertie (connection,  VORTEX_SASL_PASSWORD,
				   "secret1", NULL);

	/* begin cram-md5 auth */
	vortex_sasl_start_auth_sync (connection, VORTEX_SASL_CRAM_MD5, &status, &status_message);

	if (status != VortexError) {
		printf ("Expected to find a CRAM-MD5 mechanism failure but it wasn't found.\n");
		return axl_false;
	} /* end if */

	/* check SASL channels opened at this point */
	if (vortex_connection_channels_count (connection) != 1) {
		printf ("Expected to find only one channel but found: %d..\n", 
			vortex_connection_channels_count (connection));
		return axl_false;
	} /* end if */

	/* set cram-md5 properties */
	vortex_sasl_set_propertie (connection, VORTEX_SASL_AUTH_ID,
				   "bob", NULL);

	/* set cram-md5 properties (GOOD password) */
	vortex_sasl_set_propertie (connection,  VORTEX_SASL_PASSWORD,
				   "secret", NULL);

	/* begin cram-md5 auth */
	vortex_sasl_start_auth_sync (connection, VORTEX_SASL_CRAM_MD5, &status, &status_message);

	if (status != VortexOk) {
		printf ("Expected to find a success CRAM-MD5 mechanism but it wasn't found.\n");
		return axl_false;
	} /* end if */

	/* close the connection */
	vortex_connection_close (connection);

	printf ("[   OK   ]\n");

#ifndef AXL_OS_WIN32
	/**** CHECK DIGEST-MD5 MECHANISM ****/
	printf ("Test 06: SASL DIGEST-MD5 profile support ");

	/* create a new connection */
	connection = connection_new ();

	/* check connection created */
	if (! vortex_connection_is_ok (connection, axl_false)) {
		return axl_false;
	}

	/* set DIGEST-MD5 properties */
	vortex_sasl_set_propertie (connection, VORTEX_SASL_AUTH_ID,
				   "bob", NULL);

	/* set DIGEST-MD5 properties (BAD password) */
	vortex_sasl_set_propertie (connection,  VORTEX_SASL_PASSWORD,
				   "secret1", NULL);
	/* begin DIGEST-MD5 auth */
	vortex_sasl_start_auth_sync (connection, VORTEX_SASL_DIGEST_MD5, &status, &status_message);

	if (status != VortexError) {
		printf ("Expected to find a DIGEST-MD5 mechanism failure but it wasn't found.\n");
		return axl_false;
	} /* end if */

	if (vortex_connection_channels_count (connection) != 1) {
		printf ("Expected to find only one channel but found: %d..\n", 
			vortex_connection_channels_count (connection));
		return axl_false;
	} /* end if */

	/* set DIGEST-MD5 properties */
	vortex_sasl_set_propertie (connection, VORTEX_SASL_AUTH_ID,
				   "bob", NULL);

	/* set DIGEST-MD5 properties (GOOD password) */
	vortex_sasl_set_propertie (connection,  VORTEX_SASL_PASSWORD,
				   "secret", NULL);

	/* set DIGEST-MD5 properties realm */
	vortex_sasl_set_propertie (connection,  VORTEX_SASL_REALM,
				   "aspl.es", NULL);

	/* begin DIGEST-MD5 auth */
	vortex_sasl_start_auth_sync (connection, VORTEX_SASL_DIGEST_MD5, &status, &status_message);

	if (status != VortexOk) {
		printf ("Expected to find a success DIGEST-MD5 mechanism but it wasn't found.\n");
		return axl_false;
	} /* end if */

	/* close the connection */
	vortex_connection_close (connection);

	printf ("[   OK   ]\n");
#endif
	/* check for unified api */
	if (! use_unified_api) {
		use_unified_api = axl_true;
		goto test_06_run_test;
	}
#else
	printf ("--- WARNING: unable to run SASL tests, no sasl library was built\n");
#endif	
	return axl_true;
}

axl_bool test_06a (void) {

#if defined(ENABLE_SASL_SUPPORT)

	VortexStatus       status;
	char             * status_message = NULL;
	VortexConnection * connection;

	/* check and initialize  SASL support */
	if (! vortex_sasl_init (ctx)) {
		printf ("--- WARNING: Unable to begin SASL negotiation. Current Vortex Library doesn't support SASL");
		return axl_true;
	}

	/* do a connection */
	printf ("Test 06-a: Testing SASL serverName on auth request..\n");
	connection = vortex_connection_new_full (ctx, listener_host, LISTENER_UNIFIED_SASL_PORT, 
						 CONN_OPTS (VORTEX_SERVERNAME_FEATURE, "test_06a.server"),
						 NULL, NULL);
	/* check connection created */
	if (! vortex_connection_is_ok (connection, axl_false)) {
		return axl_false;
	}	

	/*** request plain validation ***/
	/* set plain properties */
	vortex_sasl_set_propertie (connection, VORTEX_SASL_AUTH_ID,
				   "12345", NULL);

	/* set plain properties (GOOD password) */
	vortex_sasl_set_propertie (connection,  VORTEX_SASL_PASSWORD,
				   "12345", NULL);

	/* begin plain auth */
	vortex_sasl_start_auth_sync (connection, VORTEX_SASL_PLAIN, &status, &status_message);

	if (status != VortexOk) {
		printf ("Expected to find a success PLAIN mechanism but it wasn't found.\n");
		return axl_false;
	} /* end if */

	/* close the connection */
	vortex_connection_close (connection);

#endif /* ENABLE_SASL_SUPPORT */


	return axl_true;
}

/** 
 * @brief Test XML-RPC support.
 * 
 * 
 * @return axl_true if all test pass, otherwise axl_false is returned.
 */
axl_bool  test_07 (void) {
	
#if defined(ENABLE_XML_RPC_SUPPORT)
	VortexConnection * connection;
	VortexChannel    * channel;
	int                iterator;
	/* test 02 */
	char             * result;
	/* test 03 */
	axl_bool           bresult;
	/* test 04 */
	double             dresult;
	/* test 05 */
	Values           * a;
	Values           * b;
	Values           * struct_result;
	/* test 06 */
	ItemArray        * array;
	Item             * item;
	/* test 07 */
	Node             * node;
	Node             * node_result;

	/* init xml-rpc module */
	if (! vortex_xml_rpc_init (ctx)) {
		printf ("--- WARNING: unable to start XML-RPC profile, failed to init XML-RPC library\n");
		return axl_false;
	} /* end if */

	/* create a new connection */
	connection = connection_new ();

	/* create the xml-rpc channel */
	channel = BOOT_CHANNEL (connection, NULL);

	/*** TEST 01 ***/
	printf ("Test 07: xml-rpc test 01..ok\n");
	if (7 != test_sum_int_int_s (3, 4, channel, NULL, NULL, NULL)) {
		fprintf (stderr, "ERROR: An error was found while invoking..\n");
		return axl_false;
	}

	/* perform the invocation */
	iterator = 0;
	while (iterator < 100) {

		if (-3 != test_sum_int_int_s (10, -13, channel, NULL, NULL, NULL)) {
			fprintf (stderr, "ERROR: An error was found while invoking..\n");
			return axl_false;
		}
		iterator++;
	}
	printf ("Test 07: xml-rpc test 01..ok\n");

	/*** TEST 02 ***/
	/* get the string from the function */
	result = test_get_the_string_s (channel, NULL, NULL, NULL);
	if (result == NULL) {
		fprintf (stderr, "An error was found while retreiving the result, a NULL value was received when an string was expected\n");
		return axl_false;
	}
	
	if (! axl_cmp (result, "This is a test")) {
		fprintf (stderr, "An different string value was received than expected '%s' != '%s'..",
			 "This is a test", result);
		return axl_false;
	}

	/* free the result */
	axl_free (result);
	printf ("Test 07: xml-rpc test 02..ok\n");

	/*** TEST 03 ***/
	/* get the string from the function */
	bresult = test_get_the_bool_1_s (channel, NULL, NULL, NULL);
	if (bresult != axl_false) {
		fprintf (stderr, "Expected to receive a axl_false bool value..\n");
		return axl_false;
	}

	bresult = test_get_the_bool_2_s (channel, NULL, NULL, NULL);
	if (bresult != axl_true) {
		fprintf (stderr, "Expected to receive a axl_true bool value..\n");
		return axl_false;
	}
	printf ("Test 07: xml-rpc test 03..ok\n");

	/*** TEST 04 ***/
	/* get the string from the function */
	dresult = test_get_double_sum_double_double_s (7.2, 8.3, channel, NULL, NULL, NULL);
	if (dresult != 15.5) {
		fprintf (stderr, "Expected to receive double value 15.5 but received: %g..\n", dresult);
		return axl_false;
	}
	printf ("Test 07: xml-rpc test 04..ok\n");

	/*** TEST 05 ***/
	/* get the string from the function */
	a      = test_values_new (3, 10.2, axl_false);
	b      = test_values_new (7, 7.12, axl_true);
	
	/* perform invocation */
	struct_result = test_get_struct_values_values_s (a, b, channel, NULL, NULL, NULL);

	if (struct_result == NULL) {
		fprintf (stderr, "Expected to receive a non NULL result from the service invocation\n");
		return axl_false;
	}

	if (struct_result->count != 10) {
		fprintf (stderr, "Expected to receive a value not found (count=10 != count=%d\n",
			    struct_result->count);
		return axl_false;
	}

	if (struct_result->fraction != 17.32) {
		fprintf (stderr, "Expected to receive a value not found (fraction=17.32 != fraction=%g\n",
			    struct_result->fraction);
		return axl_false;
	}

	if (! struct_result->status) {
		fprintf (stderr, "Expected to receive a axl_true value for status\n");
		return axl_false;
	}

	/* release memory allocated */
	test_values_free (a);
	test_values_free (b);
	test_values_free (struct_result);
	printf ("Test 07: xml-rpc test 05..ok\n");

	/*** TEST 06 ***/
	/* get the array content */
	array = test_get_array_s (channel, NULL, NULL, NULL);
	
	if (array == NULL) {
		fprintf (stderr, "Expected to receive a non NULL result from the service invocation\n");
		return axl_false;
	}

	/* for each item */
	iterator = 0;
	for (; iterator < 10; iterator++) {
		/* get a reference to the item */
		item = test_itemarray_get (array, iterator);

		/* check null reference */
		if (item == NULL) {
			fprintf (stderr, "Expected to find a struct reference inside..\n");
			return axl_false;
		}

		/* check the int value inside the struct */
		if (item->position != iterator) {
			fprintf (stderr, "Expected to find an integer value: (%d) != (%d)\n",
				    item->position, iterator);
			return axl_false;
		}

		/* check the string inside */
		if (! axl_cmp (item->string_position, "test content")) {
			fprintf (stderr, "Expected to find a string value: (%s) != (%s)\n",
				 item->string_position, "test content");
			return axl_false;
		}
	}
	

	/* free result */
	test_itemarray_free (array);
	printf ("Test 07: xml-rpc test 06..ok\n");

	/*** TEST 07 ***/
	/* get the array content */
	node_result = test_get_list_s (channel, NULL, NULL, NULL);
	if (node_result == NULL) {
		fprintf (stderr, "Expected to receive a non NULL node_result from the service invocation\n");
		return -1;
	}

	/* for each node */
	iterator = 0;
	node     = node_result;
	for (; iterator < 8; iterator++) {
		if (node == NULL) {
			fprintf (stderr, "Unexpected NULL received\n");
			return -1;
		}

		/* check content */
		if (node->position != (iterator + 1)) {
			fprintf (stderr, "Unexpected position received\n");
			return -1;
		}
		
		/* get the next reference */
		node = node->next;
	}

	/* free node_result */
	test_node_free (node_result);
	printf ("Test 07: xml-rpc test 07..ok\n");

	/* close the connection */
	vortex_connection_close (connection);

	/* terminate xml-rpc library */
	vortex_xml_rpc_cleanup (ctx);
#else
	printf ("--- WARNING: unable to run XML-RPC tests, no xml-rpc library was built\n");
#endif

	return axl_true;
}

/** 
 * @brief Checks if the serverName attribute is properly configured
 * into the connection once the first successfull channel is created.
 * 
 * 
 * @return axl_true if serverName is properly configured, otherwise axl_false
 * is returned.
 */
axl_bool  test_08 (void)
{
	VortexConnection * connection;
	VortexChannel    * channel;
	VortexAsyncQueue * queue;
	VortexFrame      * frame;

	/* disable automatic serverName acquire */
	vortex_ctx_server_name_acquire (ctx, axl_false);

	/* create the queue */
	queue      = vortex_async_queue_new ();

	/* create a new connection */
	connection = connection_new ();

	/* create a channel */
	channel = vortex_channel_new_full (connection, 0,
					   /* server name value */
					   "dolphin.aspl.es",
					   /* profile */
					   REGRESSION_URI,
					   /* profile piggy back initial content and its encoding */
					   EncodingNone,
					   NULL, 0,
					   /* no close handling */
					   NULL, NULL,
					   /* frame receive async handling */
					   vortex_channel_queue_reply, queue,
					   /* no async channel creation */
					   NULL, NULL);
	/* check channel created */
	if (channel == NULL) {
		printf ("Unable to create channel...\n");
		return axl_false;
	}

	/* check server name configuration */
	if (! axl_cmp ("dolphin.aspl.es", vortex_connection_get_server_name (connection))) {
		printf ("Failed to get the proper serverName value for the connection..\n");
		return axl_false;
	}

	/* ask for the server name that have the remote side */
	vortex_channel_send_msg (channel, 
				 "GET serverName",
				 14, 
				 NULL);
	
	/* get the reply */
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("Failed to get the reply from the server..\n");
		return axl_false;
	}
	if (! axl_cmp (vortex_frame_get_payload (frame), "dolphin.aspl.es")) {
		printf ("Received a different server name configured that expected: %s..\n",
			(char*) vortex_frame_get_payload (frame));
		return axl_false;
	}

	/* free frame */
	vortex_frame_unref (frame);

	/* create a channel */
	channel = vortex_channel_new_full (connection, 0,
					   /* server name value */
					   "whale.aspl.es",
					   /* profile */
					   REGRESSION_URI,
					   /* profile piggy back initial content and its encoding */
					   EncodingNone,
					   NULL, 0,
					   /* no close handling */
					   NULL, NULL,
					   /* frame receive async handling */
					   vortex_channel_queue_reply, queue,
					   /* no async channel creation */
					   NULL, NULL);
	/* check channel created */
	if (channel == NULL) {
		printf ("Unable to create channel...\n");
		return axl_false;
	}

	/* check server name configuration */
	if (! axl_cmp ("dolphin.aspl.es", vortex_connection_get_server_name (connection))) {
		printf ("Failed to get the proper serverName value for the connection..\n");
		return axl_false;
	}

	/* ask for the server name that have the remote side */
	vortex_channel_send_msg (channel, 
				 "GET serverName",
				 14, 
				 NULL);
	
	/* get the reply */
	frame = vortex_channel_get_reply (channel, queue);
	if (frame == NULL) {
		printf ("Failed to get the reply from the server..\n");
		return axl_false;
	}
	if (! axl_cmp (vortex_frame_get_payload (frame), "dolphin.aspl.es")) {
		printf ("Received a different server name configured that expected: %s..\n",
			(char*) vortex_frame_get_payload (frame));
		return axl_false;
	}

	/* free frame */
	vortex_frame_unref (frame);

	/* free queue */
	vortex_async_queue_unref (queue);

	/* close the connection */
	vortex_connection_close (connection);

	/* reenable */
	vortex_ctx_server_name_acquire (ctx, axl_true);

	return axl_true;
}

/** 
 * @brief Checks if the default handler for close channel request is
 * to accept the close operation.
 * 
 * 
 * @return axl_false if it fails, otherwise axl_true is returned.
 */
axl_bool  test_10 (void)
{
	VortexConnection * connection;
	VortexChannel    * channel;

	/* create a new connection */
	connection = connection_new ();

	/* create a channel */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI_2,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      NULL, NULL,
				      /* no async channel creation */
				      NULL, NULL);

	/* close the channel */
	if (! vortex_channel_close (channel, NULL)) {
		printf ("Test 10: failed to close the regression channel when a possitive reply was expected\n");
		return axl_false;
	}

	/* create a channel */
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      NULL, NULL,
				      /* no async channel creation */
				      NULL, NULL);

	/* close the channel */
	if (! vortex_channel_close (channel, NULL)) {
		printf ("Test 10: failed to close the regression channel when a possitive reply was expected\n");
		return axl_false;
	}

	/* close the connection */
	vortex_connection_close (connection);
	return axl_true;
}

/** 
 * @brief Check if the caller can reply to a message that is not the
 * next to be replied without being blocked.
 * 
 * 
 * @return axl_false if it fails, otherwise axl_true is returned.
 */
axl_bool  test_11 (void)
{
	VortexConnection * connection;
	VortexChannel    * channel;
	VortexAsyncQueue * queue;
	VortexFrame      * frame;
	VortexFrame      * frame2;
	VortexFrame      * frame3;
	int                iterator;
	int                pattern = 0;

	/* create a new connection */
	connection = connection_new ();

	/* create a channel */
	queue   = vortex_async_queue_new ();
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI_3,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      vortex_channel_queue_reply, queue,
				      /* no async channel creation */
				      NULL, NULL);

	if (channel == NULL) {
		printf ("Failed to create channel..\n");
		return axl_false;
	}

	/* set serialize */
	vortex_channel_set_serialize (channel, axl_true);

	/* send a message to activate the test, then the listener will
	 * ack it and send three messages that will be replied in
	 * wrong other. */
	iterator = 0;
	while (iterator < 10) {
		/* printf ("  --> activating test (%d)..\n", iterator); */
		vortex_channel_send_msg (channel, "", 0, NULL);
		
		/* get the reply and unref */
		frame = vortex_channel_get_reply (channel, queue);
		if (frame == NULL) {
			printf ("Test 11: failed, expected to find reply but NULL received..\n");
			return axl_false;
		}
		if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY) {
			printf ("Test 11: failed, expected to find RPY reply defined but received: %d, content: (length: %d) '%s'..\n",
				vortex_frame_get_type (frame), vortex_frame_get_payload_size (frame), (char*) vortex_frame_get_payload (frame));
			return axl_false;
		} /* end if */
		vortex_frame_unref (frame);

		/* Test 11: test in progress... */
		
		/* ask for all replies */
		frame     = NULL;
		frame2    = NULL;
		frame3    = NULL;

		if (! vortex_connection_is_ok (connection, axl_false)) {
			printf ("Test 11: failed, detected connection close before getting first frame..\n");
			return axl_false;
		}

		/* get the first frame */
		frame = vortex_channel_get_reply (channel, queue);
		/* printf ("."); */
		if (frame == NULL || vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_MSG) {
			printf ("Test 11: failed, expected to find defined frame and using type MSG\n");
			return axl_false;
		} /* end if */

		if (! vortex_connection_is_ok (connection, axl_false)) {
			printf ("Test 11: failed, detected connection close before getting second frame..\n");
			return axl_false;
		}
		
		/* get the first frame2 */
		frame2 = vortex_channel_get_reply (channel, queue);
		/* printf ("."); */
		if (frame2 == NULL || vortex_frame_get_type (frame2) != VORTEX_FRAME_TYPE_MSG) {
			printf ("Test 11: failed, expected to find defined frame and using type MSG\n");
			return axl_false;
		} /* end if */

		if (! vortex_connection_is_ok (connection, axl_false)) {
			printf ("Test 11: failed, detected connection close before getting third frame..\n");
			return axl_false;
		}
		
		/* get the first frame2 */
		frame3 = vortex_channel_get_reply (channel, queue);
		
		/* printf ("."); */
		if (frame3 == NULL || vortex_frame_get_type (frame3) != VORTEX_FRAME_TYPE_MSG) {
			printf ("Test 11: failed, expected to find defined frame and using type MSG\n");
			return axl_false;
		} /* end if */
		
		/* now reply in our of order */
		if (pattern == 0) {
			/* send a reply */
			vortex_channel_send_rpy (channel, "AA", 2, vortex_frame_get_msgno (frame2));
			vortex_channel_send_rpy (channel, "BB", 2, vortex_frame_get_msgno (frame));
			vortex_channel_send_rpy (channel, "CC", 2, vortex_frame_get_msgno (frame3));
			pattern++;

		} else if (pattern == 1) {
			
			/* send a reply */
			vortex_channel_send_rpy (channel, "AA", 2, vortex_frame_get_msgno (frame3));
			vortex_channel_send_rpy (channel, "BB", 2, vortex_frame_get_msgno (frame2));
			vortex_channel_send_rpy (channel, "CC", 2, vortex_frame_get_msgno (frame));
			pattern++;
			
		} else if (pattern == 2) {

			/* send a reply */
			vortex_channel_send_rpy (channel, "AA", 2, vortex_frame_get_msgno (frame2));
			vortex_channel_send_rpy (channel, "BB", 2, vortex_frame_get_msgno (frame3));
			vortex_channel_send_rpy (channel, "CC", 2, vortex_frame_get_msgno (frame));
			pattern = 0;
			
		} /* end if */

		if (! vortex_connection_is_ok (connection, axl_false)) {
			printf ("Test 11: failed, detected connection close after sending data..\n");
			return axl_false;
		}
		
		/* delete all frames */
		vortex_frame_unref (frame);
		vortex_frame_unref (frame2);
		vortex_frame_unref (frame3);
		
		/* next test */
		iterator++;
		
	} /* end while */
	
	/* close the channel */
	if (! vortex_channel_close (channel, NULL)) {
		printf ("Test 11: failed to close the regression channel when a possitive reply was expected\n");
		return axl_false;
	}

	/* close the connection */
	vortex_connection_close (connection);

	/* unref the queue */
	vortex_async_queue_unref (queue);

	return axl_true;
}

/** 
 * @brief Checks connection creating timeout.
 * 
 * 
 * @return axl_true if test pass, otherwise axl_false is returned.
 */
axl_bool  test_12 (void) {
	VortexConnection  * connection;
	VortexConnection  * control;
	VortexChannel     * c_channel;
	VortexChannel     * channel;
	int                 stamp;
	VortexAsyncQueue  * queue;
	VortexFrame       * frame;

	/* checking timeout connection activated.. */
	printf ("Test 12: .");
	fflush (stdout);

	/* check current timeout */
	if (vortex_connection_get_connect_timeout (ctx) != 0) {
		printf ("Test 12 (1): failed, expected to receive an empty timeout configuration: but received %ld..\n",
			vortex_connection_get_connect_timeout (ctx));
		return axl_false;
	}

	/* configure a new timeout (10 seconds, 10000000 microseconds) */
	vortex_connection_connect_timeout (ctx, 10000000);

	/* check new timeout configured */
	if (vortex_connection_get_connect_timeout (ctx) != 10000000) {
		printf ("Test 12 (2): failed, expected to receive 10000000 timeout configuration: but received %ld..\n",
			vortex_connection_get_connect_timeout (ctx));
		return axl_false;
	}

	/* creates a new connection against localhost:44000 */
	/* doing connect operation with timeout activated.. */
	printf (".");
	fflush (stdout);
	stamp      = time (NULL);
	connection = connection_new ();
	if (! vortex_connection_is_ok (connection, axl_false)) {
		printf ("Test 12 (3): failed to connect to: %s:%s...reason: %s\n",
			listener_host, LISTENER_PORT, vortex_connection_get_message (connection));
		vortex_connection_close (connection);
		return axl_false;
	}

	/* check stamp before continue */
	if ((time (NULL) - stamp) > 1) {
		printf ("Test 12 (3.1): failed, expected less connection time while testing..\n");
		return axl_false;
	}

	/* connected */
	printf (".");
	fflush (stdout);
	
	/* ok, close the connection */
	vortex_connection_close (connection);
	printf (".");
	fflush (stdout);

	/* disable timeout */
	vortex_connection_connect_timeout (ctx, 0);

	/* check new timeout configured */
	if (vortex_connection_get_connect_timeout (ctx) != 0) {
		printf ("Test 12 (4): failed, expected to receive 0 timeout configuration, after clearing: but received %ld..\n",
			vortex_connection_get_connect_timeout (ctx));
		return axl_false;
	}

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, axl_false)) {
		printf ("Test 12 (5): failed to connect to: %s:%s...reason: %s\n",
			listener_host, LISTENER_PORT, vortex_connection_get_message (connection));
		vortex_connection_close (connection);
		return axl_false;
	}
	
	/* ok, close the connection */
	printf (".");
	vortex_connection_close (connection);

	/*** check unreachable host with timeout activated ****/

	/* enable timeout again */
	vortex_connection_connect_timeout (ctx, 5000000);
	printf (".");
	fflush (stdout);

	/* check new timeout configured */
	if (vortex_connection_get_connect_timeout (ctx) != 5000000) {
		printf ("Test 12 (6): failed, expected to receive 10000000 timeout configuration, after configuring: but received %ld..\n",
			vortex_connection_get_connect_timeout (ctx));
		return axl_false;
	}
	printf (".");
	fflush (stdout);

	/* try to connect to an unreachable host */
	connection = vortex_connection_new (ctx, listener_host, "44012", NULL, NULL);
	stamp      = time (NULL);
	if (vortex_connection_is_ok (connection, axl_false)) {
		printf ("Test 12 (7): failed, expected to NOT to connect to: %s:%s...reason: %s\n",
			listener_host, LISTENER_PORT, vortex_connection_get_message (connection));
		vortex_connection_close (connection);
		return axl_false;
	}
	printf (".");
	fflush (stdout);
	
	/* check stamp (check unreachable host doesn't take any
	 * special time) */
	if (stamp != time (NULL) && (stamp + 1) != time (NULL)) {
		printf ("Test 12 (7.1): failed, expected no especial timeout for an unreachable connect operation..\n");
		vortex_connection_close (connection);
		return axl_false;
	} /* end if */

	/* ok, close the connection */
	vortex_connection_close (connection);
	printf (".");
	fflush (stdout);

	/*** create a fake listener which do not have response to
	 * check timemout */
	printf ("...create fake listener...");
	
	/* create a control connection and an special channel to start
	 * a listener */
	control = connection_new ();
	if (! vortex_connection_is_ok (control, axl_false)) {
		printf ("Test 12 (7.2): failed to create control connection, unable to start remote fake listener..\n");
		vortex_connection_close (connection);
		return axl_false;
	} /* end if */
	
	/* CREATE CONTROL CHANNEL: now create the fake listener */
	queue     = vortex_async_queue_new ();
	c_channel = vortex_channel_new (control, 0, REGRESSION_URI_LISTENERS,
					/* default on close */
					NULL, NULL, 
					/* default frame received */
					vortex_channel_queue_reply, queue,
					/* default on channel created */
					NULL, NULL);
	if (c_channel == NULL) {
		printf ("Test 12 (7.3): failed to create remote listener creation channel, unable to start remote fake listener..\n");
		vortex_connection_close (connection);
		return axl_false;
	} /* end if */

	/* CREATE FAKE LISTENER: call to create a remote listener
	 * (blocked) running on the 44012 port */
	if (! vortex_channel_send_msg (c_channel, "create-listener", 15, NULL)) {
		printf ("Test 12 (7.4): failed to send message to create remote fake listener...\n");
		return axl_false;
	}

	/* get reply */
	frame = vortex_channel_get_reply (c_channel, queue);
	if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY) {
		printf ("Test 12 (7.5): failed to start remote fake listener, received negative reply..\n");
		return axl_false;
	} /* end if */
	vortex_frame_unref (frame);
	printf ("remote fake listener created");
	
	/* CONNECT TO UNRESPONSIVE LISTENER: now try to connect
	 * again */
	printf ("...connect (wait 5 seconds)");
	fflush (stdout);
	stamp      = time (NULL);
	connection = vortex_connection_new (ctx, listener_host, "44012", NULL, NULL);
	if (vortex_connection_is_ok (connection, axl_false)) {
		printf ("\nTest 12 (9): failed to connect to: %s:%s...reason: %s\n",
			listener_host, "44012", vortex_connection_get_message (connection));
		vortex_connection_close (connection);
		return axl_false;
	}

	/* ok, close the connection */
	printf ("...check (step 9 ok)..");
	fflush (stdout);
	vortex_connection_close (connection);
	if ((stamp + 3) > time (NULL)) {
		printf ("Test 12 (9.1): supposed to perform a connection failed, with a timeout about of 3 seconds but only consumed: %ld seconds..\n",
			(time (NULL)) - stamp);
		return axl_false;
	}

	/* now unlock the listener */
	printf ("..unlock listener..");
	fflush (stdout);
	/* CALL TO UNLOCK THE REMOTE LISTENER: call to create a remote
	 * listener (blocked) running on the 44012 port */
	if (! vortex_channel_send_msg (c_channel, "unlock-listener", 15, NULL)) {
		printf ("Test 12 (7.4): failed to send message to create remote fake listener...\n");
		return axl_false;
	}

	/* get reply */
	frame = vortex_channel_get_reply (c_channel, queue);
	if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY) {
		printf ("Test 12 (7.5): failed to start remote fake listener, received negative reply..\n");
		return axl_false;
	} /* end if */
	vortex_frame_unref (frame);
	printf ("ok..");
	
	/* CONNECT TO RESPONSE SERVER: now try to connect again */
	printf ("..now connecting");
	fflush (stdout);
	connection = vortex_connection_new (ctx, listener_host, "44012", NULL, NULL);
	if (! vortex_connection_is_ok (connection, axl_false)) {
		printf ("Test 12 (10): failed to connect to: %s:%s...reason: %s\n",
			listener_host, LISTENER_PORT, vortex_connection_get_message (connection));
		vortex_connection_close (connection);
		return axl_false;
	} 

	/* create a channel */
	printf ("..creating a channel..");

	/* register a profile */
	vortex_profiles_register (ctx, REGRESSION_URI,
				  NULL, NULL, 
				  NULL, NULL,
				  NULL, NULL);

	fflush (stdout);
	channel = vortex_channel_new (connection, 0,
				      REGRESSION_URI,
				      /* no close handling */
				      NULL, NULL,
				      /* frame receive async handling */
				      NULL, NULL,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("Test 12 (10.1): Unable to create the channel..");
		return axl_false;
	}

	/* ok, close the channel */
	vortex_channel_close (channel, NULL);

	/* now unregister */
	vortex_profiles_unregister (ctx, REGRESSION_URI);

	/* close the connection created */
	printf (".");
	fflush (stdout);
	vortex_connection_close (connection);

	/* CLOSE REMOTE SERVER: call to close remote fake listener */
	if (! vortex_channel_send_msg (c_channel, "close-listener", 14, NULL)) {
		printf ("Test 12 (7.4): failed to send message to create remote fake listener...\n");
		return axl_false;
	}

	/* get reply */
	frame = vortex_channel_get_reply (c_channel, queue);
	if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY) {
		printf ("Test 12 (7.5): failed to start remote fake listener, received negative reply..\n");
		return axl_false;
	} /* end if */
	vortex_frame_unref (frame);
	printf ("remote fake listener created");
	
	/* close control connection */
	vortex_channel_close (c_channel, NULL);
	vortex_connection_close (control);

	/* free queue */
	vortex_async_queue_unref (queue);
	
	/* return axl_true */
	printf ("\n");
	return axl_true;
}


/** 
 * @brief Allows to check the close in transit support.
 *
 * The intention of this test is not only to check close in transit
 * support but also to show how it works.
 *
 * The idea is that provided a BEEP profile that both peers are able
 * to close the same channel at the same time you must update the
 * channel reference count to avoid having a reference to a channel
 * that was closed because your peer received a close request you
 * accepted (due to the close handler or the default action which is
 * to accept the close request).
 *
 * This test creates a channel, send a message and close channel. The
 * server receives the message, replies to it and sends a close.
 *
 * This makes two situations to appear:
 *
 * 1) 
 *     - Client creates the channel and send the message
 *     - Listener side replies and closes
 *     - Client closes a channel that is closed.
 *
 * 2)  - Client creates the channel and send the message.
 *     - Listener side replies and closes.
 *     - Client start to close a channel which is not found to be closed.
 *     - Client send the close msg and waits.
 *     - Listener receives the close msg while waiting for its close.
 *     - Listener sends a close accept <ok /> and uses that the client close
 *       as a confirmation to its close.
 *
 * The main difference is that the listenre side operates under the
 * execution of the frame received which has the channel reference
 * counting increased to avoid the reference to be lost. This is
 * because the listener do not require to update the channel reference
 * count while implementing BEEP profiles that support both peers to
 * issue close messages at the same time.
 *
 * At the client side, the channel reference is assumed to be
 * controled by the application. But if the application configures a
 * close notification handler (or uses the default action) accepting
 * to close the channel, then it is removed and the reference is lost.
 * 
 * @return axl_true if goes ok.
 */
axl_bool  test_09 (void) 
{
	VortexConnection * conn;
	VortexChannel    * channel;
	VortexAsyncQueue * queue;

	int                iterator;

	/* create a new connection */
	conn = connection_new ();

	/* create a channel */
	iterator = 0;
	printf ("Test 09: checking close in transit..");
	while (iterator < 10) {
		printf (".");
		fflush (stdout);
		channel = vortex_channel_new (conn, 0,
					      CLOSE_IN_TRANSIT_URI,
					      /* no close handling */
					      NULL, NULL,
					      /* frame receive async handling */
					      NULL, NULL,
					      /* no async channel creation */
					      NULL, NULL);
		vortex_channel_ref (channel);
		
		/* send a message and close */
		vortex_channel_send_msg (channel, "", 0, NULL);
		
		/* close the channel */
		if (! vortex_channel_close (channel, NULL)) {
			printf ("Failed to close the channel..\n");
			return axl_false;
		}

		/* unref the channel */
		vortex_channel_unref (channel);

		/* check channels */
		if (vortex_connection_channels_count (conn) != 1) {
			printf ("failed, expected to find one (1 != %d) channel running in the connection..\n",
				vortex_connection_channels_count (conn));
			return axl_false;
		}

		iterator++;
	} /* end if */

	printf ("\n");

	printf ("Test 09: now checking close in transit with a delay before calling to close..");
	iterator = 0;
	while (iterator < 10) {
		printf (".");
		fflush (stdout);
		channel = vortex_channel_new (conn, 0,
					      CLOSE_IN_TRANSIT_URI,
					      /* no close handling */
					      NULL, NULL,
					      /* frame receive async handling */
					      NULL, NULL,
					      /* no async channel creation */
					      NULL, NULL);
		vortex_channel_ref (channel);
		
		/* send a message and close */
		vortex_channel_send_msg (channel, "", 0, NULL);

		/* create the queue and wait */
		queue = vortex_async_queue_new ();
		vortex_async_queue_timedpop (queue, 1000);
		vortex_async_queue_unref (queue);
		
		/* close the channel */
		if (! vortex_channel_close (channel, NULL)) {
			printf ("Failed to close the channel..\n");
			return axl_false;
		}

		/* unref the channel */
		vortex_channel_unref (channel);

		/* check channels */
		if (vortex_connection_channels_count (conn) != 1) {
			printf ("failed, expected to find one (1 != %d) channel running in the connection..\n",
				vortex_connection_channels_count (conn));
			return axl_false;
		}

		iterator++;
	} /* end if */

	printf ("\n");

	/* close the connection */
	vortex_connection_close (conn);

	return axl_true;
	
}

#ifdef AXL_OS_UNIX
void __block_test (int value) 
{
	VortexAsyncQueue * queue;

	printf ("******\n");
	printf ("****** Received a signal (the regression test is failing)!!!\n");
	printf ("******\n");

	/* block the caller */
	queue = vortex_async_queue_new ();
	vortex_async_queue_pop (queue);

	return;
}
#endif

/** 
 * @brief Allows to check tunnel implementation.
 * 
 * @return axl_true if all test pass, otherwise axl_false is returned.
 */
axl_bool  test_13 (void)
{
	printf ("Test 13: ** \n");
	printf ("Test 13: ** INFO: Running test, under the TUNNEL profile (BEEP proxy support)!\n");
	printf ("Test 13: **\n");

	/* create tunnel settings */
	tunnel_settings = vortex_tunnel_settings_new (ctx);
	tunnel_tested   = axl_true;
	
	/* add first hop */
	vortex_tunnel_settings_add_hop (tunnel_settings,
					TUNNEL_IP4, listener_tunnel_host,
					TUNNEL_PORT, LISTENER_PROXY_PORT,
					TUNNEL_END_CONF);
	
	/* add end point behind it */
	vortex_tunnel_settings_add_hop (tunnel_settings,
					TUNNEL_IP4, listener_host,
					TUNNEL_PORT, LISTENER_PORT,
					TUNNEL_END_CONF);

	printf ("Test 13::");
	if (test_01 ())
		printf ("Test 01: basic BEEP support [   OK   ]\n");
	else {
		printf ("Test 01: basic BEEP support [ FAILED ]\n");
		return axl_false;
	}

	printf ("Test 13::");
	if (test_01a ())
		printf ("Test 01-a: transfer zeroed binary frames [   OK   ]\n");
	else {
		printf ("Test 01-a: transfer zeroed binary frames [ FAILED ]\n");
		return axl_false;
	}

	printf ("Test 13::");
	if (test_02 ())
		printf ("Test 02: basic BEEP channel support [   OK   ]\n");
	else {
		printf ("Test 02: basic BEEP channel support [ FAILED ]\n");
		return axl_false;
	}

	printf ("Test 13::");
	if (test_02a ()) 
		printf ("Test 02-a: connection close notification [   OK   ]\n");
	else {
		printf ("Test 02-a: connection close notification [ FAILED ]\n");
		return axl_false;
	}

	printf ("Test 13::");
	if (test_02b ()) 
		printf ("Test 02-b: small message followed by close  [   OK   ]\n");
	else {
		printf ("Test 02-b: small message followed by close [ FAILED ]\n");
		return axl_false;
	}

	printf ("Test 13::");
	if (test_02c ()) 
		printf ("Test 02-c: huge amount of small message followed by close  [   OK   ]\n");
	else {
		printf ("Test 02-c: huge amount of small message followed by close [ FAILED ]\n");
		return axl_false;
	}

	printf ("Test 13::");
	if (test_03 ())
		printf ("Test 03: basic BEEP channel support (large messages) [   OK   ]\n");
	else {
		printf ("Test 03: basic BEEP channel support (large messages) [ FAILED ]\n");
		return axl_false;
	}

	printf ("Test 13::");
	if (test_04_a ()) {
		printf ("Test 04-a: Check ANS/NUL support, sending large content [   OK   ]\n");
	} else {
		printf ("Test 04-a: Check ANS/NUL support, sending large content [ FAILED ]\n");
		return axl_false;
	}

	printf ("Test 13::");
	if (test_04_ab ()) {
		printf ("Test 04-ab: Check ANS/NUL support, sending different files [   OK   ]\n");
	} else {
		printf ("Test 04-ab: Check ANS/NUL support, sending different files [ FAILED ]\n");
		return axl_false;
	}

	/* free tunnel settings */
	vortex_tunnel_settings_free (tunnel_settings);
	tunnel_settings = NULL;
	tunnel_tested   = axl_false;

	return axl_true;
}

/**
 * @brief Allows to check PULL API support.
 *
 * @return axl_true if all tests are ok, otherwise axl_false is
 * returned.
 */ 
axl_bool test_14 (void)
{
	VortexEventMask * mask;

	/* create a mask */
	mask = vortex_event_mask_new ("test-mask", 
				      VORTEX_EVENT_CHANNEL_ADDED | VORTEX_EVENT_CHANNEL_REMOVED, 
				      axl_true);
	/* check if the mask detect events selected */
	if (vortex_event_mask_is_set (mask, VORTEX_EVENT_FRAME_RECEIVED)) {
		printf ("ERROR: Failed to check non-blocked event on a mask (frame received)\n");
		return axl_false;
	} /* end if */

	/* check if the mask detect events selected */
	if (vortex_event_mask_is_set (mask, VORTEX_EVENT_CHANNEL_CLOSE)) {
		printf ("ERROR: Failed to check non-blocked event on a mask (close request)\n");
		return axl_false;
	} /* end if */

	/* check if the mask detect events selected */
	if (! vortex_event_mask_is_set (mask, VORTEX_EVENT_CHANNEL_REMOVED)) {
		printf ("ERROR: Failed to check blocked event on a mask (channel removed)\n");
		return axl_false;
	} /* end if */

	/* check if the mask detect events selected */
	if (! vortex_event_mask_is_set (mask, VORTEX_EVENT_CHANNEL_ADDED)) {
		printf ("ERROR: Failed to check non-blocked event on a mask (channel added)\n");
		return axl_false;
	} /* end if */

	/* disable mask */
	vortex_event_mask_enable (mask, axl_false);

	/* check if the mask detect events selected but having the mask disabled */
	if (vortex_event_mask_is_set (mask, VORTEX_EVENT_CHANNEL_REMOVED)) {
		printf ("ERROR: Failed to check blocked event on a disabled mask (channel removed)\n");
		return axl_false;
	} /* end if */

	/* check if the mask detect events selected but having the mask disabled */
	if (vortex_event_mask_is_set (mask, VORTEX_EVENT_CHANNEL_ADDED)) {
		printf ("ERROR: Failed to check non-blocked event on a disabled mask (channel added)\n");
		return axl_false;
	} /* end if */

	/* free the mask */
	vortex_event_mask_free (mask);

	return axl_true;
}

/**
 * @brief Allows to check PULL API support.
 *
 * @return axl_true if all tests are ok, otherwise axl_false is
 * returned.
 */ 
axl_bool test_14_a (void)
{
	VortexCtx        * client_ctx;
	VortexConnection * conn;
	VortexChannel    * channel;
	VortexEvent      * event;
	VortexFrame      * frame;
	VortexEventMask  * mask;
	axlError         * error = NULL;

	/* create an indepenent client context */
	client_ctx = vortex_ctx_new ();

	/* init vortex on this context */
	if (! vortex_init_ctx (client_ctx)) {
		printf ("ERROR: failed to init client vortex context for PULL API..\n");
		return axl_false;
	} /* end if */

	/* now activate PULL api on this context */
	if (! vortex_pull_init (client_ctx)) {
		printf ("ERROR: failed to activate PULL API..\n");
		return axl_false;
	} /* end if */

	/* install mask to avoid handling some events */
	mask = vortex_event_mask_new ("client mask", 
				      VORTEX_EVENT_CONNECTION_CLOSED,
				      axl_true);
	if (! vortex_pull_set_event_mask (client_ctx, mask, &error)) {
		printf ("ERROR: failed to install client event mask, error reported (code: %d): %s\n",
			axl_error_get_code (error), axl_error_get (error));
		return axl_false;
	} /* end if */

	printf ("Test 14-a: pull API activated..\n");

	/* now do connect to the test server */
	conn = vortex_connection_new (client_ctx, listener_host, LISTENER_PORT, NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("Expected to find proper connection with regression test listener..\n");
		return axl_false;
	} /* end if */

	/* create a channel */
	channel = vortex_channel_new (conn, 0,
				      REGRESSION_URI,
				      /* no close handling */
				      NULL, NULL,
				      /* no frame received, it is disabled */
				      NULL, NULL,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("Unable to create the channel..");
		return axl_false;
	}

	/*** CHANNEL ADDED EVENT ***/
	printf ("Test 14-a: checking for added channel event..\n");
	event = vortex_pull_next_event (client_ctx, 0);
	if (event == NULL) {
		printf ("ERROR: expected to find channel added event, but found NULL reference..\n");
		return axl_false;
	}
	if (vortex_event_get_type (event) != VORTEX_EVENT_CHANNEL_ADDED) {
		printf ("ERROR: expected to find channel added event, but found %d..\n", 
			vortex_event_get_type (event));
		return axl_false;
	}
	/* check frame reference */
	if (! vortex_channel_are_equal (channel, vortex_event_get_channel (event))) {
		printf ("ERROR: expected to find same channel reference, but found different ones %p != %p..\n",
			channel, vortex_event_get_channel (event));
		return axl_false;
	}
	vortex_event_unref (event);

	/*** FRAME RECEIVED EVENT ***/
	printf ("Test 14-a: checking frame received event..\n");

	/* send some data */
	if (! vortex_channel_send_msg (channel, "this is a  PULL API test", 24, NULL)) {
		printf ("Expected to be able to send a message ..\n");
		return axl_false;
	}

	/* now wait for the reply */
	event = vortex_pull_next_event (client_ctx, 0);
	if (event == NULL) {
		printf ("Expected to find a non-null event from vortex_pull_next_event..\n");
		return axl_false;
	} /* end if */

	if (vortex_event_get_type (event) != VORTEX_EVENT_FRAME_RECEIVED) {
		printf ("Expected to find frame received event after a send operation..\n");
		return axl_false;
	} /* end if */

	/* check that the event has defined all data associated to the
	 * event */
	if (! vortex_channel_are_equal (channel, vortex_event_get_channel (event))) {
		printf ("Expected to find the same channel on event received..\n");
		return axl_false;
	} /* end if */

	if (vortex_connection_get_id (conn) != vortex_connection_get_id (vortex_event_get_conn (event))) {
		printf ("Expected to find the same connection on event received..\n");
		return axl_false;
	} /* end if */
	
	if (vortex_event_get_frame (event) == NULL) {
		printf ("Expected to find a frame reference on VORTEX_EVENT_FRAME_RECEIVED..\n");
		return axl_false;
	}

	/* get frame */
	frame = vortex_event_get_frame (event);

	if (! axl_cmp (vortex_frame_get_payload (frame), "this is a  PULL API test")) {
		printf ("Expected to find content inside frame reply '%s', but found: '%s'\n",
			"this is a  PULL API test", (char*) vortex_frame_get_payload (frame));
		return axl_false;
	} /* end if */

	/* dealloc */
	vortex_event_unref (event);
	printf ("Test 14-a: frame received event ok..\n");

	/* ok, close the channel */
	vortex_channel_close (channel, NULL);

	/* ok, close the connection */
	vortex_connection_close (conn);
	
	/*** CHANNEL REMOVED EVENT ***/
	printf ("Test 14: checking for added channel event..\n");
	event = vortex_pull_next_event (client_ctx, 0);
	if (event == NULL) {
		printf ("ERROR: expected to find channel added event, but found NULL reference..\n");
		return axl_false;
	}
	if (vortex_event_get_type (event) != VORTEX_EVENT_CHANNEL_REMOVED) {
		printf ("ERROR: expected to find channel added event, but found %d..\n", 
			vortex_event_get_type (event));
		return axl_false;
	}
	/* check frame reference */
	if (! vortex_channel_are_equal (channel, vortex_event_get_channel (event))) {
		printf ("ERROR: expected to find same channel reference, but found different ones %p != %p..\n",
			channel, vortex_event_get_channel (event));
		return axl_false;
	}
	vortex_event_unref (event);

	/* check no pending event is waiting to be read */
	if (vortex_pull_pending_events (client_ctx)) {
		printf ("ERROR: expected to not have pending events but found: %d waiting..\n",
			vortex_pull_pending_events_num (client_ctx));
		return axl_false;
	} /* end if */

	/* terminate client context */
	vortex_exit_ctx (client_ctx, axl_true);

	return axl_true;
}

void test_14_b_closed_channel (int channel_num, axl_bool was_closed, const char *code, const char *msg)
{
	/* fake function to activate threaded close */
	return;
}

/**
 * @brief Allows to check PULL API support.
 *
 * @return axl_true if all tests are ok, otherwise axl_false is
 * returned.
 */ 
axl_bool test_14_b (void)
{
	VortexCtx        * client_ctx;
	VortexCtx        * listener_ctx;
	VortexConnection * conn;
	VortexChannel    * channel;
	VortexEvent      * event;
	VortexConnection * listener;
	int                channel_num;
	VortexEventMask  * mask;
	axlError         * error = NULL;

	/* create an indepenent client context */
	client_ctx = vortex_ctx_new ();

	/*******************************/
	/* activate a client context   */
	/*******************************/
	if (! vortex_init_ctx (client_ctx)) {
		printf ("ERROR: failed to init client vortex context for PULL API..\n");
		return axl_false;
	} /* end if */

	/* now activate PULL api on this context */
	if (! vortex_pull_init (client_ctx)) {
		printf ("ERROR: failed to activate PULL API..\n");
		return axl_false;
	} /* end if */

	/* install mask to avoid handling some events */
	mask = vortex_event_mask_new ("client mask", 
				      VORTEX_EVENT_CHANNEL_ADDED | VORTEX_EVENT_CONNECTION_CLOSED,
				      axl_true);
	if (! vortex_pull_set_event_mask (client_ctx, mask, &error)) {
		printf ("ERROR: failed to install client event mask, error reported (code: %d): %s\n",
			axl_error_get_code (error), axl_error_get (error));
		return axl_false;
	} /* end if */

	printf ("Test 14-b: pull API activated (client context)..\n");

	/*******************************/
	/* activate a listener context */
	/*******************************/
	listener_ctx = vortex_ctx_new ();

	/* init vortex on this context */
	if (! vortex_init_ctx (listener_ctx)) {
		printf ("ERROR: failed to init listener vortex context for PULL API..\n");
		return axl_false;
	} /* end if */

	/* now activate PULL api on this context */
	if (! vortex_pull_init (listener_ctx)) {
		printf ("ERROR: failed to activate PULL API..\n");
		return axl_false;
	} /* end if */

	/* install mask to avoid handling some events */
	mask = vortex_event_mask_new ("listener mask", 
				      VORTEX_EVENT_CHANNEL_REMOVED | 
				      VORTEX_EVENT_CHANNEL_ADDED | 
				      VORTEX_EVENT_CONNECTION_CLOSED |
				      VORTEX_EVENT_CONNECTION_ACCEPTED |
				      VORTEX_EVENT_CHANNEL_START,
				      axl_true);
	if (! vortex_pull_set_event_mask (listener_ctx, mask, &error)) {
		printf ("ERROR: failed to install listener event mask, error reported (code: %d): %s\n",
			axl_error_get_code (error), axl_error_get (error));
		return axl_false;
	} /* end if */

	/* register a profile to accept channel creation */
	vortex_profiles_register (listener_ctx, REGRESSION_URI,
				  /* no start handling */
				  NULL, NULL, 
				  /* no close handling */
				  NULL, NULL,
				  /* no frame received */
				  NULL, NULL);
	
	/* start a listener */
	listener = vortex_listener_new (listener_ctx, 
					"localhost", "44012",
					NULL, NULL);
	
	printf ("Test 14-b: pull API activated (listener ctx)..\n");

	/* now create a connection */
	conn = vortex_connection_new (client_ctx, "localhost", "44012", NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("Expected to find proper connection with regression test listener..\n");
		return axl_false;
	} /* end if */

	/* create a channel */
	channel = vortex_channel_new (conn, 0,
				      REGRESSION_URI,
				      /* no close handling */
				      NULL, NULL,
				      /* no frame received, it is disabled */
				      NULL, NULL,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("Unable to create the channel..");
		return axl_false;
	}


	/* get channel number */
	channel_num = vortex_channel_get_number (channel);

	printf ("Test 14-b: checking close channel=%d  event..\n", channel_num);

	/* pass a handler to unlock this call, so we can simulate with
	 * a single thread the client closing the channel and the
	 * listener receiving the close channel request */
	vortex_channel_close (channel, 
			      test_14_b_closed_channel);

	/* now receive at the listener the close request */
	event = vortex_pull_next_event (listener_ctx, 0);
	if (event == NULL) {
		printf ("ERROR: Expected to find channel close request event but found NULL reference..\n");
		return axl_false;
	} /* end if */

	if (vortex_event_get_type (event) != VORTEX_EVENT_CHANNEL_CLOSE) {
		printf ("ERROR: Expected to find channel close request event but found: %d..\n",
			vortex_event_get_type (event));
		return axl_false;
	} /* end if */

	/* handle close request, reply to it */
	printf ("Test 14-b: received close channel request, completing reply..\n");
	vortex_channel_notify_close (
		vortex_event_get_channel (event), 
		vortex_event_get_msgno   (event),
		/* close the channel */
		axl_true);

	/* terminate event */
	vortex_event_unref (event);

	/* now wait for channel removed at the client */
	event = vortex_pull_next_event (client_ctx, 0);

	if (event == NULL || vortex_event_get_type (event) != VORTEX_EVENT_CHANNEL_REMOVED ||
	    vortex_channel_get_number (vortex_event_get_channel (event)) != channel_num) {
		printf ("ERROR: expected to find event type for channel removed, but found NULL or different event type (%d != %d) or different channel (%d != %d)",
			vortex_event_get_type (event), VORTEX_EVENT_CHANNEL_REMOVED,
			vortex_channel_get_number (vortex_event_get_channel (event)), channel_num);
		return axl_false;
	} /* end if */
	vortex_event_unref (event);
	
	/* check that the channel was removed */
	if (vortex_connection_channel_exists (conn, channel_num)) {
		printf ("ERROR: expected to find channel %d not available on the connection..\n", channel_num);
		return axl_false;
	} /* end if */
	printf ("Test 14-b: channel closed..\n");

	/* ok, close the connection */
	vortex_connection_close (conn);

	/* check no pending event is waiting to be read */
	if (vortex_pull_pending_events (client_ctx)) {
		printf ("ERROR: expected to not have pending events but found: %d waiting..\n",
			vortex_pull_pending_events_num (client_ctx));
		return axl_false;
	} /* end if */

	/* terminate client context */
	vortex_exit_ctx (client_ctx, axl_true);

	/* check no pending event is waiting to be read */
	if (vortex_pull_pending_events (listener_ctx)) {
		printf ("ERROR: expected to not have pending events but found: %d waiting..\n",
			vortex_pull_pending_events_num (listener_ctx));
		return axl_false;
	} /* end if */

	/* terminate listener context */
	vortex_exit_ctx (listener_ctx, axl_true);

	return axl_true;
}

/**
 * @brief Allows to check PULL API support.
 *
 * @return axl_true if all tests are ok, otherwise axl_false is
 * returned.
 */ 
axl_bool test_14_c (void)
{
	VortexCtx        * client_ctx;
	VortexCtx        * listener_ctx;
	VortexConnection * conn;
	int                conn2;
	VortexEvent      * event;
	VortexConnection * listener;

	/* create an indepenent client context */
	client_ctx = vortex_ctx_new ();

	/*******************************/
	/* activate a client context   */
	/*******************************/
	if (! vortex_init_ctx (client_ctx)) {
		printf ("ERROR: failed to init client vortex context for PULL API..\n");
		return axl_false;
	} /* end if */

	/* now activate PULL api on this context */
	if (! vortex_pull_init (client_ctx)) {
		printf ("ERROR: failed to activate PULL API..\n");
		return axl_false;
	} /* end if */

	printf ("Test 14-c: pull API activated (client context)..\n");

	/*******************************/
	/* activate a listener context */
	/*******************************/
	listener_ctx = vortex_ctx_new ();

	/* init vortex on this context */
	if (! vortex_init_ctx (listener_ctx)) {
		printf ("ERROR: failed to init listener vortex context for PULL API..\n");
		return axl_false;
	} /* end if */

	/* now activate PULL api on this context */
	if (! vortex_pull_init (listener_ctx)) {
		printf ("ERROR: failed to activate PULL API..\n");
		return axl_false;
	} /* end if */

	/* register a profile to accept channel creation */
	vortex_profiles_register (listener_ctx, REGRESSION_URI,
				  /* no start handling */
				  NULL, NULL, 
				  /* no close handling */
				  NULL, NULL,
				  /* no frame received */
				  NULL, NULL);
	
	/* start a listener */
	listener = vortex_listener_new (listener_ctx, 
					"localhost", "44012",
					NULL, NULL);

	/* now create a connection */
	conn = vortex_connection_new (client_ctx, "localhost", "44012", NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("Expected to find proper connection with regression test listener..\n");
		return axl_false;
	} /* end if */

	/* now receive at the listener the close request */
	event = vortex_pull_next_event (listener_ctx, 0);
	if (event == NULL) {
		printf ("ERROR: Expected to find connection accepted but found NULL reference..\n");
		return axl_false;
	} /* end if */

	if (vortex_event_get_type (event) != VORTEX_EVENT_CONNECTION_ACCEPTED) {
		printf ("ERROR: Expected to find connection accepted event but found: %d..\n",
			vortex_event_get_type (event));
		return axl_false;
	} /* end if */

	/* close the connection without negotiation */
	conn2 = vortex_connection_get_id (vortex_event_get_conn (event));
	vortex_connection_shutdown (vortex_event_get_conn (event));

	/* terminate event */
	vortex_event_unref (event);

	/* now wait for connection closed at the client */
	event = vortex_pull_next_event (client_ctx, 0);

	if (event == NULL || vortex_event_get_type (event) != VORTEX_EVENT_CONNECTION_CLOSED ||
	    vortex_connection_get_id (conn) != vortex_connection_get_id (vortex_event_get_conn (event))) {
		printf ("ERROR: expected to find event type for connection closed, but found NULL or different event type (%d != %d) or different connection (%d != %d)\n",
			vortex_event_get_type (event), VORTEX_EVENT_CONNECTION_CLOSED,
			vortex_connection_get_id (conn), vortex_connection_get_id (vortex_event_get_conn (event)));
		return axl_false;
	} /* end if */
	vortex_event_unref (event);
	
	/* ok, close the connection */
	vortex_connection_close (conn);

	/* now wait for connection closed at the client */
	event = vortex_pull_next_event (listener_ctx, 0);

	if (event == NULL || vortex_event_get_type (event) != VORTEX_EVENT_CONNECTION_CLOSED ||
	    conn2 != vortex_connection_get_id (vortex_event_get_conn (event))) {
		printf ("ERROR: expected to find event type for connection closed, but found NULL or different event type (%d != %d) or different connection (%d != %d)\n",
			vortex_event_get_type (event), VORTEX_EVENT_CONNECTION_CLOSED,
			conn2, vortex_connection_get_id (vortex_event_get_conn (event)));
		return axl_false;
	} /* end if */
	vortex_event_unref (event);

	/* check no pending event is waiting to be read */
	if (vortex_pull_pending_events (client_ctx)) {
		printf ("ERROR: expected to not have pending events but found: %d waiting (client)..\n",
			vortex_pull_pending_events_num (client_ctx));
		return axl_false;
	} /* end if */

	/* terminate client context */
	vortex_exit_ctx (client_ctx, axl_true);

	/* check no pending event is waiting to be read */
	if (vortex_pull_pending_events (listener_ctx)) {
		printf ("ERROR: expected to not have pending events but found: %d waiting (listener)..\n",
			vortex_pull_pending_events_num (listener_ctx));
		return axl_false;
	} /* end if */

	/* terminate listener context */
	vortex_exit_ctx (listener_ctx, axl_true);

	return axl_true;
}

/**
 * @internal Helper function for test-14d test. It creates a channel
 * in a separated thread so the main thread can handle start request.
 */
axlPointer test_14_d_create_channel (axlPointer user_data)
{
	VortexChannel    * channel;
	VortexConnection * conn = user_data;

	/* create a channel */
	channel = vortex_channel_new (conn, 0,
				      REGRESSION_URI,
				      /* no close handling */
				      NULL, NULL,
				      /* no frame received, it is disabled */
				      NULL, NULL,
				      /* no async channel creation */
				      NULL, NULL);
	if (channel == NULL) {
		printf ("Unable to create the channel (TEST 14-D IS FAILING...)..");
		return axl_false;
	} /* end if */

	/* return channel created */
	return channel;
}

/**
 * @brief Allows to check PULL API support.
 *
 * @return axl_true if all tests are ok, otherwise axl_false is
 * returned.
 */ 
axl_bool test_14_d (void)
{
	VortexCtx        * client_ctx;
	VortexCtx        * listener_ctx;
	VortexConnection * conn;
	VortexEvent      * event;
	VortexConnection * listener;
	VortexEventMask  * mask;
	axlError         * error = NULL;
	VortexThread       thread;
	int                channel_num;
	VortexChannel    * channel;

	/* create an indepenent client context */
	client_ctx = vortex_ctx_new ();

	/*******************************/
	/* activate a client context   */
	/*******************************/
	if (! vortex_init_ctx (client_ctx)) {
		printf ("ERROR: failed to init client vortex context for PULL API..\n");
		return axl_false;
	} /* end if */

	/* now activate PULL api on this context */
	if (! vortex_pull_init (client_ctx)) {
		printf ("ERROR: failed to activate PULL API..\n");
		return axl_false;
	} /* end if */

	/* install mask to avoid handling some events */
	mask = vortex_event_mask_new ("client mask", 
				      VORTEX_EVENT_CHANNEL_REMOVED | 
				      VORTEX_EVENT_CHANNEL_ADDED | 
				      VORTEX_EVENT_CONNECTION_CLOSED |
				      VORTEX_EVENT_CONNECTION_ACCEPTED,
				      axl_true);
	if (! vortex_pull_set_event_mask (client_ctx, mask, &error)) {
		printf ("ERROR: failed to install client event mask, error reported (code: %d): %s\n",
			axl_error_get_code (error), axl_error_get (error));
		return axl_false;
	} /* end if */

	printf ("Test 14-d: pull API activated (client context)..\n");

	/*******************************/
	/* activate a listener context */
	/*******************************/
	listener_ctx = vortex_ctx_new ();

	/* init vortex on this context */
	if (! vortex_init_ctx (listener_ctx)) {
		printf ("ERROR: failed to init listener vortex context for PULL API..\n");
		return axl_false;
	} /* end if */

	/* now activate PULL api on this context */
	if (! vortex_pull_init (listener_ctx)) {
		printf ("ERROR: failed to activate PULL API..\n");
		return axl_false;
	} /* end if */

	/* install mask to avoid handling some events */
	mask = vortex_event_mask_new ("listener mask", 
				      VORTEX_EVENT_CHANNEL_REMOVED | 
				      VORTEX_EVENT_CHANNEL_ADDED | 
				      VORTEX_EVENT_CHANNEL_CLOSE |
				      VORTEX_EVENT_CONNECTION_CLOSED |
				      VORTEX_EVENT_CONNECTION_ACCEPTED,
				      axl_true);
	if (! vortex_pull_set_event_mask (listener_ctx, mask, &error)) {
		printf ("ERROR: failed to install listener event mask, error reported (code: %d): %s\n",
			axl_error_get_code (error), axl_error_get (error));
		return axl_false;
	} /* end if */

	/* register a profile to accept channel creation */
	vortex_profiles_register (listener_ctx, REGRESSION_URI,
				  /* no start handling */
				  NULL, NULL, 
				  /* no close handling */
				  NULL, NULL,
				  /* no frame received */
				  NULL, NULL);
	
	/* start a listener */
	listener = vortex_listener_new (listener_ctx, 
					"localhost", "44012",
					NULL, NULL);

	/* now create a connection */
	conn = vortex_connection_new (client_ctx, "localhost", "44012", NULL, NULL);
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("Expected to find proper connection with regression test listener..\n");
		return axl_false;
	} /* end if */
	printf ("Test 14-d: checking PULL API for channel start event, creating clieng channel..\n");
	/* now start a thread where the client context request to
	 * create a channel and the listener accept it by handling the
	 * VORTEX_EVENT_CHANNEL_START */
	if (! vortex_thread_create (&thread, test_14_d_create_channel, conn,
				    VORTEX_THREAD_CONF_END)) {
		printf ("ERROR: failed to create a thread to create the client channel..\n");
		return axl_false;
	}

	/* now the listener must wait for the channel start event to
	 * accept it */
	event = vortex_pull_next_event (listener_ctx, 0);
	if (event == NULL || vortex_event_get_type (event) != VORTEX_EVENT_CHANNEL_START) {
		printf ("ERROR: expected to find event type for channel start, but found NULL or different event type (%d != %d)\n",
			vortex_event_get_type (event), VORTEX_EVENT_CHANNEL_START);
		return axl_false;
	} /* end if */

	/* get channel number */
	channel_num = vortex_channel_get_number (vortex_event_get_channel (event));
	printf ("Test 14-d: received channel=%d start request, handling..\n", channel_num);
	if (! vortex_channel_notify_start (vortex_event_get_channel (event), 
					   vortex_event_get_profile_content (event),
					   /* accept channel to be created */
					   axl_true)) {
		printf ("ERROR: failed to notify channel start..\n");
		return axl_false;
	}
	
	/* unref event */
	vortex_event_unref (event);
	
	printf ("Test 14-d: checking channel created..\n");

	/* get the channel reference on the connection */
	channel = vortex_connection_get_channel (conn, channel_num);
	if (channel == NULL) {
		printf ("ERROR: expected to find channel created after channel start event..\n");
		return axl_false;
	} /* end if */

	printf ("Test 14-d: channel created ok..\n");
	
	/* terminate thread */
	vortex_thread_destroy (&thread, axl_false);

	/* ok, close the connection */
	vortex_connection_close (conn);

	/* check no pending event is waiting to be read */
	if (vortex_pull_pending_events (client_ctx)) {
		printf ("ERROR: expected to not have pending events but found: %d waiting (client)..\n",
			vortex_pull_pending_events_num (client_ctx));
		return axl_false;
	} /* end if */

	/* terminate client context */
	vortex_exit_ctx (client_ctx, axl_true);

	/* check no pending event is waiting to be read */
	if (vortex_pull_pending_events (listener_ctx)) {
		printf ("ERROR: expected to not have pending events but found: %d waiting (listener)..\n",
			vortex_pull_pending_events_num (listener_ctx));
		return axl_false;
	} /* end if */

	/* terminate listener context */
	vortex_exit_ctx (listener_ctx, axl_true);

	return axl_true;
}

/** 
 * @brief Allows to check HTTP CONNECT implementation.
 * 
 * @return axl_true if all test pass, otherwise axl_false is returned.
 */
axl_bool  test_15 (void)
{
	VortexHttpSetup  * setup;
	VortexConnection * conn;

	/* configure proxy options */
	setup = vortex_http_setup_new (ctx);
	
	/* configure connection: assume squid running at
	 * localhost:3128 (FIXME, add general support to configure a
	 * different proxy location) */
	vortex_http_setup_conf (setup, VORTEX_HTTP_CONF_ITEM_PROXY_HOST, http_proxy_host);
	vortex_http_setup_conf (setup, VORTEX_HTTP_CONF_ITEM_PROXY_PORT, http_proxy_port);

	/* create a connection */
	conn = vortex_http_connection_new (listener_host, "443", setup, NULL, NULL);

	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR: unable to create connection to %s:443, error found (code: %d): %s",
			listener_host,
			vortex_connection_get_status (conn),
			vortex_connection_get_message (conn));
		return axl_false;
	} /* end if */

	/* close connection */
	vortex_connection_close (conn);

	/* finish setup */
	vortex_http_setup_unref (setup);

	return axl_true;

}

/** 
 * @brief Allows to check HTTP CONNECT implementation, running more
 * regression tests under HTTP connection.
 * 
 * @return axl_true if all test pass, otherwise axl_false is returned.
 */
axl_bool  test_15a (void)
{
	VortexConnection * conn;
	
	/* enable HTTP CONNECT */
	enable_http_proxy = axl_true;
	printf ("Test 15-a: checking HTTP CONNECT implementation..\n");

	conn = connection_new ();
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR: failed to create connection under HTTP CONNECT..\n");
		return axl_false;
	} /* end if */

	printf ("Test 15-a::");
	if (test_01 ())
		printf ("Test 01: basic BEEP support [   OK   ]\n");
	else {
		printf ("Test 01: basic BEEP support [ FAILED ]\n");
		return axl_false;
	}

	printf ("Test 15-a::");
	if (test_01a ())
		printf ("Test 01-a: transfer zeroed binary frames [   OK   ]\n");
	else {
		printf ("Test 01-a: transfer zeroed binary frames [ FAILED ]\n");
		return axl_false;
	}

	printf ("Test 15-a::");
	if (test_02 ())
		printf ("Test 02: basic BEEP channel support [   OK   ]\n");
	else {
		printf ("Test 02: basic BEEP channel support [ FAILED ]\n");
		return axl_false;
	}

	printf ("Test 15-a::");
	if (test_02a ()) 
		printf ("Test 02-a: connection close notification [   OK   ]\n");
	else {
		printf ("Test 02-a: connection close notification [ FAILED ]\n");
		return axl_false;
	}

	printf ("Test 15-a::");
	if (test_02b ()) 
		printf ("Test 02-b: small message followed by close  [   OK   ]\n");
	else {
		printf ("Test 02-b: small message followed by close [ FAILED ]\n");
		return axl_false;
	}

	printf ("Test 15-a::");
	if (test_02c ()) 
		printf ("Test 02-c: huge amount of small message followed by close  [   OK   ]\n");
	else {
		printf ("Test 02-c: huge amount of small message followed by close [ FAILED ]\n");
		return axl_false;
	}

	printf ("Test 15-a::");
	if (test_03 ())
		printf ("Test 03: basic BEEP channel support (large messages) [   OK   ]\n");
	else {
		printf ("Test 03: basic BEEP channel support (large messages) [ FAILED ]\n");
		return axl_false;
	}

	printf ("Test 15-a::");
	if (test_04_a ()) {
		printf ("Test 04-a: Check ANS/NUL support, sending large content [   OK   ]\n");
	} else {
		printf ("Test 04-a: Check ANS/NUL support, sending large content [ FAILED ]\n");
		return axl_false;
	}

	printf ("Test 15-a::");
	if (test_04_ab ()) {
		printf ("Test 04-ab: Check ANS/NUL support, sending different files [   OK   ]\n");
	} else {
		printf ("Test 04-ab: Check ANS/NUL support, sending different files [ FAILED ]\n");
		return axl_false;
	}

	/* close connection */
	vortex_connection_close (conn);

	return axl_true;
}


void test_16_on_close_full (VortexConnection * conn,
			    axlPointer         _queue)
{
	/* close connection */
	printf ("Test 16: Connection closed conn id=%d..\n", vortex_connection_get_id (conn));

	/* push on the queue to unlock main thread */
	vortex_async_queue_push ((VortexAsyncQueue *) _queue, INT_TO_PTR (1));

	return;
}

/** 
 * @brief Check alive profile support 
 */
axl_bool  test_16 (void)
{
	VortexConnection * conn;
	VortexChannel    * channel;
	VortexAsyncQueue * queue;

	/* create connection */
	conn = connection_new ();
	if (! vortex_connection_is_ok (conn, axl_false)) {
		printf ("ERROR: failed to create connection under HTTP CONNECT..\n");
		return axl_false;
	} /* end if */

	/* enable alive check on this connection every 20ms */
	vortex_alive_enable_check (conn, 20000, 0, NULL);

	/* now ask remote server to block the connection during 30 ms */
	channel = vortex_channel_new (conn, 0,
				      REGRESSION_URI,
				      /* no close handling */
				      NULL, NULL,
				      /* no frame received, it is disabled */
				      NULL, NULL,
				      /* no async channel creation */
				      NULL, NULL);

	/* check channel */
	if (channel == NULL) {
		printf ("Test 16: expected proper channel creation while checking alive profile..\n");
		return axl_false;
	} /* end if */

	/* configure close connection to be triggered by the alive check */
	queue = vortex_async_queue_new ();
	vortex_connection_set_on_close_full (conn, test_16_on_close_full, queue);

	if (! vortex_channel_send_msg (channel, "block-connection", 16, NULL)) {
		printf ("Test 16: failed to send block connection message..\n");
		return axl_false;
	} /* end if */

	/* lock until connection is closed */
	printf ("Test 16: waiting connection to be detected to be closed..\n");
	vortex_async_queue_pop (queue);

	printf ("Test 16: ok, connection close detected..\n");

	/* close connection */
	vortex_connection_close (conn);

	return axl_true;

}


typedef int  (*VortexRegressionTest) (void);
  
 
void run_test (VortexRegressionTest test, const char * test_name, const char * message, 
 	       long  limit_seconds, long  limit_microseconds) {

 	struct timeval      start;
 	struct timeval      stop;
 	struct timeval      result;
  
 	/* start test */
 	gettimeofday (&start, NULL);
 	if (test ()) {
 		/* stop test */
 		gettimeofday (&stop, NULL);
		
 		/* get result */
 		vortex_timeval_substract (&stop, &start, &result);
		
 		/* check timing results */
 		if ((! disable_time_checks) && limit_seconds >= 0 && limit_microseconds > 0) {
 			if (result.tv_sec >= limit_seconds && result.tv_usec > limit_microseconds) {
 				printf ("%s: %s \n",
 					test_name, message);
 				printf ("***WARNING***: should finish in less than %ld secs, %ld microseconds\n",
 					limit_seconds, limit_microseconds);
 				printf ("                          but finished in %ld secs, %ld microseconds\n", 
 					result.tv_sec, result.tv_usec);
 				exit (-1);
 			} 
 		} /* end if */
		
 		printf ("%s: %s [   OK   ] (finished in %ld secs, %ld microseconds)\n", test_name, message, result.tv_sec, result.tv_usec);
 	} else {
 		printf ("%s: %s [ FAILED ]\n", test_name, message);
 		exit (-1);
 	}
 	return;
}
  

int main (int  argc, char ** argv)
{
	int    iterator;
 	char * run_test_name = NULL;

#if defined (AXL_OS_UNIX) && defined (VORTEX_HAVE_POLL)
	/* if poll(2) mechanism is available, check it */
	axl_bool  poll_tested = axl_true;
#endif
#if defined (AXL_OS_UNIX) && defined (VORTEX_HAVE_EPOLL)
	/* if epoll(2) mechanism is available, check it */
	axl_bool  epoll_tested = axl_true;
#endif

	printf ("** Vortex Library: A BEEP core implementation.\n");
	printf ("** Copyright (C) 2008 Advanced Software Production Line, S.L.\n**\n");
	printf ("** Vortex Regression tests: version=%s\n**\n",
		VERSION);
	printf ("** To properly run this test it is required to run vortex-regression-listener.\n");
	printf ("** To gather information about time performance you can use:\n**\n");
	printf ("**     >> time ./vortex-regression-client\n**\n");
	printf ("** To gather information about memory consumed (and leaks) use:\n**\n");
	printf ("**     >> libtool --mode=execute valgrind --leak-check=yes --error-limit=no ./vortex-regression-client --disable-time-checks\n**\n");
	printf ("** Additional settings:\n");
	printf ("**\n");
	printf ("**     >> ./vortex-regression-client [--disable-time-checks] [--run-test=NAME] \n");
	printf ("**                                   [listener-host [TUNNEL-proxy-host [proxy-host [proxy-port]]]]\n");
	printf ("**\n");
	printf ("**       If no listener-host value is provided, it is used \"localhost\". \n");
	printf ("**       If no TUNNEL-proxy-host value is provided, it is used the value \n");
	printf ("**       provided for listener-host. \n");
	printf ("**\n");
	printf ("**       Values for proxy-host and proxy-port defines the HTTP proxy server to use\n");
	printf ("**       to check vortex-http implementation. If proxy-host is not provided \"localhost\"\n");
	printf ("**       is used. If proxy-port is not provided, port 3128 is used.\n");
	printf ("**\n");
	printf ("**       Providing --disable-time-checks will make regression test to skip those\n");
	printf ("**       tests that could fail due to timing issues. This is useful when using\n");
        printf ("**       valgrind or similar tools.\n");
	printf ("**\n");
	printf ("**       Providing --run-test=NAME will run only the provided regression test.\n");
	printf ("**       Test available: test_00, test_00a, test_00b, test_00c, test_01, test_01a, test_01b, test_01c, test_01d, test_01e,\n");
	printf ("**                       test_01f, test_01g, test_01h, test_01i, test_01j, test_01k, test_01l, test_01o, test_01p\n");
	printf ("**                       test_02, test_02a, test_02a1, test_02b, test_02c, test_02d, test_02e, \n"); 
	printf ("**                       test_02f, test_02g, test_02h, test_02i, test_02j, test_02k,\n");
 	printf ("**                       test_02l, test_02m, test_02m1, test_02m2, test_02n, test_02o, \n");
 	printf ("**                       test_03, test_03a, test_03b, test_03d, test_03c, test_04, test_04a, \n");
 	printf ("**                       test_04b, test_04c, test_05, test_05a, test_05b, test_06, test_06a, \n");
 	printf ("**                       test_07, test_08, test_09, test_10, test_11, test_12, test_13, test_14, \n");
 	printf ("**                       test_14a, test_14b, test_14ctest_14d, test_15, test_15a, test_16\n");
	printf ("**\n");
	printf ("** Report bugs to:\n**\n");
	printf ("**     <vortex@lists.aspl.es> Vortex Mailing list\n**\n");

	/* install default handling to get notification about
	 * segmentation faults */
#ifdef AXL_OS_UNIX
	signal (SIGSEGV, __block_test);
	signal (SIGABRT, __block_test);
#endif

	/* create the context */
	ctx = vortex_ctx_new ();

	/* check for disable-time-checks */
	if (argc > 1 && axl_cmp (argv[1], "--disable-time-checks")) {
		disable_time_checks = axl_true;
		iterator            = 1;
		argc--;
		printf ("INFO: disabling timing checks\n");
		while (iterator <= argc) {
			argv[iterator] = argv[iterator+1];
			iterator++;
		} /* end while */
	} /* end if */

	/* check for disable-time-checks */
	if (argc > 1 && axl_memcmp (argv[1], "--run-test=", 11)) {
		run_test_name  = argv[1] + 11;
		iterator       = 1;
		argc--;
		printf ("INFO: running test=%s\n", run_test_name);
		while (iterator <= argc) {
			argv[iterator] = argv[iterator+1];
			iterator++;
		} /* end while */
	} /* end if */

	/* configure default values */
	if (argc == 1) {
		listener_host        = "localhost";
		listener_tunnel_host = "localhost";

		/* HTTP proxy configuration */
		http_proxy_host      = "localhost";
		http_proxy_port      = "3128";
		
	} else if (argc == 2) {

		listener_host        = argv[1];
		listener_tunnel_host = argv[1];

		/* HTTP proxy configuration */
		http_proxy_host      = "localhost";
		http_proxy_port      = "3128";

	} else if (argc == 3) {

		listener_host        = argv[1];
		listener_tunnel_host = argv[2]; 

		/* HTTP proxy configuration */
		http_proxy_host      = "localhost";
		http_proxy_port      = "3128";

	} else if (argc == 4) {
		
		listener_host        = argv[1];
		listener_tunnel_host = argv[2]; 

		/* HTTP proxy configuration */
		http_proxy_host      = argv[3];
		http_proxy_port      = "3128";

	} else if (argc == 5) {
		
		listener_host        = argv[1];
		listener_tunnel_host = argv[2]; 

		/* HTTP proxy configuration */
		http_proxy_host      = argv[3];
		http_proxy_port      = argv[4];

	} /* end if */

	if (listener_host == NULL) {
		printf ("Error: undefined value found for listener host..\n");
		return -1;
	}

	if (listener_tunnel_host == NULL) {
		printf ("Error: undefined value found for listener proxy host..\n");
		return -1;
	}
		
	printf ("INFO: running test against %s, with BEEP TUNNEL proxy located at: %s..\n",
		listener_host, listener_tunnel_host);
	printf ("INFO: HTTP proxy located at: %s:%s..\n",
		http_proxy_host, http_proxy_port);

	/* init vortex library */
	if (! vortex_init_ctx (ctx)) {
		/* unable to init context */
		vortex_ctx_free (ctx);
		return -1;
	} /* end if */

	/* change to select if it is not the default */
	vortex_io_waiting_use (ctx, VORTEX_IO_WAIT_SELECT);

	if (run_test_name) {
		printf ("INFO: Checking to run test: %s..\n", run_test_name);

		if (axl_cmp (run_test_name, "test_00"))
			run_test (test_00, "Test 00", "Async Queue support", -1, -1);

		if (axl_cmp (run_test_name, "test_00a"))
			run_test (test_00a, "Test 00-a", "Thread pool stats", -1, -1);

		if (axl_cmp (run_test_name, "test_00b"))
			run_test (test_00b, "Test 00-b", "Thread pool stats (change number)", -1, -1);

		if (axl_cmp (run_test_name, "test_00c"))
			run_test (test_00c, "Test 00-c", "Thread pool events", -1, -1);

		if (axl_cmp (run_test_name, "test_01"))
			run_test (test_01, "Test 01", "basic BEEP support", -1, -1);

		if (axl_cmp (run_test_name, "test_01a"))
			run_test (test_01a, "Test 01-a", "transfer zeroed binary frames", -1, -1);

		if (axl_cmp (run_test_name, "test_01b"))
			run_test (test_01b, "Test 01-b", "channel close inside created notification (31/03/2008)", -1, -1);

		if (axl_cmp (run_test_name, "test_01c"))
			run_test (test_01c, "Test 01-c", "check immediately send (31/03/2008)", -1, -1);

		if (axl_cmp (run_test_name, "test_01d"))
			run_test (test_01d, "Test 01-d", "MIME support", -1, -1);

		if (axl_cmp (run_test_name, "test_01e"))
			run_test (test_01e, "Test 01-e", "Check listener douple port allocation", -1, -1);

		if (axl_cmp (run_test_name, "test_01f"))
			run_test (test_01f, "Test 01-f", "Check connection with no greetings showed (or registerered)", -1, -1);

		if (axl_cmp (run_test_name, "test_01g"))
			run_test (test_01g, "Test 01-g", "Check connection serverName feature on greetings", -1, -1);

		if (axl_cmp (run_test_name, "test_01h"))
			run_test (test_01h, "Test 01-h", "BEEP wrong header attack..", -1, -1);

		if (axl_cmp (run_test_name, "test_01i"))
			run_test (test_01i, "Test 01-i", "BEEP connect to (usually) unreachable address..", -1, -1);

		if (axl_cmp (run_test_name, "test_01j"))
			run_test (test_01j, "Test 01-j", "Log handling with prepared strings", -1, -1);

		if (axl_cmp (run_test_name, "test_01k"))
			run_test (test_01k, "Test 01-k", "Limitting channel send operations (memory consuption)", -1, -1);

		if (axl_cmp (run_test_name, "test_01l"))
			run_test (test_01l, "Test 01-l", "Memory consuption with channel serialize", -1, -1);

		if (axl_cmp (run_test_name, "test_01o"))
			run_test (test_01o, "Test 01-o", "Memory consuption with channel pool acquire/release API", -1, -1);

		if (axl_cmp (run_test_name, "test_01p"))
			run_test (test_01p, "Test 01-p", "Check upper limits for window sizes and idle handling (bug fix)", -1, -1);

		if (axl_cmp (run_test_name, "test_02"))
			run_test (test_02, "Test 02", "basic BEEP channel support", -1, -1);

		if (axl_cmp (run_test_name, "test_02a"))
			run_test (test_02a, "Test 02-a", "connection close notification", -1, -1);
		
		if (axl_cmp (run_test_name, "test_02a1"))
			run_test (test_02a1, "Test 02-a1", "connection close notification with handlers removed", -1, -1);

		if (axl_cmp (run_test_name, "test_02b"))
			run_test (test_02b, "Test 02-b", "small message followed by close", -1, -1);

		if (axl_cmp (run_test_name, "test_02c"))
			run_test (test_02c, "Test 02-c", "huge amount of small message followed by close", -1, -1);

		if (axl_cmp (run_test_name, "test_02d"))
			run_test (test_02d, "Test 02-d", "close after large reply", -1, -1);

		if (axl_cmp (run_test_name, "test_02e"))
			run_test (test_02e, "Test 02-e", "check wait reply support", -1, -1);

		if (axl_cmp (run_test_name, "test_02f"))
			run_test (test_02f, "Test 02-f", "check vortex performance under packet delay scenarios", -1, -1);

		if (axl_cmp (run_test_name, "test_02g"))
			run_test (test_02g, "Test 02-g", "check basic BEEP support with different frame sizes", -1, -1);

		if (axl_cmp (run_test_name, "test_02h"))
			run_test (test_02h, "Test 02-h", "check bandwith performance with different window and segmentator sizes", -1, -1);

		if (axl_cmp (run_test_name, "test_02i"))
			run_test (test_02i, "Test 02-i", "check enforced ordered delivery at server side", -1, -1);

		if (axl_cmp (run_test_name, "test_02j"))
			run_test (test_02j, "Test 02-j", "suddently connection close while operating", -1, -1);

		if (axl_cmp (run_test_name, "test_02k"))
			run_test (test_02k, "Test 02-k", "mixing replies to messages received in the same channel (ANS..NUL, RPY)", -1, -1);

		if (axl_cmp (run_test_name, "test_02l"))
			run_test (test_02l, "Test 02-l", "detect last reply written when using ANS/NUL reply.", -1, -1);

		if (axl_cmp (run_test_name, "test_02m"))
			run_test (test_02m, "Test 02-m", "blocking close after ANS/NUL replies.", -1, -1);

		if (axl_cmp (run_test_name, "test_02m1"))
			run_test (test_02m1, "Test 02-m1", "Transfer with big frame sizes.", -1, -1);

		if (axl_cmp (run_test_name, "test_02m2"))
			run_test (test_02m2, "Test 02-m2", "Dealloc pending queued replies on connection shutdown.", -1, -1);

		if (axl_cmp (run_test_name, "test_02n"))
			run_test (test_02n, "Test 02-n", "Checking MSG number reusing", -1, -1);

		if (axl_cmp (run_test_name, "test_02o"))
			run_test (test_02o, "Test 02-o", "Checking support for seqno transfers over 4GB (MAX SEQ NO: 4294967295)", -1, -1);

		if (axl_cmp (run_test_name, "test_03"))
			run_test (test_03, "Test 03", "basic BEEP channel support (large messages)", -1, -1);

		if (axl_cmp (run_test_name, "test_03a"))
			run_test (test_03a, "Test 03-a", "vortex channel pool support", -1, -1);

		if (axl_cmp (run_test_name, "test_03b"))
			run_test (test_03b, "Test 03-b", "vortex channel pool support (ANS/NUL reply check)", -1, -1);

		if (axl_cmp (run_test_name, "test_03c"))
			run_test (test_03c, "Test 03-c", "vortex ANS/NUL replies with serialize not attended", -1, -1);

		if (axl_cmp (run_test_name, "test_03d"))
			run_test (test_03d, "Test 03-d", "vortex channel pool support (auxiliar pointers)", -1, -1);

		if (axl_cmp (run_test_name, "test_04"))
			run_test (test_04, "Test 04", "Handling many connections support", -1, -1);

		if (axl_cmp (run_test_name, "test_04a"))
			run_test (test_04_a, "Test 04-a", "Check ANS/NUL support, sending large content", -1, -1);

		if (axl_cmp (run_test_name, "test_04ab"))
			run_test (test_04_ab, "Test 04-ab", "Check ANS/NUL support, sending different files", -1, -1);

		if (axl_cmp (run_test_name, "test_04c"))
			run_test (test_04_c, "Test 04-c", "check client adviced profiles", -1, -1);

		if (axl_cmp (run_test_name, "test_04d"))
			run_test (test_04_d, "Test 04-d", "check channel window size reduction", -1, -1);

		if (axl_cmp (run_test_name, "test_05"))
			run_test (test_05, "Test 05", "TLS profile support", -1, -1);
		
		if (axl_cmp (run_test_name, "test_05a"))
			run_test (test_05_a, "Test 05-a", "Check auto-tls on fail fix (24/03/2008)", -1, -1);

		if (axl_cmp (run_test_name, "test_05b"))
			run_test (test_05_b, "Test 05-b", "TLS client blocked during connection close (14/12/2009)", -1, -1);

		if (axl_cmp (run_test_name, "test_06"))
			run_test (test_06, "Test 06", "SASL profile support", -1, -1);

		if (axl_cmp (run_test_name, "test_06a"))
			run_test (test_06a, "Test 06-a", "SASL profile support (common handler)", -1, -1);

		if (axl_cmp (run_test_name, "test_07"))
			run_test (test_07, "Test 07", "XML-RPC profile support", -1, -1);

		if (axl_cmp (run_test_name, "test_08"))
			run_test (test_08, "Test 08", "serverName configuration", -1, -1);

		if (axl_cmp (run_test_name, "test_09"))		
			run_test (test_09, "Test 09", "close in transit support", -1, -1);

		if (axl_cmp (run_test_name, "test_10"))
			run_test (test_10, "Test 10", "default channel close action", -1, -1);

		if (axl_cmp (run_test_name, "test_11"))
			run_test (test_11, "Test 11", "reply to multiple messages in a wrong order without blocking", -1, -1);

		if (axl_cmp (run_test_name, "test_12"))
			run_test (test_12, "Test 12", "check connection creation timeout", -1, -1);

		if (axl_cmp (run_test_name, "test_13"))
			run_test (test_13, "Test 13", "test TUNNEL implementation", -1, -1);

		if (axl_cmp (run_test_name, "test_14"))
			run_test (test_14, "Test 14", "Check PULL API event masks", -1, -1);

		if (axl_cmp (run_test_name, "test_14a"))
			run_test (test_14_a, "Test 14-a", "Check PULL API implementation (frame receieved event)", -1, -1);

		if (axl_cmp (run_test_name, "test_14b"))
			run_test (test_14_b, "Test 14-b", "Check PULL API implementation (close channel request event)", -1, -1);

		if (axl_cmp (run_test_name, "test_14c"))
			run_test (test_14_c, "Test 14-c", "Check PULL API implementation (connection close/accepted)", -1, -1);

		if (axl_cmp (run_test_name, "test_14d"))
			run_test (test_14_d, "Test 14-d", "Check PULL API implementation (channel start handling)", -1, -1);

		if (axl_cmp (run_test_name, "test_15"))
			run_test (test_15, "Test 15", "Check HTTP CONNECT implementation", -1, -1);

		if (axl_cmp (run_test_name, "test_15a"))
			run_test (test_15a, "Test 15-a", "Check HTTP CONNECT implementation (run tests under HTTP CONNECT)", -1, -1);

		if (axl_cmp (run_test_name, "test_16"))
			run_test (test_16, "Test 16", "Check ALIVE profile", -1, -1);

		goto finish;
	}

	/* empty goto to avoid compiler complain about a label not
	 * used in the case only select is supported */
	goto init_test;
 init_test:
	printf ("**\n");
	printf ("** INFO: running test with I/O API: ");
	switch (vortex_io_waiting_get_current (ctx)) {
	case VORTEX_IO_WAIT_SELECT:
		printf ("select(2) system call\n");
		break;
	case VORTEX_IO_WAIT_POLL:
		printf ("poll(2) system call\n");
		break;
	case VORTEX_IO_WAIT_EPOLL:
		printf ("epoll(2) system call\n");
		break;
	} /* end if */
	printf ("**\n");

 	run_test (test_00, "Test 00", "Async Queue support", -1, -1);

 	run_test (test_00a, "Test 00-a", "Thread pool stats", -1, -1);

	run_test (test_00b, "Test 00-b", "Thread pool stats (change number)", -1, -1);

	run_test (test_00c, "Test 00-c", "Thread pool events", -1, -1);
  
 	run_test (test_01, "Test 01", "basic BEEP support", -1, -1);
  
 	run_test (test_01a, "Test 01-a", "transfer zeroed binary frames", -1, -1);
  
 	run_test (test_01b, "Test 01-b", "channel close inside created notification (31/03/2008)", -1, -1);
  
 	run_test (test_01c, "Test 01-c", "check immediately send (31/03/2008)", -1, -1);
  
 	run_test (test_01d, "Test 01-d", "MIME support", -1, -1);

 	run_test (test_01e, "Test 01-e", "Check listener douple port allocation", -1, -1);

 	run_test (test_01f, "Test 01-f", "Check connection with no greetings showed (or registerered)", -1, -1);

 	run_test (test_01g, "Test 01-g", "Check connection serverName feature on greetings", -1, -1);

	run_test (test_01h, "Test 01-h", "BEEP wrong header attack..", -1, -1);

	run_test (test_01i, "Test 01-i", "BEEP connect to (usually) unreachable address..", -1, -1);

	run_test (test_01j, "Test 01-j", "Log handling with prepared strings", -1, -1);

	run_test (test_01k, "Test 01-k", "Limitting channel send operations (memory consuption)", -1, -1);

	run_test (test_01l, "Test 01-l", "Memory consuption with channel serialize", -1, -1);

	run_test (test_01o, "Test 01-o", "Memory consuption with channel pool acquire/release API", -1, -1);

	run_test (test_01p, "Test 01-p", "Check upper limits for window sizes and idle handling (bug fix)", -1, -1);
  
 	run_test (test_02, "Test 02", "basic BEEP channel support", -1, -1);
  
 	run_test (test_02a, "Test 02-a", "connection close notification", -1, -1);

	run_test (test_02a1, "Test 02-a1", "connection close notification with handlers removed", -1, -1);
 
 	run_test (test_02b, "Test 02-b", "small message followed by close", -1, -1);
  	
 	run_test (test_02c, "Test 02-c", "huge amount of small message followed by close", -1, -1);
  	
 	run_test (test_02d, "Test 02-d", "close after large reply", -1, -1);
  
 	run_test (test_02e, "Test 02-e", "check wait reply support", -1, -1);
  
 	run_test (test_02f, "Test 02-f", "check vortex performance under packet delay scenarios", -1, -1);
  
 	run_test (test_02g, "Test 02-g", "check basic BEEP support with different frame sizes", -1, -1);
  
 	run_test (test_02h, "Test 02-h", "check bandwith performance with different window and segmentator sizes", -1, -1);

 	run_test (test_02i, "Test 02-i", "check enforced ordered delivery at server side", -1, -1);

	run_test (test_02j, "Test 02-j", "suddently connection close while operating", -1, -1);

	run_test (test_02k, "Test 02-k", "mixing replies to messages received in the same channel (ANS..NUL, RPY)", -1, -1);

	run_test (test_02l, "Test 02-l", "detect last reply written when using ANS/NUL reply.", -1, -1);

	run_test (test_02m, "Test 02-m", "blocking close after ANS/NUL replies.", -1, -1);

	run_test (test_02m1, "Test 02-m1", "Transfer with big frame sizes.", -1, -1);

  	run_test (test_02m2, "Test 02-m2", "Dealloc pending queued replies on connection shutdown.", -1, -1);

 	run_test (test_02n, "Test 02-n", "Checking MSG number reusing", -1, -1);

 	run_test (test_02o, "Test 02-o", "Checking support for seqno transfers over 4GB (MAX SEQ NO: 4294967295)", -1, -1);
 
 	run_test (test_03, "Test 03", "basic BEEP channel support (large messages)", -1, -1);
  
 	run_test (test_03a, "Test 03-a", "vortex channel pool support", -1, -1);

 	run_test (test_03b, "Test 03-b", "vortex channel pool support (ANS/NUL reply check)", -1, -1);

 	run_test (test_03c, "Test 03-c", "vortex ANS/NUL replies with serialize not attended", -1, -1);

 	run_test (test_03d, "Test 03-d", "vortex channel pool support (auxiliar pointers)", -1, -1);
  
 	run_test (test_04, "Test 04", "Handling many connections support", -1, -1);
  
 	run_test (test_04_a, "Test 04-a", "Check ANS/NUL support, sending large content", -1, -1);
  
 	run_test (test_04_ab, "Test 04-ab", "Check ANS/NUL support, sending different files", -1, -1);
  
 	run_test (test_04_c, "Test 04-c", "check client adviced profiles", -1, -1);

	run_test (test_04_d, "Test 04-d", "check channel window size reduction", -1, -1);
  
 	run_test (test_05, "Test 05", "TLS profile support", -1, -1);
  	
 	run_test (test_05_a, "Test 05-a", "Check auto-tls on fail fix (24/03/2008)", -1, -1);

	run_test (test_05_b, "Test 05-b", "TLS client blocked during connection close (14/12/2009)", -1, -1);
  
 	run_test (test_06, "Test 06", "SASL profile support", -1, -1);

	run_test (test_06a, "Test 06-a", "SASL profile support (common handler)", -1, -1);
  
 	run_test (test_07, "Test 07", "XML-RPC profile support", -1, -1);
  
 	run_test (test_08, "Test 08", "serverName configuration", -1, -1);
  
 	run_test (test_09, "Test 09", "close in transit support", -1, -1);
  
 	run_test (test_10, "Test 10", "default channel close action", -1, -1);
  
 	run_test (test_11, "Test 11", "reply to multiple messages in a wrong order without blocking", -1, -1);
  	
 	run_test (test_12, "Test 12", "check connection creation timeout", -1, -1);
  
 	run_test (test_13, "Test 13", "test TUNNEL implementation", -1, -1);

	run_test (test_14, "Test 14", "Check PULL API event masks", -1, -1);

	run_test (test_14_a, "Test 14-a", "Check PULL API implementation (frame received event)", -1, -1);

	run_test (test_14_b, "Test 14-b", "Check PULL API implementation (close channel request event)", -1, -1);

	run_test (test_14_c, "Test 14-c", "Check PULL API implementation (connection close/accepted)", -1, -1);

	run_test (test_14_d, "Test 14-d", "Check PULL API implementation (channel start handling)", -1, -1);

	run_test (test_16, "Test 15", "Check ALIVE profile", -1, -1);

	run_test (test_15, "Test 16", "Check HTTP CONNECT implementation", -1, -1);

	run_test (test_15a, "Test 16-a", "Check HTTP CONNECT implementation (run tests under HTTP CONNECT)", -1, -1);

#if defined(AXL_OS_UNIX) && defined (VORTEX_HAVE_POLL)
	/**
	 * If poll(2) I/O mechanism is available, re-run tests with
	 * the method installed.
	 */
	if (! poll_tested) {
		/* configure poll mode */
		if (! vortex_io_waiting_use (ctx, VORTEX_IO_WAIT_POLL)) {
			printf ("error: unable to configure poll I/O mechanishm");
			return axl_false;
		} /* end if */

		/* check the same run test with poll interface activated */
		poll_tested = axl_true;
		goto init_test;
	} /* end if */
#endif

#if defined(AXL_OS_UNIX) && defined (VORTEX_HAVE_EPOLL)
	/**
	 * If epoll(2) I/O mechanism is available, re-run tests with
	 * the method installed.
	 */
	if (! epoll_tested) {
		/* configure epoll mode */
		if (! vortex_io_waiting_use (ctx, VORTEX_IO_WAIT_EPOLL)) {
			printf ("error: unable to configure epoll I/O mechanishm");
			return axl_false;
		} /* end if */

		/* check the same run test with epoll interface activated */
		epoll_tested = axl_true;
		goto init_test;
	} /* end if */
#endif

 finish:

	printf ("**\n");
	printf ("** INFO: All test ok!\n");
	printf ("**\n");

	/* exit from vortex library */
	vortex_exit_ctx (ctx, axl_true);
	return 0 ;	      
}


