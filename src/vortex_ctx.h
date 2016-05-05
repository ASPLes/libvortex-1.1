/*
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2016 Advanced Software Production Line, S.L.
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
 *         C/ Antonio Suarez Nº 10, 
 *         Edificio Alius A, Despacho 102
 *         Alcalá de Henares 28802 (Madrid)
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.aspl.es/vortex
 */
#ifndef __VORTEX_CTX_H__
#define __VORTEX_CTX_H__

#include <vortex.h>

BEGIN_C_DECLS

VortexCtx * vortex_ctx_new                       (void);

void        vortex_ctx_set_data                  (VortexCtx       * ctx, 
						  const char      * key, 
						  axlPointer        value);

void        vortex_ctx_set_data_full             (VortexCtx       * ctx, 
						  const char      * key, 
						  axlPointer        value,
						  axlDestroyFunc    key_destroy,
						  axlDestroyFunc    value_destroy);

axlPointer  vortex_ctx_get_data                  (VortexCtx       * ctx,
						  const char      * key);

void        vortex_ctx_close_conn_on_write_timeout (VortexCtx      * ctx,
						    axl_bool         disable);

void        vortex_ctx_write_timeout               (VortexCtx      * ctx,
						    int              timeout);

/*** global event notificaitons ***/
void        vortex_ctx_set_frame_received        (VortexCtx             * ctx,
						  VortexOnFrameReceived   received,
						  axlPointer              received_user_data);

void        vortex_ctx_set_close_notify_handler  (VortexCtx                  * ctx,
						  VortexOnNotifyCloseChannel   close_notify,
						  axlPointer                   user_data);

void        vortex_ctx_set_channel_added_handler (VortexCtx                       * ctx,
						  VortexConnectionOnChannelUpdate   added_handler,
						  axlPointer                        user_data);

void        vortex_ctx_set_channel_removed_handler (VortexCtx                       * ctx,
						    VortexConnectionOnChannelUpdate   removed_handler,
						    axlPointer                        user_data);

void        vortex_ctx_set_channel_start_handler (VortexCtx                       * ctx,
						  VortexOnStartChannelExtended      start_handler,
						  axlPointer                        start_handler_data);

void        vortex_ctx_set_idle_handler          (VortexCtx                       * ctx,
						  VortexIdleHandler                 idle_handler,
						  long                              max_idle_period,
						  axlPointer                        user_data,
						  axlPointer                        user_data2);

void        vortex_ctx_notify_idle               (VortexCtx                       * ctx,
						  VortexConnection                * conn);

void        vortex_ctx_install_cleanup           (VortexCtx * ctx,
						  axlDestroyFunc cleanup);

void        vortex_ctx_remove_cleanup            (VortexCtx * ctx,
						  axlDestroyFunc cleanup);

void        vortex_ctx_server_name_acquire       (VortexCtx * ctx,
						  axl_bool    status);

void        vortex_ctx_ref                       (VortexCtx  * ctx);

void        vortex_ctx_ref2                      (VortexCtx  * ctx, const char * who);

void        vortex_ctx_unref                     (VortexCtx ** ctx);

void        vortex_ctx_unref2                    (VortexCtx ** ctx, const char * who);

int         vortex_ctx_ref_count                 (VortexCtx  * ctx);

void        vortex_ctx_free                      (VortexCtx * ctx);

void        vortex_ctx_free2                     (VortexCtx * ctx, const char * who);

void        vortex_ctx_set_on_finish        (VortexCtx              * ctx,
					     VortexOnFinishHandler    finish_handler,
					     axlPointer               user_data);

void        vortex_ctx_check_on_finish      (VortexCtx * ctx);

void        vortex_ctx_set_client_conn_created (VortexCtx * ctx, 
						VortexClientConnCreated conn_created,
						axlPointer              user_data);

void        vortex_ctx_reinit (VortexCtx * ctx);

void        __vortex_ctx_set_cleanup (VortexCtx * ctx);

END_C_DECLS

#endif /* __VORTEX_CTX_H__ */
