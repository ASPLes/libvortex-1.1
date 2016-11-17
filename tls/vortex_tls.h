/* 
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2016 Advanced Software Production Line, S.L.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this program; if not, write to the Free
 *  Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307 USA
 *  
 *  You may find a copy of the license under this software is released
 *  at COPYING file. This is LGPL software: you are welcome to develop
 *  proprietary applications using this library without any royalty or
 *  fee but returning back any change, improvement or addition in the
 *  form of source code, project image, documentation patches, etc.
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
#ifndef __VORTEX_TLS_H__
#define __VORTEX_TLS_H__

#include <vortex.h>

BEGIN_C_DECLS

/**
 * \addtogroup vortex_tls 
 * @{ 
 */

/** 
 * @brief Digest method provided.
 */
typedef enum {
	/** 
	 * @brief Allows to especify the DIGEST method SHA-1.
	 */
	VORTEX_SHA1 = 1,
	/** 
	 * @brief Allows to especify the DIGEST method MD5.
	 */
	VORTEX_MD5 = 2,
	/** 
	 * @internal Internal value. Do not modify.
	 */
	VORTEX_DIGEST_NUM
} VortexDigestMethod;

/** 
 * @brief TLS Profile unique URI identifier.
 */
#define VORTEX_TLS_PROFILE_URI "http://iana.org/beep/TLS"

/** 
 * @brief Async notifications for TLS activation.
 *
 * Once the process for TLS negotiation have started, using \ref
 * vortex_tls_start_negotiation function, the status for such process is notified
 * using this handler type definition.
 *
 * The <i>status</i> value have to be checked in order to know if the
 * transport negotiation have finished successfully. Along the previous
 * variable, the <i>status_message</i> have a textual diagnostic about
 * the current status received.
 * 
 * While invoking \ref vortex_tls_start_negotiation you could provide an user
 * space pointer, using the <i>user_data</i> parameter. That user data
 * is received on this handler.
 *
 * Functions using this handler:
 *  \ref vortex_tls_start_negotiation
 * 
 * 
 * @param connection The connection where the TLS activation status is
 * being notified.
 *
 * @param status The process status.
 *
 * @param status_message A textual message representing the process
 * status.
 *
 * @param user_data A user defined pointer established at function
 * which received this handler.
 */
typedef void     (*VortexTlsActivation)          (VortexConnection * connection,
						  VortexStatus       status,
						  char             * status_message,
						  axlPointer         user_data);

/** 
 * @brief Handler definition for those function used to configure if a
 * given TLS request should be accepted or denied.
 *
 * Once a TLS request is received this handler will be executed to
 * notify user space application on top of Vortex Library if the request should be denied or not. The
 * handler will receive the connection where the request was received
 * and an optional value serverName which represents a request to act as 
 * the server name the value represent.
 * 
 * This handler definition is used by:
 *   - \ref vortex_tls_accept_negotiation
 * 
 * @param connection The connection where the TLS request was received.
 * @param serverName Optional serverName value requesting, if defined, to act as the server defined by this value.
 * 
 * @return axl_true if the TLS request should be accepted. axl_false to deny
 * activating TLS for this session.
 */
typedef axl_bool      (*VortexTlsAcceptQuery) (VortexConnection * connection,
					       const char       * serverName);

/** 
 * @brief Handler definition for those functions that allows to locate
 * the certificate file to be used while enabling TLS support.
 * 
 * Once a TLS negotiation is started at least two files are required
 * to enable TLS cyphering: the certificate and the private key. Two
 * handlers are used by the Vortex Library to allow user app level to
 * configure file locations for both files.
 * 
 * This handler is used to configure location for the certificate
 * file. The function will receive the connection where the TLS is
 * being request to be activated and the serverName value which hold a
 * optional host name value requesting to act as the server configured
 * by this value.
 * 
 * The function must return a path to the certificate using a
 * dynamically allocated value or the certificate content itself. Once
 * finished, Vortex Library will unref it.
 * 
 * <b>The function should return a basename file avoiding full path file
 * names</b>. This is because the Vortex Library will use \ref
 * vortex_support_find_data_file function to locate the file
 * provided. That function is configured to lookup on the configured
 * search path provided by \ref vortex_support_add_search_path or \ref
 * vortex_support_add_search_path_ref.
 * 
 * As a consequence: 
 * 
 * - If all certificate files are located at
 *  <b>/etc/repository/certificates</b> and the <b>serverName.cert</b> is to
 *   be used <b>DO NOT</b> return on this function <b>/etc/repository/certificates/serverName.cert</b>
 *
 * - Instead, configure <b>/etc/repository/certificates</b> at \ref
 *    vortex_support_add_search_path and return <b>servername.cert</b>.
 * 
 * - Doing previous practice will allow your code to be as
 *   platform/directory-structure independent as possible. The same
 *   function works on every installation, the only question to be
 *   configured are the search paths to lookup.
 *  
 * 
 * @param connection The connection where the TLS negotiation was
 * received.
 *
 * @param serverName An optional value requesting to act as the server
 * <b>serverName</b>. This value is supposed to be used to select the
 * right certificate file (according to the common value stored on
 * it).
 * 
 * This handler is used by:
 *  - \ref vortex_tls_accept_negotiation 
 * 
 * @return A newly allocated value containing the path to the
 * certificate file or the certificate content to be used.
 */
typedef char  * (* VortexTlsCertificateFileLocator) (VortexConnection * connection,
						     const char       * serverName);

/** 
 * @brief Handler definition for those functions that allows to locate
 * the private key file to be used while enabling TLS support.
 * 
 * See \ref VortexTlsCertificateFileLocator handler. This handler
 * allows to define how is located the private key file used for the
 * session TLS-fication.
 * 
 * This handler is used by:
 *  - \ref vortex_tls_accept_negotiation 
 * 
 * @return A newly allocated value containing the path to the private
 * key file or content itself to be used.
 */
typedef char  * (* VortexTlsPrivateKeyFileLocator) (VortexConnection * connection,
						    const char       * serverName);

/** 
 * @brief Handler definition used by the TLS profile, to allow the
 * application level to provide the function that must be executed to
 * create an (SSL_CTX *) object, used to perform the TLS activation.
 *
 * This handler is used by: 
 *  - \ref vortex_tls_set_ctx_creation
 *  - \ref vortex_tls_set_default_ctx_creation
 *
 * By default the Vortex TLS implementation will use its own code to
 * create the SSL_CTX object if not provided the handler. However,
 * such code is too general, so it is recomended to provide your own
 * context creation.
 *
 * Inside this function you must configure all your stuff to tweak the
 * OpenSSL behaviour. Here is an example:
 * 
 * \code
 * axlPointer * __ctx_creation (VortexConnection * conection,
 *                              axlPointer         user_data)
 * {
 *     SSL_CTX * ctx;
 *
 *     // create the context using the TLS method (for client side)
 *     ctx = SSL_CTX_new (TLSv1_method ());
 *
 *     // configure the root CA and its directory to perform verifications
 *     if (SSL_CTX_load_verify_locations (ctx, "your-ca-file.pem", "you-ca-directory")) {
 *         // failed to configure SSL_CTX context 
 *         SSL_CTX_free (ctx);
 *         return NULL;
 *     }
 *     if (SSL_CTX_set_default_verify_paths () != 1) {
 *         // failed to configure SSL_CTX context 
 *         SSL_CTX_free (ctx);
 *         return NULL;
 *     }
 *
 *     // configure the client certificate (public key)
 *     if (SSL_CTX_use_certificate_chain_file (ctx, "your-client-certificate.pem")) {
 *         // failed to configure SSL_CTX context 
 *         SSL_CTX_free (ctx);
 *         return NULL;
 *     }
 *
 *     // configure the client private key 
 *     if (SSL_CTX_use_PrivateKey_file (ctx, "your-client-private-key.rpm", SSL_FILETYPE_PEM)) {
 *         // failed to configure SSL_CTX context 
 *         SSL_CTX_free (ctx);
 *         return NULL;
 *     }
 *
 *     // set the verification level for the client side
 *     SSL_CTX_set_verify (ctx, SSL_VERIFY_PEER, NULL);
 *     SSL_CTX_set_verify_depth(ctx, 4);
 *
 *     // our ctx is configured
 *     return ctx;
 * }
 * \endcode
 *
 * For the server side, the previous example mostly works, but you
 * must reconfigure the call to SSL_CTX_set_verify, providing
 * something like this:
 * 
 * \code
 *    SSL_CTX_set_verify (ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
 * \endcode
 *
 * See OpenSSL documenation for SSL_CTX_set_verify and SSL_CTX_set_verify_depth.
 * 
 * @param connection The connection that has been requested to be
 * activated the TLS profile, for which a new SSL_CTX must be created. 
 * 
 * @param user_data An optional user pointer defined at either \ref
 * vortex_tls_set_default_ctx_creation and \ref
 * vortex_tls_set_ctx_creation.
 * 
 * @return You must return a newly allocated SSL_CTX or NULL if the
 * handler must signal that the TLS activation must not be performed.
 */
typedef axlPointer (* VortexTlsCtxCreation) (VortexConnection * connection,
					     axlPointer         user_data);


/** 
 * @brief Allows to configure a post-condition function to be executed
 * to perform additional checkings.
 *
 * This handler is used by:
 * 
 *  - \ref vortex_tls_set_post_check
 *  - \ref vortex_tls_set_default_post_check
 *
 * The function must return axl_true to signal that checkings was
 * passed, otherwise axl_false must be returned. In such case, the
 * connection will be dropped.
 * 
 * @param connection The connection that was TLS-fixated and
 * additional checks were configured.
 * 
 * @param user_data User defined data passed to the function, defined
 * at \ref vortex_tls_set_post_check and \ref
 * vortex_tls_set_default_post_check.
 *
 * @param ssl The SSL object created for the process.
 * 
 * @param ctx The SSL_CTX object created for the process.
 * 
 * @return axl_true to accept the connection, otherwise, axl_false must be
 * returned.
 */
typedef axl_bool  (*VortexTlsPostCheck) (VortexConnection * connection, 
					 axlPointer         user_data, 
					 axlPointer         ssl, 
					 axlPointer         ctx);


/** 
 * @brief Handler called when a failure is found during TLS
 * handshake. 
 *
 * The function receives the connection where the failure * found an
 * error message and a pointer configured by the user at \ref vortex_tls_set_failure_handler.
 *
 * @param connection The connection where the failure was found.
 *
 * @param error_message The error message describing the problem found.
 *
 * @param user_data Optional user defined pointer.
 *
 * To get particular SSL info, you can use the following code inside the handler:
 * \code
 * // error variables
 * char          log_buffer [512];
 * unsigned long err;
 *
 * // show errors found
 * while ((err = ERR_get_error()) != 0) {
 *     ERR_error_string_n (err, log_buffer, sizeof (log_buffer));
 *     printf ("tls stack: %s (find reason(code) at openssl/ssl.h)", log_buffer);
 * }
 * \endcode
 * 
 *
 */
typedef void      (*VortexTlsFailureHandler) (VortexConnection * connection,
					      const char       * error_message,
					      axlPointer         user_data);


axl_bool           vortex_tls_init                       (VortexCtx            * ctx);

void               vortex_tls_set_ctx_creation           (VortexConnection     * connection,
							  VortexTlsCtxCreation   ctx_creation, 
							  axlPointer             user_data);

void               vortex_tls_set_default_ctx_creation   (VortexCtx            * ctx,
							  VortexTlsCtxCreation   ctx_creation, 
							  axlPointer             user_data);

void               vortex_tls_set_post_check             (VortexConnection     * connection,
							  VortexTlsPostCheck     post_check,
							  axlPointer             user_data);

void               vortex_tls_set_default_post_check     (VortexCtx            * ctx, 
							  VortexTlsPostCheck     post_check,
							  axlPointer             user_data);

void               vortex_tls_set_failure_handler        (VortexCtx               * ctx,
							  VortexTlsFailureHandler   failure_handler,
							  axlPointer                user_data);

axl_bool           vortex_tls_verify_cert                (VortexConnection     * connection);

void               vortex_tls_start_negotiation          (VortexConnection     * connection,
							  const char           * serverName,
							  VortexTlsActivation    process_status,
							  axlPointer             user_data);

VortexConnection * vortex_tls_start_negotiation_sync     (VortexConnection  * connection,
							  const char        * serverName,
							  VortexStatus      * status,
							  char             ** status_message);

void               vortex_tls_set_auto_tls               (VortexCtx         * ctx,
							  axl_bool            enabled,
							  axl_bool            allow_tls_failures,
							  const char        * serverName);

axl_bool           vortex_tls_accept_negotiation         (VortexCtx         * ctx, 
							  VortexTlsAcceptQuery            accept_handler, 
							  VortexTlsCertificateFileLocator certificate_handler,
							  VortexTlsPrivateKeyFileLocator  private_key_handler);

axlPointer         vortex_tls_get_ssl_object             (VortexConnection * connection);

char             * vortex_tls_get_peer_ssl_digest        (VortexConnection   * connection, 
							  VortexDigestMethod   method);

char            * vortex_tls_get_ssl_digest              (const char         * path,
							  VortexDigestMethod   method);

char             * vortex_tls_get_digest                 (VortexDigestMethod   method,
							  const char         * string);

char             * vortex_tls_get_digest_sized           (VortexDigestMethod   method,
							  const char         * content,
							  int                  content_size);

void               vortex_tls_cleanup                    (VortexCtx * ctx);

/* internal API */
axl_bool           vortex_tls_auto_tlsfixate_connection   (VortexCtx               * ctx,
							   VortexConnection        * connection,
							   VortexConnection       ** new_conn,
							   VortexConnectionStage    stage,
							   axlPointer               user_data);

END_C_DECLS

#endif
/* @} */
