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
#ifndef __VORTEX_THREAD_POOL_H__
#define __VORTEX_THREAD_POOL_H__

#include <vortex.h>

BEGIN_C_DECLS

typedef struct _VortexThreadPool VortexThreadPool;

void vortex_thread_pool_init                (VortexCtx * ctx, int  max_threads);

void vortex_thread_pool_add                 (VortexCtx * ctx, int threads);

void vortex_thread_pool_setup               (VortexCtx * ctx, 
					     int         thread_max_limit, 
					     int         thread_add_step,
					     int         thread_add_period, 
					     axl_bool    auto_remove);

void vortex_thread_pool_setup2              (VortexCtx * ctx, 
					     int         thread_max_limit, 
					     int         thread_add_step,
					     int         thread_add_period, 
					     int         thread_remove_step,
					     int         thread_remove_period, 
					     axl_bool    auto_remove,
					     axl_bool    preemtive); 

void vortex_thread_pool_remove              (VortexCtx * ctx, int threads);

void vortex_thread_pool_exit                (VortexCtx * ctx);

void vortex_thread_pool_being_closed        (VortexCtx * ctx);

void vortex_thread_pool_new_task            (VortexCtx        * ctx,
					     VortexThreadFunc   func, 
					     axlPointer         data);

int  vortex_thread_pool_new_event           (VortexCtx              * ctx,
					     long                     microseconds,
					     VortexThreadAsyncEvent   event_handler,
					     axlPointer               user_data,
					     axlPointer               user_data2);

axl_bool vortex_thread_pool_remove_event        (VortexCtx              * ctx,
						 int                      event_id);

void vortex_thread_pool_stats               (VortexCtx        * ctx,
					     int              * running_threads,
					     int              * waiting_threads,
					     int              * pending_tasks);

void vortex_thread_pool_event_stats         (VortexCtx        * ctx,
					     int              * events_installed);

int  vortex_thread_pool_get_running_threads (VortexCtx        * ctx);

void vortex_thread_pool_set_num             (int  number);

int  vortex_thread_pool_get_num             (void);

void vortex_thread_pool_set_exclusive_pool  (VortexCtx        * ctx,
					     axl_bool           value);

/* internal API */
void vortex_thread_pool_add_internal        (VortexCtx        * ctx, 
					     int                threads);
void vortex_thread_pool_remove_internal     (VortexCtx        * ctx, 
					     int                threads);

void __vortex_thread_pool_automatic_resize  (VortexCtx * ctx);

END_C_DECLS

#endif
