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
#ifndef __VORTEX_LISTENER_H__
#define __VORTEX_LISTENER_H__

#include <vortex.h>

/**
 * \addtogroup vortex_listener
 * @{
 */

VortexConnection * vortex_listener_new             (VortexCtx            * ctx,
						    const char           * host, 
						    const char           * port, 
						    VortexListenerReady    on_ready, 
						    axlPointer             user_data);

VortexConnection * vortex_listener_new6            (VortexCtx            * ctx,
						    const char           * host, 
						    const char           * port, 
						    VortexListenerReady    on_ready, 
						    axlPointer             user_data);

VortexConnection * vortex_listener_new2            (VortexCtx           * ctx,
						    const char          * host,
						    int                   port,
						    VortexListenerReady   on_ready, 
						    axlPointer            user_data);

VortexConnection * vortex_listener_new_full        (VortexCtx                * ctx,
						    const char               * host,
						    const char               * port,
						    VortexListenerReadyFull    on_ready_full, 
						    axlPointer                 user_data);

VortexConnection * vortex_listener_new_full2       (VortexCtx                * ctx,
						    const char               * host,
						    const char               * port,
						    axl_bool                   register_conn,
						    VortexListenerReadyFull    on_ready_full, 
						    axlPointer                 user_data);

VortexConnection * vortex_listener_new_full6       (VortexCtx                * ctx,
						    const char               * host,
						    const char               * port,
						    axl_bool                   register_conn,
						    VortexListenerReadyFull    on_ready_full, 
						    axlPointer                 user_data);

VORTEX_SOCKET     vortex_listener_sock_listen      (VortexCtx   * ctx,
						    const char  * host,
						    const char  * port,
						    axlError   ** error);

VORTEX_SOCKET     vortex_listener_sock_listen6     (VortexCtx   * ctx,
						    const char  * host,
						    const char  * port,
						    axlError   ** error);

void          vortex_listener_accept_connections   (VortexCtx        * ctx,
						    int                server_socket,
						    VortexConnection * listener);

void          vortex_listener_accept_connection    (VortexConnection * connection, 
						    axl_bool           send_greetings);

VORTEX_SOCKET vortex_listener_accept               (VORTEX_SOCKET server_socket);

void          __vortex_listener_second_step_accept (VortexFrame * frame, 
						    VortexConnection * connection);

void          vortex_listener_complete_register    (VortexConnection     * connection);

void          vortex_listener_wait                 (VortexCtx * ctx);

void          vortex_listener_unlock               (VortexCtx * ctx);

void          vortex_listener_init                 (VortexCtx * ctx);

void          vortex_listener_cleanup              (VortexCtx * ctx);

void          vortex_listener_send_greetings_on_connect (VortexConnection * listener, 
							 axl_bool           send_on_connect);

axl_bool      vortex_listener_parse_conf_and_start (VortexCtx * ctx);

void          vortex_listener_set_default_realm    (VortexCtx   * ctx,
						    const char  * realm);

const char  * vortex_listener_get_default_realm    (VortexCtx   * ctx);

void          vortex_listener_set_on_connection_accepted (VortexCtx                  * ctx,
							  VortexOnAcceptedConnection   on_accepted, 
							  axlPointer                   data);

axlPointer    vortex_listener_set_port_sharing_handling (VortexCtx               * ctx, 
							 const char              * local_addr,
							 const char              * local_port, 
							 VortexPortShareHandler    handler,
							 axlPointer                user_data);

void          vortex_listener_shutdown (VortexConnection * listener,
					axl_bool           also_created_conns);

/*** internal API ***/
VortexConnection * __vortex_listener_initial_accept (VortexCtx            * ctx,
						     VORTEX_SOCKET          client_socket, 
						     VortexConnection     * listener,
						     axl_bool               dont_register);

VortexConnection * __vortex_listener_initial_accept_full (VortexCtx            * ctx,
							  VORTEX_SOCKET          client_socket, 
							  VortexConnection     * listener,
							  axl_bool               register_conn,
							  axl_bool               skip_naming);

axl_bool __vortex_listener_check_port_sharing (VortexCtx * ctx, VortexConnection * connection);


/* @} */

#endif
