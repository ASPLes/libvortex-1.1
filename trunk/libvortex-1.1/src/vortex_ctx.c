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
 *         C/ Antonio Suarez Nº 10, 
 *         Edificio Alius A, Despacho 102
 *         Alcalá de Henares 28802 (Madrid)
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.aspl.es/vortex
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
VortexCtx * vortex_ctx_new (void)
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
	result->thread_pool_exclusive = axl_true;

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
 * @brief Allows to configure a global frame received handler where
 * all frames are delivered, overriding first and second level
 * handlers. The frame handler is executed using the thread created
 * for the vortex reader process, that is, without activing a new
 * thread from the pool. This means that the function must not block
 * the caller because no frame will be received until the handler
 * configured on this function finish.
 *
 * @param ctx The context to configure.
 *
 * @param received The handler to configure.
 *
 * @param received_user_data User defined data to be configured
 * associated to the handler. This data will be provided to the frame
 * received handler each time it is activated.
 */
void      vortex_ctx_set_frame_received          (VortexCtx             * ctx,
						  VortexOnFrameReceived   received,
						  axlPointer              received_user_data)
{
	v_return_if_fail (ctx);
	
	/* configure handler and data even if they are null */
	ctx->global_frame_received      = received;
	ctx->global_frame_received_data = received_user_data;

	return;
}

/** 
 * @brief Allows to configure a global close notify handler on the
 * provided on the provided context. 
 * 
 * See \ref VortexOnNotifyCloseChannel and \ref
 * vortex_channel_notify_close to know more about this function. The
 * handler configured on this function will affect to all channel
 * close notifications received on the provided context.
 * 
 * @param ctx The context to configure with a global close channel notify.
 *
 * @param close_notify The close notify handler to execute.
 *
 * @param user_data User defined data to be passed to the close notify
 * handler.
 */
void               vortex_ctx_set_close_notify_handler     (VortexCtx                  * ctx,
							    VortexOnNotifyCloseChannel   close_notify,
							    axlPointer                   user_data)
{
	/* check context received */
	if (ctx == NULL)
		return;

	/* configure handlers */
	ctx->global_notify_close      = close_notify;
	ctx->global_notify_close_data = user_data;

	/* nothing more to do over here */
	return;
}

/**
 * @brief Allows to configure a global handler that is called each
 * time a channel is added to any connection.
 *
 * @param ctx The context that is being configured.
 * @param added_handler The handler to configure.
 * @param user_data User defined data to be passed to the handler configured.
 */
void        vortex_ctx_set_channel_added_handler (VortexCtx                       * ctx,
						  VortexConnectionOnChannelUpdate   added_handler,
						  axlPointer                        user_data)
{
	/* check context received */
	if (ctx == NULL)
		return;

	/* configure handlers */
	ctx->global_channel_added      = added_handler;
	ctx->global_channel_added_data = user_data;

	/* nothing more to do over here */
	return;
}

/**
 * @brief Allows to configure a global handler that is called each
 * time a channel is removed from any connection.
 *
 * @param ctx The context that is being configured.
 * @param removed_handler The handler to configure.
 * @param user_data User defined data to be passed to the handler configured.
 */
void        vortex_ctx_set_channel_removed_handler (VortexCtx                       * ctx,
						    VortexConnectionOnChannelUpdate   removed_handler,
						    axlPointer                        user_data)
{
	/* check context received */
	if (ctx == NULL)
		return;

	/* configure handlers */
	ctx->global_channel_removed      = removed_handler;
	ctx->global_channel_removed_data = user_data;

	/* nothing more to do over here */
	return;
}

/**
 * @brief Allows to configure a global start channel handler (for
 * incoming start request) which is applicable for all profiles and
 * override vortex profiles module configuration.
 *
 * @param ctx The context to configure with the provided global start
 * handler.
 *
 * @param start_handler The start handler to configure.
 *
 * @param start_handler_data User defined pointer passed to the
 * handler.
 */
void        vortex_ctx_set_channel_start_handler (VortexCtx                       * ctx,
						  VortexOnStartChannelExtended      start_handler,
						  axlPointer                        start_handler_data)
{
	/* check context received */
	if (ctx == NULL)
		return;

	/* configure handlers */
	ctx->global_channel_start_extended       = start_handler;
	ctx->global_channel_start_extended_data  = start_handler_data;

	/* nothing more to do over here */
	return;
}

/**
 * @brief Allows to install a cleanup function which will be called
 * just before the \ref VortexCtx is finished (by a call to \ref
 * vortex_exit_ctx or a manual call to \ref vortex_ctx_free).
 *
 * This function is provided to allow Vortex Library extensions to
 * install its module deallocation or termination functions.
 *
 * @param ctx The context to configure with the provided cleanup
 * function.  @param cleanup The cleanup function to configure. This
 * function will receive a reference to the \ref VortexCtx.
 */
void        vortex_ctx_install_cleanup (VortexCtx * ctx,
					axlDestroyFunc cleanup)
{
	v_return_if_fail (ctx);
	v_return_if_fail (cleanup);
	
	/* init the list in the case it isn't */
	if (ctx->cleanups == NULL) 
		ctx->cleanups = axl_list_new (axl_list_always_return_1, NULL);
	
	/* add the cleanup function */
	axl_list_append (ctx->cleanups, cleanup);

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
	axlDestroyFunc func;
	int            iterator;

	/* do nothing */
	if (ctx == NULL)
		return;
	
	/* call to cleanup functions defined */
	if (ctx->cleanups) {
		iterator = 0;
		while (iterator < axl_list_length (ctx->cleanups)) {
			/* get clean up function */
			func = axl_list_get_nth (ctx->cleanups, iterator);

			/* call to clean */
			func (ctx);

			/* next iterator */
			iterator++;
		} /* end while */

		/* terminate list */
		axl_list_free (ctx->cleanups);
		ctx->cleanups = NULL; 
	} /* end if */

	/* clear the hash */
	vortex_hash_destroy (ctx->data);
	ctx->data = NULL;

	/* release log mutex */
	vortex_mutex_destroy (&ctx->log_mutex);

	/* free the context */
	axl_free (ctx);
	
	return;
}

/** 
 * @}
 */
