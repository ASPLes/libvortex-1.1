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

#ifndef __VORTEX_CHANNEL_POOL_H__
#define __VORTEX_CHANNEL_POOL_H__

#include <vortex.h>

VortexChannelPool * vortex_channel_pool_new               (VortexConnection           * connection,
							   const char                 * profile,
							   int                          init_num,
							   VortexOnCloseChannel         close,
							   axlPointer                   close_user_data,
							   VortexOnFrameReceived        received,
							   axlPointer                   received_user_data,
							   VortexOnChannelPoolCreated   on_channel_pool_created,
							   axlPointer                   user_data);

VortexChannelPool * vortex_channel_pool_new_full          (VortexConnection          * connection,
							   const char                * profile,
							   int                         init_num,
							   VortexChannelPoolCreate     create_channel,
							   axlPointer                  create_channel_user_data,
							   VortexOnCloseChannel        close,
							   axlPointer                  close_user_data,
							   VortexOnFrameReceived       received,
							   axlPointer                  received_user_data,
							   VortexOnChannelPoolCreated  on_channel_pool_created,
							   axlPointer                  user_data);

int                 vortex_channel_pool_get_num           (VortexChannelPool * pool);

int                 vortex_channel_pool_get_available_num (VortexChannelPool * pool);

void                vortex_channel_pool_add               (VortexChannelPool * pool,
							   int  num);

void                vortex_channel_pool_add_full          (VortexChannelPool * pool,
							   int  num,
							   axlPointer user_data);

void                vortex_channel_pool_remove            (VortexChannelPool * pool,
							   int  num);
						 
void                vortex_channel_pool_close             (VortexChannelPool * pool);

void                vortex_channel_pool_attach            (VortexChannelPool * pool,
							   VortexChannel     * channel);
  
void                vortex_channel_pool_deattach          (VortexChannelPool * pool,
							   VortexChannel     * channel);

VortexChannel     * vortex_channel_pool_get_next_ready    (VortexChannelPool * pool,
							   axl_bool            auto_inc);

VortexChannel     * vortex_channel_pool_get_next_ready_full (VortexChannelPool * pool,
							     axl_bool            auto_inc,
							     axlPointer          user_data);

void                vortex_channel_pool_release_channel   (VortexChannelPool * pool,
							   VortexChannel     * channel);

int                 vortex_channel_pool_get_id            (VortexChannelPool * pool);

VortexConnection  * vortex_channel_pool_get_connection    (VortexChannelPool * pool);

/* internal api, do not use */
void                __vortex_channel_pool_close_internal (VortexChannelPool * pool);

#endif
