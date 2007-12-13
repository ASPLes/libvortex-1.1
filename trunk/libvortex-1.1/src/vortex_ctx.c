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
 * @internal The following variable its to hold the current context
 * used by the vortex_ctx_set and vortex_ctx_get. In the future this
 * function won't be used.
 */
VortexCtx * vortex_ctx_global = NULL;

/** 
 * @brief Creates a new vortex execution context. This is mainly used
 * by the main module (called from vortex_init) and finished from
 * vortex_exit. This is a preparation to make the vortex a stateless
 * library.
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

	/**** vortex_thread_pool.c: init ****/
	result->thread_pool_exclusive = true;

	/* return context created */
	return result;
}

/** 
 * @brief Transit function that allows to get the current context
 * configured. 
 * 
 * @return A reference to the current context configured. You should
 * not release the reference returned by this function.
 */
VortexCtx * vortex_ctx_get ()
{
	return vortex_ctx_global;
}

/** 
 * @brief Transit function that allows to configure the vortex ctx to
 * be used by the library. Calling to this function must be done
 * before any operation is done by the vortex library (\ref vortex_init).
 * 
 * @param ctx The new vortex context to configure.
 */
void        vortex_ctx_set (VortexCtx * ctx)
{
	/* configure context */
	vortex_ctx_global = ctx;
	
	return;
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

	/* free the context */
	axl_free (ctx);
	
	return;
}

/** 
 * @}
 */
