/*  LibVortex:  A BEEP implementation for af-arch
 *  Copyright (C) 2010 Advanced Software Production Line, S.L.
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 */

/* include base library */
#include <vortex.h>

/* include sasl component */
#include <vortex_sasl.h>

/* include tls component */
#include <vortex_tls.h>

#define COYOTE_PROFILE "http://fact.aspl.es/profiles/coyote_profile"

/* listener context */
VortexCtx * ctx = NULL;

void on_ready (char  * host, int  port, VortexStatus status, char  * message, axlPointer user_data)
{
	
	if (status == VortexError) {
		printf ("error at: %s\n", message);

		/* exit from vortex */
		vortex_listener_unlock (ctx);

	}else {
		printf ("ready on: %s:%d, message: %s\n", host, port, message);
	}

	return;
}

void frame_received (VortexChannel    * channel,
		     VortexConnection * connection,
		     VortexFrame      * frame,
		     axlPointer           user_data)
{
	printf ("VORTEX_LISTENER: STARTED (pid: %d)\n", getpid ());
	printf ("A frame received on channl: %d\n",     vortex_channel_get_number (channel));
	printf ("Data received: '%s'\n",                (char*) vortex_frame_get_payload (frame));

	/* reply */
	vortex_channel_send_rpy (channel,
				 "I have received you message, thanks..",
				 37,
				 vortex_frame_get_msgno (frame));

#if defined(ENABLE_SASL_SUPPORT)
	/* drop a log about the sasl properties */
	if (vortex_sasl_is_authenticated (connection)) {
		/* check the connection to be authenticated because
		 * assuming anything */
		printf ("The connection is authenticated, using the method: %s\n", 
			 vortex_sasl_auth_method_used (connection));

		printf ("Auth data provided: \n  authid=%s\n  authorization id=%s\n  password=%s\n  realm=%s\n  anonymous token=%s\n",
			 vortex_sasl_get_propertie (connection, VORTEX_SASL_AUTH_ID),
			 vortex_sasl_get_propertie (connection, VORTEX_SASL_AUTHORIZATION_ID),
			 vortex_sasl_get_propertie (connection, VORTEX_SASL_PASSWORD),
			 vortex_sasl_get_propertie (connection, VORTEX_SASL_REALM),
			 vortex_sasl_get_propertie (connection, VORTEX_SASL_ANONYMOUS_TOKEN));
	}else {
		printf ("Connection not authenticated..\n");
	}
#else
	printf ("ERROR: failed to check SASL auth data. Current build does not have SASL support\n");
#endif

	printf ("VORTEX_LISTENER: CLOSE CHANNEL (pid: %d)\n", getpid ());

	/* close the channel */
	vortex_channel_close (channel, NULL);
	
	printf ("VORTEX_LISTENER: FINSHED (pid: %d)\n",       getpid ());
	return;
}

/** 
 * @brief Validates external mechanism.
 */
int      sasl_external_validation (VortexConnection * connection,
				   const char       * auth_id)
{
	if (axl_cmp ("acinom", auth_id)) {
		printf ("Received external validation for: %s, replying OK\n", auth_id);
		return axl_true;
	}
	
	printf ("Received external validation for: %s, replying FAILED\n", auth_id);
	return axl_false;
}

int      sasl_external_validation_full (VortexConnection * connection,
					const char       * auth_id,
					axlPointer           user_data)
{
	
	if (axl_cmp ("external!", (char*)user_data )) {
		return sasl_external_validation (connection, auth_id);
	}

	printf ("Received external validation (full) for: %s, replying FAILED.\nUser pointer not properly passed.",
		 auth_id);
	return axl_false;
}

/** 
 * @brief Validate anonymous SASL request
 * 
 * @param connection 
 * @param anonymous_token 
 * 
 * @return 
 */
int      sasl_anonymous_validation (VortexConnection * connection,
				    const char       * anonymous_token)
{
	if (axl_cmp ("test@aspl.es", anonymous_token)) {
		printf ("Received anonymous validation for: test@aspl.es, replying OK\n");
		return axl_true;
	}
	
	printf ("Received anonymous validation for: %s, replying FAILED\n", anonymous_token);
	return axl_false;
}

int      sasl_anonymous_validation_full (VortexConnection * connection,
				    const char       * anonymous_token,
					axlPointer user_data)
{
	if (axl_cmp ("anonymous!", (char*)user_data )) {
		printf ("Received anonymous validation (full) for: test@aspl.es, replying OK\n");
		return sasl_anonymous_validation (connection, anonymous_token);
	}
	
	printf ("Received anonymous validation (full) for: %s, replying FAILED.\nUser pointer not properly passed.",
		anonymous_token);
	return axl_false;
}


/** 
 * @brief Validates incoming SASL PLAIN requests
 * 
 * @param connection    The connection where the SASL plain negotiation is received.
 * @param auth_id       The user id to authenticate
 * @param auth_proxy_id The proxy user id 
 * @param password      The user id password
 * 
 * @return axl_true accept and validate, axl_false to deny request.
 */
int      sasl_plain_validation  (VortexConnection * connection,
				 const char       * auth_id,
				 const char       * auth_proxy_id,
				 const char       * password)
{
	printf ("received incoming SASL PLAIN request: user id='%s' auth proxy id='%s' password='%s'\n",
		 auth_id, (auth_proxy_id != NULL) ? auth_proxy_id : "", password);
	
	/* perform validation */
	if (axl_cmp (auth_id, "bob") && 
	    axl_cmp (password, "secret")) {
		return axl_true;
	}
	/* deny SASL request to authenticate remote peer */
	return axl_false;
}

int      sasl_plain_validation_full  (VortexConnection * connection,
				      const char       * auth_id,
				      const char       * auth_proxy_id,
				      const char       * password,
				      axlPointer user_data)
{
	printf ("received incoming SASL PLAIN request (full)\n");
	if (axl_cmp ("plain!", (char*)user_data )) {
		printf ("Received PLAIN validation (full) for: test@aspl.es, replying OK \n");
		return sasl_plain_validation (connection, auth_id, auth_proxy_id, password);
	}

	printf ("Received PLAIN validation (full) for: %s, replying FAILED.\nUser pointer not properly passed.", auth_id);
	return axl_false;
}

char  * sasl_cram_md5_validation (VortexConnection * connection,
				  const char  * auth_id)
{
	printf ("Received cram md5 validation request for: %s", auth_id);
	if (axl_cmp (auth_id, "bob"))
		return axl_strdup ("secret");
	return NULL;
}

char  * sasl_cram_md5_validation_full (VortexConnection * connection,
				  const char  * auth_id,
				  axlPointer user_data)
{
	printf ("Received cram md5 validation request (full) for: %s\n", auth_id);
	if (axl_cmp ("cram md5!", (char*)user_data )) {
		printf ("Received cram md5 validation (full) for: test@aspl.es, replying OK\n");
		return sasl_cram_md5_validation (connection, auth_id);
	}

	printf ("Received cram md5 validation (full) for: %s, replying FAILED.\nUser pointer not properly passed.", auth_id);
	return axl_false;
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
	printf ("Received digest md5 validation request (full) for: %s\n", auth_id);
	if (axl_cmp ("digest md5!", (char*)user_data )) {
		printf ("Received digest md5 validation (full) for: test@aspl.es, replying OK\n");
		return sasl_digest_md5_validation (connection, auth_id, authorization_id, realm);
	}

	printf ("Received digest md5 validation (full) for: %s, replying FAILED.\nUser pointer not properly passed.", auth_id);
	return axl_false;
}


int  main (int  argc, char  ** argv) 
{

	/* create the context */
	ctx = vortex_ctx_new ();

	/* init vortex library */
	if (! vortex_init_ctx (ctx)) {
		/* unable to init context */
		vortex_ctx_free (ctx);
		return -1;
	} /* end if */

#if defined(ENABLE_SASL_SUPPORT)
	/* check and initiliaze SASL */
	if (! vortex_sasl_init (ctx)) {
		printf ("Current Vortex Library is not prepared for SASL profile");
		return -1;
	}
#else
	printf ("Current build does not have SASL support..\n");
	return -1;
#endif
	
	/* Checking for parameter "full" */
	if (argc == 2 && axl_cmp("full", argv[1])) {
	
		printf ("Using user-defined pointer passing server...\n");
		
		/* set default anonymous validation handler */
		vortex_sasl_set_anonymous_validation_full (ctx, sasl_anonymous_validation_full);

		/* accept SASL ANONYMOUS incoming requests */
		if (! vortex_sasl_accept_negotiation_full (ctx, VORTEX_SASL_ANONYMOUS, "anonymous!")) {
			printf ("Unable to make Vortex Libray to accept SASL ANONYMOUS profile");
			return -1;
		}

		/* set default external validation handler */
		vortex_sasl_set_external_validation_full (ctx, sasl_external_validation_full);

		/* accept SASL EXTERNAL incoming requests */
		if (! vortex_sasl_accept_negotiation_full (ctx, VORTEX_SASL_EXTERNAL, "external!")) {
			printf ("Unable to make Vortex Libray to accept SASL EXTERNAL profile");
			return -1;
		}

		/* set default plain validation handler */
		vortex_sasl_set_plain_validation_full (ctx, sasl_plain_validation_full);

		/* accept SASL PLAIN incoming requests */
		if (! vortex_sasl_accept_negotiation_full (ctx, VORTEX_SASL_PLAIN, "plain!")) {
			printf ("Unable to make Vortex Libray to accept SASL PLAIN profile");
			return -1;
		}

		/* set default CRAM-MD5 validation handler */
		vortex_sasl_set_cram_md5_validation_full (ctx, sasl_cram_md5_validation_full);

		/* accept SASL CRAM-MD5 incoming requests */
		if (! vortex_sasl_accept_negotiation_full (ctx, VORTEX_SASL_CRAM_MD5, "cram md5!")) {
			printf ("Unable to make Vortex Library to accept SASL CRAM-MD5 profile");
			return -1;
		}

		/* set default DIGEST-MD5 validation handler */
		vortex_sasl_set_digest_md5_validation_full (ctx, sasl_digest_md5_validation_full);

		/* accept SASL DIGEST-MD5 incoming requests */
		if (! vortex_sasl_accept_negotiation_full (ctx, VORTEX_SASL_DIGEST_MD5, "digest md5!")) {
			printf ("Unable to make Vortex Library to accept SASL DIGEST-MD5 profile");
			return -1;
		}
	}
	else
	{
	
		/* set default anonymous validation handler */
		vortex_sasl_set_anonymous_validation (ctx, sasl_anonymous_validation);

		/* accept SASL ANONYMOUS incoming requests */
		if (! vortex_sasl_accept_negotiation (ctx, VORTEX_SASL_ANONYMOUS)) {
			printf ("Unable to make Vortex Libray to accept SASL ANONYMOUS profile");
			return -1;
		}

		/* set default anonymous validation handler */
		vortex_sasl_set_external_validation (ctx, sasl_external_validation);

		/* accept SASL ANONYMOUS incoming requests */
		if (! vortex_sasl_accept_negotiation (ctx, VORTEX_SASL_EXTERNAL)) {
			printf ("Unable to make Vortex Libray to accept SASL EXTERNAL profile");
			return -1;
		}

		/* set default plain validation handler */
		vortex_sasl_set_plain_validation (ctx, sasl_plain_validation);

		/* accept SASL PLAIN incoming requests */
		if (! vortex_sasl_accept_negotiation (ctx, VORTEX_SASL_PLAIN)) {
			printf ("Unable to make Vortex Libray to accept SASL PLAIN profile");
			return -1;
		}

		/* set default CRAM-MD5 validation handler */
		vortex_sasl_set_cram_md5_validation (ctx, sasl_cram_md5_validation);

		/* accept SASL CRAM-MD5 incoming requests */
		if (! vortex_sasl_accept_negotiation (ctx, VORTEX_SASL_CRAM_MD5)) {
			printf ("Unable to make Vortex Library to accept SASL CRAM-MD5 profile");
			return -1;
		}

		/* set default DIGEST-MD5 validation handler */
		vortex_sasl_set_digest_md5_validation (ctx, sasl_digest_md5_validation);

		/* accept SASL DIGEST-MD5 incoming requests */
		if (! vortex_sasl_accept_negotiation (ctx, VORTEX_SASL_DIGEST_MD5)) {
			printf ("Unable to make Vortex Library to accept SASL DIGEST-MD5 profile");
			return -1;
		}
	}


#if defined(ENABLE_TLS_SUPPORT)
	/* check for TLS initialization */
	if (! vortex_tls_init (ctx)) {
		printf ("Current Vortex Library is not prepared for TLS profile");
		return -1;
	}
#else
	printf ("Current build does not have TLS support.\n");
	return -1;
#endif

	/* enable accepting incoming tls connections, this step could
	 * also be read as register the TLS profile */
	if (! vortex_tls_accept_negotiation (ctx, NULL, NULL, NULL)) {
		printf ("Unable to start accepting TLS profile requests");
	}

	/* register a profile */
	vortex_profiles_register (ctx, COYOTE_PROFILE, 
				  NULL, NULL, NULL, NULL,
				  frame_received, NULL);

	/* create a vortex server */
	vortex_listener_new (ctx, "0.0.0.0", "44000", on_ready, NULL);

	/* wait for listeners (until vortex_exit is called) */
	vortex_listener_wait (ctx);

	/* do not call vortex_exit here if you define an on ready
	 * function which actually ends the execution */
	vortex_exit_ctx (ctx, axl_true);

	return 0;
}

