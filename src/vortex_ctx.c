/*
 *  LibVortex:  A BEEP (RFC3080/RFC3081) implementation.
 *  Copyright (C) 2010 Advanced Software Production Line, S.L.
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
 * finish it with \ref vortex_ctx_free. Reference returned must be
 * checked to be not NULL (in which case, memory allocation have
 * failed).
 */
VortexCtx * vortex_ctx_new (void)
{
	VortexCtx * result;

	/* create a new context */
	result           = axl_new (VortexCtx, 1);
	VORTEX_CHECK_REF (result, NULL);

	/* create the hash to store data */
	result->data     = vortex_hash_new (axl_hash_string, axl_hash_equal_string);
	VORTEX_CHECK_REF2 (result->data, NULL, result, axl_free);

	/**** vortex_frame_factory.c: init module ****/
	result->frame_id = 1;

	/* init mutex for the log */
	vortex_mutex_create (&result->log_mutex);

	/**** vortex_thread_pool.c: init ****/
	result->thread_pool_exclusive = axl_true;

	/* init reference counting */
	vortex_mutex_create (&result->ref_mutex);
	result->ref_count = 1;

	/* set default serverName acquire value */
	result->serverName_acquire = axl_true;

	/* return context created */
	return result;
}

/** 
 * @internal Function used to reinit the VortexCtx. This function is
 * highly unix dependant.
 */
void      vortex_ctx_reinit (VortexCtx * ctx)
{
	vortex_mutex_create (&ctx->log_mutex);
	vortex_mutex_create (&ctx->ref_mutex);

	/* the rest of mutexes are initialized by vortex_init_ctx. */

	return;
}

/** 
 * @brief Allows to configure a finish handler which is called once
 * the process (vortex reader) detects no more pending connections are
 * available.
 * 
 * @param ctx The context where the finish handler will be installed.
 *
 * @param finish_handler Finish handler to be called as described by
 * \ref VortexOnFinishHandler. If the value passed is NULL, then the
 * handler will be unconfigured. Calling again with a handler will
 * unconfigure previous one and set the value passed.
 *
 * @param user_data User defined data to be passed to the handler.
 */
void        vortex_ctx_set_on_finish        (VortexCtx              * ctx,
					     VortexOnFinishHandler    finish_handler,
					     axlPointer               user_data)
{
	if (ctx == NULL)
		return;

	/* removes previous configured handler */
	if (finish_handler == NULL) {
		ctx->finish_handler = NULL;
		ctx->finish_handler_data = NULL;
		return;
	} /* end if */

	/* set handler */
	ctx->finish_handler      = finish_handler;
	ctx->finish_handler_data = user_data;
	return;
}

/** 
 * @internal Function used by vortex reader module to check and call
 * the finish handler defined.
 */
void        vortex_ctx_check_on_finish      (VortexCtx * ctx)
{
	/* check */
	if (ctx == NULL || ctx->finish_handler == NULL)
		return;
	/* call handler */
	ctx->finish_handler (ctx, ctx->finish_handler_data);
	return;
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
 * @param value The value to be stored. If the value to be stored is NULL, the function calls to remove previous content stored on the same key.
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

	/* check if the value is not null. It it is null, remove the
	 * value. */
	if (value == NULL) {
		vortex_hash_remove (ctx->data, key);
		return;
	} /* end if */

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
 * @brief Allows to configure idle handler at context level. This
 * handler will be called when no traffic was produced/received on a
 * particular connection during the max idle period. This is
 * configured when the handler is passed. 
 *
 * @param ctx The context where the idle period and the handler will be configured.
 *
 * @param idle_handler The handler to be called in case a connection
 * have no activity within max_idle_period. The handler will be called
 * each max_idle_period.
 *
 * @param max_idle_period Amount of seconds to wait until considering
 * a connection was idle because no activity was registered within
 * that period.
 *
 * @param user_data Optional user defined pointer to be passed to the
 * idle_handler when activated.
 *
 * @param user_data2 Second optional user defined pointer to be passed
 * to the idle_handler when activated.
 */
void        vortex_ctx_set_idle_handler          (VortexCtx                       * ctx,
						  VortexIdleHandler                 idle_handler,
						  long                              max_idle_period,
						  axlPointer                        user_data,
						  axlPointer                        user_data2)
{
	v_return_if_fail (ctx);

	/* set handlers */
	ctx->global_idle_handler       = idle_handler;
	ctx->max_idle_period           = max_idle_period;
	ctx->global_idle_handler_data  = user_data;
	ctx->global_idle_handler_data2 = user_data2;

	/* do nothing more for now */
	return;
}

axlPointer __vortex_ctx_notify_idle (VortexConnection * conn)
{
	VortexCtx          * ctx     = CONN_CTX (conn);
	VortexIdleHandler    handler = ctx->global_idle_handler;

	/* check null handler */
	if (handler == NULL)
		return NULL;

	/* call to notify idle */
	vortex_log (VORTEX_LEVEL_DEBUG, "notifying idle connection on id=%d because %ld was exceeded", 
		    vortex_connection_get_id (conn), ctx->max_idle_period);
	handler (ctx, conn, ctx->global_idle_handler_data, ctx->global_idle_handler_data2);
	
	/* reduce reference acquired */
	vortex_log (VORTEX_LEVEL_DEBUG, "Calling to reduce reference counting now finished idle handler for id=%d",
		    vortex_connection_get_id (conn));

	/* reset idle state to current time and notify idle notification finished */
	vortex_connection_set_receive_stamp (conn);
	vortex_connection_set_data (conn, "vo:co:idle", NULL);

	vortex_connection_unref (conn, "notify-idle");

	return NULL;
}

/** 
 * @internal Function used to implement idle notify on a connection.
 */
void        vortex_ctx_notify_idle               (VortexCtx                       * ctx,
						  VortexConnection                * conn)
{
	/* do nothing if handler or context aren't defined */
	if (ctx == NULL || ctx->global_idle_handler == NULL)
		return;

	/* check if the connection was already notified */
	if (PTR_TO_INT (vortex_connection_get_data (conn, "vo:co:idle")))
		return;
	/* notify idle notification is in progress */
	vortex_connection_set_data (conn, "vo:co:idle", INT_TO_PTR (axl_true));

	/* check unchecked reference (always acquire reference) */
	vortex_connection_ref_internal (conn, "notify-idle", axl_false);

	/* call to notify */
	vortex_thread_pool_new_task (ctx, (VortexThreadFunc) __vortex_ctx_notify_idle, conn);

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
 * @brief Allows to perform a global configuration to enable/disable
 * serverName (or x-serverName) on greetings getting the value from
 * the current connection host name configured. 
 * 
 * By default serverName feature is requested based on the current
 * host name. This function allows to disable this feature globally on
 * the provided context.
 *
 * @param ctx The context where the configuration will take place.
 *
 * @param status axl_true (default) to let vortex to request
 * serverName feature based on current connection host
 * name. Otherwise, axl_false to disable this feature.
 */
void        vortex_ctx_server_name_acquire       (VortexCtx * ctx,
						  axl_bool    status)
{
	v_return_if_fail (ctx);
	/* update serverName acquire */
	ctx->serverName_acquire = status;
	return;
}

/** 
 * @brief Allows to increase reference count to the VortexCtx
 * instance.
 *
 * @param ctx The reference to update its reference count.
 */
void        vortex_ctx_ref                       (VortexCtx  * ctx)
{
	/* do nothing */
	if (ctx == NULL)
		return;

	/* acquire the mutex */
	vortex_mutex_lock (&ctx->ref_mutex);
	ctx->ref_count++;
	vortex_mutex_unlock (&ctx->ref_mutex);

	return;
}

/** 
 * @brief Decrease reference count and nullify caller's pointer in the
 * case the count reaches 0.
 *
 * @param ctx The context to decrement reference count. In the case 0
 * is reached the VortexCtx instance is deallocated and the callers
 * reference is nullified.
 */
void        vortex_ctx_unref                     (VortexCtx ** ctx)
{
	VortexCtx * _ctx;
	axl_bool   nullify;

	/* do nothing with a null reference */
	if (ctx == NULL || (*ctx) == NULL)
		return;

	/* check if we have to nullify after unref */
	_ctx = (*ctx);
	vortex_mutex_lock (&_ctx->ref_mutex);
	nullify =  (_ctx->ref_count == 1);
	vortex_mutex_unlock (&_ctx->ref_mutex);

	/* call to unref */
	vortex_ctx_free (*ctx);
	
	/* check to nullify */
	if (nullify)
		(*ctx) = NULL;
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

	/* acquire the mutex */
	vortex_mutex_lock (&ctx->ref_mutex);
	ctx->ref_count--;

	if (ctx->ref_count != 0) {
		/* release mutex */
		vortex_mutex_unlock (&ctx->ref_mutex);
		return;
	} /* end if */
	
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
	
	/* release and clean mutex */
	vortex_mutex_unlock (&ctx->ref_mutex);
	vortex_mutex_destroy (&ctx->ref_mutex);

	/* free the context */
	axl_free (ctx);
	
	return;
}

/** 
 * @}
 */
