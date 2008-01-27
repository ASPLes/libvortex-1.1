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

	/* create the hash to store data */
	result->data     = vortex_hash_new (axl_hash_string, axl_hash_equal_string);

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
 * @brief Allows to store arbitrary data associated to the provided
 * context, which can later retrieved using a particular key. 
 * 
 * @param ctx The ctx where the data will be stored.
 * @param key The key to index the value stored.
 * @param value The value to be stored. 
 */
void        vortex_ctx_set_data (VortexCtx       * ctx, 
				 axlPointer        key, 
				 axlPointer        value)
{
	v_return_if_fail (ctx && key);

	/* call to configure using full version */
	vortex_ctx_set_data_full (ctx, key, value, NULL, NULL);
	return;
}


/** 
 * @brief Allows to store arbitrary data associated to the provided
 * context, which can later retrieved using a particular key. It is
 * also possible to configure a destroy handler for the key and the
 * value stored, ensuring the memory used will be deallocated once the
 * context is terminated (\ref vortex_ctx_free) or the value is
 * replaced by a new one.
 * 
 * @param ctx The ctx where the data will be stored.
 * @param key The key to index the value stored.
 * @param value The value to be stored. 
 * @param key_destroy Optional key destroy function (use NULL to set no destroy function).
 * @param value_destroy Optional value destroy function (use NULL to set no destroy function).
 */
void        vortex_ctx_set_data_full (VortexCtx       * ctx, 
				      axlPointer        key, 
				      axlPointer        value,
				      axlDestroyFunc    key_destroy,
				      axlDestroyFunc    value_destroy)
{
	v_return_if_fail (ctx && key);

	/* store the data */
	vortex_hash_replace_full (ctx->data, 
				  /* key and function */
				  key, key_destroy,
				  /* value and function */
				  value, value_destroy);
	return;
}


/** 
 * @brief Allows to retreive data stored on the given context (\ref
 * vortex_ctx_set_data) using the provided index key.
 * 
 * @param ctx The context where to lookup the data.
 * @param key The key to use as index for the lookup.
 * 
 * @return A reference to the pointer stored or NULL if it fails.
 */
axlPointer  vortex_ctx_get_data (VortexCtx       * ctx,
				 axlPointer        key)
{
	v_return_val_if_fail (ctx && key, NULL);

	/* lookup */
	return vortex_hash_lookup (ctx->data, key);
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

	/* clear the hash */
	vortex_hash_destroy (ctx->data);
	ctx->data = NULL;

	/* free the context */
	axl_free (ctx);
	
	return;
}

/** 
 * @}
 */
