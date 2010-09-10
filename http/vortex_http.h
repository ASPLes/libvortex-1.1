/* 
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2010 Advanced Software Production Line, S.L.
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
#ifndef __VORTEX_HTTP_H__
#define __VORTEX_HTTP_H__

#include <vortex.h>

BEGIN_C_DECLS

/** 
 * \addtogroup vortex_http
 * @{
 */

/** 
 * @brief Connection setup object. Allows to configure additional
 * settings required to perform a connection by using \ref
 * vortex_http_connection_new.
 */
typedef struct _VortexHttpSetup VortexHttpSetup;

/** 
 * @brief Configurations allowed to be set on \ref VortexHttpSetup.
 */
typedef enum {
	/** 
	 * @brief Allows to configure where is located the host
	 * running the HTTP proxy with CONNECT support.
	 */
	VORTEX_HTTP_CONF_ITEM_PROXY_HOST = 1,
	/** 
	 * @brief Allows to configure on which port is running the
	 * HTTP proxy with CONNECT support.
	 */
	VORTEX_HTTP_CONF_ITEM_PROXY_PORT = 2,
	/** 
	 * @brief Optional connection options reference (\ref
	 * VortexConnectionOpts, \ref vortex_connection_opts_new) to
	 * be used on connection setup.
	 */
	VORTEX_HTTP_CONF_ITEM_CONN_OPTS  = 3
} VortexHttpConfItem;

VortexHttpSetup  * vortex_http_setup_new      (VortexCtx * ctx);

axl_bool           vortex_http_setup_ref      (VortexHttpSetup * setup);

void               vortex_http_setup_unref    (VortexHttpSetup * setup);

void               vortex_http_setup_conf     (VortexHttpSetup      * setup,
					       VortexHttpConfItem     item,
					       axlPointer             str_value);
					       

VortexConnection * vortex_http_connection_new (const char           * host, 
					       const char           * port,
					       VortexHttpSetup      * setup,
					       VortexConnectionNew    on_connected, 
					       axlPointer             user_data);

axl_bool           vortex_http_connection_is_proxied (VortexConnection * conn);

END_C_DECLS

#endif

/* @} */
