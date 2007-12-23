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

#include <vortex.h>

/* local include */
#include <vortex_ctx_private.h>

#define LOG_DOMAIN "vortex-thread-pool"

struct _VortexThreadPool {
	/* new tasks to be procesed */
	VortexAsyncQueue * queue;
	
	/* list of threads */
	axlList          * threads;

	/* context */
	VortexCtx        * ctx;

};

/* vortex thread pool struct used by vortex library to notify to tasks
 * to be performed to vortex thread pool */
typedef struct _VortexThreadPoolTask {
	VortexThreadFunc   func;
	axlPointer         data;
}VortexThreadPoolTask;

/**
 * @internal
 * 
 * This helper function dispatch the work to the right handler
 **/
axlPointer __vortex_thread_pool_dispatcher (axlPointer data)
{
	/* get current context */
	VortexThreadPoolTask * task;
	VortexThreadPool     * pool  = data;
	VortexCtx            * ctx   = pool->ctx;
	VortexAsyncQueue     * queue = pool->queue;

	vortex_log (VORTEX_LEVEL_DEBUG, "thread from pool started");

	/* get a reference to the queue, waiting for the next work */
	while (1) {

		vortex_log (VORTEX_LEVEL_DEBUG, "--> thread from pool waiting for jobs");
		/* get next task to process */
		task = vortex_async_queue_pop (queue);

		/* check stop in progress signal */
		if ((PTR_TO_INT (task) == 1) && ctx->thread_pool_being_stoped) {
			vortex_log (VORTEX_LEVEL_DEBUG, "--> thread from pool stoping, found finish beacon");

			/* unref the queue and return */
			vortex_async_queue_unref (queue);
			
			return NULL;
		} /* end if */

		vortex_log (VORTEX_LEVEL_DEBUG, "--> thread from pool processing new job");

		/* at this point we already are executing inside a thread */
		if (! ctx->thread_pool_being_stoped)
			task->func (task->data);

		/* free the task */
		axl_free (task);

	} /* end if */
		
	/* That's all! */
	return NULL;
}

/** 
 * @internal Function that terminates the thread and deallocates the
 * memory hold by the thread.
 * 
 * @param _thread The thread reference to terminate.
 */
void __vortex_thread_pool_terminate_thread (axlPointer _thread)
{
	/* cast a get a proper reference */
	VortexThread * thread = (VortexThread *) _thread;

	/* dealloc the node allocated */
	vortex_thread_destroy (thread, true);
	return;
}

/**
 * \defgroup vortex_thread_pool Vortex Thread Pool: Pool of threads which runns user defined async notifications.
 */

/**
 * \addtogroup vortex_thread_pool
 * @{
 */

/**
 * @brief Init the Vortex Thread Pool subsystem.
 * 
 * Initializes the vortex thread pool. This pool is mainly used to
 * invoke frame receive handler at any level. Because we have always
 * running threads to send messages (the vortex writer and sequencer)
 * this pool is not needed. 
 *
 * Among the vortex features are listed to create connection, channels
 * and also close channel in a asynchronous way. This means the those
 * process are run inside a separate thread. All those process are
 * also run inside the thread pool.
 *
 * @param ctx The context where the operation will be performed.
 *
 * @param max_threads how many threads to start.
 *
 **/
void vortex_thread_pool_init     (VortexCtx * ctx, 
				  int         max_threads)
{
	/* get current context */
	VortexThread * thread;
	int          iterator;
	vortex_log (VORTEX_LEVEL_DEBUG, "creating thread pool threads=%d", max_threads);

	/* create the thread pool and its internal values */
	ctx->thread_pool              = axl_new (VortexThreadPool, 1);
	ctx->thread_pool->queue       = vortex_async_queue_new ();
	ctx->thread_pool->threads     = axl_list_new (axl_list_always_return_1, __vortex_thread_pool_terminate_thread);
	ctx->thread_pool->ctx         = ctx;
	
	/* init all threads required */
	iterator = 0;
	while (iterator < max_threads) {
		/* create the thread */
		thread = axl_new (VortexThread, 1);
		if (! vortex_thread_create (thread,
					    /* function to execute */
					    __vortex_thread_pool_dispatcher,
					    /* a reference to the thread pool */
					    ctx->thread_pool,
					    /* finish thread configuration */
					    VORTEX_THREAD_CONF_END)) {
			/* free the reference */
			vortex_thread_destroy (thread, true);
			
			/* call to stop the thread pool */
			vortex_thread_pool_exit (ctx);
			
			vortex_log (VORTEX_LEVEL_CRITICAL, "unable to create a thread required for the pool");
			return;
		} /* end if */

		/* store the thread reference */
		axl_list_add (ctx->thread_pool->threads, thread);

		/* update the reference counting for this thread to
		 * the queue */
		vortex_async_queue_ref (ctx->thread_pool->queue);

		/* update the iterator */
		iterator++;

	} /* end if */
	return;
}

/** 
 * @internal
 *
 * Stops the vortex thread pool, that was initialized by
 * vortex_thread_pool_init.
 * 
 */
void vortex_thread_pool_exit (VortexCtx * ctx) 
{
	/* get current context */
	int         iterator;

	vortex_log (VORTEX_LEVEL_DEBUG, "stopping thread pool..");

	/* flag the queue to be stoping */
	ctx->thread_pool_being_stoped = true;

	/* push beacons to notify eacy thread created to stop */
	iterator = 0;
	while (iterator < axl_list_length (ctx->thread_pool->threads)) {
		vortex_log (VORTEX_LEVEL_DEBUG, "pushing beacon to stop thread from the pool..");
		/* push a notifier */
		vortex_async_queue_push (ctx->thread_pool->queue, INT_TO_PTR (1));

		/* update the iterator */
		iterator++;
	} /* end if */

	/* stop all threads */
	axl_list_free (ctx->thread_pool->threads);

	/* unref the queue */
	vortex_async_queue_unref (ctx->thread_pool->queue);

	/* free the node itself */
	axl_free (ctx->thread_pool);

	vortex_log (VORTEX_LEVEL_DEBUG, "thread pool is stopped..");
	return;
}

/** 
 * @internal Allows to flag the pool be in close process.
 */
void vortex_thread_pool_being_closed        (VortexCtx * ctx)
{
	/* do not configure if a null reference is received */
	if (ctx == NULL)
		return;

	/* flag the queue to be stoping */
	ctx->thread_pool_being_stoped = true;
	
	return;
}

/**
 * @brief Queue a new task inside the VortexThreadPool.
 *
 * 
 * Queue a new task to be performed. This function is used by vortex
 * for internal purpose so it should not be useful for vortex library
 * consumers.
 *
 * @param ctx The context where the operation will be performed.
 * @param func the function to execute.
 * @param data the data to be passed in to the function.
 *
 * 
 **/
void vortex_thread_pool_new_task (VortexCtx * ctx, VortexThreadFunc func, axlPointer data)
{
	/* get current context */
	VortexThreadPoolTask * task;

	/* check parameters */
	if (func == NULL || ctx == NULL || ctx->thread_pool == NULL || ctx->thread_pool_being_stoped)
		return;

	/* create the task data */
	task       = axl_new (VortexThreadPoolTask, 1);
	task->func = func;
	task->data = data;

	/* queue the task for the next available thread */
	vortex_async_queue_push (ctx->thread_pool->queue, task);

	return;
}


/**
 * @brief Returns the running threads the given pool have.
 * 
 * Returns the actual running threads the vortex thread pool have.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @return the thread number or -1 if fails
 **/
int  vortex_thread_pool_get_running_threads (VortexCtx * ctx)
{
	/* get current context */
	if (ctx == NULL || ctx->thread_pool == NULL)
		return -1;

	return axl_list_length (ctx->thread_pool->threads);
}


/** 
 * @brief Allows to configure the number of threads inside the Vortex
 * Thread Pool module.
 *
 * This function modifies the environment variable
 * <b>"VORTEX_THREADS"</b>, setting the value provided. Later, the
 * value will be used by the \ref vortex_init_ctx function to initialize
 * the Vortex Library.
 *
 * This function must be called before \ref vortex_init_ctx to take
 * effect. See also:
 *
 *  - \ref vortex_thread_pool_get_num
 *  - \ref vortex_thread_pool_get_running_threads.
 * 
 * @param number The number of working threads that the Vortex Thread
 * Pool will try to start.
 */
void vortex_thread_pool_set_num             (int  number)
{
	char * _number;
	
	/* do not configure anything .. */
	if (number <= 0)
		return;
	
	/* translate the number into an string representation */
	_number = axl_strdup_printf ("%d", number);
	
	/* set the value */
	vortex_support_setenv ("VORTEX_THREADS", _number);

	/* relese the string */
	axl_free (_number);
	
	return;
}

/**
 * @brief Returns how many threads have the given VortexThreadPool.
 * 
 * This function is for internal vortex library purpose. This function
 * returns how many thread must be created inside the vortex thread
 * pool which actually is the one which dispatch all frame received
 * callback and other async notifications. 
 *
 * This function pay attention to the environment var <b>"VORTEX_THREADS"</b>
 * so user can control how many thread are created. In case this var
 * is not defined the function will return 5. In case the var is
 * defined but using a wrong value, that is, an non positive integer,
 * the function will abort the vortex execution and consequently the
 * application over it.
 * 
 * @return the number of threads to be created.
 **/
int  vortex_thread_pool_get_num             (void)
{
#if defined(AXL_OS_WIN32) && ! defined(__GNUC__)
	char * temp;
	int    requiredSize;
#endif
	int  value;

	/* get the number of threads to start */
	value = vortex_support_getenv_int ("VORTEX_THREADS");

	if (value == 0)
		return 5;
#if defined(AXL_OS_WIN32) && ! defined(__GNUC__)
	getenv_s( &requiredSize, NULL, 0, "HOME");
	temp = axl_new (char, requiredSize + 1);
	getenv_s( &requiredSize, temp, requiredSize, "HOME" );
	value = atoi (temp);
	axl_free (temp);
#else
	value = atoi (getenv ("VORTEX_THREADS"));
#endif
	if (value <= 0) {
		exit (-1);
	} /* end if */
	
	return value;
}

/** 
 * @brief Allows to configure current configuration for threads inside
 * the thread pool created.
 *
 * By default, once the thread pool is started, all threads available
 * inside are not usable from outside the pool. However, it could be
 * required to allow the thread pool to share (and use) threads
 * available not only from its thread pool but also from other thread
 * pools created outside the Vortex Library context.
 *
 * This function must be called before \ref vortex_init_ctx to take
 * effect.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param value The new behaviour to configure. By default, internal
 * value is already configured to true. Set false to make the thread
 * pool to behave in a non-exclusive form.
 */
void vortex_thread_pool_set_exclusive_pool  (VortexCtx * ctx,
					     bool        value)
{
	/* set the new value */
	ctx->thread_pool_exclusive = value;

	return;
}
       

/* @} */
