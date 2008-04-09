/**
 *  LibVortex:  A BEEP implementation for af-arch
 *  Copyright (C) 2006 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the
 *  License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 */

/* include vortex library */
#include <vortex.h>

/* include vortex tunnel support */
#include <vortex_tunnel.h>

/* include local headers produced by xml-rpc-gen */
#include <test_xml_rpc.h>

#ifdef AXL_OS_UNIX
#include <signal.h>
#endif

/* listener location */
char   * listener_host = NULL;
#define LISTENER_PORT       "44010"

/* listener proxy location */
char   * listener_proxy_host = NULL;
#define LISTENER_PROXY_PORT "44110"

/* substract */
void subs (struct timeval stop, struct timeval start, struct timeval * _result)
{
	long int result;

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
 * Profile use to identify the regression test server.
 */
#define REGRESSION_URI "http://iana.org/beep/transient/vortex-regression"

/** 
 * A profile to check default close channel action.
 */
#define REGRESSION_URI_2 "http://iana.org/beep/transient/vortex-regression/2"

/** 
 * A profile to check wrong order reply.
 */
#define REGRESSION_URI_3 "http://iana.org/beep/transient/vortex-regression/3"

/** 
 * A profile to check wrong order reply.
 */
#define REGRESSION_URI_4 "http://iana.org/beep/transient/vortex-regression/4"

/** 
 * A profile to check ans content transfered for a series of files
 */
#define REGRESSION_URI_5 "http://iana.org/beep/transient/vortex-regression/5"

/** 
 * A profile to check ans content transfered for a series of files
 */
#define REGRESSION_URI_6 "http://iana.org/beep/transient/vortex-regression/6"

/** 
 * A profile to check ans content transfered for a series of files
 */
#define REGRESSION_URI_6bis "http://iana.org/beep/transient/vortex-regression/6bis"

/**
 * A profile to check close in transit support.
 */ 
#define CLOSE_IN_TRANSIT_URI "http://iana.org/beep/transient/close-in-transit"

/** 
 * A profile to check sending zeroed binary frames.
 */
#define REGRESSION_URI_ZERO "http://iana.org/beep/transient/vortex-regression/zero"

/** 
 * A profile to check connection timeout against unresponsive
 * listeners.
 */
#define REGRESSION_URI_LISTENERS "http://iana.org/beep/transient/vortex-regression/fake-listener"

/** 
 * A profile to check sending zeroed binary frames.
 */
#define REGRESSION_URI_BLOCK_TLS "http://iana.org/beep/transient/vortex-regression/block-tls"

/** 
 * A regression test profile that allows to check if the listener can
 * send data just after accepting the channel to be created.
 */
#define REGRESSION_URI_FAST_SEND "http://iana.org/beep/transient/vortex-regression/fast-send"

/** 
 * @internal Allows to know if the connection must be created directly or
 * through the tunnel.
 */
bool                   tunnel_tested   = false;
VortexTunnelSettings * tunnel_settings = NULL;

/* listener context */
VortexCtx * ctx = NULL;

VortexConnection * connection_new ()
{
	/* create a new connection */
	if (tunnel_tested) {
		/* create a tunnel */
		return vortex_tunnel_new (tunnel_settings, NULL, NULL);
	} else {
		/* create a direct connection */
		return vortex_connection_new (ctx, listener_host, LISTENER_PORT, NULL, NULL);
	}
}

/** 
 * @brief Checks current implementation for async queues.
 *
 * @return true if checks runs ok, otherwise false is returned.
 */
bool test_00 () 
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
		return false;
	}

	/* check data poping */
	if (PTR_TO_INT (vortex_async_queue_pop (queue)) != 1) {
		fprintf (stderr, "expected to find value=1");
		return false;
	}

	/* check data poping */
	if (PTR_TO_INT (vortex_async_queue_pop (queue)) != 2) {
		fprintf (stderr, "expected to find value=1");
		return false;
	}

	/* check data poping */
	if (PTR_TO_INT (vortex_async_queue_pop (queue)) != 3) {
		fprintf (stderr, "expected to find value=1");
		return false;
	}

	if (vortex_async_queue_length (queue) != 0) {
		fprintf (stderr, "found different queue length, expected 0");
		return false;
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

	return true;
}

bool test_01 () {
	VortexConnection  * connection;
	VortexChannel     * channel;
	VortexAsyncQueue  * queue;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, false)) {
		vortex_connection_close (connection);
		return false;
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
		return false;
	}

	/* ok, close the channel */
	vortex_channel_close (channel, NULL);

	/* ok, close the connection */
	vortex_connection_close (connection);

	/* free queue */
	vortex_async_queue_unref (queue);

	/* return true */
	return true;
}

bool test_01a () {
	VortexConnection  * connection;
	VortexChannel     * channel;
	VortexAsyncQueue  * queue;
	unsigned char     * content;
	int                 iterator;
	int                 iterator2;
	VortexFrame       * frame;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, false)) {
		vortex_connection_close (connection);
		return false;
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
		return false;
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
			return false;
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
			return false;
		} /* end if */
		

		/* check the content */
		if (vortex_frame_get_payload_size (frame) != 1024 * 64) {
			printf ("Expected to find a frame of size %d, but found %d..\n",
				1024 * 64, vortex_frame_get_payload_size (frame));
			return false;
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

	/* return true */
	return true;
}

void test_01b_created (int             channel_num, 
		       VortexChannel * channel, 
		       axlPointer      user_data)
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

bool test_01b () {
	VortexConnection  * connection;
	VortexAsyncQueue  * queue;
	int                 iterator;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, false)) {
		vortex_connection_close (connection);
		return false;
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
			return false;
		} /* end if */

		iterator++;
	} /* end while */

	/* close the connection */
	vortex_connection_close (connection);

	vortex_async_queue_unref (queue);

	return true;

} /* end test_01b */

bool test_01c () {
	VortexConnection  * connection;
	VortexAsyncQueue  * queue;
	VortexChannel     * channel;
	VortexFrame       * frame;
	int                 iterator;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, false)) {
		vortex_connection_close (connection);
		return false;
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
		
		if (channel == NULL) {
			printf ("Unable to create channel for fast send..\n");
			return false;
		} /* end if */

		/* check connection here */
		if (! vortex_connection_is_ok (connection, false)) {
			printf ("Test 01-c: (1) Failed to check connection, it should be running..\n");
			return false;
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
			return false;
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
			return false;
		} /* end if */

		vortex_frame_unref (frame);
		
		/* close channel */
		if (! vortex_channel_close (channel, NULL)) {
			printf ("Unable to close the channel..\n");
			return false;
		}

		/* check connection here */
		if (! vortex_connection_is_ok (connection, false)) {
			printf ("Test 01-c: (2) Failed to check connection, it should be running..\n");
			return false;
		} /* end if */
		
		iterator++;
	} /* end while */

	/* check connection here */
	if (! vortex_connection_is_ok (connection, false)) {
		printf ("Test 01-c: (3) Failed to check connection, it should be running..\n");
		return false;
	} /* end if */

	/* close the connection */
	vortex_connection_close (connection);

	vortex_async_queue_unref (queue);
	
	return true;

} /* end test_01c */

#define TEST_02_MAX_CHANNELS 24

bool test_02 () {

	VortexConnection * connection;
	VortexChannel    * channel[TEST_02_MAX_CHANNELS];
	VortexAsyncQueue * queue;
	VortexFrame      * frame;
	int                iterator;
	char             * message;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, false)) {
		vortex_connection_close (connection);
		return false;
		
	} /* end if */
	
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
			return false;
		}

		/* enable serialize */
		vortex_channel_set_serialize (channel[iterator], true);

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
			return false;
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
		return false;
	}

	/* now send data */
	iterator = 0;
	while (iterator < TEST_02_MAX_CHANNELS) {
		
		/* build the message */
		message = axl_strdup_printf ("Message: %d\n", iterator);
		
		/* send message */
		if (! vortex_channel_send_msg (channel[0], message, strlen (message), NULL)) {
			printf ("Unable to send message over channel=%d\n", vortex_channel_get_number (channel[0]));
			return false;
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
			return false;
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
		return false;
	}

	/* close all channels */
	iterator = 0;
	while (iterator < TEST_02_MAX_CHANNELS) {

		/* close the channel */
		if (! vortex_channel_close (channel[iterator], NULL)) {
			printf ("failed to close channel=%d\n", vortex_channel_get_number (channel[iterator]));
			return false;
		}

		/* update the iterator */
		iterator++;

	} /* end while */

	/* ok, close the connection */
	if (! vortex_connection_close (connection)) {
		printf ("failed to close the BEEP session\n");
		return false;
	} /* end if */

	/* free queue */
	vortex_async_queue_unref (queue);

	/* return true */
	return true;
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
 * @return true if ok, otherwise false is returned.
 */
bool test_02a () {

	VortexConnection * connection;
	int                count;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, false)) {
		vortex_connection_close (connection);
		return false;
		
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
		return false;
	}

	/* unref the queue */
	vortex_async_queue_unref (test_02a_queue);

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, false)) {
		vortex_connection_close (connection);
		return false;
		
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
		return false;
	}

	/* unref the queue */
	vortex_async_queue_unref (test_02a_queue);
	

	return true;
}

bool test_02b () {
	VortexConnection  * connection;
	VortexChannel     * channel;
	VortexAsyncQueue  * queue;
	VortexFrame       * frame;


	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, false)) {
		vortex_connection_close (connection);
		return false;
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
		return false;
	}

	/* send a message the */
	if (! vortex_channel_send_msg (channel, "a", 1, NULL)) {
		printf ("failed to send a small message\n");
		return false;
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

	/* return true */
	return true;
}

bool test_02c () {
	VortexConnection  * connection;
	VortexChannel     * channel;
	VortexAsyncQueue  * queue;
	VortexFrame       * frame;
	int                 iterator;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, false)) {
		vortex_connection_close (connection);
		return false;
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
		return false;
	}

	/* send a message the */
	iterator = 0;
	while (iterator < 1000) {
	
		if (! vortex_channel_send_msg (channel, "a", 1, NULL)) {
			printf ("failed to send a small message\n");
			return false;
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

	/* return true */
	return true;
}

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

/** 
 * @brief Checks BEEP support to send large messages that goes beyond
 * default window size advertised.
 * 
 * 
 * @return true if the test is ok, otherwise false is returned.
 */
bool test_03 () {
	VortexConnection * connection;
	VortexChannel    * channel;
	VortexFrame      * frame;
	VortexAsyncQueue * queue;
	char             * message;
	int                iterator;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, false)) {
		vortex_connection_close (connection);
		return false;
	}

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
		return false;
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
			return false;
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
			
			return false;
		}

		/* check result */
		if (memcmp (message, vortex_frame_get_payload (frame), TEST_03_MSGSIZE)) {
			printf ("Test 03: Messages aren't equal\n");
			return false;
		}

		/* check the reference of the channel associated */
		if (vortex_frame_get_channel_ref (frame) == NULL) {
			printf ("Test 03: Frame received doesn't have a valid channel reference configured\n");
			return false;
		} /* end if */

		/* check channel reference */
		if (! vortex_channel_are_equal (vortex_frame_get_channel_ref (frame),
						channel)) {
			printf ("Test 03: Frame received doesn't have the spected channel reference configured\n");
			return false;
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

	/* ok, close the connection */
	vortex_connection_close (connection);

	/* free queue */
	vortex_async_queue_unref (queue);

	/* return true */
	return true;
}

bool test_03a () {
	
	VortexConnection   * connection;
	VortexChannelPool  * pool;
	axlList            * channels;
	VortexChannel      * channel;
	

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, false)) {
		vortex_connection_close (connection);
		return false;
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
	if (!vortex_connection_is_ok (connection, false)) {
		vortex_connection_close (connection);
		return false;
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
		return false;
	}

	/* use every channel */
	channels = axl_list_new (axl_list_always_return_1, NULL);

	/* get next channel */
	channel = vortex_channel_pool_get_next_ready (pool, false);
	if (channel == NULL) 
		return false;
	axl_list_add (channels, channel);

	/* get next channel */
	channel = vortex_channel_pool_get_next_ready (pool, false);
	if (channel == NULL) 
		return false;
	axl_list_add (channels, channel);

	/* get next channel */
	channel = vortex_channel_pool_get_next_ready (pool, false);
	if (channel == NULL) 
		return false;
	axl_list_add (channels, channel);


	/* get next channel */
	channel = vortex_channel_pool_get_next_ready (pool, false);
	if (channel == NULL) 
		return false;
	axl_list_add (channels, channel);

	/* get next channel */
	channel = vortex_channel_pool_get_next_ready (pool, false);
	if (channel != NULL) 
		return false;

	/* free the list */
	axl_list_free (channels);

	/* ok, close the connection */
	vortex_connection_close (connection);

	/* return true */
	return true;
}

/* constant for test_04 */
#define MAX_NUM_CON 1000

bool test_04 ()
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
		if (!vortex_connection_is_ok (connections[iterator], false)) {
			printf ("Test 04: Unable to connect remote server, error was: %s",
				vortex_connection_get_message (connections[iterator]));
			return false;
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
			return false;

		/* update iterator */
		iterator++;
	}

	/* return true */
	return true;
}

#define TEST_05_MSGSIZE (65536 * 8)

/** 
 * @brief Checking TLS profile support.
 * 
 * @return true if ok, otherwise, false is returned.
 */
bool test_05 ()
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

	/* initialize and check if current vortex library supports TLS */
	if (! vortex_tls_is_enabled (ctx)) {
		printf ("--- WARNING: Unable to activate TLS, current vortex library has not TLS support activated. \n");
		return true;
	}

	/* create a new connection */
	connection = connection_new ();

	/* enable TLS negociation */
	connection = vortex_tls_start_negociation_sync (connection, NULL, 
							&status,
							&status_message);
	if (vortex_connection_get_data (connection, "being_closed")) {
		printf ("Found TLS connection flagges a being_closed\n");
		return false;
	}

	if (status != VortexOk) {
		printf ("Test 05: Failed to activate TLS support: %s\n", status_message);
		/* return false */
		return false;
	}

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
		return false;
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
			return false;
		}

		/* update the iterator */
		iterator++;
		
	} /* end while */
	
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
			
			return false;
		}

		/* check result */
		if (memcmp (message, vortex_frame_get_payload (frame), TEST_05_MSGSIZE)) {
			printf ("Test 05: Messages aren't equal\n");
			return false;
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
		return false;

	/* ok, close the connection */
	if (! vortex_connection_close (connection))
		return false;

	/* free the queue */
	vortex_async_queue_unref (queue);

	/* close connection */
	return true;
}

/** 
 * @brief Checking TLS profile support.
 * 
 * @return true if ok, otherwise, false is returned.
 */
bool test_05_a ()
{
	/* TLS status notification */
	VortexStatus       status;
	char             * status_message = NULL;
	VortexChannel    * channel;
	int                connection_id;

	/* vortex connection */
	VortexConnection * connection;
	VortexConnection * connection2;

	/* initialize and check if current vortex library supports TLS */
	if (! vortex_tls_is_enabled (ctx)) {
		printf ("--- WARNING: Unable to activate TLS, current vortex library has not TLS support activated. \n");
		return true;
	}

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
		return false;
	}

	/* enable TLS negociation but get the connection id first */
	connection_id = vortex_connection_get_id (connection);
	connection    = vortex_tls_start_negociation_sync (connection, NULL, 
							   &status,
							   &status_message);

	if (connection == NULL) {
		printf ("Test 05-a: expected an error but not NULL reference..\n");
		return false;
	} /* end if */

	if (vortex_connection_get_id (connection) != connection_id) {
		printf ("Test 05-a: expected an error but not a connection change..\n");
		return false;
	} /* end if */

	if (status == VortexOk) {
		printf ("Test 05-a: Failed to activate TLS support: %s\n", status_message);
		/* return false */
		return false;
	}

	/* check that the connection is ok */
	if (! vortex_connection_is_ok (connection, false)) {
		printf ("Test 05-a: expected an error but not a connection closed..\n");
		return false;
	} /* end if */

	/* now, do the same text to force a TLS error at the remote
	 * side (activated due to previous exchange) */
	connection_id = vortex_connection_get_id (connection);
	connection    = vortex_tls_start_negociation_sync (connection, NULL, 
							   &status,
							   &status_message);

	if (connection == NULL) {
		printf ("Test 05-a: expected an error but not NULL reference..\n");
		return false;
	} /* end if */

	if (status == VortexOk) {
		printf ("Test 05-a: Failed to activate TLS support: %s\n", status_message);
		/* return false */
		return false;
	}

	/* check that the connection is ok */
	if (vortex_connection_is_ok (connection, false)) {
		printf ("Test 05-a: expected an error but  a connection properly running..\n");
		return false;
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
		return false;
	}

	/* enable autotls */
	vortex_connection_set_auto_tls (ctx, true, false, NULL);

	/* now check autotls */
	connection2 = connection_new ();

	if (connection2 == NULL) {
		printf ("Test 05-a: expected a failure using auto-tls but not a null reference..\n");
		return false;
	}

	if (vortex_connection_is_ok (connection2, false)) {
		printf ("Test 05-a: expected a failure using auto-tls but a proper connection status was found...\n");
		return false;
	}

	/* close the connection */
	vortex_connection_close (connection2);
	vortex_connection_close (connection);

	/* now create a connection with auto tls activated */
	connection2 = connection_new ();

	if (connection2 == NULL) {
		printf ("Test 05-a: expected a proper connection using auto-tls but not a null reference..\n");
		return false;
	}

	/* check connection status */
	if (! vortex_connection_is_ok (connection2, false)) {
		printf ("Test 05-a: expected a proper connection using auto-tls but a failure was found...\n");
		return false;
	}

	/* check tls fixate status */
	if (! vortex_connection_is_tlsficated (connection2)) {
		printf ("Test 05-a: expected proper TLS fixate status..\n");
		return false;
	}
	
	/* close the connection */
	vortex_connection_close (connection2);
	
	/* restore auto-tls */
	vortex_connection_set_auto_tls (ctx, false, false, NULL);

	/* now create a connection with auto tls activated */
	connection2 = connection_new ();

	if (connection2 == NULL) {
		printf ("Test 05-a: expected a proper connection using auto-tls but not a null reference..\n");
		return false;
	}

	/* check connection status */
	if (! vortex_connection_is_ok (connection2, false)) {
		printf ("Test 05-a: expected a proper connection using auto-tls but a failure was found...\n");
		return false;
	}

	/* check tls fixate status */
	if (vortex_connection_is_tlsficated (connection2)) {
		printf ("Test 05-a: expected to find disable automatic TLS negotiation, but it found enabled..\n");
		return false;
	}
	
	/* close the connection */
	vortex_connection_close (connection2);


	return true;
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
/*	printf ("Received message reply: ansno=%d type=%d\n", 
	vortex_frame_get_ansno (frame), vortex_frame_get_type (frame));  */
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
		
		printf ("Test 04-a:   Operation completed..Ok!\n");
		vortex_async_queue_push ((axlPointer) user_data, INT_TO_PTR (1));
		return;
	} /* end if */
	
	/* check content */
	if (!axl_cmp ((char *) vortex_frame_get_payload (frame), 
		      TEST_REGRESION_URI_4_MESSAGE)) {
		printf ("Expected to find different message at test: %s != '%s'..\n",
			(char *) vortex_frame_get_payload (frame), TEST_REGRESION_URI_4_MESSAGE);
		return;
	} /* end if */
	
	/* check size */
	if (vortex_frame_get_payload_size (frame) != 4096) {
		printf ("Expected to find different message(%d) size %d at test, but found: %d..\n",
			vortex_frame_get_ansno (frame), 4096, vortex_frame_get_payload_size (frame));
		return;
	}
	
	return;
}

/** 
 * @brief Checks BEEP support for ANS/NUL while sending large messages
 * that goes beyond default window size advertised.
 * 
 * 
 * @return true if the test is ok, otherwise false is returned.
 */
bool test_04_a () {

	VortexConnection * connection;
	VortexChannel    * channel;
	VortexAsyncQueue * queue;
	int                iterator;
	VortexFrame      * frame;

#if defined(AXL_OS_UNIX)
	struct timeval      start;
	struct timeval      stop;
#endif
	bool                second_round = true;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, false)) {
		vortex_connection_close (connection);
		return false;
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
		return false;
	}

 second_round_label:
	/* request for the file */
#if defined(AXL_OS_UNIX)	
	/* take a start measure */
	gettimeofday (&start, NULL);
#endif
	printf ("Test 04-a:   sending initial request\n");
	if (! vortex_channel_send_msg (channel, "return large message", 20, NULL)) {
		printf ("Failed to send message requesting for large file..\n");
		return false;
	} /* end if */

	/* wait for all replies */
	iterator = 0;
	printf ("Test 04-a:   waiting replies\n");
	while (true) {
		/* get the next message, blocking at this call. */
		frame = vortex_channel_get_reply (channel, queue);
		
		if (frame == NULL) {
			printf ("Timeout received for regression test: %s\n", REGRESSION_URI_4);
			continue;
		}
		
		if (iterator == 8192) {
			if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_NUL) {
				printf ("Expected to find NUL terminator message, but found: %d frame type\n",
					vortex_frame_get_type (frame));
				return false;
			} /* end if */

			/* check size */
			if (vortex_frame_get_payload_size (frame) != 0) {
				printf ("Expected to find NUL terminator message, with empty content, but found: %d frame size\n",
					vortex_frame_get_payload_size (frame));
				return false;
			} /* end if */

			/* deallocate the frame received */
			vortex_frame_unref (frame);

			printf ("Test 04-a:   operation completed, ok\n");
			break;
		}
		
		/* check content */
		if (!axl_cmp ((char *) vortex_frame_get_payload (frame),
			      TEST_REGRESION_URI_4_MESSAGE)) {
			printf ("Expected to find different message(%d) at test: %s != '%s'..\n",
				iterator, (char *) vortex_frame_get_payload (frame), TEST_REGRESION_URI_4_MESSAGE);
			return false;
		} /* end if */

		/* check size */
		if (vortex_frame_get_payload_size (frame) != 4096) {
			printf ("Expected to find different message(%d) size %d at test, but found: %d..\n",
				iterator, 4096, vortex_frame_get_payload_size (frame));
			return false;
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

	printf ("Test 04-a:   Test ok, operation completed in: %ld.%ld seconds!  (bytes transfered: %d)!\n", 
		stop.tv_sec, stop.tv_usec, 8192 * 4096);
#endif

	printf ("Test 04-a:   now, perform the same operation without queue/reply, using frame received handler\n");

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
	if (! vortex_channel_send_msg (channel, "return large message", 20, NULL)) {
		printf ("Failed to send message requesting for large file..\n");
		return false;
	} /* end if */

	/* wait until the file is received */
	printf ("Test 04-a:   waiting for file to be received\n");
	vortex_async_queue_pop (queue);
	printf ("Test 04-a:   file received, ok\n");

#if defined(AXL_OS_UNIX)
	/* take a stop measure */
	gettimeofday (&stop, NULL);

	/* substract */
	subs (stop, start, &stop);

	printf ("Test 04-a:   Test ok, operation completed in: %ld.%ld seconds! (bytes transfered: %d)!\n", 
		stop.tv_sec, stop.tv_usec, 8192 * 4096);
#endif

	/* free the queue */
	vortex_async_queue_unref (queue);

	/* if second round label activated, rerun tests .. */
	if (second_round) {
		/* release the queue */
		queue = vortex_async_queue_new ();

		/* reconfigure frame received */
		vortex_channel_set_received_handler (channel, vortex_channel_queue_reply, queue);

		second_round = false;
		goto second_round_label;
	} /* end if */

	/* ok, close the channel */
	if (! vortex_channel_close (channel, NULL))
		return false;

	/* ok, close the connection */
	if (! vortex_connection_close (connection))
		return false;

	/* close connection */
	return true;
}

char * test_04_ab_gen_md5 (const char * file)
{
	char * result;
	char * resultAux;
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
		fprintf (stdout, "Unable to properly read the file, size expected to read %ld, wasn't fulfilled",
			 status.st_size);
		axl_free (result);
		fclose (handle);
		return NULL;
	} /* end if */
	
	/* close the file and return the content */
	fclose (handle);

	/* now create the md5 representation */
	resultAux = vortex_tls_get_digest_sized (VORTEX_MD5, result, status.st_size);
	axl_free (result);

	return resultAux;
}

/** 
 * @brief Checks BEEP support for ANS/NUL while sending different
 * files in the same channel.
 *
 * @return true if the test is ok, otherwise false is returned.
 */
bool test_04_ab () {

	VortexConnection * connection;
	VortexChannel    * channel;
	VortexAsyncQueue * queue;
	VortexFrame      * frame;
	char             * file_name;
	char             * md5;
	char             * md5Aux;
	FILE             * file;
	int                bytes_written;
	int                iterator = 0;

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, false)) {
		vortex_connection_close (connection);
		return false;
	}

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
		return false;
	}

	/* configure serialize */
	vortex_channel_set_serialize (channel, true);

	/* configure the file requested: first file */
	file_name = "vortex-regression-client.c";

 transfer_file:
	/* create the md5 to track content transfered */
	md5       = test_04_ab_gen_md5 (file_name);
	printf ("Test 04-ab:   request files: %s (md5: %s)\n", file_name, md5);
	if (! vortex_channel_send_msg (channel, file_name, strlen (file_name), NULL)) {
		printf ("Failed to send message request to retrieve file: %s..\n", file_name);
		return false;
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
		return false;
	}

	/* wait for all replies */
	printf ("Test 04-ab:   waiting replies having file: %s\n", file_name);
	while (true) {
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
			return false;
		} /* end if */
			
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
		return false;
	}
	
	printf ("Test 04-ab:   content transfered for %s ok\n", file_name);

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

	/* free the queue */
	vortex_async_queue_unref (queue);

	/* ok, close the channel */
	if (! vortex_channel_close (channel, NULL))
		return false;

	/* ok, close the connection */
	if (! vortex_connection_close (connection))
		return false;

	/* close connection */
	return true;
}

/** 
 * @brief Checks client adviced profiles.
 *
 * @return true if the test is ok, otherwise false is returned.
 */
bool test_04_c () {

	VortexConnection * connection;
	VortexChannel    * channel;

	/* register two profiles for this session */
	vortex_profiles_register (ctx, 
				  "urn:vortex:regression-test:uri:1", 
				  /* start channel */
				  NULL, NULL, 
				  /* stop channel */
				  NULL, NULL, 
				  /* frame received */
				  NULL, NULL);

	/* register two profiles for this session */
	vortex_profiles_register (ctx,
				  "urn:vortex:regression-test:uri:2", 
				  /* start channel */
				  NULL, NULL, 
				  /* stop channel */
				  NULL, NULL, 
				  /* frame received */
				  NULL, NULL);

	/* register two profiles for this session */
	vortex_profiles_register (ctx, 
				  "urn:vortex:regression-test:uri:3", 
				  /* start channel */
				  NULL, NULL, 
				  /* stop channel */
				  NULL, NULL, 
				  /* frame received */
				  NULL, NULL);

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, false)) {
		vortex_connection_close (connection);
		return false;
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
		return false;
	}

	/* ok, close the channel */
	if (! vortex_channel_close (channel, NULL))
		return false;

	/* ok, close the connection */
	if (! vortex_connection_close (connection))
		return false;

	/* register two profiles for this session */
	vortex_profiles_unregister (ctx, "urn:vortex:regression-test:uri:1");
	vortex_profiles_unregister (ctx, "urn:vortex:regression-test:uri:2");
	vortex_profiles_unregister (ctx, "urn:vortex:regression-test:uri:3");

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, false)) {
		vortex_connection_close (connection);
		return false;
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
		return false;
	}

	/* ok, close the channel */
	if (! vortex_channel_close (channel, NULL))
		return false;

	/* ok, close the connection */
	if (! vortex_connection_close (connection))
		return false;

	/* close connection */
	return true;
}


/** 
 * @brief Checking SASL profile support.
 * 
 * @return true if ok, otherwise, false is returned.
 */
bool test_06 ()
{
	VortexStatus       status;
	char             * status_message = NULL;
	VortexConnection * connection;

	/* check for SASL support */
	if (! vortex_sasl_is_enabled ()) {
		printf ("--- WARNING: Unable to begin SASL negociation. Current Vortex Library doesn't support SASL");
		return true;
	}

	/**** CHECK SASL ANONYMOUS MECHANISM ****/
	
	/* create a new connection */
	connection = connection_new ();

	/* check connection created */
	if (! vortex_connection_is_ok (connection, false)) {
		return false;
	}

	printf ("Test 06: SASL ANONYMOUS profile support ");

	/* begin SASL ANONYMOUS negociation */ 
	vortex_sasl_set_propertie (connection, VORTEX_SASL_ANONYMOUS_TOKEN,
				   "test-fail@aspl.es", NULL);
	
	vortex_sasl_start_auth_sync (connection, VORTEX_SASL_ANONYMOUS, &status, &status_message);

	if (status != VortexError) {
		printf ("Expected failed anonymous SASL login..\n");
		return false;
	} /* end if */

	/* begin SASL ANONYMOUS negociation */ 
	vortex_sasl_set_propertie (connection, VORTEX_SASL_ANONYMOUS_TOKEN,
				   "test@aspl.es", NULL);
	
	vortex_sasl_start_auth_sync (connection, VORTEX_SASL_ANONYMOUS, &status, &status_message);

	if (status != VortexOk) {
		printf ("Failed to authenticate expected anonymous to work..\n");
		return false;
	} /* end if */


	/* close the connection */
	vortex_connection_close (connection);

	printf ("[   OK   ]\n");

	/**** CHECK SASL EXTERNAL MECHANISM ****/

	/* create a new connection */
	connection = connection_new ();

	/* check connection created */
	if (! vortex_connection_is_ok (connection, false)) {
		return false;
	}

	printf ("Test 06: SASL EXTERNAL profile support ");

	/* set external properties */
	vortex_sasl_set_propertie (connection, VORTEX_SASL_AUTHORIZATION_ID,
				   "acinom1", NULL);

	/* begin external auth */
	vortex_sasl_start_auth_sync (connection, VORTEX_SASL_EXTERNAL, &status, &status_message);

	if (status != VortexError) {
		printf ("Expected to find an authentication failed for the EXTERNAL mechanism..\n");
		return false;
	} /* end if */

	/* check SASL channels opened at this point */
	if (vortex_connection_channels_count (connection) != 1) {
		printf ("Expected to find only one channel but found: %d..\n", 
			vortex_connection_channels_count (connection));
		return false;
	} /* end if */

	/* set external properties */
	vortex_sasl_set_propertie (connection, VORTEX_SASL_AUTHORIZATION_ID,
				   "acinom", NULL);

	/* begin external auth */
	vortex_sasl_start_auth_sync (connection, VORTEX_SASL_EXTERNAL, &status, &status_message);

	if (status != VortexOk) {
		printf ("Failed to authenticate expected anonymous to work..\n");
		return false;
	} /* end if */

	/* close the connection */
	vortex_connection_close (connection);

	printf ("[   OK   ]\n");

	/**** CHECK PLAIN MECHANISM ****/

	printf ("Test 06: SASL PLAIN profile support ");
	
	/* create a new connection */
	connection = connection_new ();

	/* check connection created */
	if (! vortex_connection_is_ok (connection, false)) {
		return false;
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
		return false;
	} /* end if */

	/* check SASL channels opened at this point */
	if (vortex_connection_channels_count (connection) != 1) {
		printf ("Expected to find only one channel but found: %d..\n", 
			vortex_connection_channels_count (connection));
		return false;
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
		return false;
	} /* end if */

	/* close the connection */
	vortex_connection_close (connection);

	printf ("[   OK   ]\n");

	/**** CHECK CRAM-MD5 MECHANISM ****/

	printf ("Test 06: SASL CRAM-MD5 profile support ");

	/* create a new connection */
	connection = connection_new ();

	/* check connection created */
	if (! vortex_connection_is_ok (connection, false)) {
		return false;
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
		return false;
	} /* end if */

	/* check SASL channels opened at this point */
	if (vortex_connection_channels_count (connection) != 1) {
		printf ("Expected to find only one channel but found: %d..\n", 
			vortex_connection_channels_count (connection));
		return false;
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
		return false;
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
	if (! vortex_connection_is_ok (connection, false)) {
		return false;
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
		return false;
	} /* end if */

	if (vortex_connection_channels_count (connection) != 1) {
		printf ("Expected to find only one channel but found: %d..\n", 
			vortex_connection_channels_count (connection));
		return false;
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
		return false;
	} /* end if */

	/* close the connection */
	vortex_connection_close (connection);

	printf ("[   OK   ]\n");
#endif
	
	
	return true;
	
}

/** 
 * @brief Test XML-RPC support.
 * 
 * 
 * @return true if all test pass, otherwise false is returned.
 */
bool test_07 () {
	
	VortexConnection * connection;
	VortexChannel    * channel;
	int                iterator;
	/* test 02 */
	char             * result;
	/* test 03 */
	bool               bresult;
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

	/* check xml rpc profile support */
	if (! vortex_xml_rpc_is_enabled ()) {
		printf ("--- WARNING: XML-RPC profile support is not enabled, unable to check\n");
		return true;
	} /* end if */

	/* create a new connection */
	connection = connection_new ();

	/* create the xml-rpc channel */
	channel = BOOT_CHANNEL (connection, NULL);

	/*** TEST 01 ***/
	if (7 != test_sum_int_int_s (3, 4, channel, NULL, NULL, NULL)) {
		fprintf (stderr, "ERROR: An error was found while invoking..\n");
		return false;
	}

	/* perform the invocation */
	iterator = 0;
	while (iterator < 100) {

		if (-3 != test_sum_int_int_s (10, -13, channel, NULL, NULL, NULL)) {
			fprintf (stderr, "ERROR: An error was found while invoking..\n");
			return false;
		}
		iterator++;
	}

	/*** TEST 02 ***/
	/* get the string from the function */
	result = test_get_the_string_s (channel, NULL, NULL, NULL);
	if (result == NULL) {
		fprintf (stderr, "An error was found while retreiving the result, a NULL value was received when an string was expected\n");
		return false;
	}
	
	if (! axl_cmp (result, "This is a test")) {
		fprintf (stderr, "An different string value was received than expected '%s' != '%s'..",
			 "This is a test", result);
		return false;
	}

	/* free the result */
	axl_free (result);

	/*** TEST 03 ***/
	/* get the string from the function */
	bresult = test_get_the_bool_1_s (channel, NULL, NULL, NULL);
	if (bresult != false) {
		fprintf (stderr, "Expected to receive a false bool value..\n");
		return false;
	}

	bresult = test_get_the_bool_2_s (channel, NULL, NULL, NULL);
	if (bresult != true) {
		fprintf (stderr, "Expected to receive a true bool value..\n");
		return false;
	}

	/*** TEST 04 ***/
	/* get the string from the function */
	dresult = test_get_double_sum_double_double_s (7.2, 8.3, channel, NULL, NULL, NULL);
	if (dresult != 15.5) {
		fprintf (stderr, "Expected to receive double value 15.5 but received: %g..\n", dresult);
		return false;
	}

	/*** TEST 05 ***/
	/* get the string from the function */
	a      = test_values_new (3, 10.2, false);
	b      = test_values_new (7, 7.12, true);
	
	/* perform invocation */
	struct_result = test_get_struct_values_values_s (a, b, channel, NULL, NULL, NULL);

	if (struct_result == NULL) {
		fprintf (stderr, "Expected to receive a non NULL result from the service invocation\n");
		return false;
	}

	if (struct_result->count != 10) {
		fprintf (stderr, "Expected to receive a value not found (count=10 != count=%d\n",
			    struct_result->count);
		return false;
	}

	if (struct_result->fraction != 17.32) {
		fprintf (stderr, "Expected to receive a value not found (fraction=17.32 != fraction=%g\n",
			    struct_result->fraction);
		return false;
	}

	if (! struct_result->status) {
		fprintf (stderr, "Expected to receive a true value for status\n");
		return false;
	}

	/* release memory allocated */
	test_values_free (a);
	test_values_free (b);
	test_values_free (struct_result);

	/*** TEST 06 ***/
	/* get the array content */
	array = test_get_array_s (channel, NULL, NULL, NULL);
	
	if (array == NULL) {
		fprintf (stderr, "Expected to receive a non NULL result from the service invocation\n");
		return false;
	}

	/* for each item */
	iterator = 0;
	for (; iterator < 10; iterator++) {
		/* get a reference to the item */
		item = test_itemarray_get (array, iterator);

		/* check null reference */
		if (item == NULL) {
			fprintf (stderr, "Expected to find a struct reference inside..\n");
			return false;
		}

		/* check the int value inside the struct */
		if (item->position != iterator) {
			fprintf (stderr, "Expected to find an integer value: (%d) != (%d)\n",
				    item->position, iterator);
			return false;
		}

		/* check the string inside */
		if (! axl_cmp (item->string_position, "test content")) {
			fprintf (stderr, "Expected to find a string value: (%s) != (%s)\n",
				 item->string_position, "test content");
			return false;
		}
	}
	

	/* free result */
	test_itemarray_free (array);


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

	/* close the connection */
	vortex_connection_close (connection);

	return true;
}

/** 
 * @brief Checks if the serverName attribute is properly configured
 * into the connection once the first successfull channel is created.
 * 
 * 
 * @return true if serverName is properly configured, otherwise false
 * is returned.
 */
bool test_08 ()
{
	VortexConnection * connection;
	VortexChannel    * channel;
	VortexAsyncQueue * queue;
	VortexFrame      * frame;

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
		return false;
	}

	/* check server name configuration */
	if (! axl_cmp ("dolphin.aspl.es", vortex_connection_get_server_name (connection))) {
		printf ("Failed to get the proper serverName value for the connection..\n");
		return false;
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
		return false;
	}
	if (! axl_cmp (vortex_frame_get_payload (frame), "dolphin.aspl.es")) {
		printf ("Received a different server name configured that expected: %s..\n",
			(char*) vortex_frame_get_payload (frame));
		return false;
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
		return false;
	}

	/* check server name configuration */
	if (! axl_cmp ("dolphin.aspl.es", vortex_connection_get_server_name (connection))) {
		printf ("Failed to get the proper serverName value for the connection..\n");
		return false;
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
		return false;
	}
	if (! axl_cmp (vortex_frame_get_payload (frame), "dolphin.aspl.es")) {
		printf ("Received a different server name configured that expected: %s..\n",
			(char*) vortex_frame_get_payload (frame));
		return false;
	}

	/* free frame */
	vortex_frame_unref (frame);

	/* free queue */
	vortex_async_queue_unref (queue);

	/* close the connection */
	vortex_connection_close (connection);
	return true;
}

/** 
 * @brief Checks if the default handler for close channel request is
 * to accept the close operation.
 * 
 * 
 * @return false if it fails, otherwise true is returned.
 */
bool test_10 ()
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
		return false;
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
		return false;
	}

	/* close the connection */
	vortex_connection_close (connection);
	return true;
}

/** 
 * @brief Check if the caller can reply to a message that is not the
 * next to be replied without being blocked.
 * 
 * 
 * @return false if it fails, otherwise true is returned.
 */
bool test_11 ()
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

	/* send a message to activate the test, then the listener will
	 * ack it and send three messages that will be replied in
	 * wrong other. */
	iterator = 0;
	while (iterator < 10) {
		/* printf ("  --> activating test (%d)..\n", iterator); */
		vortex_channel_send_msg (channel, "", 0, NULL);
		
		/* get the reply and unref */
		frame = vortex_channel_get_reply (channel, queue);
		if (frame == NULL || vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY) {
			printf ("Test 11: failed, expected to find reply defined or to be RPY..\n");
			return false;
		} /* end if */
		vortex_frame_unref (frame);
		
		/* Test 11: test in progress... */
		
		/* ask for all replies */
		frame     = NULL;
		frame2    = NULL;
		frame3    = NULL;

		if (! vortex_connection_is_ok (connection, false)) {
			printf ("Test 11: failed, detected connection close before getting first frame..\n");
			return false;
		}

		/* get the first frame */
		frame = vortex_channel_get_reply (channel, queue);
		/* printf ("."); */
		if (frame == NULL || vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_MSG) {
			printf ("Test 11: failed, expected to find defined frame and using type MSG\n");
			return false;
		} /* end if */

		if (! vortex_connection_is_ok (connection, false)) {
			printf ("Test 11: failed, detected connection close before getting second frame..\n");
			return false;
		}
		
		/* get the first frame2 */
		frame2 = vortex_channel_get_reply (channel, queue);
		/* printf ("."); */
		if (frame2 == NULL || vortex_frame_get_type (frame2) != VORTEX_FRAME_TYPE_MSG) {
			printf ("Test 11: failed, expected to find defined frame and using type MSG\n");
			return false;
		} /* end if */

		if (! vortex_connection_is_ok (connection, false)) {
			printf ("Test 11: failed, detected connection close before getting third frame..\n");
			return false;
		}
		
		/* get the first frame2 */
		frame3 = vortex_channel_get_reply (channel, queue);
		
		/* printf ("."); */
		if (frame3 == NULL || vortex_frame_get_type (frame3) != VORTEX_FRAME_TYPE_MSG) {
			printf ("Test 11: failed, expected to find defined frame and using type MSG\n");
			return false;
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

		if (! vortex_connection_is_ok (connection, false)) {
			printf ("Test 11: failed, detected connection close after sending data..\n");
			return false;
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
		return false;
	}

	/* close the connection */
	vortex_connection_close (connection);

	/* unref the queue */
	vortex_async_queue_unref (queue);

	return true;
}

/** 
 * @brief Checks connection creating timeout.
 * 
 * 
 * @return true if test pass, otherwise false is returned.
 */
bool test_12 () {
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
		return false;
	}

	/* configure a new timeout (10 seconds, 10000000 microseconds) */
	vortex_connection_connect_timeout (ctx, 10000000);

	/* check new timeout configured */
	if (vortex_connection_get_connect_timeout (ctx) != 10000000) {
		printf ("Test 12 (2): failed, expected to receive 10000000 timeout configuration: but received %ld..\n",
			vortex_connection_get_connect_timeout (ctx));
		return false;
	}

	/* creates a new connection against localhost:44000 */
	/* doing connect operation with timeout activated.. */
	printf (".");
	fflush (stdout);
	stamp      = time (NULL);
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, false)) {
		printf ("Test 12 (3): failed to connect to: %s:%s...reason: %s\n",
			listener_host, LISTENER_PORT, vortex_connection_get_message (connection));
		vortex_connection_close (connection);
		return false;
	}

	/* check stamp before continue */
	if ((time (NULL) - stamp) > 1) {
		printf ("Test 12 (3.1): failed, expected less connection time while testing..\n");
		return false;
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
		return false;
	}

	/* creates a new connection against localhost:44000 */
	connection = connection_new ();
	if (!vortex_connection_is_ok (connection, false)) {
		printf ("Test 12 (5): failed to connect to: %s:%s...reason: %s\n",
			listener_host, LISTENER_PORT, vortex_connection_get_message (connection));
		vortex_connection_close (connection);
		return false;
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
		return false;
	}
	printf (".");
	fflush (stdout);

	/* try to connect to an unreachable host */
	connection = vortex_connection_new (ctx, listener_host, "44012", NULL, NULL);
	stamp      = time (NULL);
	if (vortex_connection_is_ok (connection, false)) {
		printf ("Test 12 (7): failed, expected to NOT to connect to: %s:%s...reason: %s\n",
			listener_host, LISTENER_PORT, vortex_connection_get_message (connection));
		vortex_connection_close (connection);
		return false;
	}
	printf (".");
	fflush (stdout);
	
	/* check stamp (check unreachable host doesn't take any
	 * special time) */
	if (stamp != time (NULL) && (stamp + 1) != time (NULL)) {
		printf ("Test 12 (7.1): failed, expected no especial timeout for an unreachable connect operation..\n");
		vortex_connection_close (connection);
		return false;
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
	if (! vortex_connection_is_ok (control, false)) {
		printf ("Test 12 (7.2): failed to create control connection, unable to start remote fake listener..\n");
		vortex_connection_close (connection);
		return false;
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
		return false;
	} /* end if */

	/* CREATE FAKE LISTENER: call to create a remote listener
	 * (blocked) running on the 44012 port */
	if (! vortex_channel_send_msg (c_channel, "create-listener", 15, NULL)) {
		printf ("Test 12 (7.4): failed to send message to create remote fake listener...\n");
		return false;
	}

	/* get reply */
	frame = vortex_channel_get_reply (c_channel, queue);
	if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY) {
		printf ("Test 12 (7.5): failed to start remote fake listener, received negative reply..\n");
		return false;
	} /* end if */
	vortex_frame_unref (frame);
	printf ("remote fake listener created");
	
	/* CONNECT TO UNRESPONSIVE LISTENER: now try to connect
	 * again */
	printf ("...connect (wait 5 seconds)");
	fflush (stdout);
	stamp      = time (NULL);
	connection = vortex_connection_new (ctx, listener_host, "44012", NULL, NULL);
	if (vortex_connection_is_ok (connection, false)) {
		printf ("\nTest 12 (9): failed to connect to: %s:%s...reason: %s\n",
			listener_host, "44012", vortex_connection_get_message (connection));
		vortex_connection_close (connection);
		return false;
	}

	/* ok, close the connection */
	printf ("...check (step 9 ok)..");
	fflush (stdout);
	vortex_connection_close (connection);
	if ((stamp + 3) > time (NULL)) {
		printf ("Test 12 (9.1): supposed to perform a connection failed, with a timeout about of 3 seconds but only consumed: %ld seconds..\n",
			(time (NULL)) - stamp);
		return false;
	}

	/* now unlock the listener */
	printf ("..unlock listener..");
	fflush (stdout);
	/* CALL TO UNLOCK THE REMOTE LISTENER: call to create a remote
	 * listener (blocked) running on the 44012 port */
	if (! vortex_channel_send_msg (c_channel, "unlock-listener", 15, NULL)) {
		printf ("Test 12 (7.4): failed to send message to create remote fake listener...\n");
		return false;
	}

	/* get reply */
	frame = vortex_channel_get_reply (c_channel, queue);
	if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY) {
		printf ("Test 12 (7.5): failed to start remote fake listener, received negative reply..\n");
		return false;
	} /* end if */
	vortex_frame_unref (frame);
	printf ("ok..");
	
	/* CONNECT TO RESPONSE SERVER: now try to connect again */
	printf ("..now connecting");
	fflush (stdout);
	connection = vortex_connection_new (ctx, listener_host, "44012", NULL, NULL);
	if (! vortex_connection_is_ok (connection, false)) {
		printf ("Test 12 (10): failed to connect to: %s:%s...reason: %s\n",
			listener_host, LISTENER_PORT, vortex_connection_get_message (connection));
		vortex_connection_close (connection);
		return false;
	} 

	/* create a channel */
	printf ("..creating a channel..");

	/* register a profile */
	vortex_profiles_register (ctx, 
				  REGRESSION_URI,
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
		return false;
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
		return false;
	}

	/* get reply */
	frame = vortex_channel_get_reply (c_channel, queue);
	if (vortex_frame_get_type (frame) != VORTEX_FRAME_TYPE_RPY) {
		printf ("Test 12 (7.5): failed to start remote fake listener, received negative reply..\n");
		return false;
	} /* end if */
	vortex_frame_unref (frame);
	printf ("remote fake listener created");
	
	/* close control connection */
	vortex_channel_close (c_channel, NULL);
	vortex_connection_close (control);

	/* free queue */
	vortex_async_queue_unref (queue);
	
	/* return true */
	printf ("\n");
	return true;
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
 * @return true if goes ok.
 */
bool test_09 () 
{
	VortexConnection * conn;
	VortexChannel    * channel;

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
		
/*	vortex_color_log_enable (true);
	vortex_log_enable (true);
	vortex_log2_enable (true); */
		
		/* send a message and close */
		vortex_channel_send_msg (channel, "", 0, NULL);
		
		/* close the channel */
		if (! vortex_channel_close (channel, NULL)) {
			printf ("Failed to close the channel..\n");
			return false;
		}

		/* unref the channel */
		vortex_channel_unref (channel);

		/* check channels */
		if (vortex_connection_channels_count (conn) != 1) {
			printf ("failed, expected to find one 1 channel running in the connection..\n");
			return false;
		}

		iterator++;
	} /* end if */

	printf ("\n");

	/* close the connection */
	vortex_connection_close (conn);

	return true;
	
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
 * @return true if all test pass, otherwise false is returned.
 */
bool test_13 ()
{
	printf ("Test 13: ** \n");
	printf ("Test 13: ** INFO: Running test, under the TUNNEL profile (BEEP proxy support)!\n");
	printf ("Test 13: **\n");

	/* create tunnel settings */
	tunnel_settings = vortex_tunnel_settings_new (ctx);
	tunnel_tested   = true;
	
	/* add first hop */
	vortex_tunnel_settings_add_hop (tunnel_settings,
					TUNNEL_IP4, listener_proxy_host,
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
		return false;
	}

	printf ("Test 13::");
	if (test_01a ())
		printf ("Test 01-a: transfer zeroed binary frames [   OK   ]\n");
	else {
		printf ("Test 01-a: transfer zeroed binary frames [ FAILED ]\n");
		return false;
	}

	printf ("Test 13::");
	if (test_02 ())
		printf ("Test 02: basic BEEP channel support [   OK   ]\n");
	else {
		printf ("Test 02: basic BEEP channel support [ FAILED ]\n");
		return false;
	}

	printf ("Test 13::");
	if (test_02a ()) 
		printf ("Test 02-a: connection close notification [   OK   ]\n");
	else {
		printf ("Test 02-a: connection close notification [ FAILED ]\n");
		return false;
	}

	printf ("Test 13::");
	if (test_02b ()) 
		printf ("Test 02-b: small message followed by close  [   OK   ]\n");
	else {
		printf ("Test 02-b: small message followed by close [ FAILED ]\n");
		return false;
	}

	printf ("Test 13::");
	if (test_02c ()) 
		printf ("Test 02-c: huge amount of small message followed by close  [   OK   ]\n");
	else {
		printf ("Test 02-c: huge amount of small message followed by close [ FAILED ]\n");
		return false;
	}

	printf ("Test 13::");
	if (test_03 ())
		printf ("Test 03: basic BEEP channel support (large messages) [   OK   ]\n");
	else {
		printf ("Test 03: basic BEEP channel support (large messages) [ FAILED ]\n");
		return false;
	}

	printf ("Test 13::");
	if (test_04_a ()) {
		printf ("Test 04-a: Check ANS/NUL support, sending large content [   OK   ]\n");
	} else {
		printf ("Test 04-a: Check ANS/NUL support, sending large content [ FAILED ]\n");
		return false;
	}

	printf ("Test 13::");
	if (test_04_ab ()) {
		printf ("Test 04-ab: Check ANS/NUL support, sending different files [   OK   ]\n");
	} else {
		printf ("Test 04-ab: Check ANS/NUL support, sending different files [ FAILED ]\n");
		return false;
	}

	/* free tunnel settings */
	vortex_tunnel_settings_free (tunnel_settings);
	tunnel_settings = NULL;
	tunnel_tested   = false;

	return true;
}

int main (int  argc, char ** argv)
{

#if defined (AXL_OS_UNIX) && defined (VORTEX_HAVE_POLL)
	/* if poll(2) mechanism is available, check it */
	bool poll_tested = true;
#endif
#if defined (AXL_OS_UNIX) && defined (VORTEX_HAVE_EPOLL)
	/* if epoll(2) mechanism is available, check it */
	bool epoll_tested = true;
#endif

	printf ("** Vortex Library: A BEEP core implementation.\n");
	printf ("** Copyright (C) 2008 Advanced Software Production Line, S.L.\n**\n");
	printf ("** Vortex Regression tests: version=%s\n**\n",
		VERSION);
	printf ("** To properly run this test it is required to run vortex-regression-listener.\n");
	printf ("** To gather information about time performance you can use:\n**\n");
	printf ("**     >> time ./vortex-regression-client\n**\n");
	printf ("** To gather information about memory consumed (and leaks) use:\n**\n");
	printf ("**     >> libtool --mode=execute valgrind --leak-check=yes --error-limit=no ./vortex-regression-client\n**\n");
	printf ("** Additional settings:\n");
	printf ("**\n");
	printf ("**     >> ./vortex-regression-client [listener-host [listener-host-proxy]]\n");
	printf ("**\n");
	printf ("**       If no listener-host value is provided, it is used \"localhost\". \n");
	printf ("**       If no listener-host-proxy value is provided, it is used the value \n");
	printf ("**       provided for listener-host. \n");
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

	/* configure default values */
	if (argc == 1) {
		listener_host       = "localhost";
		listener_proxy_host = "localhost";
	} else if (argc == 2) {
		listener_host       = argv[1];
		listener_proxy_host = argv[1];
	} else if (argc == 3) {
		listener_host       = argv[1];
		listener_proxy_host = argv[2];
	}

	if (listener_host == NULL) {
		printf ("Error: undefined value found for listener host..\n");
		return -1;
	}

	if (listener_proxy_host == NULL) {
		printf ("Error: undefined value found for listener proxy host..\n");
		return -1;
	}
		
	printf ("INFO: running test against %s, with BEEP proxy located at: %s..\n",
		listener_host, listener_proxy_host);

	/* init vortex library */
	if (! vortex_init_ctx (ctx)) {
		/* unable to init context */
		vortex_ctx_free (ctx);
		return -1;
	} /* end if */

	/* change to select if it is not the default */
	vortex_io_waiting_use (ctx, VORTEX_IO_WAIT_SELECT);

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

	if (test_00 ()) 
		printf ("Test 00: Async Queue support [   OK   ]\n");
	else {
		printf ("Test 00: Async Queue support [ FAILED ]\n");
		return -1;
	}

	if (test_01 ())
		printf ("Test 01: basic BEEP support [   OK   ]\n");
	else {
		printf ("Test 01: basic BEEP support [ FAILED ]\n");
		return -1;
	}

	if (test_01a ())
		printf ("Test 01-a: transfer zeroed binary frames [   OK   ]\n");
	else {
		printf ("Test 01-a: transfer zeroed binary frames [ FAILED ]\n");
		return -1;
	}

	if (test_01b ())
		printf ("Test 01-b: channel close inside created notification (31/03/2008) [   OK   ]\n");
	else {
		printf ("Test 01-b: channel close inside created notification (31/03/2008) [ FAILED ]\n");
		return -1;
	}

	if (test_01c ())
		printf ("Test 01-c: check immediately send (31/03/2008) [   OK   ]\n");
	else {
		printf ("Test 01-c: check immediately send (31/03/2008) [ FAILED ]\n");
		return -1;
	}

	if (test_02 ())
		printf ("Test 02: basic BEEP channel support [   OK   ]\n");
	else {
		printf ("Test 02: basic BEEP channel support [ FAILED ]\n");
		return -1;
	}

	if (test_02a ()) 
		printf ("Test 02-a: connection close notification [   OK   ]\n");
	else {
		printf ("Test 02-a: connection close notification [ FAILED ]\n");
		return -1;
	}

	if (test_02b ()) 
		printf ("Test 02-b: small message followed by close  [   OK   ]\n");
	else {
		printf ("Test 02-b: small message followed by close [ FAILED ]\n");
		return -1;
	}

	if (test_02c ()) 
		printf ("Test 02-c: huge amount of small message followed by close  [   OK   ]\n");
	else {
		printf ("Test 02-c: huge amount of small message followed by close [ FAILED ]\n");
		return -1;
	}

	if (test_03 ())
		printf ("Test 03: basic BEEP channel support (large messages) [   OK   ]\n");
	else {
		printf ("Test 03: basic BEEP channel support (large messages) [ FAILED ]\n");
		return -1;
	}

	if (test_03a ()) {
		printf ("Test 03-a: vortex channel pool support [   OK   ]\n");
	} else {
		printf ("Test 03-a: vortex channel pool support [ FAILED ]\n");
		return -1;
	}

	if (test_04 ()) {
		printf ("Test 04: Handling many connections support [   OK   ]\n");
	} else {
		printf ("Test 04: Handling many connections support [ FAILED ]\n");
		return -1;
	}

	if (test_04_a ()) {
		printf ("Test 04-a: Check ANS/NUL support, sending large content [   OK   ]\n");
	} else {
		printf ("Test 04-a: Check ANS/NUL support, sending large content [ FAILED ]\n");
		return -1;
	}

	if (test_04_ab ()) {
		printf ("Test 04-ab: Check ANS/NUL support, sending different files [   OK   ]\n");
	} else {
		printf ("Test 04-ab: Check ANS/NUL support, sending different files [ FAILED ]\n");
		return -1;
	}

	if (test_04_c ()) {
		printf ("Test 04-c: check client adviced profiles [   OK   ]\n");
	} else {
		printf ("Test 04-c: check client adviced profiles [ FAILED ]\n");
		return -1;
	}

	if (test_05 ()) {
		printf ("Test 05: TLS profile support [   OK   ]\n");
	} else {
		printf ("Test 05: TLS profile support [ FAILED ]\n");
		return -1;
	}
	
	if (test_05_a ()) {
		printf ("Test 05-a: Check auto-tls on fail fix (24/03/2008) [   OK   ]\n");
	} else {
		printf ("Test 05-a: Check auto-tls on fail fix (24/03/2008) [ FAILED ]\n");
		return -1;
	}

	if (test_06 ()) {
		printf ("Test 06: SASL profile support [   OK   ]\n");
	} else {
		printf ("Test 06: SASL profile support [ FAILED ]\n");
		return -1;
	}

	if (test_07 ()) {
		printf ("Test 07: XML-RPC profile support [   OK   ]\n");
	} else {
		printf ("Test 07: XML-RPC profile support [ FAILED ]\n");
		return -1;
	}

	if (test_08 ()) {
		printf ("Test 08: serverName configuration [   OK   ]\n");
	} else {
		printf ("Test 08: serverName [ FAILED ]\n");
		return -1;
	}

	if (test_09 ()) {
		printf ("Test 09: close in transit support [   OK   ]\n");
	} else {
		printf ("Test 09: close in transit support [ FAILED ]\n");
		return -1;
	}

	if (test_10 ()) {
		printf ("Test 10: default channel close action [   OK   ]\n");
	} else {
		printf ("Test 10: default channel close action [ FAILED ]\n");
		return -1;
	}

	if (test_11 ()) {
		printf ("Test 11: reply to multiple messages in a wrong order without blocking [   OK   ]\n");
	} else {
		printf ("Test 11: reply to multiple messages in a wrong order without blocking  [ FAILED ]\n");
		return -1;
	}

	if (test_12 ()) {
		printf ("Test 12: check connection creation timeout [   OK   ]\n");
	} else {
		printf ("Test 12: check connection creation timeout [ FAILED ]\n");
		return -1;
	}

	if (test_13 ()) {
		printf ("Test 13: test TUNNEL implementation [   OK   ]\n");
	} else {
		printf ("Test 13: test TUNNEL implementation [ FAILED ]\n");
		return -1;
	}

#if defined(AXL_OS_UNIX) && defined (VORTEX_HAVE_POLL)
	/**
	 * If poll(2) I/O mechanism is available, re-run tests with
	 * the method installed.
	 */
	if (! poll_tested) {
		/* configure poll mode */
		if (! vortex_io_waiting_use (ctx, VORTEX_IO_WAIT_POLL)) {
			printf ("error: unable to configure poll I/O mechanishm");
			return false;
		} /* end if */

		/* check the same run test with poll interface activated */
		poll_tested = true;
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
			return false;
		} /* end if */

		/* check the same run test with epoll interface activated */
		epoll_tested = true;
		goto init_test;
	} /* end if */
#endif

	printf ("**\n");
	printf ("** INFO: All test ok!\n");
	printf ("**\n");

	/* exit from vortex library */
	vortex_exit_ctx (ctx, true);
	return 0 ;	      
}


