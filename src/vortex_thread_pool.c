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

#include <vortex.h>

/* local include */
#include <vortex_ctx_private.h>

#define LOG_DOMAIN "vortex-thread-pool"

struct _VortexThreadPool {
	/* new tasks to be procesed */
	VortexAsyncQueue * queue;
	VortexMutex        mutex;
	
	/* list of threads */
	axlList          * threads;
	axlList          * stopped;
	VortexMutex        stopped_mutex;

	/* list of events */
	axlList          * events;
	axlListCursor    * events_cursor;
	axl_bool           processing_events;

	/* context */
	VortexCtx        * ctx;

	/* automatic thread pool resize */
	axl_bool           automatic_resize_status;
	int                base_thread_num;
	int                thread_max_limit;
	int                thread_add_step;
	int                thread_add_period;
	int                thread_remove_step;
	int                thread_remove_period;
	axl_bool           auto_remove;
	axl_bool           preemtive;
	struct timeval     last;

};

/* vortex thread pool struct used by vortex library to notify to tasks
 * to be performed to vortex thread pool */
typedef struct _VortexThreadPoolTask {
	VortexThreadFunc   func;
	axlPointer         data;
} VortexThreadPoolTask;

/* struct used to represent async events */
typedef struct _VortexThreadPoolEvent {
	VortexThreadAsyncEvent   func;
	axlPointer               data;
	axlPointer               data2;
	long                     delay;
	struct timeval           next_step;
	int                      ref_count;
} VortexThreadPoolEvent;

typedef struct _VortexThreadPoolStarter {
	VortexThreadPool * pool;
	VortexThread     * thread;
	VortexAsyncQueue * queue;
} VortexThreadPoolStarter;

/* update next step to the appropiate value */
void __vortex_thread_pool_increase_stamp (VortexThreadPoolEvent * event)
{
	/* increase seconds part */
	if ((event->next_step.tv_usec + event->delay) > 1000000) {
		/* update seconds part */
		event->next_step.tv_sec += ((event->next_step.tv_usec + event->delay) / 1000000);
	} /* end if */

	/* now increase microseconds part */
	event->next_step.tv_usec = ((event->next_step.tv_usec + event->delay) % 1000000);

	return;
}

void __vortex_thread_pool_unref_event (axlPointer _event)
{
	VortexThreadPoolEvent * event = _event;
	event->ref_count--;
	if (event->ref_count == 0) 
		axl_free (event);
	return;
}

void __vortex_thread_pool_process_events (VortexCtx * ctx, VortexThreadPool * pool)
{
	int                     length;
	int                     iterator;
	struct timeval          now;
	VortexThreadPoolEvent * event;

	/* reference to implement the call */
	VortexThreadAsyncEvent   func;
	axlPointer               data;
	axlPointer               data2;

	/* ensure only one thread is processing */
	if (vortex_is_exiting (ctx))
		return;

	/* acquire lock */
	vortex_mutex_lock (&pool->mutex);
	/* ensure again we can continue */
	if (pool->processing_events || axl_list_length (pool->events) == 0 || vortex_is_exiting (ctx)) {
		vortex_mutex_unlock (&pool->mutex);
		return;
	} /* end if */

	/* flag we are processing */
	pool->processing_events = axl_true;
	length = axl_list_length (pool->events);
	vortex_mutex_unlock (&pool->mutex);
	
	/* get current stamp */
	gettimeofday (&now, NULL);
	iterator = 0;
	/* vortex_log2 (VORTEX_LEVEL_DEBUG, "Found (%d) events to process: now (%d, %d)..", length, (int) now.tv_sec, (int) now.tv_usec); */
	while (iterator < length) {

		/* get event reference */
		vortex_mutex_lock (&pool->mutex);
		event = axl_list_get_nth (pool->events, iterator);
		if (event == NULL) {
			vortex_mutex_unlock (&pool->mutex);
			break;
		}

		/* increase ref count now we have the look */
		event->ref_count++;

		/* get stamp */
		if ((now.tv_sec > event->next_step.tv_sec) ||
		    ((now.tv_sec == event->next_step.tv_sec) &&
		     (now.tv_usec >= event->next_step.tv_usec))) {

			func  = event->func;
			data  = event->data;
			data2 = event->data2;

			/* unlock before calling */
			vortex_mutex_unlock (&pool->mutex);

			/* call to notify event */
			if (func (ctx, data, data2)) {
				vortex_mutex_lock (&pool->mutex);

				/* remove event using the pointer not
				 * the position because it may be
				 * pointing to a different location
				 * (i.e: someone removed or added an
				 * event durint the last func()
				 * call) */
				axl_list_remove_ptr (pool->events, event);

				/* decrease local reference */
				__vortex_thread_pool_unref_event (event);

				vortex_mutex_unlock (&pool->mutex);

				/* don't update iterator to manage next position */
				continue;
			}

			/* now recalculate event to be executed in the
			 * future (because the user did selected to
			 * keep it) */
			vortex_mutex_lock (&pool->mutex);
			__vortex_thread_pool_increase_stamp (event);
			
		} /* end if */

		/* decrease local reference */
		__vortex_thread_pool_unref_event (event);

		/* unlock for the next lock */
		vortex_mutex_unlock (&pool->mutex);

		/* next position */
		iterator++;
	}

	/* flag that no more processing events */
	vortex_mutex_lock (&pool->mutex);
	pool->processing_events = axl_false;
	vortex_mutex_unlock (&pool->mutex);
	return;
}

/** 
 * @internal Code that resizes the thread pool adding or removing
 * threads according to current status and user configuration.
 */
void __vortex_thread_pool_automatic_resize (VortexCtx * ctx)
{
	struct timeval   now;
	struct timeval   diff;
	int              waiting_threads;
	int              pending_tasks;
	int              running_threads;

	/* check before acquiring the look if the user changed during
	 * our lock */
	if (! ctx->thread_pool || vortex_is_exiting (ctx))  
		return;

	/* lock thread pool */
	vortex_mutex_lock (&ctx->thread_pool->mutex);

	/* check if the user changed during our lock */
	if (! ctx->thread_pool || ! ctx->thread_pool->automatic_resize_status || 
	    ctx->thread_pool_being_stopped || vortex_is_exiting (ctx)) {
		vortex_mutex_unlock (&ctx->thread_pool->mutex);
		return;
	}

	running_threads = axl_list_length (ctx->thread_pool->threads);
	waiting_threads = vortex_async_queue_waiters (ctx->thread_pool->queue);
	pending_tasks   = vortex_async_queue_items (ctx->thread_pool->queue);

	/* now get difference in diff */
	gettimeofday (&now, NULL);
	vortex_timeval_substract (&now, &(ctx->thread_pool->last), &diff);

/*	vortex_log2 (VORTEX_LEVEL_DEBUG, 
		     "Checking thread pool resize: running_threads=%d, base threads=%d, waiting_threads=%d, pending_tasks=%d, thread_max_limit=%d diff=%ld, add_period=%d, auto_remove=%d\n",
		    running_threads, ctx->thread_pool->base_thread_num, waiting_threads, pending_tasks, ctx->thread_pool->thread_max_limit, diff.tv_sec, 
		    ctx->thread_pool->thread_add_period, ctx->thread_pool->auto_remove); */
	/* if we have waiting threads equal to 0 and our last update
	 * was thread_pool->last seconds ago and there is at least 1
	 * pending_tasks do */
	if (running_threads < ctx->thread_pool->thread_max_limit &&
	    waiting_threads == 0 && 
	    pending_tasks > 0 && 
	    diff.tv_sec >= ctx->thread_pool->thread_add_period){

		vortex_log (VORTEX_LEVEL_DEBUG, "Adding %d threads because there are pending tasks %d and limit was not reached %d (waiting_threads=%d, running_threads=%d)\n",
			ctx->thread_pool->thread_add_step, pending_tasks, ctx->thread_pool->thread_max_limit, waiting_threads, running_threads);
		fflush (stdout);

		/* add a thread to the pool (call internal unlocked) */
		vortex_thread_pool_add_internal (ctx, ctx->thread_pool->thread_add_step);
		gettimeofday (&(ctx->thread_pool->last), NULL);

	} else if (ctx->thread_pool->auto_remove && 
		   pending_tasks == 0 && 
		   running_threads > ctx->thread_pool->base_thread_num &&
		   (waiting_threads + 2) > ctx->thread_pool->base_thread_num && 
		   diff.tv_sec > (ctx->thread_pool->thread_remove_period)) {

		/* remove threads if no pending task is found, and
		   there are more waiting_threads than base thread
		   number and that no reduce operation had happend
		   during last thread_add_period * 2. */

		vortex_log (VORTEX_LEVEL_DEBUG, "Removing %d threads because there are no pending tasks, and there are more waiting threads %d than base number %d\n",
			ctx->thread_pool->thread_remove_step, waiting_threads, ctx->thread_pool->base_thread_num);
		fflush (stdout);
		   
		/* remove a thread from the pool */
		vortex_thread_pool_remove_internal (ctx, ctx->thread_pool->thread_remove_step);
		gettimeofday (&(ctx->thread_pool->last), NULL);

	} /* end if */

	/* unlock the mutex */
	vortex_mutex_unlock (&ctx->thread_pool->mutex);
	
	return;
}

/** 
 * @internal
 * 
 * This helper function dispatch the work to the right handler
 **/
axlPointer __vortex_thread_pool_dispatcher (VortexThreadPoolStarter * _data)
{
	/* get current context */
	VortexThreadPoolTask * task;
	VortexThread         * thread = _data->thread;
	VortexThreadPool     * pool   = _data->pool;
	VortexCtx            * ctx    = pool->ctx;
	VortexAsyncQueue     * queue  = _data->queue;

	/* local pointers to release soon data object */
	VortexThreadFunc       func;
	axlPointer             data;

	axl_free (_data);

	vortex_log (VORTEX_LEVEL_DEBUG, "thread from pool started");

	/* get a reference to the queue, waiting for the next work */
	while (axl_true) {

		/* get next task to process: precision=100ms */
		task = vortex_async_queue_timedpop (queue, 100000);
		
		if (task == NULL) {
			/* call to process events */
			__vortex_thread_pool_process_events (ctx, pool);

			/* do automatic reasize */
			__vortex_thread_pool_automatic_resize (ctx);

			continue;
		}

		if (PTR_TO_INT (task) == 3) {
			/* collect thread data terminated */
			vortex_mutex_lock (&(ctx->thread_pool->stopped_mutex));
			axl_list_remove_first (pool->stopped);
			vortex_mutex_unlock (&(ctx->thread_pool->stopped_mutex));
			continue;
		}

		/* check to stop current thread because pool was reduced */
		if (PTR_TO_INT (task) == 2) {
			vortex_log (VORTEX_LEVEL_DEBUG, "--> thread from pool stoping, found thread stop beacon");

			/* do not lock because this is already done by
			 * vortex_thread_pool_remove .. */
			vortex_mutex_lock (&(ctx->thread_pool->stopped_mutex));

			/* remove thread from the pool */
			vortex_mutex_lock (&pool->mutex);
			axl_list_unlink_ptr (pool->threads, thread);
			vortex_mutex_unlock (&pool->mutex);

			axl_list_append (pool->stopped, thread);
			vortex_mutex_unlock (&(ctx->thread_pool->stopped_mutex));

			vortex_async_queue_push (queue, INT_TO_PTR (3));

			/* unref the queue and return */
			vortex_async_queue_unref (queue);

			/* unref ctx */
			vortex_ctx_unref2 (&ctx, "end pool dispatcher");
			return NULL;
		} /* end if */

		/* check stop in progress signal */
		if ((PTR_TO_INT (task) == 1) && ctx->thread_pool_being_stopped) {
			vortex_log (VORTEX_LEVEL_DEBUG, "--> thread from pool stoping, found finish beacon");

			/* unref the queue and return */
			vortex_async_queue_unref (queue);
			
			vortex_ctx_unref2 (&ctx, "end pool dispatcher");
			return NULL;
		} /* end if */

		vortex_log (VORTEX_LEVEL_DEBUG, "--> thread from pool processing new job");

		/* grab references to release before call */
		func = task->func;
		data = task->data;
		axl_free (task);

		/* do automatic reasize (preemtive) */
		if (ctx && ctx->thread_pool && ctx->thread_pool->preemtive)
			__vortex_thread_pool_automatic_resize (ctx);

		/* at this point we already are executing inside a thread */
		if (! ctx->thread_pool_being_stopped && ! ctx->vortex_exit) 
			func (data);

		/* call to process events after finishing tasks */
		__vortex_thread_pool_process_events (ctx, pool);

		/* do automatic reasize */
		if (ctx && ctx->thread_pool && ! ctx->thread_pool->preemtive)
			__vortex_thread_pool_automatic_resize (ctx);

		vortex_log (VORTEX_LEVEL_DEBUG, "--> thread from pool waiting for jobs");

	} /* end if */
		
	/* That's all! */
	vortex_ctx_unref2 (&ctx, "end pool dispatcher");
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

#if defined(AXL_OS_WIN32)
	/* check to not dealock ourselves */
	if (thread->id == (int) GetCurrentThreadId ()) {
		axl_free (thread);
		return;
	}
#endif

	/* dealloc the node allocated */
	vortex_thread_destroy (thread, axl_true);
	return;
}

/**
 * \defgroup vortex_thread_pool Vortex Thread Pool: Pool of threads which runns user defined async notifications.
 */

/**
 * \addtogroup vortex_thread_pool
 * @{
 */

int  __vortex_thread_pool_soon_events_first (axlPointer a, axlPointer b)
{
	VortexThreadPoolEvent * event_a = a;
	VortexThreadPoolEvent * event_b = a;

	/* first a if microseconds is sort */
	if (event_a->delay < event_b->delay)
		return -1;
	return 1;
}

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
	vortex_log (VORTEX_LEVEL_DEBUG, "creating thread pool threads=%d", max_threads);

	/* create the thread pool and its internal values */
	if (ctx->thread_pool == NULL)
		ctx->thread_pool      = axl_new (VortexThreadPool, 1);

	if (ctx->thread_pool->threads != NULL) {
		/* clear list */
		while (axl_list_length (ctx->thread_pool->threads) > 0) {
			vortex_log (VORTEX_LEVEL_DEBUG, "releasing previous thread object allocated, length: %d", axl_list_length (ctx->thread_pool->threads));

			/* get thread object */
			thread = axl_list_get_first (ctx->thread_pool->threads);
			/* remove from the list */
			axl_list_unlink_first (ctx->thread_pool->threads);
			/* release */
			axl_free (thread);
		} /* end while */
		axl_list_free (ctx->thread_pool->threads);
		axl_list_free (ctx->thread_pool->events);
		axl_list_cursor_free (ctx->thread_pool->events_cursor);
		axl_list_free (ctx->thread_pool->stopped);
	} /* end if */

	ctx->thread_pool->threads       = axl_list_new (axl_list_always_return_1, __vortex_thread_pool_terminate_thread);
	ctx->thread_pool->stopped       = axl_list_new (axl_list_always_return_1, __vortex_thread_pool_terminate_thread);
	ctx->thread_pool->events        = axl_list_new (__vortex_thread_pool_soon_events_first, __vortex_thread_pool_unref_event);
	ctx->thread_pool->events_cursor = axl_list_cursor_new (ctx->thread_pool->events);
	ctx->thread_pool->ctx           = ctx;

	/* init the queue */
	if (ctx->thread_pool->queue != NULL)
		vortex_async_queue_release (ctx->thread_pool->queue);
	ctx->thread_pool->queue       = vortex_async_queue_new ();

	/* init mutex */
	vortex_mutex_create (&(ctx->thread_pool->mutex));
	vortex_mutex_create (&(ctx->thread_pool->stopped_mutex));
	
	/* init all threads required */
	vortex_thread_pool_add (ctx, max_threads);
	return;
}

/** 
 * @brief Allows to configure thread pool with several parameters to
 * react to working peaks on thread in pool.
 *
 * By default, thread pool has a number of threads that are used to
 * dispatch work generated by the engine (especially frame received,
 * channel start handlers, etc).
 *
 * In the case more work is received than available workers on the
 * pool, then these work is queued as pending tasks (internally at
 * thread pools queue).
 *
 * However, in the case it is required to make the vortex engine to
 * react to working peaks adding more threads to the pool, this
 * function can be used.
 *
 * @param ctx The context where the thread pool to be configured resides. 
 *
 * @param thread_max_limit Max thread limit (which is the upper number
 * of threads that will never be exceeded. 
 *
 * @param thread_add_step In the case the engine detects there are not
 * enough threads, how many thread add to the pool at once. 
 *
 * @param thread_add_period This is the adding period, that is, in the
 * case threads are added to the pool, no additional add operation
 * will happen until next thread_add_period. Period is measured in
 * seconds. This value can also be seen as the period during which the
 * thread pool must find all threads from the pool are working (busy)
 * so he can start adding more threads (as defined by
 * thread_add_step).
 *
 * @param auto_remove In the case axl_true is provided, the thread
 * pool will remove threads added when it is detected no pending tasks
 * are available, using thread_add_period as reference, and remove
 * threads as provided by the thread_add_step variable. In the case
 * axl_false is removed, no thread will be removed.
 *
 * For example, in the case you want to add 1 thread each 5 seconds
 * when a working peak is detected, setup thread_add_step=1 and
 * thread_add_period=5.
 *
 * To disable thread pool automatic add operation, do a call with
 * thread_max_limit equal to -1. This will leave the engine with
 * current threads created.
 *
 * You can call to this function at any time to change current thread
 * pool configuration.
 * 
 */
void vortex_thread_pool_setup               (VortexCtx * ctx, 
					     int         thread_max_limit, 
					     int         thread_add_step,
					     int         thread_add_period, 
					     axl_bool    auto_remove)
{
	/* call full function */
	vortex_thread_pool_setup2 (ctx, thread_max_limit, 
				   thread_add_step, thread_add_period,
				   thread_add_step, thread_add_period * 2,
				   auto_remove, axl_false);
	return;
}

void vortex_thread_pool_setup2              (VortexCtx * ctx, 
					     int         thread_max_limit, 
					     int         thread_add_step,
					     int         thread_add_period, 
					     int         thread_remove_step,
					     int         thread_remove_period, 
					     axl_bool    auto_remove,
					     axl_bool    preemtive)
{
	v_return_if_fail (ctx);

	/* lock the thread pool */
	vortex_mutex_lock (&(ctx->thread_pool->mutex));

	/* setup thread pool automatic add */
	ctx->thread_pool->automatic_resize_status = (thread_max_limit != -1);
	ctx->thread_pool->thread_max_limit        = thread_max_limit;
	ctx->thread_pool->thread_add_step         = thread_add_step;
	ctx->thread_pool->thread_add_period       = thread_add_period;
	ctx->thread_pool->thread_remove_step      = thread_remove_step;
	ctx->thread_pool->thread_remove_period    = thread_remove_period;
	ctx->thread_pool->auto_remove             = auto_remove;
	ctx->thread_pool->preemtive               = preemtive;

	vortex_log2 (VORTEX_LEVEL_DEBUG, "Automatic thread pool resize status: %d", ctx->thread_pool->automatic_resize_status);

	/* get current thread number to use it as reference */
	ctx->thread_pool->base_thread_num         = axl_list_length (ctx->thread_pool->threads);

	/* init last */
	gettimeofday (&(ctx->thread_pool->last), NULL);

	/* unlock the thread pool */
	vortex_mutex_unlock (&(ctx->thread_pool->mutex));

	return;
}

/** 
 * @internal Unlocked thread add implementation.
 *
 * @param ctx The context where the thread pool to be increased.
 * @param threads The amount of threads to add into the pool.
 *
 */
void vortex_thread_pool_add_internal                 (VortexCtx        * ctx, 
						      int                threads)
{
	int                       iterator;
	VortexThread            * thread;
	VortexThreadPoolStarter * starter;
	VortexCtx               * local_ctx;

	v_return_if_fail (ctx);
	v_return_if_fail (threads > 0);

	vortex_log (VORTEX_LEVEL_DEBUG, "adding %d threads to the pool %p, current threads are=%d",
		    threads, ctx, axl_list_length (ctx->thread_pool->threads));

	iterator = 0;
	while (iterator < threads) {
		/* create the thread */
		thread          = axl_new (VortexThread, 1);
		if (thread == NULL)
			break;
		starter         = axl_new (VortexThreadPoolStarter, 1);
		if (starter == NULL) {
			axl_free (thread);
			break;
		} /* end if */
		starter->thread = thread;
		starter->pool   = ctx->thread_pool;

		/* update the reference counting for this thread to
		 * the queue */
		if (! vortex_async_queue_ref (ctx->thread_pool->queue)) {
			axl_free (thread);
			break;
		}
		starter->queue = ctx->thread_pool->queue;

		/* acquire a reference to the context */
		vortex_ctx_ref2 (ctx, "begin pool dispatcher");

		if (! vortex_thread_create (thread,
					    /* function to execute */
					    (VortexThreadFunc)__vortex_thread_pool_dispatcher,
					    /* a reference to the thread pool and the thread reference started */
					    starter,
					    /* finish thread configuration */
					    VORTEX_THREAD_CONF_END)) {

			/* unref the queue */
			vortex_async_queue_unref (ctx->thread_pool->queue);

			/* failed, release ctx */
			local_ctx = ctx;
			vortex_ctx_unref2 (&local_ctx, "(failed) begin pool dispatcher");

			/* free the reference */
			vortex_thread_destroy (thread, axl_true);

			vortex_log (VORTEX_LEVEL_CRITICAL, "unable to create a thread required for the pool");
			return;
		} /* end if */

		/* store the thread reference */
		axl_list_add (ctx->thread_pool->threads, thread);

		/* update the iterator */
		iterator++;

	} /* end if */	

	return;
}

/** 
 * @brief Allows to increase the thread pool running on the provided
 * context with the provided amount of threads.
 *
 * @param ctx The context where the thread pool to be increased.
 * @param threads The amount of threads to add into the pool.
 *
 */
void vortex_thread_pool_add                 (VortexCtx        * ctx, 
					     int                threads)
{

	v_return_if_fail (ctx);
	v_return_if_fail (threads > 0);

	/* lock the thread pool */
	vortex_mutex_lock (&(ctx->thread_pool->mutex));
	
	/* call internal unlocked implementation */
	vortex_thread_pool_add_internal (ctx, threads);

	/* (un)lock the thread pool */
	vortex_mutex_unlock (&(ctx->thread_pool->mutex));
	return;
}

/** 
 * @internal Unlocked Implementation that allows to decrease the
 * thread pool running on the provided context with the provided
 * amount of threads.
 *
 * @param ctx The context where the thread pool will be decreased.
 * @param threads The amount of threads to remove from the pool.
 * 
 * The amount of threads can't be lower than current available threads
 * from the pool and, the thread pool must have at minimum one thread
 * running.
 */
void vortex_thread_pool_remove_internal        (VortexCtx        * ctx, 
						int                threads)
{
	int threads_running;
	if (ctx == NULL || threads <= 0)
		return;

	threads_running = axl_list_length (ctx->thread_pool->threads);
	while (threads > 0 && threads_running > 1) {
		/* push a task to stop one thread */
		vortex_async_queue_push (ctx->thread_pool->queue, INT_TO_PTR (2));
		threads--;
		threads_running--;
	} /* end if */

	return;
}

/** 
 * @brief Allows to decrease the thread pool running on the provided
 * context with the provided amount of threads.
 *
 * @param ctx The context where the thread pool will be decreased.
 * @param threads The amount of threads to remove from the pool.
 * 
 * The amount of threads can't be lower than current available threads
 * from the pool and, the thread pool must have at minimum one thread
 * running.
 */
void vortex_thread_pool_remove        (VortexCtx        * ctx, 
				       int                threads)
{
	if (ctx == NULL || threads <= 0)
		return;

	/* lock the mutex and get current started threads */
	vortex_mutex_lock (&ctx->thread_pool->mutex);

	/* call unlocked internal */
	vortex_thread_pool_remove_internal (ctx, threads);

	vortex_mutex_unlock (&ctx->thread_pool->mutex);
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
	int             iterator;
	VortexThread  * thread;

	vortex_log (VORTEX_LEVEL_DEBUG, "stopping thread pool..");

	/* flag the queue to be stoping */
	vortex_mutex_lock (&ctx->thread_pool->mutex);
	ctx->thread_pool_being_stopped = axl_true;
	vortex_mutex_unlock (&ctx->thread_pool->mutex);

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
	if (ctx->skip_thread_pool_wait) {
		vortex_log (VORTEX_LEVEL_DEBUG, "found skip thread finish wait");
		/* remove all threads */
		while (axl_list_length (ctx->thread_pool->threads) > 0) {
			/* get reference to the thread to be not waited */
			thread = axl_list_get_first (ctx->thread_pool->threads);

			/* remove from the list */
			axl_list_unlink_first (ctx->thread_pool->threads);

#if defined(AXL_OS_UNIX)
			/* flag is as detached */
			pthread_detach (*thread);
#endif
			/* release object */
			axl_free (thread);

		}
	} /* end if */

	axl_list_free (ctx->thread_pool->threads);
	axl_list_free (ctx->thread_pool->events);
	axl_list_cursor_free (ctx->thread_pool->events_cursor);
	axl_list_free (ctx->thread_pool->stopped);

	/* unref the queue */
	vortex_async_queue_unref (ctx->thread_pool->queue);

	/* terminate mutex */
	vortex_mutex_destroy (&ctx->thread_pool->mutex);
	vortex_mutex_destroy (&(ctx->thread_pool->stopped_mutex));

	/* free the node itself */
	axl_free (ctx->thread_pool);
	ctx->thread_pool = NULL;

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

	/* lock thread pool */
	vortex_mutex_lock (&ctx->thread_pool->mutex);

	ctx->thread_pool_being_stopped = axl_true;

	/* unlock thread pool */
	vortex_mutex_unlock (&ctx->thread_pool->mutex);
	
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
	if (func == NULL || ctx == NULL || ctx->thread_pool == NULL || ctx->thread_pool_being_stopped)
		return;

	/* create the task data */
	task       = malloc (sizeof (VortexThreadPoolTask));

	/* check allocated result */
	if (task == NULL)
		return;
	task->func = func;
	task->data = data;

	/* queue the task for the next available thread */
	vortex_async_queue_push (ctx->thread_pool->queue, task);

	return;
}

/** 
 * @brief Allows to install a new async event represented by the event
 * handler provided. This async event represents a handler called at
 * the interval defined by microseconds, optionally refreshing that
 * period if the event handler returns axl_false.
 *
 * The event handler will be called after microseconds provided has
 * expired. And if the handler returns axl_true (remove) the event
 * will be cleared and called no more.
 *
 * Note that events installed on this function must be tasks that
 * aren't loops or takes too long to complete. This is because the
 * thread pool asigns one thread to check and execute pending events,
 * so, if one of those events delays, the rest won't be executed until
 * the former finishes. In the case you want to install a loop handler
 * or some handler that executes a long running code, then use \ref
 * vortex_thread_pool_new_task.
 *
 * @param ctx The VortexCtx context where the event will be
 * installed. This is provided because event handlers are handled by
 * the vortex thread pool. This parameter can't be NULL.
 *
 * @param microseconds The amount of time to wait before calling to
 * event handler. This value must be > 0.
 *
 * @param event_handler The handler to be called after microseconds value has
 * expired. This parameter can't be NULL.
 *
 * @param user_data User defined pointer to data to be passed to the
 * event handler.
 *
 * @param user_data2 Second user defined pointer to data to be passed
 * to the event handler.
 *
 * @return The method returns the event identifier. This identifier
 * can be used to remove the event by using
 * vortex_thread_pool_remove_event. The function returns -1 in case of
 * failure.
 */
int  vortex_thread_pool_new_event           (VortexCtx              * ctx,
					     long                     microseconds,
					     VortexThreadAsyncEvent   event_handler,
					     axlPointer               user_data,
					     axlPointer               user_data2)
{
	/* get current context */
	VortexThreadPoolEvent * event;

	/* check parameters */
	if (event_handler == NULL || ctx == NULL || ctx->thread_pool == NULL || ctx->thread_pool_being_stopped)
		return -1;

	/* lock the thread pool */
	vortex_mutex_lock (&(ctx->thread_pool->mutex));

	/* create the event data */
	event            = axl_new (VortexThreadPoolEvent, 1);
	/* check alloc result */
	if (event) {
		event->func      = event_handler;
		event->ref_count = 1;
		event->data      = user_data;
		event->data2     = user_data2;
		event->delay     = microseconds;
		gettimeofday (&event->next_step, NULL);

		/* update next step to the appropiate value */
		__vortex_thread_pool_increase_stamp (event);

		/* add into the event event */
		axl_list_add (ctx->thread_pool->events, event);
	} /* end if */

	/* (un)lock the thread pool */
	vortex_mutex_unlock (&(ctx->thread_pool->mutex));

	/* in case of failure */
	if (event == NULL)
		return PTR_TO_INT (-1);
	
	return PTR_TO_INT (event);
}

/** 
 * @brief Allows to remove an event installed by \ref vortex_thread_pool_new_event.
 *
 * @param ctx The context where the event was created.
 * @param event_id The event id to remove.
 *
 * @return axl_true if the event was removed, otherwise axl_false is
 * returned. Note the function also returns axl_false when ctx == NULL
 * is received.
 */
axl_bool vortex_thread_pool_remove_event        (VortexCtx              * ctx,
						 int                      event_id)
{
	VortexThreadPoolEvent * event;

	if (ctx == NULL || ctx->thread_pool == NULL)
		return axl_true; /* remove event because we are
				  * receiving ctx or ctx->thread_pool
				  * NULL */

	/* v_return_val_if_fail (ctx, axl_false); */

	/* lock the thread pool */
	vortex_mutex_lock (&(ctx->thread_pool->mutex));

	/* reset cursor list */
	axl_list_cursor_first (ctx->thread_pool->events_cursor);
	while (axl_list_cursor_has_item (ctx->thread_pool->events_cursor)) {

		/* get event at the current position */
		event = axl_list_cursor_get (ctx->thread_pool->events_cursor);

		if (PTR_TO_INT (event) == event_id) {
			/* found event to remove */
			axl_list_cursor_remove (ctx->thread_pool->events_cursor);

			vortex_log (VORTEX_LEVEL_DEBUG, "Removing event id %d, total events registered after removal: %d",
				    event_id, axl_list_length (ctx->thread_pool->events));

			/* unlock the thread pool */
			vortex_mutex_unlock (&(ctx->thread_pool->mutex));

			return axl_true; /* event removed */
		} /* end if */
		
		/* next position */
		axl_list_cursor_next (ctx->thread_pool->events_cursor);
	} /* end if */

	/* unlock the thread pool */
	vortex_mutex_unlock (&(ctx->thread_pool->mutex));
	return axl_false; /* not removed */
}

/** 
 * @brief Allows to get current stats of the vortex thread pool. The
 * function returns the number of started threads (threads initialized
 * at \ref vortex_init_ctx), waiting threads (threads that aren't
 * processing any job) and pending tasks (the amount of pending tasks
 * to be processed by the pool (this includes frame notifications,
 * connection close notifications and so on).
 *
 * @param ctx The vortex context. If NULL is received, the function do not return any stat. 
 *
 * @param running_threads The number of threads currently
 * running. Optional argument. -1 in case of NULL ctx.
 *
 * @param waiting_threads The number of waiting threads. Optional
 * argument. -1 in case of NULL ctx.
 *
 * @param pending_tasks The number of pending tasks found in the pool
 * (jobs still not processed). Optional argument. -1 in case of NULL
 * ctx.
 */
void vortex_thread_pool_stats               (VortexCtx        * ctx,
					     int              * running_threads,
					     int              * waiting_threads,
					     int              * pending_tasks)
{
	/* clear variables received */
	if (running_threads)
		*running_threads = 0;
	if (waiting_threads)
		*waiting_threads = 0;
	if (pending_tasks)
		*pending_tasks = 0;
	/* check ctx reference */
	if (ctx == NULL)
		return;

	/* lock the thread pool */
	vortex_mutex_lock (&(ctx->thread_pool->mutex));

	/* update values */
	if (running_threads)
		*running_threads = axl_list_length (ctx->thread_pool->threads);
	if (waiting_threads)
		*waiting_threads = vortex_async_queue_waiters (ctx->thread_pool->queue);
	if (pending_tasks)
		*pending_tasks = vortex_async_queue_items (ctx->thread_pool->queue);

	/* lock the thread pool */
	vortex_mutex_unlock (&(ctx->thread_pool->mutex));

	return;
}

/** 
 * @brief Allows to get various stats from events installed.
 */
void vortex_thread_pool_event_stats         (VortexCtx        * ctx,
					     int              * events_installed)
{
	/* clear variables received */
	if (events_installed)
		*events_installed = 0;

	/* check ctx reference */
	if (ctx == NULL)
		return;

	/* lock the thread pool */
	vortex_mutex_lock (&(ctx->thread_pool->mutex));

	/* update values */
	if (events_installed)
		*events_installed = axl_list_length (ctx->thread_pool->events);

	/* lock the thread pool */
	vortex_mutex_unlock (&(ctx->thread_pool->mutex));
	
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
	int  value;

	/* get the number of threads to start */
	value = vortex_support_getenv_int ("VORTEX_THREADS");

	/* set as default value 5 if 0 or lower threads are returned */
	if (value <= 0)
		return 5;
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
 * value is already configured to axl_true. Set axl_false to make the
 * thread pool to behave in a non-exclusive form.
 */
void vortex_thread_pool_set_exclusive_pool  (VortexCtx * ctx,
					     axl_bool    value)
{
	/* set the new value */
	ctx->thread_pool_exclusive = value;

	return;
}
       

/* @} */
