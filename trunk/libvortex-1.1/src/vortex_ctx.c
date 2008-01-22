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

/* global include */
#include <vortex.h>

/* private include */
#include <vortex_ctx_private.h>

/**
 * \defgroup vortex_ctx Vortex context: functions to manage vortex context, an object that represent a vortex library state.
 */

/**
 * \addtogroup vortex_ctx
 * @{
 */

/** 
 * @brief Creates a new vortex execution context. This is mainly used
 * by the main module (called from \ref vortex_init_ctx) and finished from
 * \ref vortex_exit_ctx. 
 *
 * A context is required to make vortex library to work. This object
 * stores a single execution context. Several execution context can be
 * created inside the same process.
 *
 * After calling to this function, a new \ref VortexCtx is created and
 * all configuration required previous to \ref vortex_init_ctx can be
 * done. Once prepared, a call to \ref vortex_init_ctx starts vortex
 * library.
 *
 * Once you want to stop the library execution you must call to \ref
 * vortex_exit_ctx.
 *
 * See http://lists.aspl.es/pipermail/vortex/2008-January/000343.html
 * for more information.
 *
 * @return A newly allocated reference to the \ref VortexCtx. You must
 * finish it with \ref vortex_ctx_free.
 */
VortexCtx * vortex_ctx_new ()
{
	VortexCtx * result;

	/* create a new context */
	result           = axl_new (VortexCtx, 1);

	/**** vortex_frame_factory.c: init module ****/
	result->frame_id = 1;

	/* init mutex for the log */
	vortex_mutex_create (&result->log_mutex);

	/**** vortex_thread_pool.c: init ****/
	result->thread_pool_exclusive = true;

	/* return context created */
	return result;
}

/** 
 * @brief Releases the memory allocated by the provided \ref
 * VortexCtx.
 * 
 * @param ctx A reference to the context to deallocate.
 */
void        vortex_ctx_free (VortexCtx * ctx)
{
	/* do nothing */
	if (ctx == NULL)
		return;

	/* release log mutex */
	vortex_mutex_create (&ctx->log_mutex);

	/* free the context */
	axl_free (ctx);
	
	return;
}

/** 
 * @}
 */
