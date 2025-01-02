/* 
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2025 Advanced Software Production Line, S.L.
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
#ifndef __VORTEX_WEBSOCKET_H__
#define __VORTEX_WEBSOCKET_H__

#include <vortex.h>
#include <nopoll.h>

BEGIN_C_DECLS

/** 
 * \addtogroup vortex_websocket
 * @{
 */

/** 
 * @brief Connection setup object. Allows to configure additional
 * settings required to perform a connection by using \ref
 * vortex_websocket_connection_new.
 */
typedef struct _VortexWebsocketSetup VortexWebsocketSetup;

/** 
 * @brief Configurations allowed to be set on \ref VortexWebsocketSetup.
 */
typedef enum {
	/** 
	 * @brief Allows to configure where is located the host
	 * running the WEBSOCKET proxy with CONNECT support.
	 */
	VORTEX_WEBSOCKET_CONF_ITEM_PROXY_HOST = 1,
	/** 
	 * @brief Allows to configure on which port is running the
	 * WEBSOCKET proxy with CONNECT support.
	 */
	VORTEX_WEBSOCKET_CONF_ITEM_PROXY_PORT = 2,
	/** 
	 * @brief Optional connection options reference (\ref
	 * VortexConnectionOpts, \ref vortex_connection_opts_new) to
	 * be used on connection setup.
	 */
	VORTEX_WEBSOCKET_CONF_ITEM_CONN_OPTS  = 3,
	/** 
	 * @brief Optional connection Origin header to be passed to
	 * the WebSocket initial handshake.
	 */
	VORTEX_WEBSOCKET_CONF_ITEM_ORIGIN = 4,
	/** 
	 * @brief Optional connection Host header to be passed to the
	 * WebSocket initial handshake. If nothing is defined, the
	 * connection's host value is used.
	 */
	VORTEX_WEBSOCKET_CONF_ITEM_HOST = 5,
	/** 
	 * @brief Optional connection flag that allows indicating the
	 * connection creation that the TLS protocol must be enabled
	 * first before proceeding (support wss://).
	 */
	VORTEX_WEBSOCKET_CONF_ITEM_ENABLE_TLS = 6,
	/** 
	 * @brief Allows to to enable debug at noPollCtx context
	 * object created by the connection. By default is disabled.
	 */
	VORTEX_WEBSOCKET_CONF_ITEM_ENABLE_DEBUG = 7,
	/** 
	 * @brief Allows to control if certificate verification should
	 * be enabled or disabled. By default, certificate
	 * verification is enabled.
	 */ 
	VORTEX_WEBSOCKET_CONF_CERT_VERIFY = 8
} VortexWebsocketConfItem;

VortexWebsocketSetup  * vortex_websocket_setup_new      (VortexCtx * ctx);

axl_bool           vortex_websocket_setup_ref      (VortexWebsocketSetup * setup);

void               vortex_websocket_setup_unref    (VortexWebsocketSetup * setup);

void               vortex_websocket_setup_conf     (VortexWebsocketSetup      * setup,
						    VortexWebsocketConfItem     item,
						    axlPointer             str_value);
					       

VortexConnection * vortex_websocket_connection_new (const char                * host, 
						    const char                * port,
						    VortexWebsocketSetup      * setup,
						    VortexConnectionNew         on_connected, 
						    axlPointer                  user_data);

VortexConnection * vortex_websocket_listener_new   (VortexCtx                * ctx,
						    noPollConn               * listener,
						    VortexListenerReadyFull    on_ready_full,
						    axlPointer                 user_data);

noPollCtx        * vortex_websocket_connection_get_ctx (VortexConnection * conn);

axl_bool           vortex_websocket_connection_is  (VortexConnection * conn);

axl_bool           vortex_websocket_connection_is_tls_running (VortexConnection * conn);

axlPointer         vortex_websocket_listener_port_sharing (VortexCtx  * ctx, 
							   noPollCtx  * nopoll_ctx,
							   const char * local_addr, 
							   const char * local_port);

END_C_DECLS

#endif

/* @} */
