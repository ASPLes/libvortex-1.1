/*
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2005 Advanced Software Production Line, S.L.
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
 *         C/ Dr. Michavila N� 14
 *         Coslada 28820 Madrid
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://fact.aspl.es
 */

#include <vortex.h>

/* local include */
#include <vortex_ctx_private.h>

#if defined(AXL_OS_UNIX)
#include <unistd.h> /* gethostname */
#endif

/**
 * \defgroup vortex_sasl Vortex SASL: SASL profile support and related functions
 */

/**
 * \addtogroup vortex_sasl
 * @{
 */

#ifdef ENABLE_SASL_SUPPORT
#include <gsasl.h>
#define SASL_DATA         "gsasl:data"
#endif

#define LOG_DOMAIN "vortex-sasl"

#define SASL_PROFILE_BEGIN_NEGOCIATED   "sasl:profile:being-negociated"



/** 
 * @brief Allows to check and initialize SASL support for Vortex Library.
 * 
 * Because Vortex Library SASL support building could be disabled, you
 * should do a call to \ref vortex_sasl_is_enabled to ensure you
 * application requiring SASL support will work.
 * 
 * You can call this function as many times as your application requires. Here
 * is an example:
 * \code
 * if (!vortex_sasl_is_enabled ()) {
 *     printf ("Unable to initialize SASL support\n");
 *     return -1;
 * }
 * printf ("SASL support activated!\n");
 * 
 * // do SASL stuff
 * \endcode
 * 
 * 
 * @return true if current Vortex Library is already compiled with
 * SASL profile support. Otherwise false is returned.
 */
bool               vortex_sasl_is_enabled                (void)
{
#ifndef ENABLE_SASL_SUPPORT
	/* return false if the library wasn't compiled with support
	 * for sasl */
	vortex_log (VORTEX_LEVEL_WARNING, "current Vortex Library doesn't have SASL support");
	return false;
#else
	/* just return true to start working with SASL */
	return true;
#endif	
}

/** 
 * @brief Allows to configure SASL properties used for the SASL authentication process.
 *
 * While using SASL profiles, several properties are required. These
 * properties are the user to authenticate, the password, etc.
 * 
 * This function allows to set properties using values that are
 * required to be unrefered or not. In the case a propertie provided
 * is required to be deallocated you should define the destroy handler
 * for: <b>value_destroy</b>. Once the session is closed, the value
 * destroy handler is run to deallocate provided value.
 * 
 * Properties that are available are: 
 *  - \ref VORTEX_SASL_AUTH_ID
 *  - \ref VORTEX_SASL_AUTHORIZATION_ID
 *  - \ref VORTEX_SASL_PASSWORD
 *  - \ref VORTEX_SASL_REALM
 *  - \ref VORTEX_SASL_ANONYMOUS_TOKEN
 *
 * Here is an example:
 * \code 
 * // set the user (without providing a value_destroy handler because
 * // bob is statically allocated)
 * vortex_sasl_set_propertie (connection,  VORTEX_SASL_AUTH_ID, 
 *                            "bob", NULL);
 *
 * // set the password (providing a value_destroy handler because the
 * // password was dynamically allocated)
 * vortex_sasl_set_propertie (connection, VORTEX_SASL_PASSWORD, 
 *                            password, axl_free); 
 * \endcode
 *
 * 
 * @param connection The connection where the properties will be set.
 *
 * @param prop       The properties to set.
 *
 * @param value      The value associated to the properties.
 *
 * @param value_destroy A optional destroy function allowing to pass
 * in properties values that are required to be unrefered when the
 * session is closed.
 */
bool               vortex_sasl_set_propertie             (VortexConnection     * connection,
							  VortexSaslProperties   prop,
							  char                 * value,
							  axlDestroyFunc         value_destroy)
{
#ifndef ENABLE_SASL_SUPPORT
	vortex_log (VORTEX_LEVEL_DEBUG, "calling to set_properties with a Vortex Library without SASL support");
	return false;
#else
	VortexCtx * ctx = vortex_connection_get_ctx (connection);
	
	/* check the connection status before setting the
	 * properties */
	if (connection == NULL) {
		return false;
	}

	/* check the properties being set */
	if ((prop <= 0) || (prop >= VORTEX_SASL_PROP_NUM)) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Setting SASL properties which is not a registered value.");
		return false;
	}
	
	/* check the value itself */
	if (value == NULL) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Setting SASL properties which its value is null.");
		return false;
	}

	/* store the property */
	switch (prop) {
	case VORTEX_SASL_AUTH_ID:
		vortex_connection_set_data_full (connection, SASL_AUTHID, 
						 value, NULL, value_destroy);
		break;
	case VORTEX_SASL_AUTHORIZATION_ID:
		vortex_connection_set_data_full (connection, SASL_AUTHZID, 
						 value, NULL, value_destroy);
		break;
	case VORTEX_SASL_PASSWORD:
		vortex_connection_set_data_full (connection, SASL_PASSWORD, 
						 value, NULL, value_destroy);
		break;
	case VORTEX_SASL_REALM:
		vortex_connection_set_data_full (connection, SASL_REALM, 
						 value, NULL, value_destroy);
		break;
	case VORTEX_SASL_ANONYMOUS_TOKEN:
		/* especial case, anonymous token will be also the
		 * authid, that's why we are setting the same
		 * value. This will cause successful anonymous SASL
		 * negotiation to return the anonymous token while
		 * calling to: vortex_sasl_get_propertie (con,
		 * VORTEX_SASL_AUTHID) */
		vortex_connection_set_data_full (connection, SASL_ANONYMOUS_TOKEN,
						 value, NULL, value_destroy);

		vortex_connection_set_data_full (connection, SASL_AUTHID, 
						 value, NULL, NULL);
	case VORTEX_SASL_PROP_NUM:
		/* nothing to do */
		break;
	}

	return true;
#endif	
}

/** 
 * @brief Allows to get current SASL properties from the given connection.
 *
 * Allows to get already set properties using \ref
 * vortex_sasl_set_propertie. See documentation for \ref
 * vortex_sasl_set_propertie to know more about properties available.
 * 
 * @param connection The connection where the propertie will be searched.
 * @param prop The propertie to search
 * 
 * @return The propertie value requested or NULL if it is not defined
 * or Vortex Library does not support SASL.
 */
char             * vortex_sasl_get_propertie             (VortexConnection     * connection,
							  VortexSaslProperties   prop)
{
#ifndef ENABLE_SASL_SUPPORT
	vortex_log (VORTEX_LEVEL_DEBUG, "calling to get_properties with a Vortex Library without SASL support");
	return NULL;
#else
	VortexCtx * ctx = vortex_connection_get_ctx (connection);

	/* check the connection status before setting the propertie */
	if (connection == NULL) {
		return NULL;
	}

	/* check the propertie being set */
	if ((prop <= 0) || (prop >= VORTEX_SASL_PROP_NUM)) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Getting SASL propertie which is not a registered value.");
		return false;
	}
	
	/* store the property */
	switch (prop) {
	case VORTEX_SASL_AUTH_ID:
		return vortex_connection_get_data (connection, SASL_AUTHID);
	case VORTEX_SASL_AUTHORIZATION_ID:
		return vortex_connection_get_data (connection, SASL_AUTHZID);
	case VORTEX_SASL_PASSWORD:
		return vortex_connection_get_data (connection, SASL_PASSWORD);
	case VORTEX_SASL_REALM:
		return vortex_connection_get_data (connection, SASL_REALM);
	case VORTEX_SASL_ANONYMOUS_TOKEN:
		return vortex_connection_get_data (connection, SASL_ANONYMOUS_TOKEN);
	case VORTEX_SASL_PROP_NUM:
		/* nothing to do */
		break;
	}
	/* no propertie found */
	vortex_log (VORTEX_LEVEL_CRITICAL, "unable to find propertie value, maybe buggie implementation");
	return NULL;
#endif
}

/** 
 * @brief Allows to check if the given connection have been
 * successfully authenticated.
 * 
 * Keep in mind this function will return false either because the
 * connection is not successfully authenticated or because current
 * Vortex Library doesn't have SASL support. 
 * 
 * Another issue is that this function will return true for SASL
 * operations requiring any SASL profile that have successfully
 * ended. This includes ANONYMOUS profile. 
 *
 * Obviously, ANONYMOUS profile <b>*DOES NOT*</b> not provide any
 * security measure. Inside environments that allows \ref VORTEX_SASL_ANONYMOUS "ANONYMOUS" and
 * other profiles such as \ref VORTEX_SASL_PLAIN "PLAIN", \ref VORTEX_SASL_CRAM_MD5 "CRAM-MD5" and \ref VORTEX_SASL_DIGEST_MD5 "DIGEST-MD5" could yield to
 * authorize users that only have provided an anonymous token. 
 * 
 * A good practice is not to mix on the same session ANONYMOUS profile
 * and authentication based profiles. Design you code keeping this
 * issue in mind.
 *
 * Once the connection is authenticated you can use the following
 * function to get the method that was used (selected by either the
 * user or the server side): 
 * 
 *  - \ref vortex_sasl_auth_method_used
 *
 * Additionally, you can use the \ref vortex_sasl_get_propertie
 * function to get auth properties used for the authentication. This
 * function and the previous one, \ref vortex_sasl_auth_method_used,
 * must be used only once called to \ref vortex_sasl_is_authenticated
 * and getting an afirmative result. Otherwise, data returned by \ref
 * vortex_sasl_auth_method_used or \ref vortex_sasl_get_propertie must
 * not be trusted.
 * 
 * @param connection The connection to check for authentication status.
 * 
 * @return true if the connection is authenticated, false if not. 
 */
bool               vortex_sasl_is_authenticated          (VortexConnection     * connection)
{
#ifndef ENABLE_SASL_SUPPORT
	vortex_log (VORTEX_LEVEL_CRITICAL, "Current Vortex Library doesn't support SASL profiles. Unable to return current connection authentication status.");
	return false;
#else
	if (connection == NULL) {
		return false;
	}

	/* search if the flag if the auth flag is found and a channel
	 * running the sasl method. This is because the connection can
	 * be closed and reconected using the same SASL data. */
	return (vortex_connection_get_data (connection, SASL_IS_AUTHENTICATED) != NULL) &&
		vortex_connection_get_channel_by_uri (connection, vortex_sasl_auth_method_used (connection)) != NULL;
		

#endif	
}

/** 
 * @brief Allows to get SASL method used to authenticate the connection. 
 *
 * This function could be used, once used \ref
 * vortex_sasl_is_authenticated, to get the method that was used to
 * authenticate the connection. In environments where several methods
 * are provided, this function is useful to get the security level the
 * user has negotiated.
 * 
 * @param connection The connection that has been authenticated.
 * 
 * @return The SASL method used, represented by an string or NULL if
 * the connection wasn't authenticated. String reference returned must
 * not be deallocated. Values returned are the one represented by \ref
 * VORTEX_SASL_ANONYMOUS, \ref VORTEX_SASL_EXTERNAL, \ref
 * VORTEX_SASL_PLAIN, \ref VORTEX_SASL_CRAM_MD5, \ref
 * VORTEX_SASL_DIGEST_MD5, \ref VORTEX_SASL_GSSAPI.
 */
char             * vortex_sasl_auth_method_used          (VortexConnection     * connection)
{
#ifndef ENABLE_SASL_SUPPORT
	vortex_log (VORTEX_LEVEL_CRITICAL, "Current Vortex Library doesn't have SASL profiles support. Unable to return which method was used to authenticate.");
	return NULL;
#else
	if (connection == NULL) {
		return NULL;
	}
	
	/* return auth method used */
	return (vortex_connection_get_data (connection, SASL_METHOD_USED));
#endif
}

/** 
 * @internal
 * @brief Support user space notification with SASL activation.
 * 
 * @param connection The connection where the SASL activation is being run.
 * @param status     A process status
 * @param message    The message to report. A textual diagnostic.
 * @param user_data  User space data.
 */
void __vortex_sasl_notify (VortexSaslAuthNotify   process_status, 
			   VortexConnection     * connection, 
			   VortexStatus           status,
			   char                 * message, 
			   axlPointer             user_data)
{
	/* get the context */
	VortexCtx * ctx = vortex_connection_get_ctx (connection);

	/* drop to the console a log */
	switch (status) {
	case VortexError:
		vortex_log (VORTEX_LEVEL_CRITICAL, (message != NULL) ? message : "no message to report");
		/* flag the connection to be *NOT* authenticated
		 * (remove current authid and authzid and all previous
		 * SASL auth info) */
		vortex_connection_set_data (connection, SASL_IS_AUTHENTICATED, NULL);
		vortex_connection_set_data (connection, SASL_AUTHID,   NULL);
		vortex_connection_set_data (connection, SASL_AUTHZID,  NULL);
		vortex_connection_set_data (connection, SASL_PASSWORD, NULL);
		vortex_connection_set_data (connection, SASL_REALM,    NULL);
		break;
	case VortexOk:
		vortex_log (VORTEX_LEVEL_DEBUG, (message != NULL) ? message : "no message to report");
		/* flag the connection to be authenticated */
		vortex_connection_set_data (connection, SASL_IS_AUTHENTICATED, INT_TO_PTR (true));
		break;
	}

	/* notify user space that something to be notify have
	 * happen */
	if (process_status != NULL)
		process_status (connection, status, message, user_data);

	/* that's all man */
	return;
}



#ifdef ENABLE_SASL_SUPPORT

typedef struct _GsaslData {
	/* context used for a particular connection */
	Gsasl            * ctx;
	
	/* session used for a particular connection */
	Gsasl_session    * session;
	
}GsaslData;

void __vortex_sasl_destroy_context (axlPointer _data)
{
	GsaslData        * data = _data;
	VortexConnection * connection;

	/* get the connection reference where the SASL request was received */
	connection      = (VortexConnection *) gsasl_callback_hook_get (data->ctx);

	if (data->session != NULL) {
		/* finish session */
		gsasl_finish (data->session);
	} /* end if */

	/* dealloc sasl context */
	gsasl_done (data->ctx);

	/* free gsasl holder */
	axl_free (data);

	return;
}

/** 
 * @internal
 *
 * @brief Allows to check and initialize only once the SASL library
 * for the client side.
 * 
 * @return true if the library have been initialized or it is already
 * initialized. Otherwise false is returned.
 */
bool     __vortex_sasl_create_context (VortexConnection * connection) 
{
	GsaslData * data;
	int         rc;
	VortexCtx * ctx = vortex_connection_get_ctx (connection);

	/* initialize the SASL client side */
	data = axl_new (GsaslData, 1);

	/* initialize sasl */
	rc = gsasl_init (&data->ctx);
	if (rc != GSASL_OK) {
		/* free data */
		axl_free (data);

		vortex_log (VORTEX_LEVEL_DEBUG, "SASL initialization failure (%d): %s\n",
			    rc, gsasl_strerror (rc));
		return false;
	}

	/* save SASL context for this connection */
	vortex_connection_set_data_full (connection, SASL_DATA, data, 
					 NULL, __vortex_sasl_destroy_context);

	/* save on the SASL ctx the connection */
	gsasl_callback_hook_set (data->ctx, connection);

	/* just return true  */
	return true;
}

/** 
 * @internal Ugly hack to avoid having gcc complaining because this
 * function wasn't found having -ansi activated.
 */
#ifndef AXL_OS_WIN32
int gethostname(char *name, size_t len);
#endif

/** 
 * @internal
 * @brief Configure current Vortex SASL properties into the GNU SASL session.
 * 
 * @param connection 
 */
void vortex_sasl_configure_current_properties (VortexConnection * connection)
{
	char        hostname[512];
	VortexCtx * ctx = vortex_connection_get_ctx (connection);
	GsaslData * data = vortex_connection_get_data (connection, SASL_DATA);
	
	gsasl_property_set (data->session, GSASL_AUTHID,   
			    vortex_sasl_get_propertie (connection, VORTEX_SASL_AUTH_ID));

	gsasl_property_set (data->session, GSASL_PASSWORD, 
			    vortex_sasl_get_propertie (connection, VORTEX_SASL_PASSWORD));

	gsasl_property_set (data->session, GSASL_AUTHZID, 
			    vortex_sasl_get_propertie (connection, VORTEX_SASL_AUTHORIZATION_ID));

	gsasl_property_set (data->session, GSASL_REALM, 
			    vortex_sasl_get_propertie (connection, VORTEX_SASL_REALM));

	gsasl_property_set (data->session, GSASL_ANONYMOUS_TOKEN, 
			    vortex_sasl_get_propertie (connection, VORTEX_SASL_ANONYMOUS_TOKEN));

	gsasl_property_set (data->session, GSASL_SERVICE, "beep");
	
	/* get current hostname */
	if (gethostname (hostname, 512) == 0) {
		vortex_log (VORTEX_LEVEL_DEBUG, "using as hostname for SASL service: %s", hostname);
		gsasl_property_set (data->session, GSASL_HOSTNAME, hostname);
	}
	return;
}


typedef struct __VortexSaslStartData {
	VortexConnection     * connection;
	const char           * profile;
	VortexSaslAuthNotify   process_status;
	axlPointer             user_data;
	bool                   threaded;
}VortexSaslStartData ;

/** 
 * @internal
 *
 * @brief Perform the initial SASL step, returning the initial base64
 * blob to be sent to the server side. 
 * 
 * Support function for __vortex_sasl_start_auth
 *
 * @return the function return an already base64 encoded SASL
 * content. The caller must unref the memory returned once not needed.
 */
char  * __vortex_sasl_initiator_do_initial_step (const char           * profile,
						 VortexConnection     * connection,
						 VortexSaslAuthNotify   process_status,
						 axlPointer             user_data)
{
	/* variable definitions for the gsasl_client_start function */
	GsaslData     * data;
	VortexCtx     * ctx = vortex_connection_get_ctx (connection);
	char          * base64_chunk;
	int             rc;

	/* drop a log */
	vortex_log (VORTEX_LEVEL_DEBUG, 
		    "Starting a SASL session for the selected profile: '%s'",  &(profile[26]));

	/* get sasl data */
	data = vortex_connection_get_data (connection, SASL_DATA);
	
	rc = gsasl_client_start (
		/* get the SASL context for this connection */
		data->ctx,
		/* a list of profiles selected (actually only one) */
		&(profile[26]), 
		/* the session object */
		&data->session);
		
	/* check for sasl result */
	if (rc != GSASL_OK) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Unable to initialize the first SASL step, code=%d, message='%s'",
		       rc, gsasl_strerror (rc));
		return NULL;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "Setting SASL properties to start the handshake");
	
	/* set properties (username, password, authorization id,
	 * service name ..) */
	vortex_sasl_configure_current_properties (connection);

	vortex_log (VORTEX_LEVEL_DEBUG, "Creating initial blob..");

	/* build initial message */
	rc = gsasl_step64 (data->session, "", &base64_chunk);
	vortex_log (VORTEX_LEVEL_DEBUG, "unblocked from step64");
	if (rc == GSASL_NEEDS_MORE || rc == GSASL_OK) {
		vortex_log (VORTEX_LEVEL_DEBUG, 
			    "SASL initial start have finished, sending start message to remote server: [size %d] '%s', GSASL message: (%d)",
			    strlen (base64_chunk), base64_chunk, rc);
		return base64_chunk;
	}
	vortex_log (VORTEX_LEVEL_CRITICAL, "initial step have failed: (%d): %s",
		    rc, gsasl_strerror (rc));
	return NULL;
}

/**  
 * @internal
 * @brief Support function to check for an error reply inside the content profile reply.
 * 
 * @return false if the frame received implies that the SASL
 * authentication couldn't be performed. Otherwise true is returned.
 */
bool     __vortex_sasl_is_error_content (VortexFrame * frame,
					 VortexConnection      * connection, 
					 VortexSaslAuthNotify    process_status, 
					 axlPointer              user_data)
{
	if (frame == NULL) {
		__vortex_sasl_notify (process_status, connection, VortexError, 
				      "Received a NULL frame while expenting a SASL server reply", user_data);
		return false;
	}
	
	if (axl_memcmp (vortex_frame_get_payload (frame), "<error", 6)) {
		__vortex_sasl_notify (process_status, connection, VortexError, 
				      "Received an negative reply from the server side to authenticate the PEER", user_data);
		return false;
	}
	return true;
}

/** 
 * @internal
 *
 * @brief Support function to extract from a frame received the SASL
 * base64 blob.
 * 
 * @return The blob content reply already decoded or NULL if fails.
 */
char    * __vortex_sasl_get_base64_blob (char                  * frame_content,
					 VortexConnection      * connection, 
					 VortexSaslAuthNotify    process_status, 
					 axlPointer              user_data,
					 char                 ** status)
{
	/* xml related variable declaration */
	axlDtd       * sasl_dtd;
	axlDoc       * doc;
	axlNode      * blob_node;

	/* SASL related variable declaration */
	char          * blob;
	VortexCtx    * ctx = vortex_connection_get_ctx (connection);

	vortex_log (VORTEX_LEVEL_DEBUG, "getting context from connection id=%d", vortex_connection_get_id (connection));

	/* first check for a empty blob received */
	if (frame_content == NULL) {
		/* return empty blob and set status to be be null */
		(*status) = NULL;
		return axl_strdup ("");
	}

	/* get channel management DTD */
	if ((sasl_dtd = vortex_dtds_get_sasl_dtd (ctx)) == NULL) {
		__vortex_sasl_notify (process_status, connection, VortexError,
				      "unable to load dtd file (sasl.dtd), cannot validate incoming message, returning error frame",
				      user_data);
		return NULL;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "about to parse SASL blob='%s'", frame_content);

	/* parser xml document */
	doc = axl_doc_parse (frame_content, strlen (frame_content), NULL);
	if (!doc) {
		__vortex_sasl_notify (process_status, connection, VortexError,
				      "unable to parse SASL reply, we have a buggy remote peer", user_data);
		return NULL;
	}

	/* validate de document */
	if (! axl_dtd_validate (doc, sasl_dtd, NULL)) {
		/* Validation failed */
		__vortex_sasl_notify (process_status, connection, VortexError,
				      "validation failed for SASL reply, we have a buggy remote peer", user_data);
		/* free document loaded */
		axl_doc_free (doc);
		return NULL;		 
	}
	
	/* get a reference to document root (the blob) */
	blob_node  = axl_doc_get_root (doc);
	if (! NODE_CMP_NAME (blob_node, "blob")) {
		__vortex_sasl_notify (process_status, connection, VortexError,
				      "not found error element on err reply, we have a buggy remote peer",
				      user_data);
		/* free the document */
		axl_doc_free (doc);
		return NULL;
	}
 
	/* get error code returned */
	(* status ) = axl_node_get_attribute_value_copy (blob_node, "status");

	/* get the message value returned */
	blob        = axl_node_get_content_copy (blob_node, NULL);

	/* free not needed data */
	axl_doc_free (doc);

	/* return the decoded blob */
	return blob;
}

/* perform the next additional steps needed to authenticate the
 * peer */
bool     __vortex_sasl_initiator_do_steps (VortexChannel         * channel, 
					   VortexAsyncQueue           * queue,
					   VortexConnection      * connection, 
					   VortexSaslAuthNotify    process_status, 
					   axlPointer              user_data)
{
	/* variable definitions for the sasl_client_step function */
	int           rc;

	/* variable definitions for the blob decoding */
	VortexFrame * reply    = NULL;
	char        * blob     = NULL;
	char        * new_blob = NULL;
	char        * status   = NULL;
	GsaslData   * data;
	VortexCtx   * ctx      = vortex_connection_get_ctx (connection);

	/* get sasl data */
	data = vortex_connection_get_data (connection, SASL_DATA);

	do {
		/* now process replies received */
		reply = vortex_channel_get_reply (channel, queue);
		
		vortex_log (VORTEX_LEVEL_DEBUG, "Reply received, checking it");
		
		/* for the first reply, that is a piggyback or a reply
		 * following the channel start response we have to
		 * check if an error code was returned. */
		if (!__vortex_sasl_is_error_content (reply, connection, process_status, user_data)) {
			vortex_frame_unref (reply);
			return false;
		}

		vortex_log (VORTEX_LEVEL_DEBUG, "Getting blob and status reply");

		/* get the base64 blob reply inside the frame but
		 * already decoded */
		blob = __vortex_sasl_get_base64_blob ((char*) vortex_frame_get_payload (reply), 
						      connection, process_status, user_data, &status);
		/* free frame reply */
		vortex_frame_unref (reply);

		/* check status value */
		vortex_log (VORTEX_LEVEL_DEBUG, "current blob value is: '%s' with status='%s'",
		       (blob != NULL) ? blob : "" , (status != NULL) ? status : "");
		if (status != NULL) {
			if (axl_cmp (status, "complete")) {
				/* undef status value and blob */
				vortex_support_free (2, blob, axl_free, status, axl_free);
				vortex_log (VORTEX_LEVEL_DEBUG, "authentication Ok due to status code");
				return true;
			}

			/* unref status value */
			vortex_support_free (1, status, axl_free);
		}

		/* check returned value */
		if (blob == NULL) {
			vortex_log (VORTEX_LEVEL_DEBUG, "received empty blob and status code doesn't indicate the SASL negotiation have ended.");
			return false;
		}

		vortex_log (VORTEX_LEVEL_DEBUG, "doing a SASL step..");

		/* feed the SASL engine with the data received from
		 * the remote side */
                rc = gsasl_step64 (data->session,
				   blob, &new_blob);

		vortex_log (VORTEX_LEVEL_DEBUG, "before doing sasl step, status is: (%d) %s, with new_blob=%s",
		       rc, gsasl_strerror (rc), (new_blob != NULL) ? new_blob : "");
		
		/* free blob received */
		vortex_support_free (1, blob, axl_free);
		
		/* manage continue case, that is, to keep on sending
		 * base64 pieces of code */
		if (rc == GSASL_NEEDS_MORE || ((new_blob != NULL) && (strlen (new_blob) > 0)) ) {
			/* work around to make gsasl to keep on iterating (REMOVE THIS) once
			 * solved */
			rc = GSASL_NEEDS_MORE;

			/* we need to send more data to the server */
			if (!vortex_channel_send_msgv (channel, NULL, "<blob>%s</blob>", new_blob)) {
				/* free new blob */
				axl_free (new_blob);

				__vortex_sasl_notify (process_status, connection, VortexError,
						      "Unable to negotiate SASL profile selected, an error have happen while sending data",
						      user_data);
				return false;
			}
		}

		/* unref blob generated and nullify loop variables */
		axl_free (new_blob);
		new_blob = NULL;
		blob     = NULL;
		status   = NULL;

	} while (rc == GSASL_NEEDS_MORE);

	if (rc != GSASL_OK) {
		/* unable to negotiate the SASL auth */
		__vortex_sasl_notify (process_status, connection, VortexError, 
				      "SASL negotiation have failed", user_data);
		return false;
	}

	/* seems we couldn't do more to authenticate the peer:
	 *
	 * Peter: Does this mean the SASL fun have ended Jack?
	 *
	 * Jack: That is, my SASLized friend, I'm afraid user space
	 * code is waiting for our reply! */
	return true;
}

/**
 * @internal Mask installed to ensure that SASL profile isn't reused
 * again for the particular connection.
 */
bool __vortex_sasl_mask (VortexConnection * connection,
			 int                channel_num,
			 const char       * uri,
			 const char       * profile_content,
			 const char       * serverName,
			 axlPointer         user_data)
{
	/* if both strings are equal, filter the profile */
	return (axl_memcmp (uri, VORTEX_SASL_FAMILY, 25));
} /* end if */

/** 
 * @internal
 * 
 * Support function to __vortex_sasl_start_auth, which filters current
 * SASL auth profiles from the profile list that the provided
 * connection has. Because the auth part of the SASL profiles doesn't
 * include a connection reset, as part of the tuning process, this
 * function removes all profiles from the SASL family to avoid
 * confusion about using them again.
 *
 * In fact this is only a stetic question, because proper SASL BEEP
 * peers only could accept one authentication process for each
 * connection. Receiving such notification, and accepting it twice,
 * would be a protocol violation.
 *
 * @param connection The connection where the SASL profile list will
 * be filtered.
 */
void __vortex_sasl_filter_profiles (VortexConnection * connection)
{

	/* install a mask into the connection to avoid showing againt
	 * any SASL profile */
	vortex_connection_set_profile_mask (connection, __vortex_sasl_mask, NULL);

	/* now the profile list is filtered */
	return;
}

void               __vortex_sasl_start_auth              (VortexSaslStartData * data)
{
	/* local variable declarations received from main function */
	VortexConnection     * connection      = data->connection;
	VortexCtx            * ctx             = vortex_connection_get_ctx (connection);
	const char           * profile         = data->profile;
	VortexSaslAuthNotify   process_status  = data->process_status;
	axlPointer             user_data       = data->user_data;
	
	/* local variable declarations for this function */
	VortexChannel        * channel;
	VortexAsyncQueue          * queue;
	char                 * base64_blob;

	/* free no longer needed node */
	axl_free (data);

	/* do initial client step creating the initial
	 * <blob>base64-content-auth</blob> */
	base64_blob = __vortex_sasl_initiator_do_initial_step (profile,
							       connection, 
							       process_status, 
							       user_data);
	/* check returned blob value */
	if (base64_blob == NULL) {
		/* notify user space  */
		__vortex_sasl_notify (process_status, connection, VortexError, 
				      "Unable initial SASL layer, required to produce first SASL message",  user_data);
		return;
	}

	/* create the channel */
	queue   = vortex_async_queue_new ();
	channel = vortex_channel_new_fullv (/* the connection where the SASL profile will be negotiated */
		                            connection,   
					    /* use the next free channel to be used */
					    0,
					    /* no serverName value to be used */
					    NULL,         
					    /* the profile to negotiate */
					    profile,      
					    /* no encoding done to the
					     * profile part, actually
					     * enclosed as CDATA */
					    EncodingNone, 
					    /* no close channel specification (it doesn't matter
					     * if the channel negotiated is closed once finished
					     * the process. Auth id get from such process will
					     * remain to the connection. */
					    NULL, NULL,
					    /* make all frames received, including those from the
					     * piggyback, to be received through vortex_channel_get_reply */
					    vortex_channel_queue_reply, queue,
					    /* don't use channel creation notify, we want this
					     * function to block us. */
					    NULL, NULL,
					    /* the profile content to send */
					    (strlen (base64_blob) != 0) ? "<blob>%s</blob>" : "",
					    (strlen (base64_blob) != 0) ? base64_blob       : NULL);
	/* unref no longer needed blob */
	axl_free (base64_blob);

	vortex_log (VORTEX_LEVEL_DEBUG, "SASL channel creation reply received");
	if (channel == NULL) {
		/* unref queue */
		vortex_async_queue_unref (queue);
		
		/* notify user space  */
		__vortex_sasl_notify (process_status, connection, VortexError, 
				      "Unable to create the SASL channel",  user_data);
		return;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "Seems the SASL channel have being created");

	/* perform the next additional steps needed to authenticate the peer */
	if (!__vortex_sasl_initiator_do_steps (channel, queue,
					       connection, process_status, user_data)) {

		/* unref queue */
		vortex_async_queue_unref (queue);

		/* because the SASL profile negotiation have failed,
		 * close the channel */
		vortex_channel_close (channel, NULL);
		return;
	}

	/* unref queue */
	vortex_async_queue_unref (queue);

	/* SASL operation is OK */

	/* because the SASL profiles could not require to perform a
	 * new greetings message, because the session is not secured
	 * but only authenticated, this function must filter all SASL
	 * mechanism if the connection detects that SASL is already
	 * running */
	__vortex_sasl_filter_profiles (connection);

	/* configure the profile used by the authentication process */
	vortex_connection_set_data (connection, SASL_METHOD_USED, (axlPointer) profile);

	/* notify user space that the SASL profile have successfully
	 * completed */
	__vortex_sasl_notify (process_status, connection, VortexOk,
			      "SASL authentication Ok, let's continue!", user_data);
	return;
}

/** 
 * @internal
 * @brief Ensures all properties needed for the SASL profile used are provided.
 *
 * The function not only checks for properties, it also notify a
 * textual error message in case some properties wasn't set properly.
 * 
 * @param conection      The connection where the SASL operations will be performed
 * @param profile        The SASL mech selected to be checked
 * @param process_status User space SASL profile notify
 * @param user_data      User defined data.
 * 
 * @return true if all properties was provided for the selected mech
 * or false if something is wrong.
 */
bool     __vortex_sasl_auth_data_sanity_check (VortexConnection     * connection,
					       const char           * profile, 
					       VortexSaslAuthNotify   process_status,
					       axlPointer             user_data)
{
	/* SASL ANONYMOUS checkings */
	if (axl_cmp (profile, VORTEX_SASL_ANONYMOUS)) {
		if (!vortex_sasl_get_propertie (connection, VORTEX_SASL_ANONYMOUS_TOKEN)) {
			__vortex_sasl_notify (process_status, connection, VortexError,
					      "ANONYMOUS profile requires the anonymous token to be set in order to start SASL negotiation",
					      user_data);
			return false;
		}
	} /* end anonymous */

	/* SASL PLAIN and SASL CRAM-MD5 checking */
	if (axl_cmp (profile, VORTEX_SASL_PLAIN) ||
	    axl_cmp (profile, VORTEX_SASL_DIGEST_MD5) ||
	    axl_cmp (profile, VORTEX_SASL_CRAM_MD5)) {
		if (!vortex_sasl_get_propertie (connection, VORTEX_SASL_AUTH_ID)) {
			__vortex_sasl_notify (process_status, connection, VortexError,
					      "selected profile requires the auth id to be set in order to start SASL negotiation",
					      user_data);
			return false;
		}

		if (!vortex_sasl_get_propertie (connection, VORTEX_SASL_PASSWORD)) {
			__vortex_sasl_notify (process_status, connection, VortexError,
					      "selected profile requires the password to be set in order to start SASL negotiation",
					      user_data);
			return false;
		}
	} /* end plain */

	/* checks for EXTERNAL mech */
	if (axl_cmp (profile, VORTEX_SASL_EXTERNAL)) {
		if (!vortex_sasl_get_propertie (connection, VORTEX_SASL_AUTHORIZATION_ID)) {
			if (!vortex_sasl_get_propertie (connection, VORTEX_SASL_AUTH_ID)) {
				__vortex_sasl_notify (process_status, connection, VortexError,
						      "selected profile requires the authorization id to be set in order to start SASL negotiation",
						      user_data);
				return false;
			}
		}
	} /* end external */
	    
	return true;
}

#endif

/** 
 * @brief Begin SASL authentication process using the selected profile.
 * 
 * Starts the SASL authentication process using the selected profile
 * for the client side. Available values for <b>profile</b> that are
 * supported are:
 * 
 *  - \ref VORTEX_SASL_ANONYMOUS
 *  - \ref VORTEX_SASL_EXTERNAL
 *  - \ref VORTEX_SASL_PLAIN
 *  - \ref VORTEX_SASL_CRAM_MD5
 *  - \ref VORTEX_SASL_DIGEST_MD5
 * 
 * Each profile requires different properties to be set using \ref
 * vortex_sasl_set_propertie. Check \ref
 * vortex_manual_sasl_for_client_side "using SASL at client side" for
 * details.
 *
 * This function performs all SASL operations in a non-blocking way
 * for the caller. This means that the function returns once called
 * and all notification about SASL process status is done through \ref
 * VortexSaslAuthNotify "process_status".
 *
 * If it is needed a blocking version for this function then \ref
 * vortex_sasl_start_auth_sync should be used.
 *
 * See \ref vortex_manual_sasl_for_client_side "Using SASL at client side" 
 * for a detailed explanation about SASL support inside Vortex
 * for the client side.
 * 
 * @param connection     The connection where the SASL authentication will be performed.
 *
 * @param profile        The SASL profile selected.
 *
 * @param process_status Asynchronous notification for SASL process
 * finish status. This handler <b>is not optional</b>. It has to
 * be defined to get report about SASL status. 
 *
 * @param user_data      User space defined data to be passed to <b>process_status</b>
 */
void               vortex_sasl_start_auth                (VortexConnection     * connection,
							  const char           * profile, 
							  VortexSaslAuthNotify   process_status,
							  axlPointer             user_data)
{
#ifndef ENABLE_SASL_SUPPORT
	/* implement non-SASL supported case */
	__vortex_sasl_notify (process_status, connection, VortexError, 
			      "current Vortex Library doesn't support SASL profiles",
			      user_data);

	return;
#else
	/* SASL enabled case */
	VortexSaslStartData * data;
	VortexCtx           * ctx = vortex_connection_get_ctx (connection);

	/* check connection status */
	if (!vortex_connection_is_ok (connection, false)) {
		__vortex_sasl_notify (process_status, connection, VortexError,
				      "received a not valid connection while trying to start SASL negotiation",
				      user_data);
		return;
	}

	/* check profile received */
	if ((profile == NULL) || !axl_memcmp (VORTEX_SASL_FAMILY, profile, 25)) {
		__vortex_sasl_notify (process_status, connection, VortexError,
				      "profile provided for SASL auth is null or is not a SASL one.",
				      user_data);
		return;
	}

	/* if the given connection is already authenticated */
	if (vortex_sasl_is_authenticated (connection)) {
		__vortex_sasl_notify (process_status, connection, VortexError,
				      "provided connection is already authenticated",
				      user_data);
		return;
	}

	/* perform auth incoming data sanity check */
	if (!__vortex_sasl_auth_data_sanity_check (connection, profile, process_status, user_data))
		return;

	/* try to initizalice the library */
	if (!__vortex_sasl_create_context (connection)) {
		/* notify user space that shit happens while init the
		 * SASL library */
		__vortex_sasl_notify (process_status, connection, VortexError, 
				      "Unable to initialize SASL library",  user_data);
		return;
	}

	vortex_log (VORTEX_LEVEL_DEBUG, "SASL initialized..");
	data                 = axl_new (VortexSaslStartData, 1);
	data->connection     = connection;
	data->profile        = profile;
	data->process_status = process_status;
	data->user_data      = user_data;
	data->threaded       = (process_status != NULL);

	/* invoke the process */
	if (data->threaded) {
		/* threaded mode, the caller will be unblocked */
		vortex_log (VORTEX_LEVEL_DEBUG, "begin SASL negotiation under threaded mode");
		vortex_thread_pool_new_task (ctx, (VortexThreadFunc) __vortex_sasl_start_auth, data);
		return;
	}
	/* non-threaded mode, the caller will be blocked until SASL process finish */
	vortex_log (VORTEX_LEVEL_DEBUG, "begin SASL negotiation non-threaded mode (blocking the caller)");
	__vortex_sasl_start_auth (data);
	return;
#endif	
}

/** 
 * @internal
 * @brief Support function for \ref vortex_start_auth_sync
 */
void __vortex_sasl_start_auth_sync_process  (VortexConnection * connection,
					     VortexStatus       status,
					     char             * status_message,
					     axlPointer         user_data)
{
	VortexAsyncQueue * queue = user_data;
	VortexCtx        * ctx   = vortex_connection_get_ctx (connection);

	/* push status and status message and unref man! */
	QUEUE_PUSH  (queue, INT_TO_PTR (status));
	QUEUE_PUSH  (queue, status_message);
	vortex_async_queue_unref (queue);

	return;
}


/** 
 * @brief Perform SASL negotiation in a synchronous mode (blocking the caller until process finish).
 * 
 * This function does the same like \ref vortex_sasl_start_auth but in
 * a blocking manner. Check documentation this this function at \ref vortex_sasl_start_auth.
 * 
 * Here is an example:
 * \code
 * // some variable declaration for SASL status
 * VortexStatus   status;
 * char         * status_message;
 * 
 * // set required properties
 * vortex_sasl_set_propertie (connection, VORTEX_SASL_AUTH_ID,  
 *                            "bob", NULL);
 * vortex_sasl_set_propertie (connection, VORTEX_SASL_PASSWORD, 
 *                            "secret", NULL);
 *                            
 * // begin SASL negotiation
 * vortex_sasl_start_auth_sync (connection, VORTEX_SASL_CRAM_MD5,
 *                              &status, &status_message);
 * // write out error
 * switch (status) {
 * case VortexOk:
 *	printf ("OK: message reported: %s\n", status_message);
 *	break;
 * case VortexError:
 *	printf ("FAIL: message reported: %s\n", status_message);
 *	break;
 * }
 * \endcode
 *
 * See \ref vortex_manual_sasl_for_client_side "Using SASL at client side" 
 * for a detailed explanation about SASL support inside Vortex
 * for the client side.
 * 
 * @param connection The connection where the SASL negotiation will take place.
 * @param profile    The SASL profile selected.
 * @param status     A reference to a \ref VortexStatus variable to notify caller SASL finish status.
 * @param status_message A reference to notify the caller a textual diagnostic about SASL finish status.
 */
void               vortex_sasl_start_auth_sync           (VortexConnection     * connection,
							  const char           * profile,
							  VortexStatus         * status,
							  char                ** status_message)

{
	/* timeout object */
	axlPointer         _status;

	/* create an async queue and increase queue reference so the
	 * function could unref it without worry about race conditions
	 * with sync_process function. */
	VortexAsyncQueue * queue = vortex_async_queue_new ();
	VortexCtx        * ctx   = vortex_connection_get_ctx (connection);

	/* ref the queue (due to previous comment) */
	vortex_async_queue_ref (queue);

	/* start SASL negotiation */
	vortex_sasl_start_auth (connection,  profile, __vortex_sasl_start_auth_sync_process,
				queue);

	/* get status */
	_status = vortex_async_queue_timedpop (queue, vortex_connection_get_timeout (ctx));
	if (_status == NULL) {
		/* seems timeout have happen while waiting for SASL to end */
		(* status)         = VortexError;
		(* status_message) = "Timeout have been reached while waiting for SASL to finish";
		return;
	}

	/* set status value */
	if (status != NULL)
		(* status) = PTR_TO_INT (_status);
	
	/* get message */
	if (status_message != NULL)
		(* status_message) = vortex_async_queue_pop (queue);
	else
		vortex_async_queue_pop (queue);

	/* unref the queue */
	vortex_async_queue_unref (queue);

	return;
}


#ifdef ENABLE_SASL_SUPPORT

/** 
 * @internal
 * 
 * Internal functions which allows to check properties to be passed to
 * each mechanism, ensuring all of them receives the minimum set of
 * required values.
 */
bool     __validation_handler_check_properties (Gsasl_session * sctx, 
						char          * sasl_profile,
						VortexCtx     * ctx)
{
	/* check for a proper SASL profile name defined */
	if (sasl_profile == NULL) {
		vortex_log (VORTEX_LEVEL_WARNING, "SASL profile not defined, giving up");
		return false;
	}

	/* check for appropiate values for anonymous profile */
	if (axl_cmp (sasl_profile, "ANONYMOUS")) {
		if (gsasl_property_fast (sctx, GSASL_ANONYMOUS_TOKEN) == NULL) {
			vortex_log (VORTEX_LEVEL_WARNING, "ANONYMOUS method requires the anonymous token to be configured, giving up");
			return false;
		}	

		/* nothing more to check */
		return true;
	}

	/* check for appropiate values for external profile */
	if (axl_cmp (sasl_profile, "EXTERNAL")) {

		/* check for authorization id value */
		if (gsasl_property_fast (sctx, GSASL_AUTHZID) == NULL) {
			vortex_log (VORTEX_LEVEL_WARNING, 
				    "EXTERNAL method requires the authorization id to be configured, giving up");
			return false;
		}	

		/* nothing more to check */
		return true;
	}

	/* check for auth attribute */
	if (axl_cmp (sasl_profile, "PLAIN") ||
	    axl_cmp (sasl_profile, "CRAM-MD5") ||
	    axl_cmp (sasl_profile, "DIGEST-MD5")) {

		/* check for auth id */
		if (gsasl_property_fast (sctx, GSASL_AUTHID) == NULL) {
			vortex_log (VORTEX_LEVEL_WARNING, 
				    "%s method requires the authorization id to be configured, giving up", sasl_profile);
			return false;
		}	

		/* nothing more to check */
		return true;
	}
	
	vortex_log (VORTEX_LEVEL_WARNING, "SASL profile=%s not supported, giving up", sasl_profile);
	return false;
}

/** 
 * @internal
 * @brief Validation support.
 * 
 * @param ctx 
 * @param sctx 
 * @param prop 
 * 
 * @return 
 */
int __vortex_sasl_auth_validation (Gsasl * gctx, Gsasl_session * sctx,
				   Gsasl_property prop)
{
	/* get current context */
	const char       * anonymous_token;
	VortexConnection * connection;
	char             * password;
	char             * profile;
	axlPointer	   user_data;
	VortexCtx        * ctx;

	/* get the connection reference where the SASL request was received */
	connection      = (VortexConnection *) gsasl_callback_hook_get (gctx);
	
	/* get the context */
	ctx             = vortex_connection_get_ctx (connection);

	/* get profile begin validated */
	profile         = vortex_connection_get_data (connection, SASL_PROFILE_BEGIN_NEGOCIATED);

	/* Get user_data */
	user_data = NULL;
	if (axl_cmp (profile, VORTEX_SASL_ANONYMOUS+strlen(VORTEX_SASL_FAMILY)+1))
		user_data = vortex_connection_get_data (connection, VORTEX_SASL_ANONYMOUS_USER_DATA);
	else if (axl_cmp (profile, VORTEX_SASL_PLAIN+strlen(VORTEX_SASL_FAMILY)+1))
		user_data = vortex_connection_get_data (connection, VORTEX_SASL_PLAIN_USER_DATA);
	else if (axl_cmp (profile, VORTEX_SASL_CRAM_MD5+strlen(VORTEX_SASL_FAMILY)+1))
		user_data = vortex_connection_get_data (connection, VORTEX_SASL_CRAM_MD5_USER_DATA);
	else if (axl_cmp (profile, VORTEX_SASL_DIGEST_MD5+strlen(VORTEX_SASL_FAMILY)+1))
		user_data = vortex_connection_get_data (connection, VORTEX_SASL_DIGEST_MD5_USER_DATA);
	else if (axl_cmp (profile, VORTEX_SASL_EXTERNAL+strlen(VORTEX_SASL_FAMILY)+1))
		user_data = vortex_connection_get_data (connection, VORTEX_SASL_EXTERNAL_USER_DATA);	

	vortex_log (VORTEX_LEVEL_DEBUG, "received notification to validate an incoming SASL request for=%s over connection id=%d",
		    profile, vortex_connection_get_id (connection));

	/* check for the realm */
	if (prop == GSASL_REALM) {
		vortex_log (VORTEX_LEVEL_DEBUG, "received request to return current REALM configured");

		/* try to get current realm configured for the listener */
		if (vortex_listener_get_default_realm (ctx) == NULL) {
			vortex_log (VORTEX_LEVEL_DEBUG, "no realm configured, returning no realm found");
			/* no default realm configured, return no value */
			return GSASL_NO_CALLBACK;
		} /* end if */

		/* configure realm */
		vortex_log (VORTEX_LEVEL_DEBUG, "returning that realm configured is=%s", vortex_listener_get_default_realm (ctx));
		gsasl_property_set (sctx, GSASL_REALM, vortex_listener_get_default_realm (ctx));
		
		/* return that the value was configured */
		return GSASL_OK;
	}

	/* check for properties configured */
	if (! __validation_handler_check_properties (sctx, profile, ctx))
		return GSASL_NO_CALLBACK;

	/* according to properties the SASL listener engine should
	 * validate value received.  ANONYMOUS SASL profile validation
	 * support */
	if (prop == GSASL_VALIDATE_ANONYMOUS) {
		/* check if an anonymous handler was defined */
		if (ctx->sasl_anonymous_auth_handler == NULL && ctx->sasl_anonymous_auth_handler_full == NULL) {
			vortex_log (VORTEX_LEVEL_CRITICAL, 
			       "A SASL anonymous validation handler was not set. Unable to authenticate current request. Please check Vortex documentation to set this handler.");
			return GSASL_NO_CALLBACK;
		}
		
		/* get current anonymous token received from remote
		 * side */
		anonymous_token = gsasl_property_get (sctx, GSASL_ANONYMOUS_TOKEN);

		/* validate anonymous request */
		if ( (
			ctx->sasl_anonymous_auth_handler &&
			ctx->sasl_anonymous_auth_handler (connection, 
							   /* get current anonymous token received from remote side */
							   gsasl_property_get (sctx, GSASL_ANONYMOUS_TOKEN))
			) || (
			ctx->sasl_anonymous_auth_handler_full &&
			ctx->sasl_anonymous_auth_handler_full (connection, 
							  /* get current anonymous token received from remote side */
							  gsasl_property_get (sctx, GSASL_ANONYMOUS_TOKEN),
							  user_data))
			) {
			vortex_log (VORTEX_LEVEL_DEBUG, "user space have accepted anonymous login");
			/* configure current connections anonymous
			 * token so the server side could get it
			 * later */
			vortex_sasl_set_propertie (connection, VORTEX_SASL_ANONYMOUS_TOKEN, 
						   (char  *) gsasl_property_get (sctx, GSASL_ANONYMOUS_TOKEN), NULL);
			
			/* configure method used to authenticate */
			vortex_connection_set_data (connection, SASL_METHOD_USED, VORTEX_SASL_ANONYMOUS);

			return GSASL_OK;
		}
		
		vortex_log (VORTEX_LEVEL_DEBUG, "anonymous validation failed: '%s'", 
		       gsasl_property_get (sctx, GSASL_ANONYMOUS_TOKEN));
		return GSASL_AUTHENTICATION_ERROR;
	} /* end anonymous profile validation */

	/* PLAIN SASL profile validation support */
	if (prop == GSASL_VALIDATE_SIMPLE) {
		/* check if an plain handler was defined  */
		if (ctx->sasl_plain_auth_handler == NULL && ctx->sasl_plain_auth_handler_full == NULL) {
			vortex_log (VORTEX_LEVEL_CRITICAL, 
			       "A SASL plain validation handler was not set. Unable to authenticate current request. Please check Vortex documentation to set this handler.");
			return GSASL_NO_CALLBACK;
		}
		
		/* validate PLAIN profile */
		if ( (
			ctx->sasl_plain_auth_handler &&
			ctx->sasl_plain_auth_handler (connection,
						      /* get current auth_id value received from remote side  */
						      gsasl_property_get (sctx, GSASL_AUTHID),
						      /* get current authorization_id value received from remote side */
						      gsasl_property_get (sctx, GSASL_AUTHZID),
						      /* get current password value received from remote side */
						      gsasl_property_get (sctx, GSASL_PASSWORD))
			) || (
			ctx->sasl_plain_auth_handler_full &&
			ctx->sasl_plain_auth_handler_full (connection,
						      /* get current auth_id value received from remote side  */
						      gsasl_property_get (sctx, GSASL_AUTHID),
						      /* get current authorization_id value received from remote side */
						      gsasl_property_get (sctx, GSASL_AUTHZID),
						      /* get current password value received from remote side */
						      gsasl_property_get (sctx, GSASL_PASSWORD),
							  user_data))
			) {
			vortex_log (VORTEX_LEVEL_DEBUG, "user space have accepted plain login");
			/* configure current plain validation data
			 * used: authid, authorization id and
			 * password */
			vortex_sasl_set_propertie (connection, VORTEX_SASL_AUTH_ID, 
						   (char  *) gsasl_property_get (sctx, GSASL_AUTHID), NULL);

			vortex_sasl_set_propertie (connection, VORTEX_SASL_AUTHORIZATION_ID, 
						   (char  *) gsasl_property_get (sctx, GSASL_AUTHZID), NULL);

			vortex_sasl_set_propertie (connection, VORTEX_SASL_PASSWORD, 
						   (char  *) gsasl_property_get (sctx, GSASL_PASSWORD), NULL);

			/* configure method used to authenticate */
			vortex_connection_set_data (connection, SASL_METHOD_USED, VORTEX_SASL_PLAIN);

			return GSASL_OK;
		}
		vortex_log (VORTEX_LEVEL_DEBUG, "plain profile validation failed for: '%s'", 
		       gsasl_property_get (sctx, GSASL_AUTHID));
		return GSASL_AUTHENTICATION_ERROR;						      
	} /* end plain profile validation */

	if (prop == GSASL_PASSWORD) {
		/* check if we are validating cram-md5 profile */
		if (axl_cmp (profile, "CRAM-MD5")) {
			if (ctx->sasl_cram_md5_auth_handler == NULL && ctx->sasl_cram_md5_auth_handler_full == NULL) {
				vortex_log (VORTEX_LEVEL_CRITICAL, 
				       "A SASL CRAM-MD5 validation handler was not set. Unable to authenticate current request. Please check Vortex documentation to set this handler.");
				return GSASL_NO_CALLBACK;
			}
			
			/* get password  */
			password = NULL;
			if (ctx->sasl_cram_md5_auth_handler)
				password = ctx->sasl_cram_md5_auth_handler (connection, 
									     gsasl_property_get (sctx, GSASL_AUTHID));
			else if (ctx->sasl_cram_md5_auth_handler_full)
				password = ctx->sasl_cram_md5_auth_handler_full (connection, 
										  gsasl_property_get (sctx, GSASL_AUTHID),
										  user_data);
			
			
			if (password != NULL) {
				vortex_log (VORTEX_LEVEL_DEBUG, "CRAM-MD5 handler defined and password returned from it");
				gsasl_property_set (sctx, GSASL_PASSWORD, password);
				/* Cram md5 accepted, configure current data for the connection. 
				   Here, at this point there is a problem. Because the connection 
				   isn't still validated, because the password returned must be checked
				   with current HASH received, the following lines could be setting a 
				   validation status for a connection that is finally not authenticated 
				   due a HASH mismatch. This is because the user application must use allways 
				   vortex_sasl_is_authenticated before getting any data about the method used 
				   or its values */
				
				/* configure current cram-md5 validation data
				 * used: authid, and password */
				vortex_sasl_set_propertie (connection, VORTEX_SASL_AUTH_ID, 
							   (char  *) gsasl_property_get (sctx, GSASL_AUTHID), NULL);

				/* the next instruction store the password duplicating it and
				 * providing a destroy function because the SASL engine already
				 * deallocated this value. */
				vortex_sasl_set_propertie (connection, VORTEX_SASL_PASSWORD, 
							   (char  *) password, axl_free);
				
				/* configure method used to authenticate */
				vortex_connection_set_data (connection, SASL_METHOD_USED, VORTEX_SASL_CRAM_MD5);

				return GSASL_OK;
			}
			vortex_log (VORTEX_LEVEL_CRITICAL, "cram md5 handler defined but no password returned from it.");
			return GSASL_AUTHENTICATION_ERROR;
		}

		/* check if we are validating digest-md5 profile */
		if (axl_cmp (profile, "DIGEST-MD5")) {
			if (ctx->sasl_digest_md5_auth_handler == NULL && ctx->sasl_digest_md5_auth_handler_full == NULL) {
				vortex_log (VORTEX_LEVEL_CRITICAL, 
				       "A SASL DIGEST-MD5 validation handler was not set. Unable to authenticate current request. Please check Vortex documentation to set this handler.");
				return GSASL_NO_CALLBACK;
			}
			
			/* get password  */
			password = NULL;
			if (ctx->sasl_digest_md5_auth_handler)
				password = ctx->sasl_digest_md5_auth_handler (connection, 
									       gsasl_property_get (sctx, GSASL_AUTHID),
									       gsasl_property_get (sctx, GSASL_AUTHZID),
									       gsasl_property_get (sctx, GSASL_REALM));
			else if (ctx->sasl_digest_md5_auth_handler_full)
				password = ctx->sasl_digest_md5_auth_handler_full (connection, 
										    gsasl_property_get (sctx, GSASL_AUTHID),
										    gsasl_property_get (sctx, GSASL_AUTHZID),
										    gsasl_property_get (sctx, GSASL_REALM),
										    user_data);
			if (password != NULL) {
				vortex_log (VORTEX_LEVEL_DEBUG, "digest md5 handler defined and password returned from it");
				gsasl_property_set (sctx, GSASL_PASSWORD, password);

				/* Please read the coment done to the
				 * SASL CRAM-MD5 method. The same
				 * applies to this method. */

				/* configure current digest-md5 validation data
				 * used: authid, and password */
				vortex_sasl_set_propertie (connection, VORTEX_SASL_AUTH_ID, 
							   (char  *) gsasl_property_get (sctx, GSASL_AUTHID), NULL);
				
				vortex_sasl_set_propertie (connection, VORTEX_SASL_AUTHORIZATION_ID, 
							   (char  *) gsasl_property_get (sctx, GSASL_AUTHZID), NULL);
				
				vortex_sasl_set_propertie (connection, VORTEX_SASL_REALM, 
							   (char  *) gsasl_property_get (sctx, GSASL_REALM), NULL);
				
				/* the next instruction store the password duplicating it and
				 * providing a destroy function because the SASL engine already
				 * deallocated this value. */
				vortex_sasl_set_propertie (connection, VORTEX_SASL_PASSWORD, 
							   (char  *) password, axl_free);
				
				/* configure method used to authenticate */
				vortex_connection_set_data (connection, SASL_METHOD_USED, VORTEX_SASL_DIGEST_MD5);

				return GSASL_OK;
			}
			vortex_log (VORTEX_LEVEL_CRITICAL, "digest md5 handler defined but not password returned from it.");
			return GSASL_AUTHENTICATION_ERROR;
		}
	} /* end cram and digest md5 validation */

	/* EXTERNAL SASL profile validation support */
	if (prop == GSASL_VALIDATE_EXTERNAL) {
		
		/* check if an external handler was defined  */
		if (ctx->sasl_external_auth_handler == NULL && ctx->sasl_external_auth_handler_full == NULL) {
			vortex_log (VORTEX_LEVEL_CRITICAL,
				    "A SASL external validation handler was not set. Unable to authenticate current request. Please check Vortex documentation to set this handler.");
			return GSASL_NO_CALLBACK;
		}

		/* validate EXTERNAL profile */
		if ((ctx->sasl_external_auth_handler != NULL && 
		     ctx->sasl_external_auth_handler (connection, gsasl_property_get (sctx, GSASL_AUTHZID))) 
		    || 
		    (ctx->sasl_external_auth_handler_full != NULL && 
		     ctx->sasl_external_auth_handler_full (connection, gsasl_property_get (sctx, GSASL_AUTHZID), user_data))) {
			
			/* configure current external validation data
			 * used: authid */
			vortex_sasl_set_propertie (connection, VORTEX_SASL_AUTH_ID, 
						   (char  *) gsasl_property_get (sctx, GSASL_AUTHZID), NULL);

			/* configure method used to authenticate */
			vortex_connection_set_data (connection, SASL_METHOD_USED, VORTEX_SASL_EXTERNAL);

			vortex_log (VORTEX_LEVEL_DEBUG, "user space have accepted external login");
			return GSASL_OK;
		}
		vortex_log (VORTEX_LEVEL_DEBUG, "external profile validation failed for: '%s'", gsasl_property_get (sctx, GSASL_AUTHZID));
		
		return GSASL_AUTHENTICATION_ERROR;
	} /* end external profile validation */

	/* nothing about the propertie received */
	return GSASL_NO_CALLBACK;
}


/** 
 * @internal
 * @brief Configure default callback handler for listener instances
 * 
 * @param connection The listener connection
 */
void __vortex_sasl_set_default_listener_handler (VortexConnection * connection)
{
	/* get sasl data */
	GsaslData * data = vortex_connection_get_data (connection, SASL_DATA);

	/* set default validation handlers */
	gsasl_callback_set (data->ctx, __vortex_sasl_auth_validation);
	return;
}

/** 
 * @internal
 * @brief Build the SASL reply message.
 * 
 * @param status Current status to be set: "complete", "abort", "continue".
 * @param blob_content Current blob content
 * 
 * @return Return a newly allocated SASL blob reply.
 */
char  * __build_blob_reply (char  * status, char  * blob_content)
{
	char  * result =  axl_strdup_printf ("<blob%s%s%s%s%s%s",
					     (status       != NULL) ? " status='"  : "",
					     (status       != NULL) ? status       : "",
					     (status       != NULL) ? "'"          : "",
					     (blob_content != NULL && (strlen (blob_content) > 0)) ? ">"          : " />",
					     (blob_content != NULL && (strlen (blob_content) > 0)) ? blob_content : "",
					     (blob_content != NULL && (strlen (blob_content) > 0)) ? "</blob>"    : "");
	/* deallocate the blob content */
	if (blob_content != NULL)
		axl_free (blob_content);

	/* return built message */
	return result;
}

/** 
 * @internal
 * @brief Function which actually perform sasl server iterations.
 * 
 * @param connection The connection where the sasl server iteration will be performed.
 * @param profile_content profile content 
 * 
 * @return 
 */
bool     __vortex_sasl_server_iterate (VortexConnection * connection, 
				       char             * payload,
				       char            ** payload_reply)
{
	char          * blob          = NULL;
	char          * base64_chunk  = NULL;
	char          * status        = NULL;
	GsaslData     * data          = vortex_connection_get_data (connection, SASL_DATA);
	VortexCtx     * ctx           = vortex_connection_get_ctx (connection);
	int             rc;

	/* get received blob */
	if (payload != NULL && strlen (payload) > 0) {
		blob = __vortex_sasl_get_base64_blob (payload, connection, NULL, NULL, &status);
		if (blob == NULL) {
			(* payload_reply) = vortex_frame_get_error_message ("500", "General syntax error: received not properly formated SASL message.", NULL);
			return false;
		}
		
		vortex_log (VORTEX_LEVEL_DEBUG, "Received blob from client: %s", blob);
		
		/* free status value (enable checking this value): in the case
		 * of "abort" stop the SASL process. */
		if (status != NULL) {
			if (axl_cmp (status, "abort")) {
				/* free current context and session  */
				vortex_connection_set_data (connection, SASL_DATA, NULL);
				vortex_log (VORTEX_LEVEL_WARNING, "SASL profile negociation cancelled");
				axl_free (status);
				return false;
			}

			/* free status value if not processed */
			axl_free (status);
		}
	} else {
		/* empty blob */
		blob = axl_strdup ("");
	}

	/* now feed the server SASL context with the blob received from the remote peer */
	rc = gsasl_step64 (data->session, blob, &base64_chunk);

	/* unref blob */
	axl_free (blob);

	vortex_log (VORTEX_LEVEL_DEBUG, "getting error code (%d): %s", rc, gsasl_strerror (rc));

	vortex_log (VORTEX_LEVEL_DEBUG, "unblocked from step64 with output=%s", 
	       (base64_chunk != NULL) ? base64_chunk : "");

	switch (rc) {
	case GSASL_NEEDS_MORE:
		vortex_log (VORTEX_LEVEL_DEBUG, "more data is required");
		(* payload_reply ) = __build_blob_reply (NULL, base64_chunk);
		/* accept the channel to be created in order to
		 * receive next base64 chunks there is no need to
		 * unref base64_chunck, it is already don at
		 * __build_blob_reply */
		return true;
	case GSASL_AUTHENTICATION_ERROR:
		vortex_log (VORTEX_LEVEL_CRITICAL, "application level have denied SASL auth");
		(* payload_reply ) = vortex_frame_get_error_message ("535", "Application level have denied to accept SASL auth. Authentication failure.", NULL);
		/* deny on going authentication there is no need to
		 * unref base64_chunck, it is already don at
		 * __build_blob_reply */
		return false;
	case GSASL_OK:
		vortex_log (VORTEX_LEVEL_DEBUG, "SASL negotiation have finished");
		(* payload_reply ) = __build_blob_reply ("complete", base64_chunk);
		/* accept the channel to be created and successfully
		 * authenticated */
		vortex_connection_set_data (connection, SASL_IS_AUTHENTICATED, INT_TO_PTR (true));
		/* there is no need to unref base64_chunck, it is
		 * already don at __build_blob_reply */

		/* filter all sasl methods to avoid reusing them */
		__vortex_sasl_filter_profiles (connection);

		return true;
	}

	/* in other case, false is returned */
	(* payload_reply) = vortex_frame_get_error_message ("454", "Temporary authentication failure.", NULL);
	return false;
}

/** 
 * @internal 
 * @brief Support for extended channel start received.
 * 
 * @param profile 
 * @param channel_num 
 * @param connection 
 * @param serverName 
 * @param profile_content 
 * @param profile_content_reply 
 * @param encoding 
 * @param user_data 
 * 
 * @return 
 */
bool     __vortex_sasl_accept_negociation_start (char              * profile,
						 int                 channel_num,
						 VortexConnection  * connection,
						 char              * serverName,
						 char              * profile_content,
						 char             ** profile_content_reply,
						 VortexEncoding      encoding,
						 axlPointer          user_data)
{

	GsaslData      * data;
	int              rc;
	VortexCtx      * ctx = vortex_connection_get_ctx (connection);

	vortex_log (VORTEX_LEVEL_DEBUG, "Received channel start for mech: %s, profile_content=%s", 
	       profile, profile_content);

	/* create the server context */
	if (!__vortex_sasl_create_context (connection))
		return false;

	vortex_log (VORTEX_LEVEL_DEBUG, "setting default auth validation handler for the listener");
	
	/* associate user_data with connection */
	if (axl_cmp (profile, VORTEX_SASL_ANONYMOUS))
		vortex_connection_set_data (connection, VORTEX_SASL_ANONYMOUS_USER_DATA, user_data);
	else if (axl_cmp (profile, VORTEX_SASL_PLAIN))
		vortex_connection_set_data (connection, VORTEX_SASL_PLAIN_USER_DATA, user_data);
	else if (axl_cmp (profile, VORTEX_SASL_CRAM_MD5))
		vortex_connection_set_data (connection, VORTEX_SASL_CRAM_MD5_USER_DATA, user_data);
	else if (axl_cmp (profile, VORTEX_SASL_DIGEST_MD5))
		vortex_connection_set_data (connection, VORTEX_SASL_DIGEST_MD5_USER_DATA, user_data);
	else if (axl_cmp (profile, VORTEX_SASL_EXTERNAL))
		vortex_connection_set_data (connection, VORTEX_SASL_EXTERNAL_USER_DATA, user_data);
	else
		vortex_log (VORTEX_LEVEL_WARNING,
			"Unsupported SASL profile used (%s), cannot associate user_data pointer", profile);

	/* configure default server handler for the context created */
	__vortex_sasl_set_default_listener_handler (connection);

	/* get sasl data */
	data = vortex_connection_get_data (connection, SASL_DATA);

	vortex_log (VORTEX_LEVEL_DEBUG, "begin initial server step");

	rc = gsasl_server_start (
		/* get the SASL context for this connection */
		data->ctx,
		/* a list of profiles selected (actually only one) */
		&(profile[26]), 
		/* the session object */
		&data->session);

	vortex_log (VORTEX_LEVEL_DEBUG, "getting error code (%d): %s", rc, gsasl_strerror (rc));

	/* check for sasl result */
	if (rc != GSASL_OK) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "Unable to initialize the first SASL step, code=%d, message='%s'",
		       rc, gsasl_strerror (rc));
		return false;
	}

	/* store SASL profile being negotiated */
	vortex_connection_set_data_full (connection, SASL_PROFILE_BEGIN_NEGOCIATED, axl_strdup (&(profile[26])), NULL, axl_free);

	vortex_log (VORTEX_LEVEL_DEBUG, "starting sasl server iteration on connection id=%d", 
		    vortex_connection_get_id (connection));

	if (!__vortex_sasl_server_iterate (connection, profile_content, profile_content_reply))
		return false;

	/* accept the channel to be created */
	return true;
}

void __vortex_sasl_accept_negociation_frame_receive (VortexChannel    * channel,
						     VortexConnection * connection,
						     VortexFrame      * frame,
						     axlPointer user_data)
{
	char      * payload_reply = NULL;
	VortexCtx * ctx           = vortex_connection_get_ctx (connection);

	vortex_log (VORTEX_LEVEL_DEBUG, "Received SASL frame\n");

	/* perform a SASL iteration */
	__vortex_sasl_server_iterate (connection, (char*) vortex_frame_get_payload (frame), &payload_reply);

	vortex_log (VORTEX_LEVEL_DEBUG, "SASL reply: %s", payload_reply);

	/* reply SASL iteration result */
	if (! vortex_channel_send_rpy (channel, payload_reply, strlen (payload_reply),
				       vortex_frame_get_msgno (frame))) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "unable to send SASL iteration reply");
	}

	/* free payload reply */
	axl_free (payload_reply);
	return;
}

#endif

/** 
 * @brief Allows to set current auth validation handler for the the SASL EXTERNAL profile.
 *
 * @param ctx The context where the operation will be performed.
 * @param auth_handler Auth handler to be executed when a query for EXTERNAL authentication is received.
 */
void           vortex_sasl_set_external_validation  (VortexCtx              * ctx, 
						     VortexSaslAuthExternal   auth_handler)
{
#ifndef ENABLE_SASL_SUPPORT
	vortex_log (VORTEX_LEVEL_WARNING, "Setting external validation handler on a Vortex Library instance without SASL support");
	return;
#else
	/* check reference received */
	if (ctx == NULL)
		return;

	ctx->sasl_external_auth_handler = auth_handler;
#endif
}

/** 
 * @brief Allows to set current auth validation handler for the the SASL EXTERNAL profile.
 *
 * @param ctx The context where the operation will be performed.
 * @param auth_handler Auth handler to be executed when a query for EXTERNAL authentication is received.
 */
void           vortex_sasl_set_external_validation_full  (VortexCtx * ctx, VortexSaslAuthExternalFull auth_handler)
{
#ifndef ENABLE_SASL_SUPPORT
	vortex_log (VORTEX_LEVEL_WARNING, "Setting external validation handler on a Vortex Library instance without SASL support");
	return;
#else
	/* check context received */
	if (ctx == NULL)
		return;

	ctx->sasl_external_auth_handler_full = auth_handler;
#endif
}

/** 
 * @brief Allows to set current auth validation handler for the the
 * SASL ANONYMOUS profile.
 *
 * @param ctx The context where the operation will be performed.
 * @param auth_handler Auth handler to be executed when a query for
 * ANONYMOUS authentication is received.
 */
void           vortex_sasl_set_anonymous_validation  (VortexCtx * ctx, VortexSaslAuthAnonymous auth_handler)
{
#ifndef ENABLE_SASL_SUPPORT
	vortex_log (VORTEX_LEVEL_WARNING, "Setting anonymous validation handler on a Vortex Library instance without SASL support");
	return;
#else
	/* check context received */
	if (ctx == NULL)
		return;

	ctx->sasl_anonymous_auth_handler = auth_handler;
#endif
}

/** 
 * @brief Allows to set current auth validation handler for the the
 * SASL ANONYMOUS profile.
 *
 * @param ctx The context where the operation will be performed.
 * @param auth_handler Auth handler to be executed when a query for
 * ANONYMOUS authentication is received.
 */
void           vortex_sasl_set_anonymous_validation_full  (VortexCtx * ctx, VortexSaslAuthAnonymousFull auth_handler)
{
#ifndef ENABLE_SASL_SUPPORT
	vortex_log (VORTEX_LEVEL_WARNING, "Setting anonymous validation handler on a Vortex Library instance without SASL support");
	return;
#else
	/* check context received */
	if (ctx == NULL)
		return;

	ctx->sasl_anonymous_auth_handler_full = auth_handler;
#endif
}

/** 
 * @brief Allows to set the validation handler to be used while
 * authenticating PLAIN SASL profile.
 * 
 * @param ctx The context where the operation will be performed.
 * @param auth_handler The handler to be executed.
 */
void               vortex_sasl_set_plain_validation      (VortexCtx * ctx, VortexSaslAuthPlain     auth_handler)
{
#ifndef ENABLE_SASL_SUPPORT
	vortex_log (VORTEX_LEVEL_WARNING, "Setting plain validation handler on a Vortex Library instance without SASL support");
	return;
#else
	/* check context received */
	if (ctx == NULL)
		return;

	ctx->sasl_plain_auth_handler = auth_handler;
#endif
}

/** 
 * @brief Allows to set the validation handler to be used while
 * authenticating PLAIN SASL profile.
 * 
 * @param ctx The context where the operation will be performed.
 * @param auth_handler The handler to be executed.
 */
void               vortex_sasl_set_plain_validation_full      (VortexCtx * ctx, VortexSaslAuthPlainFull     auth_handler)
{
#ifndef ENABLE_SASL_SUPPORT
	vortex_log (VORTEX_LEVEL_WARNING, "Setting plain validation handler on a Vortex Library instance without SASL support");
	return;
#else
	/* check context received */
	if (ctx == NULL)
		return;
	
	ctx->sasl_plain_auth_handler_full = auth_handler;
#endif
}

/** 
 * @brief Allows to set the validation handler to be used while authenticating CRAM-MD5 SASL profile.
 * 
 * @param ctx The context where the operation will be performed.
 * @param auth_handler The handler to be executed.
 */
void               vortex_sasl_set_cram_md5_validation      (VortexCtx * ctx, VortexSaslAuthCramMd5     auth_handler)
{
#ifndef ENABLE_SASL_SUPPORT
	vortex_log (VORTEX_LEVEL_WARNING, "Setting CRAM-MD5 validation handler on a Vortex Library instance without SASL support");
	return;
#else
	/* check context received */
	if (ctx == NULL)
		return;
	
	ctx->sasl_cram_md5_auth_handler = auth_handler;
#endif
}

/** 
 * @brief Allows to set the validation handler to be used while authenticating CRAM-MD5 SASL profile.
 * 
 * @param ctx The context where the operation will be performed.
 * @param auth_handler The handler to be executed.
 */
void               vortex_sasl_set_cram_md5_validation_full      (VortexCtx * ctx, VortexSaslAuthCramMd5Full     auth_handler)
{
#ifndef ENABLE_SASL_SUPPORT
	vortex_log (VORTEX_LEVEL_WARNING, "Setting CRAM-MD5 validation handler on a Vortex Library instance without SASL support");
	return;
#else
	/* check context received */
	if (ctx == NULL)
		return;

	ctx->sasl_cram_md5_auth_handler_full = auth_handler;
#endif
}

/** 
 * @brief Allows to set current auth validation handler for the the SASL DIGEST-MD5 profile.
 *
 * @param ctx The context where the operation will be performed.
 * @param auth_handler Auth handler to be executed when a query for DIGEST-MD5 authentication is received.
 */
void           vortex_sasl_set_digest_md5_validation  (VortexCtx * ctx, VortexSaslAuthDigestMd5 auth_handler)
{
#ifndef ENABLE_SASL_SUPPORT
	vortex_log (VORTEX_LEVEL_WARNING, "Setting DIGEST-MD5 validation handler on a Vortex Library instance without SASL support");
	return;
#else
	/* check context received */
	if (ctx == NULL)
		return;

	ctx->sasl_digest_md5_auth_handler = auth_handler;
#endif
}

/** 
 * @brief Allows to set current auth validation handler for the the SASL DIGEST-MD5 profile.
 *
 * @param ctx The context where the operation will be performed.
 * @param auth_handler Auth handler to be executed when a query for DIGEST-MD5 authentication is received.
 */
void           vortex_sasl_set_digest_md5_validation_full  (VortexCtx * ctx, VortexSaslAuthDigestMd5Full auth_handler)
{
#ifndef ENABLE_SASL_SUPPORT
	vortex_log (VORTEX_LEVEL_WARNING, "Setting DIGEST-MD5 validation handler on a Vortex Library instance without SASL support");
	return;
#else
	/* check context received */
	if (ctx == NULL)
		return;
	
	ctx->sasl_digest_md5_auth_handler_full = auth_handler;
#endif
}

/** 
 * @brief Allows to configure current Vortex Library process to accept
 * incoming SASL negotiations.
 *
 * This function allows to activate the SASL profile support selected
 * and to provide a handler that will be executed once received an
 * authentication request. There are two functions to activate SASL
 * profile support: this one and the previous API: \ref
 * vortex_sasl_accept_negociation.
 *
 * Both works the same way but this one is able to pass the user data
 * configured to the SASL auth handler once it is executed. Allowed
 * value for <b>mech</b> are:
 *
 *  - \ref VORTEX_SASL_ANONYMOUS
 *  - \ref VORTEX_SASL_EXTERNAL
 *  - \ref VORTEX_SASL_PLAIN
 *  - \ref VORTEX_SASL_CRAM_MD5
 *  - \ref VORTEX_SASL_DIGEST_MD5
 *
 * See \ref vortex_manual_sasl_for_server_side "Using SASL at server side" 
 * for a detailed explanation about SASL support inside Vortex
 * for the server side.
 * 
 * @param ctx The context where the operation will be performed.
 * @param mech The SASL mech to be accepted from remote BEEP peers.
 * @param user_data User-defined pointer to be passed to SASL callbacks.
 * 
 * @return true the mechanism was enabled to be accepted.
 */
bool               vortex_sasl_accept_negociation_full        (VortexCtx * ctx, char  * mech, axlPointer user_data)
{
#ifndef ENABLE_SASL_SUPPORT
	/* current Vortex Library doesn't support SASL */
	vortex_log (VORTEX_LEVEL_WARNING, "current Vortex Library doesn't have SASL support");
	return false;
#else
	/* check context received */
	if (ctx == NULL)
		return false;

	/* check the mech definition received */
	if (!axl_memcmp (mech, "http://iana.org/beep/SASL/", 26)) {
		vortex_log (VORTEX_LEVEL_CRITICAL, "you didn't provide a valid mech string specification.");
		return false;
	}

	/* register the profile received  */
	vortex_profiles_register (ctx, mech, 
				  /* do not set the normal channel
				   * start handler because it is need
				   * support to process profile content
				   * received. This is done by the
				   * extended channel start
				   * handler. This is actually set on
				   * the next sentence. */
				  NULL, NULL,
				  /* do not set the close handler,
				   * which implies to accept the
				   * channel to be closed whenever it
				   * is requested. However, negotiated
				   * authid will still remain for the
				   * context of the connection. */
				  NULL, NULL,
				  /* set frame received handlers to be
				   * able to handle and reply SASL
				   * frames received */
				  __vortex_sasl_accept_negociation_frame_receive, NULL);
	
	/* register an extended channel start handler for the mech registered */
	vortex_profiles_register_extended_start	 (ctx, mech, 
						  __vortex_sasl_accept_negociation_start, 
						  user_data);

	return true;
#endif	
}



/** 
 * @brief Allows to configure current Vortex Library process to accept
 * incoming SASL negotiations.
 *
 * See also \ref vortex_sasl_accept_negociation_full. Allowed value
 * for <b>mech</b> are:
 *
 *  - \ref VORTEX_SASL_ANONYMOUS
 *  - \ref VORTEX_SASL_EXTERNAL
 *  - \ref VORTEX_SASL_PLAIN
 *  - \ref VORTEX_SASL_CRAM_MD5
 *  - \ref VORTEX_SASL_DIGEST_MD5
 *
 * See \ref vortex_manual_sasl_for_server_side "Using SASL at server side" 
 * for a detailed explanation about SASL support inside Vortex
 * for the server side.
 * 
 * @param ctx The context where the operation will be performed.
 * @param mech The SASL mech to be accepted from remote BEEP peers.
 * 
 * @return true the mechanism was enabled to be accepted.
 */
bool               vortex_sasl_accept_negociation        (VortexCtx * ctx, char  * mech)
{
    return vortex_sasl_accept_negociation_full (ctx, mech, NULL);
}

/* @} */
