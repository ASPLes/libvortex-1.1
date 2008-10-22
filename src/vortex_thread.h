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
#ifndef __VORTEX_THREAD_H__
#define __VORTEX_THREAD_H__

#include <vortex.h>

/**
 * \addtogroup vortex_thread
 * @{
 */

BEGIN_C_DECLS

int                vortex_thread_create   (VortexThread      * thread_def,
					   VortexThreadFunc    func,
					   axlPointer          user_data,
					   ...);

int                vortex_thread_destroy  (VortexThread      * thread_def, 
					   int                 free_data);

int                vortex_mutex_create    (VortexMutex       * mutex_def);

int                vortex_mutex_destroy   (VortexMutex       * mutex_def);

void               vortex_mutex_lock      (VortexMutex       * mutex_def);

void               vortex_mutex_unlock    (VortexMutex       * mutex_def);

int                vortex_cond_create     (VortexCond        * cond);

void               vortex_cond_signal     (VortexCond        * cond);

void               vortex_cond_broadcast  (VortexCond        * cond);

/** 
 * @brief Useful macro that allows to perform a call to
 * vortex_cond_wait registering the place where the call was started
 * and ended.
 * 
 * @param c The cond variable to use.
 * @param mutex The mutex variable to use.
 */
#define VORTEX_COND_WAIT(c, mutex) do{\
vortex_cond_wait (c, mutex);\
}while(0);

int                vortex_cond_wait       (VortexCond        * cond, 
					   VortexMutex       * mutex);

/** 
 * @brief Useful macro that allows to perform a call to
 * vortex_cond_timewait registering the place where the call was
 * started and ended. 
 * 
 * @param r Wait result
 * @param c The cond variable to use.
 * @param mutex The mutex variable to use.
 * @param m The amount of microseconds to wait.
 */
#define VORTEX_COND_TIMEDWAIT(r, c, mutex, m) do{\
r = vortex_cond_timedwait (c, mutex, m);\
}while(0)


int                vortex_cond_timedwait  (VortexCond        * cond, 
					   VortexMutex       * mutex,
					   long int            microseconds);

void               vortex_cond_destroy    (VortexCond        * cond);

VortexAsyncQueue * vortex_async_queue_new       (void);

void               vortex_async_queue_push      (VortexAsyncQueue * queue,
						 axlPointer         data);

void               vortex_async_queue_priority_push  (VortexAsyncQueue * queue,
						      axlPointer         data);

axlPointer         vortex_async_queue_pop       (VortexAsyncQueue * queue);

axlPointer         vortex_async_queue_timedpop  (VortexAsyncQueue * queue,
						 long int           microseconds);

int                vortex_async_queue_length    (VortexAsyncQueue * queue);

int                vortex_async_queue_waiters   (VortexAsyncQueue * queue);

int                vortex_async_queue_items     (VortexAsyncQueue * queue);

void               vortex_async_queue_ref       (VortexAsyncQueue * queue);

void               vortex_async_queue_unref     (VortexAsyncQueue * queue);

void               vortex_async_queue_foreach   (VortexAsyncQueue         * queue,
						 VortexAsyncQueueForeach    foreach_func,
						 axlPointer                 user_data);

END_C_DECLS

#endif

/**
 * @}
 */ 
