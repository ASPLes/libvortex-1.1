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
#ifndef __VORTEX_READER_H__
#define __VORTEX_READER_H__

#include <vortex.h>

void vortex_reader_watch_listener              (VortexCtx        * ctx,
						VortexConnection * listener);

void vortex_reader_watch_connection            (VortexCtx        * ctx,
						VortexConnection * connection);

void vortex_reader_unwatch_connection          (VortexCtx        * ctx,
						VortexConnection * connection);

int  vortex_reader_connections_watched         (VortexCtx        * ctx);

int  vortex_reader_run                         (VortexCtx * ctx);

void vortex_reader_stop                        (VortexCtx * ctx);

int  vortex_reader_notify_change_io_api        (VortexCtx * ctx);

void vortex_reader_notify_change_done_io_api   (VortexCtx * ctx);

axl_bool  vortex_reader_invoke_frame_received  (VortexCtx        * ctx,
						VortexConnection * connection,
						VortexChannel    * channel,
						VortexFrame      * frame);

/* internal API */
typedef void (*VortexForeachFunc) (VortexConnection * conn, axlPointer user_data);
typedef void (*VortexForeachFunc3) (VortexConnection * conn, 
				    axlPointer         user_data, 
				    axlPointer         user_data2,
				    axlPointer         user_data3);

VortexAsyncQueue * vortex_reader_foreach       (VortexCtx            * ctx,
						VortexForeachFunc      func,
						axlPointer             user_data);

void               vortex_reader_foreach_offline (VortexCtx           * ctx,
						  VortexForeachFunc3    func,
						  axlPointer            user_data,
						  axlPointer            user_data2,
						  axlPointer            user_data3);

void               vortex_reader_restart (VortexCtx * ctx);

#endif
