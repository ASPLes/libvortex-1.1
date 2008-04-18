/* 
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2008 Advanced Software Production Line, S.L.
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

bool               vortex_tls_is_enabled                 (VortexCtx            * ctx);

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

void               vortex_tls_start_negotiation          (VortexConnection     * connection,
							  const char           * serverName,
							  VortexTlsActivation    process_status,
							  axlPointer             user_data);

VortexConnection * vortex_tls_start_negotiation_sync     (VortexConnection  * connection,
							  const char        * serverName,
							  VortexStatus      * status,
							  char             ** status_message);

bool               vortex_tls_accept_negotiation         (VortexCtx         * ctx, 
							  VortexTlsAcceptQuery            accept_handler, 
							  VortexTlsCertificateFileLocator certificate_handler,
							  VortexTlsPrivateKeyFileLocator  private_key_handler);

axlPointer         vortex_tls_get_ssl_object             (VortexConnection * connection);

char             * vortex_tls_get_peer_ssl_digest        (VortexConnection   * connection, 
							  VortexDigestMethod   method);

char             * vortex_tls_get_digest                 (VortexDigestMethod   method,
							  const char         * string);

char             * vortex_tls_get_digest_sized           (VortexDigestMethod   method,
							  const char         * content,
							  int                  content_size);

void               vortex_tls_cleanup                    (VortexCtx * ctx);

#endif
/* @} */
