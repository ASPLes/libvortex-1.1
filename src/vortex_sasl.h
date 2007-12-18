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
 *         C/ Dr. Michavila Nº 14
 *         Coslada 28820 Madrid
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://fact.aspl.es
 */
#ifndef __VORTEX_SASL_H__
#define __VORTEX_SASL_H__

#include <vortex.h>

/**
 * \addtogroup vortex_sasl
 * @{
 */

/** 
 * @internal
 * @brief This is actually not a SASL profile. It is only used to
 * perform some internal checks.
 */
#define VORTEX_SASL_FAMILY        "http://iana.org/beep/SASL"
/** 
 * @brief ANONYMOUS profile identification to be used at \ref
 * vortex_sasl_start_auth or \ref vortex_sasl_accept_negociation.
 */
#define VORTEX_SASL_ANONYMOUS     "http://iana.org/beep/SASL/ANONYMOUS"
/** 
 * @brief EXTERNAL profile identification to be used at \ref
 * vortex_sasl_start_auth or \ref vortex_sasl_accept_negociation.
 */
#define VORTEX_SASL_EXTERNAL      "http://iana.org/beep/SASL/EXTERNAL"
/** 
 * @brief PLAIN profile identification to be used at \ref
 * vortex_sasl_start_auth or \ref vortex_sasl_accept_negociation.
 */
#define VORTEX_SASL_PLAIN         "http://iana.org/beep/SASL/PLAIN"
/** 
 * @brief CRAM-MD5 profile identification to be used at \ref
 * vortex_sasl_start_auth or \ref vortex_sasl_accept_negociation.
 */
#define VORTEX_SASL_CRAM_MD5      "http://iana.org/beep/SASL/CRAM-MD5"
/** 
 * @brief DIGEST-MD5 profile identification to be used at \ref
 * vortex_sasl_start_auth or \ref vortex_sasl_accept_negociation.
 */
#define VORTEX_SASL_DIGEST_MD5    "http://iana.org/beep/SASL/DIGEST-MD5"
/** 
 * @brief GSSAPI profile identification to be used at \ref
 * vortex_sasl_start_auth or \ref vortex_sasl_accept_negociation.
 * Currently GSSAPI is not supported by Vortex Library.
 */
#define VORTEX_SASL_GSSAPI        "http://iana.org/beep/SASL/GASSAPI"
/** 
 * @brief KERBEROS_V4 profile identification to be used at \ref
 * vortex_sasl_start_auth or \ref vortex_sasl_accept_negociation.
 * Currently KERBEROS_V4 is not supported by Vortex Library.
 */
#define VORTEX_SASL_KERBEROS_V4   "http://iana.org/beep/SASL/KERBEROS_V4"

/** 
 * @brief Key value to access the user-defined pointer associated with a connection
 * using SASL ANONYMOUS authentication method. Use vortex_connection_set_data
 * and vortex_connection_get_data to access the data. 
 * 
 * This data is configured at \ref
 * vortex_sasl_accept_negociation_full, and then passed to the handler
 * configured. Because the SASL module doesn't remove this data once
 * the authentication process have finished, you can still get access
 * to the data configured using this key.
 */
#define VORTEX_SASL_ANONYMOUS_USER_DATA  "__VORTEX_SASL_ANONYMOUS_USER_DATA"

/** 
 * @brief Key value to access the user-defined pointer associated with a connection
 * using SASL PLAIN authentication method. Use vortex_connection_set_data
 * and vortex_connection_get_data to access the data.
 *
 * This data is configured at \ref
 * vortex_sasl_accept_negociation_full, and then passed to the handler
 * configured. Because the SASL module doesn't remove this data once
 * the authentication process have finished, you can still get access
 * to the data configured using this key.
 */
#define VORTEX_SASL_PLAIN_USER_DATA  "__VORTEX_SASL_PLAIN_USER_DATA"

/** 
 * @brief Key value to access the user-defined pointer associated with a connection
 * using SASL EXTERNAL authentication method. Use vortex_connection_set_data
 * and vortex_connection_get_data to access the data.
 *
 * This data is configured at \ref
 * vortex_sasl_accept_negociation_full, and then passed to the handler
 * configured. Because the SASL module doesn't remove this data once
 * the authentication process have finished, you can still get access
 * to the data configured using this key.
 */
#define VORTEX_SASL_EXTERNAL_USER_DATA       "__VORTEX_SASL_EXTERNAL_USER_DATA"

/** 
 * @brief Key value to access the user-defined pointer associated with a connection
 * using SASL CRAM MD5 authentication method. Use vortex_connection_set_data
 * and vortex_connection_get_data to access the data.
 *
 * This data is configured at \ref
 * vortex_sasl_accept_negociation_full, and then passed to the handler
 * configured. Because the SASL module doesn't remove this data once
 * the authentication process have finished, you can still get access
 * to the data configured using this key.
 */
#define VORTEX_SASL_CRAM_MD5_USER_DATA       "__VORTEX_SASL_CRAM_MD5_USER_DATA"

/** 
 * @brief Key value to access the user-defined pointer associated with a connection
 * using SASL DIGEST MD5 authentication method. Use vortex_connection_set_data
 * and vortex_connection_get_data to access the data.
 *
 * This data is configured at \ref
 * vortex_sasl_accept_negociation_full, and then passed to the handler
 * configured. Because the SASL module doesn't remove this data once
 * the authentication process have finished, you can still get access
 * to the data configured using this key.
 */
#define VORTEX_SASL_DIGEST_MD5_USER_DATA       "__VORTEX_SASL_DIGEST_MD5_USER_DATA"

/** 
 * @brief Mark used by the sasl module to store the authorization id
 * used for a successful SASL negotation (that is the user login).
 */
#define SASL_AUTHID                     "sasl:authid"

/** 
 * @brief Mark used by the sasl module to store the proxy
 * authorization id. This value is used by some SASL profiles.
 */
#define SASL_AUTHZID                    "sasl:authzid"

/** 
 * @internal Value used by the sasl module to but not exported as a
 * mark because this value is deleted once the SASL auth mechanism was
 * completed.
 */
#define SASL_PASSWORD                   "sasl:password"

/** 
 * @brief Mark used to store the realm value used by some SASL
 * mechanism.
 */
#define SASL_REALM                      "sasl:realm"

/** 
 * @brief Mark used to store the anonymous token provided to comple
 * the SASL ANONYMOUS mechanism.
 */
#define SASL_ANONYMOUS_TOKEN            "sasl:anonymous:token"

/** 
 * @brief Mark used to check if a connection was completely
 * authenticated. This mark is especially used by turbulence to check
 * if a particular SASL channel not only exists but also has completed
 * its auth process.
 */
#define SASL_IS_AUTHENTICATED           "sasl:is:authenticated"

/** 
 * @brief Mark used to store the SASL mechanism used.
 */
#define SASL_METHOD_USED                "sasl:method:used"


/** 
 * @brief Set of properties to be used to configure client authentication. 
 *
 * This properties allows to configure the user id, password,
 * authorization id, etc, that are to be used while performing SASL negotiations. 
 *
 * These properties are set to or get from a connection using \ref
 * vortex_sasl_set_propertie and \ref vortex_sasl_get_propertie.
 */
typedef enum {
	/** 
	 * @brief User identifier to be used while SASL negotiation is running. An example of this value could be: "bob".
	 */
	VORTEX_SASL_AUTH_ID           = 1,
	/** 
	 * @brief Inside SASL definition, there is support to authenticate as "bob" and act as "alice". 
	 */
	VORTEX_SASL_AUTHORIZATION_ID  = 2,
	/** 
	 * @brief User password to be used while SASL negotiation is running. An example of this value could be: "secret".
	 */
	VORTEX_SASL_PASSWORD          = 3,
	/** 
	 * @brief Some SASL mechanism requires realm value to be defined. This allows to have a set of users to be authenticated according to the realm provided.
	 */
	VORTEX_SASL_REALM             = 4,
	/** 
	 * @brief Allows to configure anonymous token while using ANONYMOUS profile. This propertie is especific for ANONYMOUS profile.
	 */
	VORTEX_SASL_ANONYMOUS_TOKEN   = 5,
	/** 
	 * @internal
	 * The following value must be always be the last. While adding new properties, they must go before this one.
	 */
	VORTEX_SASL_PROP_NUM          = 6
} VortexSaslProperties;

/** 
 * @brief Convenience macro that allows to get the auth Id for the
 * connection that is running the channel provided.
 *
 * The macro also check if the connection holding the channel was
 * authenticated (by calling to \ref
 * vortex_sasl_is_authenticated). The macro doesn't check the sasl
 * method used or if it is appropiated to your context. You must use
 * other means to check/enforce SASL method allowed.
 *
 * See also: \ref AUTH_ID_FROM_CONN. 
 *
 * @param channel The channel that is supposed to be running inside a
 * connection autenticated.
 * 
 * @return The auth id or NULL if it fails.
 */
#define AUTH_ID_FROM_CHANNEL(channel) (vortex_sasl_is_authenticated (vortex_channel_get_connection (channel)) ? vortex_sasl_get_propertie (vortex_channel_get_connection (channel), VORTEX_SASL_AUTH_ID) : NULL)

/** 
 * @brief Convenience macro that allows to get the auth Id for the
 * connection provided.
 *
 * The macro also check if the connection was authenticated (by
 * calling to \ref vortex_sasl_is_authenticated). The macro doesn't
 * check the sasl method used or if it is appropiated to your
 * context. You must use other means to check/enforce SASL method
 * allowed.
 *
 * See also: \ref AUTH_ID_FROM_CONN. 
 *
 * @param conn The connection to be checked for its auth id
 * associated.
 * 
 * @return The auth id or NULL if it fails.
 */
#define AUTH_ID_FROM_CONN(conn) (vortex_sasl_is_authenticated (conn) ? vortex_sasl_get_propertie (conn, VORTEX_SASL_AUTH_ID) : NULL)

bool               vortex_sasl_is_enabled                ();

bool               vortex_sasl_set_propertie             (VortexConnection     * connection,
							  VortexSaslProperties   prop,
							  char                 * value,
							  axlDestroyFunc         value_destroy);

char             * vortex_sasl_get_propertie             (VortexConnection     * connection,
							  VortexSaslProperties   prop);

bool               vortex_sasl_is_authenticated          (VortexConnection     * connection);

char             * vortex_sasl_auth_method_used          (VortexConnection     * connection);


void               vortex_sasl_start_auth                (VortexConnection     * connection,
							  const char           * profile, 
							  VortexSaslAuthNotify   process_status,
							  axlPointer             user_data);

void               vortex_sasl_start_auth_sync           (VortexConnection     * connection,
							  const char           * profile,
							  VortexStatus         * status,
							  char                ** status_message);

void               vortex_sasl_set_external_validation        (VortexCtx * ctx, VortexSaslAuthExternal  auth_handler);
void               vortex_sasl_set_external_validation_full   (VortexCtx * ctx, VortexSaslAuthExternalFull  auth_handler);

void               vortex_sasl_set_anonymous_validation       (VortexCtx * ctx, VortexSaslAuthAnonymous auth_handler);
void               vortex_sasl_set_anonymous_validation_full  (VortexCtx * ctx, VortexSaslAuthAnonymousFull auth_handler);

void               vortex_sasl_set_plain_validation           (VortexCtx * ctx, VortexSaslAuthPlain     auth_handler);
void               vortex_sasl_set_plain_validation_full      (VortexCtx * ctx, VortexSaslAuthPlainFull     auth_handler);

void               vortex_sasl_set_cram_md5_validation        (VortexCtx * ctx, VortexSaslAuthCramMd5   auth_handler);
void               vortex_sasl_set_cram_md5_validation_full   (VortexCtx * ctx, VortexSaslAuthCramMd5Full   auth_handler);

void               vortex_sasl_set_digest_md5_validation      (VortexCtx * ctx, VortexSaslAuthDigestMd5 auth_handler);
void               vortex_sasl_set_digest_md5_validation_full (VortexCtx * ctx, VortexSaslAuthDigestMd5Full auth_handler);


bool               vortex_sasl_accept_negociation             (VortexCtx * ctx, 
							       char      * mech);
bool               vortex_sasl_accept_negociation_full        (VortexCtx * ctx,
							       char      * mech, 
							       axlPointer user_data);

       
#endif

/* @} */
