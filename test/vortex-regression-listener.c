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

/* include source code generated by xml-rpc-gen to test xml-rpc
 * profile */
#include <service_dispatch.h>

#ifdef AXL_OS_UNIX
#include <signal.h>
#endif

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
 * A profile to check wrong order reply.
 */
#define REGRESSION_URI_5 "http://iana.org/beep/transient/vortex-regression/5"

/** 
 * A profile to check wrong order reply.
 */
#define REGRESSION_URI_6 "http://iana.org/beep/transient/vortex-regression/6"

/**
 * A profile to check close in transit support.
 */ 
#define CLOSE_IN_TRANSIT_URI "http://iana.org/beep/transient/close-in-transit"

/** 
 * A profile to check sending zeroed binary frames.
 */
#define REGRESSION_URI_ZERO "http://iana.org/beep/transient/vortex-regression/zero"

void frame_received (VortexChannel    * channel,
		     VortexConnection * connection,
		     VortexFrame      * frame,
		     axlPointer           user_data)
{
	/* check some commands */
	if (axl_cmp (vortex_frame_get_payload (frame), 
		     "GET serverName")) {
		printf ("Received request to return serverName=%s..\n", vortex_connection_get_server_name (connection));
		
		/* reply the peer client with the same content */
		vortex_channel_send_rpy (channel,
					 vortex_connection_get_server_name (connection),
					 strlen (vortex_connection_get_server_name (connection)),
					 vortex_frame_get_msgno (frame));
		return;
	} /* end if */

	/* DEFAULT REPLY, JUST ECHO */
	/* reply the peer client with the same content */
	vortex_channel_send_rpy (channel,
				 vortex_frame_get_payload (frame),
				 vortex_frame_get_payload_size (frame),
				 vortex_frame_get_msgno (frame));
	return;
}

/** 
 * @internal Frame received handler used to check wrong reply order
 * support.
 */
void frame_received_replies (VortexChannel    * channel,
			     VortexConnection * connection,
			     VortexFrame      * frame,
			     axlPointer         user_data)
{
	
	/* ack message received */
	if (vortex_frame_get_type (frame) == VORTEX_FRAME_TYPE_MSG) {
		vortex_channel_send_rpy (channel, "", 0, vortex_frame_get_msgno (frame));
		vortex_channel_send_msg (channel, "MSG###1###", 10, NULL);
		vortex_channel_send_msg (channel, "MSG###2###", 10, NULL);
		vortex_channel_send_msg (channel, "MSG###3###", 10, NULL);
	} /* end if */
	return;
}

/* message size: 4096 */
#define TEST_REGRESION_URI_4_MESSAGE "This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary content. This is a large file that contains arbitrary ."

/** 
 * @internal Frame received handler used to check wrong reply order
 * support.
 */
void frame_received_ans_replies (VortexChannel    * channel,
				 VortexConnection * connection,
				 VortexFrame      * frame,
				 axlPointer         user_data)
{
	int iterator;

	/* ack message received */
	if (vortex_frame_get_type (frame) == VORTEX_FRAME_TYPE_MSG) {
		printf ("Sending content (ANS/NUL test: %s)\n", REGRESSION_URI_4);
		/* reply with a file (simulating it) */ 
		iterator = 0;
		
		/* send 8192 messages of 4096 octects = 32768K = 32M */
		while (iterator < 8192) {
			/* send the reply */
			if (!vortex_channel_send_ans_rpy (channel, 
							  /* the message reply */
							  TEST_REGRESION_URI_4_MESSAGE, 4096,
							  /* the MSG num we are replying to */
							  vortex_frame_get_msgno (frame))) {

				/* well, we have found an error while sending a message */
				printf ("Regression test for URI %s is failed, unable to send ans reply\n",
					REGRESSION_URI_4);
				return;
			} /* end if */
			
			/* update iterator */
			iterator++;
		} /* end while */

		/* finaly transmission */
		if (!vortex_channel_finalize_ans_rpy (channel, vortex_frame_get_msgno (frame))) {
			/* well, we have found an error while sending a message */
				printf ("Regression test for URI %s is failed, unable finalize ANS/NUL transmission\n",
					REGRESSION_URI_4);
				return;
		} /* end if */

		printf ("Finished sending content (ANS/NUL test: %s)\n", REGRESSION_URI_4);
	} /* end if */

	return;
}

/** 
 * @internal Frame received handler used to check wrong reply order
 * support.
 */
void frame_received_ans_transfer_selected_file (VortexChannel    * channel,
						VortexConnection * connection,
						VortexFrame      * frame,
						axlPointer         user_data)
{
	FILE * file;
	char * buffer[4096];
	int    bytes_read;
	int    total_bytes;

	/* ack message received */
	if (vortex_frame_get_type (frame) == VORTEX_FRAME_TYPE_MSG) {
		
		printf ("Sending content (ANS/NUL test: %s)\n", REGRESSION_URI_5);
		/* reply with a file (simulating it) */ 
		
		file = fopen ((char *) vortex_frame_get_payload (frame), "r");
		if (file == NULL) {
			printf ("FAILED to open file: %s..\n", (char*) vortex_frame_get_payload (frame));
			vortex_channel_send_err (channel, 
						 "Unable to open file requested",
						 29, 
						 vortex_frame_get_msgno (frame));
			return;
		} /* end if */
		printf ("Sending file: %s\n", (char*) vortex_frame_get_payload (frame));
		
		/* reply the peer client with the same content 10 times */
		total_bytes = 0;
		do {
			/* read content */
			bytes_read = fread (buffer, 1, 4096, file);
			
			/* update total count */
			total_bytes += bytes_read;
			
			/* break if eof is found */
			if (bytes_read == 0) 
				break;
			
			/* printf ("Sending the reply: bytes %d..\n", bytes_read); */
			if (! vortex_channel_send_ans_rpy (channel,
							   buffer, 
							   bytes_read, 
							   vortex_frame_get_msgno (frame))) {
				fprintf (stderr, "ERROR: There was an error while sending the reply message");
				break;
				
			} /* end if */
			
		} while (true);

		/* close file */
		fclose (file);
	
		/* finaly transmission */
		if (!vortex_channel_finalize_ans_rpy (channel, vortex_frame_get_msgno (frame))) {
			/* well, we have found an error while sending a message */
				printf ("Regression test for URI %s is failed, unable finalize ANS/NUL transmission for: %s\n",
					REGRESSION_URI_5, (char *) vortex_frame_get_payload (frame));
				return;
		} /* end if */

		printf ("Finished sending content (ANS/NUL test: %s)\n", REGRESSION_URI_4);
		
	} /* end if */

	return;
}

bool     start_channel (int                channel_num, 
			VortexConnection * connection, 
			axlPointer           user_data)
{
	/* implement profile requirement for allowing starting a new
	 * channel to return false denies channel creation to return
	 * true allows create the channel */
	return true;
}

bool     close_channel (int                channel_num, 
			VortexConnection * connection, 
			axlPointer           user_data)
{
	/* implement profile requirement for allowing to closeing a
	 * the channel to return false denies channel closing to
	 * return true allows to close the channel */
	return true;
}

bool     on_accepted (VortexConnection * connection, axlPointer data)
{
	printf ("New connection accepted from: %s:%s\n", 
		 vortex_connection_get_host (connection),
		 vortex_connection_get_port (connection));

	/* return true to accept the connection to be created */
	return true;
}

bool     sasl_anonymous_validation (VortexConnection * connection,
				    const char       * anonymous_token)
{
	if (axl_cmp ("test@aspl.es", anonymous_token)) {
		printf ("Received anonymous validation for: test@aspl.es, replying OK\n");
		return true;
	}
	return false;
}

bool     sasl_external_validation (VortexConnection * connection,
				   const char       * auth_id)
{
	if (axl_cmp ("acinom", auth_id)) {
		return true;
	}
	
	printf ("Received external validation for: %s, replying FAILED\n", auth_id);
	return false;
}

bool     sasl_external_validation_full (VortexConnection * connection,
					const char       * auth_id,
					axlPointer           user_data)
{
	
	if (axl_cmp ("external!", (char*)user_data )) {
		return sasl_external_validation (connection, auth_id);
	}

	printf ("Received external validation (full) for: %s, replying FAILED.\nUser pointer not properly passed.",
		 auth_id);
	return false;
}



/* sasl validation handlers */
bool     sasl_plain_validation  (VortexConnection * connection,
				 const char       * auth_id,
				 const char       * auth_proxy_id,
				 const char       * password)
{
	/* perform validation */
	if (axl_cmp (auth_id, "bob") && 
	    axl_cmp (password, "secret")) {
		return true;
	}
	/* deny SASL request to authenticate remote peer */
	return false;
}

bool     sasl_plain_validation_full  (VortexConnection * connection,
				 const char       * auth_id,
				 const char       * auth_proxy_id,
				 const char       * password,
				 axlPointer user_data)
{
	if (axl_cmp ("plain!", (char*)user_data )) {
		return sasl_plain_validation (connection, auth_id, auth_proxy_id, password);
	}
	printf ("Received PLAIN validation (full) for: %s, replying FAILED.\nUser pointer not properly passed.", auth_id);
	return false;
}

char  * sasl_cram_md5_validation (VortexConnection * connection,
				  const char  * auth_id)
{
	if (axl_cmp (auth_id, "bob"))
		return axl_strdup ("secret");
	return NULL;
}

char  * sasl_cram_md5_validation_full (VortexConnection * connection,
				  const char  * auth_id,
				  axlPointer user_data)
{
	if (axl_cmp ("cram md5!", (char*)user_data )) {
		return sasl_cram_md5_validation (connection, auth_id);
	}
	printf ("Received cram md5 validation (full) for: %s, replying FAILED.\nUser pointer not properly passed.", auth_id);
	return false;
}

char  * sasl_digest_md5_validation (VortexConnection * connection,
				    const char  * auth_id,
				    const char  * authorization_id,
				    const char  * realm)
{
	/* use the same code for the cram md5 validation */
	return sasl_cram_md5_validation (connection, auth_id);
}

char  * sasl_digest_md5_validation_full (VortexConnection * connection,
					 const char  * auth_id,
					 const char  * authorization_id,
					 const char  * realm,
					 axlPointer user_data)
{
	if (axl_cmp ("digest md5!", (char*)user_data )) {
		return sasl_digest_md5_validation (connection, auth_id, authorization_id, realm);
	}

	printf ("Received digest md5 validation (full) for: %s, replying FAILED.\nUser pointer not properly passed.", auth_id);
	return false;
}


#ifdef AXL_OS_UNIX
void __block_test (int value) 
{
	VortexAsyncQueue * queue;

	printf ("******\n");
	printf ("****** Received a signal (the regression test is failing): pid %d..locking..!!!\n", vortex_getpid ());
	printf ("******\n");

	/* block the caller */
	queue = vortex_async_queue_new ();
	vortex_async_queue_pop (queue);

	return;
}
#endif

VortexMutex doing_exit_mutex;
bool        __doing_exit = false;

/* listener context */
VortexCtx * ctx = NULL;

void __terminate_vortex_listener (int value)
{
	
	vortex_mutex_lock (&doing_exit_mutex);
	if (__doing_exit) {
		vortex_mutex_unlock (&doing_exit_mutex);

		return;
	}
	printf ("Terminating vortex regression listener..\n");
	__doing_exit = true;
	vortex_mutex_unlock (&doing_exit_mutex);

	/* unlocking listener */
	vortex_listener_unlock (ctx);

	return;
}

/** 
 * @brief Close in transit frame received handler. 
 */
void close_in_transit_received (VortexChannel    * channel,
				VortexConnection * conn,
				VortexFrame      * frame,
				axlPointer         user_data)
{
	/* reply to the frame received and close the channel */
	vortex_channel_send_rpy (channel,
				 "", 0, vortex_frame_get_msgno (frame));
	
	/* now close the channel */
	if (! vortex_channel_close (channel, NULL)) {
		fprintf (stderr, "FAILED TO CLOSE CHANNEL (close in transit profile)");
		return;
	}
	return;
}


int  main (int  argc, char ** argv) 
{

	/* install default handling to get notification about
	 * segmentation faults */
#ifdef AXL_OS_UNIX
	signal (SIGSEGV, __block_test);
	signal (SIGABRT, __block_test);
	signal (SIGTERM,  __terminate_vortex_listener);
#endif

	vortex_mutex_create (&doing_exit_mutex);

	/* create the context */
	ctx = vortex_ctx_new ();

	/* init vortex library */
	if (! vortex_init_ctx (ctx)) {
		/* unable to init context */
		vortex_ctx_free (ctx);
		return -1;
	} /* end if */

	if (argc > 1 && 0 == strcmp (argv[1], "-v")) {
		vortex_log_enable (ctx, true);
		vortex_log2_enable (ctx, true);
	}

	/* register a profile */
	vortex_profiles_register (ctx, REGRESSION_URI,
				  NULL, NULL, 
				  NULL, NULL,
				  frame_received, NULL);

	/* register a extended start */
	vortex_profiles_register_extended_start (ctx, REGRESSION_URI_2,
						 NULL, NULL);

	/* register more profiles */
	vortex_profiles_register (ctx, REGRESSION_URI_3,
				  NULL, NULL, 
				  NULL, NULL,
				  frame_received_replies, NULL);

	/* register the profile used to test ANS/NUL replies */
	vortex_profiles_register (ctx, REGRESSION_URI_4,
				  NULL, NULL, 
				  NULL, NULL,
				  frame_received_ans_replies, NULL);

	/* register the profile used to test ANS/NUL replies */
	vortex_profiles_register (ctx, REGRESSION_URI_5,
				  NULL, NULL, 
				  NULL, NULL,
				  frame_received_ans_transfer_selected_file, NULL);

	/* register a profile */
	vortex_profiles_register (ctx, REGRESSION_URI_ZERO,
				  NULL, NULL, 
				  NULL, NULL,
				  frame_received, NULL);

	/* enable accepting incoming tls connections, this step could
	 * also be read as register the TLS profile */
	if (! vortex_tls_accept_negociation (ctx, NULL, NULL, NULL)) {
		printf ("Unable to start accepting TLS profile requests");
		return -1;
	}

	if (vortex_sasl_is_enabled () ) {
		/* set default ANONYMOUS validation handler */
		vortex_sasl_set_anonymous_validation (ctx, sasl_anonymous_validation);

		/* accept SASL ANONYMOUS incoming requests */
		if (! vortex_sasl_accept_negociation (ctx, VORTEX_SASL_ANONYMOUS)) {
			printf ("Unable to make Vortex Libray to accept SASL ANONYMOUS profile");
			return -1;
		}

		/* set default EXTERNAL validation handler */
		vortex_sasl_set_external_validation (ctx, sasl_external_validation);
		
		/* accept SASL EXTERNAL incoming requests */
		if (! vortex_sasl_accept_negociation (ctx, VORTEX_SASL_EXTERNAL)) {
			printf ("Unable to make Vortex Libray to accept SASL EXTERNAL profile");
			return -1;
		}
		
		/* set default PLAIN validation handler */
		vortex_sasl_set_plain_validation (ctx, sasl_plain_validation);
		
		/* accept SASL PLAIN incoming requests */
		if (! vortex_sasl_accept_negociation (ctx, VORTEX_SASL_PLAIN)) {
			printf ("Unable to make Vortex Libray to accept SASL PLAIN profile");
			return -1;
		}
		
		/* set default CRAM-MD5 validation handler */
		vortex_sasl_set_cram_md5_validation (ctx, sasl_cram_md5_validation);
		
		/* accept SASL CRAM-MD5 incoming requests */
		if (! vortex_sasl_accept_negociation (ctx, VORTEX_SASL_CRAM_MD5)) {
			printf ("Unable to make Vortex Library to accept SASL CRAM-MD5 profile");
			return -1;
		}
		
		/* set default DIGEST-MD5 validation handler */
		vortex_sasl_set_digest_md5_validation (ctx, sasl_digest_md5_validation);
		
		/* accept SASL DIGEST-MD5 incoming requests */
		if (! vortex_sasl_accept_negociation (ctx, VORTEX_SASL_DIGEST_MD5)) {
			printf ("Unable to make Vortex Library to accept SASL DIGEST-MD5 profile");
			return -1;
		} /* end if */

		/* configure default realm for all connections for the DIGEST-MD5 */
		vortex_listener_set_default_realm (ctx, "aspl.es");  

	} else {
		printf("Skipping SASL setup, since Vortex is not configured with SASL support\n");
	}
	
	/* configure support for TUNNEL profile support */
	vortex_tunnel_accept_negotiation (ctx, NULL, NULL);
	
	/* enable XML-RPC profile */
        vortex_xml_rpc_accept_negociation (
		/* context */
		ctx, 
                /* no resource validation function */
                NULL,
                /* no user space data for the validation resource
		 * function. */
                NULL,
                service_dispatch,
                /* no user space data for the dispatch function. */
                NULL);

	/* configure close in transit profile */
	vortex_profiles_register (ctx, CLOSE_IN_TRANSIT_URI,
				  /* just accept all channels to be created */
				  NULL, NULL,
				  /* just accept all channels to be closed */
				  NULL, NULL,
				  close_in_transit_received, NULL);
	
	/* create a vortex server */
	vortex_listener_new (ctx, "0.0.0.0", "44010", NULL, NULL);

	/* create a vortex server to check the tunnel profile
	 * support */
	vortex_listener_new (ctx, "0.0.0.0", "44110", NULL, NULL);

	/* configure connection notification  */
	vortex_listener_set_on_connection_accepted (ctx, on_accepted, NULL);

	/* wait for listeners (until vortex_exit is called) */
	printf ("ready and waiting..\n");

	vortex_listener_wait (ctx);

	printf ("terminating the listener ..\n");

	/* terminate process */
	vortex_exit_ctx (ctx, true);

	return 0;
}

