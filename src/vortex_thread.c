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

#define LOG_DOMAIN "vortex-thread"

/** 
 * \defgroup vortex_thread Vortex Thread: Portable threading API for vortex
 */

/** 
 * \addtogroup vortex_thread
 * @{
 */

#if defined(AXL_OS_WIN32)

/** 
 * @internal Windows hack to allow executing threads as normal without
 * making the user to suffer the painfull windows thread api, look at
 * the following document and draw your own conclusions: 
 * 
 *   - http://msdn2.microsoft.com/en-us/library/kdzttdcb(VS.71).aspx
 *
 * Because:
 *
 *   - http://msdn2.microsoft.com/en-us/library/ms682453.aspx
 */
typedef struct _VortexThreadData {
	VortexThreadFunc func;
	axlPointer       user_data;
}VortexThreadData;

/** 
 * @internal Proxy function to properly call the user space function
 * for the thread.
 */
static unsigned __stdcall vortex_thread_proxy (axlPointer _data) 
{
	VortexThreadData * data = (VortexThreadData *) _data;
	
	/* call to the threading function */
	data->func (data->user_data);

	/* terminate thread */
	_endthreadex (0);

	/* return nothing */
	return 0;
	
} /* end vortex_thread_proxy */


#endif

/** 
 * @internal Creates a new thread, executing the function provided,
 * passing the referece received to the function (user_data).
 *
 * For complete examples on how to create threads, see \ref  VortexThreadConf documentation.
 *
 * @param thread_def A reference to the thread identifier created by
 * the function. This parameter is not optional.
 * 
 * @param func The function to execute.
 *
 * @param user_data User defined data to be passed to the function to
 * be executed by the newly created thread.
 * 
 * @return The function returns axl_true if the thread was created
 * properly and the variable thread_def is defined with the particular
 * thread reference created.
 *
 * @see vortex_thread_destroy
 */
axl_bool  vortex_thread_create_internal (VortexThread      * thread_def,
					 VortexThreadFunc    func, 
					 axlPointer          user_data,
					 va_list             args)
{
	/* default configuration for joinable state, axl_true */
#if defined(AXL_OS_UNIX)
	VortexThreadConf   conf;
	axl_bool           joinable = axl_true;
	pthread_attr_t     attr;
#elif defined(AXL_OS_WIN32)
	VortexThreadData * data;
	/* unsigned           thread_id; */
#endif

	/* do some basic checks */
	v_return_val_if_fail (thread_def, axl_false);
	v_return_val_if_fail (func, axl_false);
	
	/* open arguments to get the thread configuration */
#if defined(AXL_OS_UNIX)
	do {
		/* get next configuration */
		conf = va_arg (args, axl_bool);
		if (conf == 0) {
			/* finished argument processing */
			break;
		} /* end if */
		switch (conf) {
		case VORTEX_THREAD_CONF_JOINABLE:
			/* thread joinable state configuration, get the parameter */
			joinable = va_arg (args, axl_bool);
			break;
		case VORTEX_THREAD_CONF_DETACHED:
			joinable = axl_false;
			break;
		default:
			return axl_false;
		} /* end if */
	} while (1);
#endif

	

#if defined(AXL_OS_WIN32)
	/* windows implementation */
	/* create the thread data to pass it to the proxy function */
	data                 = axl_new(VortexThreadData, 1);
	VORTEX_CHECK_REF (data, axl_false);

	data->func           = func;
	data->user_data      = user_data;
	thread_def->data     = data;
	thread_def->handle   = (HANDLE) _beginthreadex  (
		/* use default SECURITY ATTRIBUTES */
		NULL, 
		/* use default stack size */
		0,  
		/* function to execute */
		vortex_thread_proxy,
		/* data to be passed to the function */
		data,
		/* no init flags */
		0,
		/* provide a reference that we will ignore */
		&thread_def->id);
	
	if (thread_def->handle == NULL) {
		/* free data because thread wasn't created */
		axl_free (data);
		
		/* report */
		return axl_false;
	} /* end if */

#elif defined(AXL_OS_UNIX)
	/* unix implementation */
	/* init pthread attributes variable */
	if (pthread_attr_init (&attr) != 0) {
		return axl_false;
	} /* end if */
	
	/* configure joinable state */
	if (pthread_attr_setdetachstate (&attr, joinable ? PTHREAD_CREATE_JOINABLE : PTHREAD_CREATE_DETACHED) != 0) {
		/* vortex_log (VORTEX_LEVEL_CRITICAL, "failed to configure detached state for the thread (pthread_attr_setdetachstate system call failed)"); */
		pthread_attr_destroy(&attr);
		return axl_false;
	} /* end if */
	
	/* create the thread man! */
	if (pthread_create (thread_def,
			    &attr, 
			    /* function and user data */
			    func, user_data) != 0) {
		/* vortex_log (VORTEX_LEVEL_CRITICAL, "unable to create the new thread (pthread_create system call failed)");*/
		pthread_attr_destroy(&attr);
		return axl_false;
	} /* end if */
	
	pthread_attr_destroy (&attr);

#endif
	return axl_true;
}

/** 
 * @internal Wait for the provided thread to finish, destroy its
 * resources and optionally release its pointer.
 * 
 * @param thread_def A reference to the thread that must be destroyed.
 *
 * @param free_data Boolean that set whether the thread pointer should
 * be released or not.
 * 
 * @return axl_true if the destroy operation was ok, otherwise axl_false is
 * returned.
 */
axl_bool  vortex_thread_destroy_internal (VortexThread * thread_def, axl_bool  free_data)
{
#if defined(AXL_OS_WIN32)

	DWORD err;
	err = WaitForSingleObject(thread_def->handle, INFINITE);
	switch (err) {
	default:
	case WAIT_OBJECT_0:
		/* vortex_log (VORTEX_LEVEL_DEBUG, "thread %p stopped", thread_def->handle); */
		break;
	case WAIT_ABANDONED:
		/* vortex_log (VORTEX_LEVEL_DEBUG, "unable to stop thread %p, wait_abandoned", thread_def->handle); */
		break;

	case WAIT_TIMEOUT:
		/* vortex_log (VORTEX_LEVEL_DEBUG, "unable to stop thread %p, wait_timeout", thread_def); */
		break;
	}
	
	CloseHandle (thread_def->handle);
	axl_free (thread_def->data);
	if (free_data) 
		axl_free (thread_def);
	
	return err == WAIT_OBJECT_0;

#elif defined(AXL_OS_UNIX)

	void      * status;
	int         err = pthread_join (*thread_def, &status);
	/* VortexCtx * ctx = NULL; */
	switch (err) {
	default:
	case 0:
		/* vortex_log (VORTEX_LEVEL_DEBUG, "thread %p stopped with status %p", thread_def, status);  */
		break;

	case EINVAL:
		/* vortex_log (VORTEX_LEVEL_CRITICAL, "unable to stop non-joinable thread %p", thread_def);  */
		break;
		
	case ESRCH:
		/* vortex_log (VORTEX_LEVEL_CRITICAL, "unable to stop invalid thread %p", thread_def);  */
		break;

	case EDEADLK:
		/* vortex_log (VORTEX_LEVEL_CRITICAL, "unable to stop thread %p, deadlock detected", thread_def);  */
		pthread_detach (*thread_def);
		break;
	}
	
	if (free_data)
		axl_free(thread_def);
	return err == 0;
	
#endif
}

/** 
 * @internal Variables to hold the active thread management function
 * pointers.
 *
 * They are initialised to use the default Vortex functions. If the
 * user are not interested in using external threading functions he
 * doesn't need to do anything, or even know about this functionality.
 */
VortexThreadCreateFunc  __vortex_thread_create  = vortex_thread_create_internal;
VortexThreadDestroyFunc __vortex_thread_destroy = vortex_thread_destroy_internal;

/** 
 * @brief Creates a new thread, executing the function provided,
 * passing the referece received to the function (user_data).
 *
 * For complete examples on how to create threads, see \ref  VortexThreadConf documentation.
 *
 * @param thread_def A reference to the thread identifier created by
 * the function. This parameter is not optional.
 *
 * @param func The function to execute.
 *
 * @param user_data User defined data to be passed to the function to
 * be executed by the newly created thread.
 *
 * @return The function returns axl_true if the thread was created
 * properly and the variable thread_def is defined with the particular
 * thread reference created.
 *
 * @see vortex_thread_destroy
 */
axl_bool  vortex_thread_create (VortexThread      * thread_def,
                                VortexThreadFunc    func,
                                axlPointer          user_data,
                                ...)
{
	axl_bool     result;
	va_list      args;
	
	/* initialize the args value */
	va_start (args, user_data);
	
	/* create the thread */
	result = __vortex_thread_create (thread_def, func, user_data, args);
	
	/* end args values */
	va_end (args);
	
	return result;
}

/** 
 * @brief Wait for the provided thread to finish, destroy its
 * resources and optionally release its pointer.
 *
 * @param thread_def A reference to the thread that must be destroyed.
 *
 * @param free_data Boolean that set whether the thread pointer should
 * be released or not.
 *
 * @return axl_true if the destroy operation was ok, otherwise axl_false is
 * returned.
 */
axl_bool  vortex_thread_destroy (VortexThread * thread_def, axl_bool  free_data)
{
	return __vortex_thread_destroy (thread_def, free_data);
}

/** 
 * @brief Allows to specify the function Vortex library will call to create a
 * new thread.
 *
 * If the user does not have reason to change the default thread
 * creation mechanism this function can be ignored.
 *
 * NOTE: The thread mechanism functions (\ref vortex_thread_set_create
 * and \ref vortex_thread_set_destroy) must be set before any other
 * Vortex API calls are made. Changing the thread mechanism functions
 * while Vortex is running will most likely lead to memory corruption
 * or program crashes.
 *
 * @param create_fn The function to be executed to create a new
 * thread. Passing a NULL value restores to the default create
 * handler.
 *
 * @see vortex_thread_set_destroy
 */
void vortex_thread_set_create (VortexThreadCreateFunc create_fn)
{
	if (NULL != create_fn) 
		__vortex_thread_create = create_fn;
	else
		__vortex_thread_create = vortex_thread_create_internal;

	return;
}

/** 
 * @brief Allows to specify the function Vortex library will call to
 * destroy a thread's resources.
 *
 * If the user does not have reason to change the default thread
 * cleanup mechanism this function can be ignored.
 *
 * NOTE: The thread mechanism functions (\ref vortex_thread_set_create
 * and \ref vortex_thread_set_destroy) must be set before any other
 * Vortex API calls are made. Changing the thread mechanism functions
 * while Vortex is running will most likely lead to memory corruption
 * or program crashes.
 *
 * @param destroy_fn The function to be executed to clean up a
 * thread. Passing a NULL value restores to the default destroy
 * handler.
 *
 * @see vortex_thread_set_create
 */
void vortex_thread_set_destroy (VortexThreadDestroyFunc destroy_fn)
{
	if (NULL != destroy_fn) 
		__vortex_thread_destroy = destroy_fn;
	else 
		__vortex_thread_destroy = vortex_thread_destroy_internal;
}

/** 
 * @brief Allows to create a new non-recursive mutex to protect
 * critical sections to be executed by several threads at the same
 * time.
 *
 * To create a mutex you must:
 * \code
 * // declare a mutex 
 * VortexMutex mutex;
 *
 * // init it 
 * if (! vortex_mutex_create (&mutex)) {
 *    // failed to init mutex
 * } 
 * // mutex created
 * \endcode
 * 
 * @param mutex_def A reference to the mutex to be initialized.
 * 
 * @return axl_true if the function created the mutex, otherwise axl_false is
 * returned.
 */
axl_bool  vortex_mutex_create  (VortexMutex       * mutex_def)
{
	return vortex_mutex_create_full (mutex_def, VORTEX_MUTEX_CONF_NONRECURSIVE);
}


/** 
 * @brief Allows to create a new mutex to protect critical sections to
 * be executed by several threads at the same time, with extended options.
 *
 * To create a mutex you must:
 * \code
 * // declare a mutex 
 * VortexMutex mutex;
 *
 * // init it 
 * if (! vortex_mutex_create_full (&mutex)) {
 *    // failed to init mutex
 * } 
 * // mutex created
 * \endcode
 * 
 * @param mutex_def A reference to the mutex to be initialized.
 * 
 * @param conf 
 *
 * @return axl_true if the function created the mutex, otherwise axl_false is
 * returned.
 */
axl_bool           vortex_mutex_create_full (VortexMutex       * mutex_def, VortexMutexConf conf)
{
#if defined(AXL_OS_WIN32)
	VortexWin32Mutex* mutexWin32_def;
#endif

#if defined(AXL_OS_UNIX)
	pthread_mutexattr_t attr;
#endif

	v_return_val_if_fail (mutex_def, axl_false);

#if defined(AXL_OS_WIN32)
	/* create the mutex, without a name */
	mutexWin32_def = axl_new (VortexWin32Mutex, 1);
	if (mutexWin32_def == NULL)
		return axl_false; /* report failure on memory allocation */
	
	if ((conf & VORTEX_MUTEX_CONF_RECURSIVE) == VORTEX_MUTEX_CONF_RECURSIVE) {
		mutexWin32_def->mutex     = CreateMutex (NULL, FALSE, NULL); 
		mutexWin32_def->recursive = axl_true;
	} else if ((conf & VORTEX_MUTEX_CONF_NONRECURSIVE) == VORTEX_MUTEX_CONF_NONRECURSIVE) {
		mutexWin32_def->mutex     = CreateSemaphore (NULL, 1, 1, NULL);
		mutexWin32_def->recursive = axl_false;
	}

	/* record configuration */
	mutexWin32_def->conf      = conf;
	*mutex_def = mutexWin32_def;
	
#elif defined(AXL_OS_UNIX)
	if ((conf & VORTEX_MUTEX_CONF_NONRECURSIVE) == VORTEX_MUTEX_CONF_NONRECURSIVE) {
		/* init the mutex using default values */
		if (pthread_mutex_init (mutex_def, NULL) != 0) {
			/* vortex_log (VORTEX_LEVEL_CRITICAL, "unable to create mutex (system call pthread_mutex_init have failed)"); */
			return axl_false;
		} /* end if */
	} else 	if ((conf & VORTEX_MUTEX_CONF_RECURSIVE) == VORTEX_MUTEX_CONF_RECURSIVE) {
		pthread_mutexattr_init (&attr);
		pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE);

		/* create recursive mutex */
		if (pthread_mutex_init (mutex_def, &attr) != 0) {
			pthread_mutexattr_destroy (&attr);
			/* vortex_log (VORTEX_LEVEL_CRITICAL, "unable to create mutex (system call pthread_mutex_init have failed)"); */
			return axl_false;
		} /* end if */
		pthread_mutexattr_destroy (&attr);
	} /* end if */
#endif
	/* mutex created */
	return axl_true;
}

/** 
 * @brief Destroy the provided mutex, freeing the resources it might
 * hold. The mutex must be unlocked on entrance
 * 
 * @param mutex_def A reference to the mutex that must be deallocated
 * its resources, and report the system to close it.
 * 
 * @return axl_true if the destroy operation was ok, otherwise axl_false is
 * returned.
 */
axl_bool  vortex_mutex_destroy (VortexMutex       * mutex_def)
{
#if defined(AXL_OS_WIN32)
	VortexWin32Mutex* mutexWin32_def;
#endif
	v_return_val_if_fail (mutex_def, axl_false);

#if defined(AXL_OS_WIN32)
	/* close the mutex */
	mutexWin32_def = (VortexWin32Mutex*) *mutex_def;
	if (mutexWin32_def == NULL)
		return axl_false; /* skip working with nullified versions */
	
	CloseHandle (mutexWin32_def->mutex);
	mutexWin32_def->mutex = NULL;
	axl_free (mutexWin32_def);
	
#elif defined(AXL_OS_UNIX)
	/* close the mutex */
	if (pthread_mutex_destroy (mutex_def) != 0) {
		/* vortex_log (VORTEX_LEVEL_CRITICAL, "unable to destroy the mutex (system call pthread_mutex_destroy have failed)"); */
		return axl_false;
	} /* end if */
	
#endif
	/* mutex closed */
	return axl_true;
	
} /* end vortex_mutex_destroy */

/** 
 * @brief Locks the given mutex. If the mutex is currently unlocked,
 * it becomes locked and owned by the calling thread, and
 * \ref vortex_mutex_lock returns immediately. If the mutex is already
 * locked by another thread, \ref vortex_mutex_lock suspends the calling
 * thread until the mutex is unlocked.
 * 
 * @param mutex_def The reference to the mutex to be locked. If the
 * mutex reference is NULL no lock operation is performed.
 *
 * NOTE: It is important to use the pair of calls to \ref
 * vortex_mutex_lock and \ref vortex_mutex_unlock from the same
 * thread. This is because under windows, the couple of functions
 * WaitForSingleObject and ReleaseMutex are used to implement lock and
 * unlocking. Under this context, a thread not owning a mutex
 * (acquired via WaitForSingleObject) can't release it with
 * ReleaseMutex. This has the case consequence that a thread that has
 * not called to vortex_mutex_lock cannot do a vortex_mutex_unlock
 * until the owner thread do it.
 */
void vortex_mutex_lock    (VortexMutex       * mutex_def)
{
#if defined(AXL_OS_WIN32)
	VortexWin32Mutex* mutexWin32_def;
#endif
	if (mutex_def == NULL)
		return;

#if defined(AXL_OS_WIN32)
	/* lock the mutex */
	mutexWin32_def = (VortexWin32Mutex*) *mutex_def;
	WaitForSingleObject (mutexWin32_def->mutex, INFINITE);
#elif defined(AXL_OS_UNIX)
	/* lock the mutex */
	if (pthread_mutex_lock (mutex_def) != 0) {
		/* vortex_log (VORTEX_LEVEL_CRITICAL, "unable to lock the mutex (system call pthread_mutex_lock have failed)"); */
		return;
	} /* end if */
#endif
	/* mutex locked */
	return;
}

/** 
 * @brief Unlocks the given mutex. The mutex is assumed to be locked
 * and owned by the calling thread on entrance to
 * \ref vortex_mutex_unlock.
 * 
 * @param mutex_def The mutex handle to unlock. If the reference is
 * NULL, no unlock operation is performed.
 */
void vortex_mutex_unlock  (VortexMutex       * mutex_def)
{
#if defined(AXL_OS_WIN32)
	VortexWin32Mutex* mutexWin32_def;
#endif
	if (mutex_def == NULL)
		return;

#if defined(AXL_OS_WIN32)
	/* unlock mutex */
	mutexWin32_def = (VortexWin32Mutex*) *mutex_def;
	if (mutexWin32_def->recursive)
		ReleaseMutex (mutexWin32_def->mutex);
	else
		ReleaseSemaphore (mutexWin32_def->mutex, 1, NULL);
	
#elif defined(AXL_OS_UNIX)
	/* unlock mutex */
	if (pthread_mutex_unlock (mutex_def) != 0) {
		/* vortex_log (VORTEX_LEVEL_CRITICAL, "unable to unlock the mutex (system call pthread_mutex_unlock have failed)"); */
		return;
	} /* end if */
#endif
	/* mutex unlocked */
	return;
}

/** 
 * @brief Initializes the condition variable cond
 * 
 * @param cond The conditional variable to initialize.
 * 
 * @return axl_true if the conditional variable was initialized, otherwise
 * axl_false is returned.
 */
axl_bool  vortex_cond_create    (VortexCond        * cond)
{
	v_return_val_if_fail (cond, axl_false);

#if defined(AXL_OS_WIN32)
	
	cond->waiters_count_ = 0;
	cond->was_broadcast_ = 0;
	
	cond->sema_ = CreateSemaphore (
		/* no security */
		NULL,       
		/* initially 0 */
		0,
		/* max count */
		0x7fffffff, 
		/* unnamed */
		NULL);      

	InitializeCriticalSection (&cond->waiters_count_lock_);

	cond->waiters_done_ = CreateEvent (
		/* no security */
		NULL,  
		/* auto-reset */
		axl_false, 
		/* non-signaled initially */
		axl_false, 
		/* unnamed */
		NULL); 


#elif defined(AXL_OS_UNIX)
	/* init the conditional variable */
	if (pthread_cond_init (cond, NULL) != 0) {
		/* vortex_log (VORTEX_LEVEL_CRITICAL, "unable to init conditional variable (system call pthread_cond_init have failed)"); */
		return axl_false;
	} /* end if */
#endif
	/* cond created */
	return axl_true;
}

/** 
 * @brief Restarts one of the threads that are waiting on the
 * condition variable cond. If no threads are waiting on cond, nothing
 * happens. If several threads are waiting on cond, exactly one is
 * restarted, but it is not specified which.
 * 
 * @param cond The conditional variable to signal.
 */
void vortex_cond_signal    (VortexCond        * cond)
{
#if defined(AXL_OS_WIN32)
	int have_waiters;
#endif
	v_return_if_fail (cond);

#if defined(AXL_OS_WIN32)

	/* enter inside the critical section and check current
	 * state */
	EnterCriticalSection (&cond->waiters_count_lock_);

	have_waiters = (cond->waiters_count_ > 0);

	LeaveCriticalSection (&cond->waiters_count_lock_);

	/* If there aren't any waiters, then this is a no-op.  */
	if (have_waiters)
		ReleaseSemaphore (cond->sema_, 1, 0);
	
#elif defined(AXL_OS_UNIX)
	/* signal condition */
	if (pthread_cond_signal (cond) != 0) {
		/* vortex_log (VORTEX_LEVEL_CRITICAL, "unable to signal conditional variable (system call pthread_cond_signal have failed)"); */
		return;
	} /* end if */
#endif
	/* signal ended */
	return;
}

/** 
 * @brief Restarts all the threads that are waiting on the condition
 * variable cond. Noth- ing happens if no threads are waiting on cond.
 * 
 * @param cond The conditional variable to broadcast to all its
 * waiters.
 */
void vortex_cond_broadcast (VortexCond        * cond)
{
#if defined(AXL_OS_WIN32)
	int have_waiters = 0;
#endif

	v_return_if_fail (cond);

#if defined(AXL_OS_WIN32)

	/* This is needed to ensure that <waiters_count_> and
	 * <was_broadcast_> are consistent relative to each other. */
	EnterCriticalSection (&cond->waiters_count_lock_);

	if (cond->waiters_count_ > 0) {
		/* We are broadcasting, even if there is just one
		 * waiter...  Record that we are broadcasting, which
		 * helps optimize <pthread_cond_wait> for the
		 * non-broadcast case. */
		 cond->was_broadcast_ = 1;
		 have_waiters = 1;
	} /* end if */

	if (have_waiters) {
		/* Wake up all the waiters atomically. */
		ReleaseSemaphore (cond->sema_, cond->waiters_count_, 0);

		LeaveCriticalSection (&cond->waiters_count_lock_);

		/* Wait for all the awakened threads to acquire the
		 * counting semaphore. */
		WaitForSingleObject (cond->waiters_done_, INFINITE);

		/* This assignment is okay, even without the
		 * <waiters_count_lock_> held because no other waiter
		 * threads can wake up to access it. */
		cond->was_broadcast_ = 0;
	} else 
		LeaveCriticalSection (&cond->waiters_count_lock_);	
	
#elif defined(AXL_OS_UNIX)
	/* broadcast condition */
	if (pthread_cond_broadcast (cond) != 0) {
		/* vortex_log (VORTEX_LEVEL_CRITICAL, "unable to signal conditional variable (system call pthread_cond_broadcast have failed)"); */
		return;
	} /* end if */
#endif
	/* broadcast ended */
	return;
}

#if defined(AXL_OS_WIN32)
/**
 * @internal Implementation to support vortex_cond_wait and
 * vortex_cond_timedwait under windows.
 */
axl_bool  __vortex_cond_common_wait_win32 (VortexCond * cond, VortexMutex * mutex, 
					   int milliseconds, axl_bool  wait_infinite)
{
	axl_bool   last_waiter;
	VortexWin32Mutex* mutexWin32_def;
	mutexWin32_def = (VortexWin32Mutex*) *mutex;

	/* Avoid race conditions. */
	EnterCriticalSection (&cond->waiters_count_lock_);
	cond->waiters_count_++;
	LeaveCriticalSection (&cond->waiters_count_lock_);

	/* This call atomically releases the mutex and waits on the
	 * semaphore until \ref vortex_cond_signal or \ref
	 * vortex_cond_broadcast> are called by another thread. */
	if (wait_infinite)
		SignalObjectAndWait (mutexWin32_def->mutex, cond->sema_, INFINITE, FALSE);
	else
		SignalObjectAndWait (mutexWin32_def->mutex, cond->sema_, milliseconds, FALSE);


	/* Reacquire lock to avoid race conditions. */
	EnterCriticalSection (&cond->waiters_count_lock_);

	/* We're no longer waiting... */
	cond->waiters_count_--;

	/* Check to see if we're the last waiter after
	 * \ref vortex_cond_broadcast>. */
	last_waiter = (cond->was_broadcast_ && cond->waiters_count_ == 0);

	LeaveCriticalSection (&cond->waiters_count_lock_);

	/* If we're the last waiter thread during this particular
	 * broadcast then let all the other threads proceed. */
	if (last_waiter) {
		/* This call atomically signals the <waiters_done_>
		 * event and waits until it can acquire the
		 * <mutex>.  This is required to ensure
		 * fairness.  */
		SignalObjectAndWait (cond->waiters_done_, mutexWin32_def->mutex, INFINITE, FALSE);
	} else {
		/* Always regain the mutex since that's the guarantee we
		 * give to our callers. */
		WaitForSingleObject (mutexWin32_def->mutex, INFINITE);
	} /* end if */

	return axl_true;
}
#endif

/** 
 * @brief Atomically unlocks the mutex (as per \ref
 * vortex_mutex_unlock) and waits for the condition variable cond to
 * be signaled. 
 * 
 * The thread execution is suspended and does not consume any CPU time
 * until the condition variable is signaled. The mutex must be locked
 * by the calling thread on entrance to \ref vortex_cond_wait.  Before
 * returning to the calling thread, \ref vortex_cond_wait re-acquires
 * mutex (as per \ref vortex_mutex_lock).
 * 
 * @param cond The conditional variable to oper.
 *
 * @param mutex The mutex that was to associate the condition.
 *
 * @return axl_true if no error was found, otherwise axl_false is returned.
 */
axl_bool  vortex_cond_wait      (VortexCond        * cond, 
				 VortexMutex       * mutex)
{
	v_return_val_if_fail (cond, axl_false);
	v_return_val_if_fail (mutex, axl_false);

#if defined(AXL_OS_WIN32)
	/* use common implementation */
	return __vortex_cond_common_wait_win32 (cond, mutex, 0, axl_true);

#elif defined(AXL_OS_UNIX)
	/* wait for the condition */
	if (pthread_cond_wait (cond, mutex) != 0) {
		/* vortex_log (VORTEX_LEVEL_CRITICAL, "unable to wait on conditional variable (system call pthread_cond_wait have failed)"); */
		return axl_false;
	} /* end if */
#endif
	/* wait ended */
	return axl_true;
}

/** 
 * @brief Atomically unlocks mutex and waits on cond, as
 * pthread_cond_wait does, but it also bounds the duration of the
 * wait.
 * 
 * @param cond The conditional variable to perform the wait operation.
 * @param mutex Mutex associated.
 * @param microseconds Amount of time to wait.
 *
 * @return axl_true if no error was found, otherwise axl_false is returned.
 */
axl_bool  vortex_cond_timedwait (VortexCond        * cond, 
				 VortexMutex       * mutex,
				 long                microseconds)
{
#if defined(AXL_OS_UNIX)
	/* variables for the unix case */
	int                  rc;
	struct timespec      timeout;
	struct timeval       now;
#endif
	v_return_val_if_fail (cond, axl_false);
	v_return_val_if_fail (mutex, axl_false);
	

#if defined(AXL_OS_WIN32)
	/* get the future stamp when the request must expire */
	__vortex_cond_common_wait_win32 (cond, mutex, microseconds / 1000, axl_false);
	
#elif defined(AXL_OS_UNIX) 

	/* get current stamp */
	rc              = gettimeofday(&now, NULL);
	timeout.tv_sec  = now.tv_sec;
	timeout.tv_nsec = now.tv_usec * 1000;

	/* vortex_log (VORTEX_LEVEL_DEBUG, "waiting from: %d.%d", timeout.tv_sec, timeout.tv_nsec);*/

	/* Convert from timeval to timespec */
	if ((microseconds + now.tv_usec) > 1000000) {
		/* microseconds configuration contains seconds */
		timeout.tv_sec += ((microseconds + now.tv_usec) / 1000000);
	}

	timeout.tv_nsec = ((microseconds + now.tv_usec) % 1000000) * 1000;
	/* vortex_log (VORTEX_LEVEL_DEBUG, "to (microseconds=%ld): %d.%d", 
	   microseconds, timeout.tv_sec, timeout.tv_nsec); */

	/* check result returned */
	rc = pthread_cond_timedwait (cond, mutex, (axlPointer) &timeout);
	if (rc != 0) {
		/* check timeout */
		if (rc == ETIMEDOUT) {
			/* vortex_log (VORTEX_LEVEL_WARNING, 
			   "timeout reached for conditional variable (system call pthread_cond_wait finished)"); */
			return axl_true;
		} /* end if */
		if (rc == VORTEX_EINTR) {
			/* vortex_log (VORTEX_LEVEL_WARNING, 
			   "timeout reached due to a signal received, restarting"); */
			return axl_false;
		}

		/* vortex_log (VORTEX_LEVEL_CRITICAL, 
		   "unable to wait timed on conditional variable (system call pthread_cond_wait have failed): code=%d", rc); */
		return axl_false;
	} /* end if */
#endif
	/* wait ended */
	return axl_true;
}

/** 
 * @brief Destroys a condition variable, freeing the resources it
 * might hold. No threads must be waiting on the condition variable on
 * entrance to pthread_cond_destroy.
 * 
 * @param cond The conditional variable to destroy.
 */
void vortex_cond_destroy   (VortexCond        * cond)
{
	v_return_if_fail (cond);

#if defined(AXL_OS_WIN32)
	/* close semaphore */
	CloseHandle (cond->sema_);

	/* close the event */
	CloseHandle (cond->waiters_done_);

	/* delete critical section */
	DeleteCriticalSection (&cond->waiters_count_lock_);

#elif defined(AXL_OS_UNIX)
	/* destroy condition */
	if (pthread_cond_destroy (cond) != 0) {
		/* vortex_log (VORTEX_LEVEL_CRITICAL, "unable to destroy conditional variable (system call pthread_cond_destroy have failed)");*/
		return;
	} /* end if */
#endif
	/* wait ended */
	return;
}

/** 
 * @internal Definition for the async queue.
 */
struct _VortexAsyncQueue {
	/** 
	 * @internal Mutex used to synchronize the implemetnation.
	 */
	VortexMutex   mutex;
	/** 
	 * @internal Conditional variable used to hang threads inside
	 * the mutex condition when no data is available.
	 */
	VortexCond    cond;
	/** 
	 * @internal The list of items stored in the list.
	 */
	axlList     * data;
	/** 
	 * @internal The number of waiting threads.
	 */
	int           waiters;
	/** 
	 * @internal Reference counting support.
	 */
	int           reference;
};

/** 
 * @brief Creates a new async message queue, a inter thread
 * communication that allows to communicate and synchronize data
 * between threads inside the same process.
 * 
 * Once created, you can use the following function to push and
 * retrieve data from the queue:
 * 
 *  - \ref vortex_async_queue_push
 *  - \ref vortex_async_queue_pop
 *  - \ref vortex_async_queue_timedpop
 *
 * You can increase the reference counting by using \ref
 * vortex_async_queue_ref. If the reference count reaches zero value,
 * the queue is deallocated.
 *
 * A particular useful function is \ref vortex_async_queue_length
 * which returns the number of queue items minus waiting threads. 
 * 
 * @return A newly created async queue, with a reference count equal
 * to 1. To dealloc it when no longer needed, use \ref
 * vortex_async_queue_unref. Note reference returned must be checked
 * to be not NULL (caused by memory allocation error).
 */
VortexAsyncQueue * vortex_async_queue_new       (void)
{
	VortexAsyncQueue * result;

	/* create the node */
	result            = axl_new (VortexAsyncQueue, 1);
	VORTEX_CHECK_REF (result, NULL);

	/* init list of stored items */
	result->data      = axl_list_new (axl_list_always_return_1, NULL);
	VORTEX_CHECK_REF2 (result->data, NULL, result, axl_free);

	/* init mutex and conditional variable */
	vortex_mutex_create (&result->mutex);
	vortex_cond_create  (&result->cond);
	
	/* reference counting support initialized to 1 */
	result->reference = 1;

	return result;
}

/** 
 * @brief Allows to push data into the queue.
 * 
 * @param queue The queue where data will be pushed.
 *
 * @param data A reference to the data to be pushed. It is not allowed
 * to push null references.
 *
 * @return axl_true In the case the item was pushed into the queue,
 * otherwise axl_false is returned.
 */
axl_bool           vortex_async_queue_push      (VortexAsyncQueue * queue,
						 axlPointer         data)
{
	v_return_val_if_fail (queue, axl_false);
	v_return_val_if_fail (data, axl_false);
	
	/* get the mutex */
	vortex_mutex_lock (&queue->mutex);

	/* push the data */
	axl_list_prepend (queue->data, data);

	/* signal if waiters are available */
	if (queue->waiters > 0)
		vortex_cond_signal (&queue->cond);

	/* unlock the mutex */
	vortex_mutex_unlock (&queue->mutex);
	
	return axl_true;
}

/** 
 * @brief Allows to push data into the queue withtout acquiring the
 * internal lock. This function must be used in conjuntion with \ref
 * vortex_async_queue_lock and \ref vortex_async_queue_unlock.
 * 
 * @param queue The queue where data will be pushed.
 *
 * @param data A reference to the data to be pushed. It is not allowed
 * to push null references.
 *
 * @return axl_true In the case the item was pushed into the queue,
 * otherwise axl_false is returned.
 */
axl_bool           vortex_async_queue_unlocked_push  (VortexAsyncQueue * queue,
						      axlPointer         data)
{

	v_return_val_if_fail (queue, axl_false);
	v_return_val_if_fail (data, axl_false);

	/* push the data */
	axl_list_prepend (queue->data, data);
	
	return axl_true;
}

/** 
 * @brief Allows to push data into the queue but moving the reference
 * provided into the queue head (causing next call to
 * vortex_async_queue_pop to receive this reference). This function
 * performs the same as \ref vortex_async_queue_push but skiping all
 * items already pushed.
 * 
 * @param queue The queue where data will be pushed.
 *
 * @param data A reference to the data to be pushed. It is not allowed
 * to push null references.
 *
 * @return axl_true In the case the item was pushed into the queue,
 * otherwise axl_false is returned.
 */
axl_bool           vortex_async_queue_priority_push  (VortexAsyncQueue * queue,
						      axlPointer         data)
{
	v_return_val_if_fail (queue, axl_false);
	v_return_val_if_fail (data, axl_false);
	
	/* get the mutex */
	vortex_mutex_lock (&queue->mutex);

	/* push the data at the head */
	axl_list_append (queue->data, data);

	/* signal if waiters are available */
	if (queue->waiters > 0)
		vortex_cond_signal (&queue->cond);

	/* unlock the mutex */
	vortex_mutex_unlock (&queue->mutex);
	
	return axl_true;
}

/** 
 * @brief Pop the first data available in the queue, locking
 * the calling if no data is available.
 *
 * The function is ensured to return with a reference to some data. 
 * 
 * @param queue The queue where data will be required.
 * 
 * @return A reference to the next data available.
 */
axlPointer         vortex_async_queue_pop       (VortexAsyncQueue * queue)
{
	axlPointer _result;

	v_return_val_if_fail (queue, NULL);

	/* get the mutex */
	vortex_mutex_lock (&queue->mutex);

	/* update the number of waiters */
	queue->waiters++;

	/* check if data is available */
	while (axl_list_length (queue->data) == 0)
		VORTEX_COND_WAIT (&queue->cond, &queue->mutex);

	/* get data from the queue */
	_result = axl_list_get_last (queue->data);
	
	/* remove the data from the queue */
	axl_list_remove_last (queue->data);

	/* decrease the number of waiters */
	queue->waiters--;

	/* unlock the mutex */
	vortex_mutex_unlock (&queue->mutex);
	
	return _result;
}

/** 
 * @brief Pop the first data available in the queue, locking the
 * calling if no data is available, but bounding the waiting to the
 * value provided.
 *
 * The function is ensured to return with a reference to some data.
 * 
 * @param queue The queue where data will be required.
 * 
 * @param microseconds The period to wait. 1 second = 1.000.000 microseconds. 
 * 
 * @return A reference to the next data available.
 * 
 * @param queue 
 * @param microseconds 
 * 
 * @return A reference to the data queue, or NULL if the timeout is
 * reached.
 */
axlPointer         vortex_async_queue_timedpop  (VortexAsyncQueue * queue,
						 long               microseconds)
{
	axlPointer _result;

#if defined(AXL_OS_WIN32)
	struct timeval stamp;
	struct timeval now;
	struct timeval diff;
#endif

	v_return_val_if_fail (queue, NULL);
	v_return_val_if_fail (microseconds > 0, NULL);

	/* get the mutex */
	vortex_mutex_lock (&queue->mutex);

	/* update the number of waiters */
	queue->waiters++;

	/* check timed wait */
	if (axl_list_length (queue->data) == 0) {

#if defined(AXL_OS_WIN32)
		/* get stamp to check after the following function
		 * returns */
		gettimeofday (&stamp, NULL);
	wait_again:
#endif
		/* check if data is available */
		vortex_cond_timedwait (&queue->cond, &queue->mutex, microseconds);

		/* check again the queue */
		if (axl_list_length (queue->data) == 0) {
#if defined(AXL_OS_WIN32)
			/* if the function finished and there is no
			 * data, check if the amount of time to
			 * perform the wait have really expired */
			gettimeofday (&now, NULL);
			
			/* substract and check */
			vortex_timeval_substract (&now, &stamp, &diff);
			
			/* check this extrange bevahiour provided by
			 * the win32 implementation of
			 * SignalObjectAndWait which seems to have a
			 * faulty behaviour on high load
			 * notifications */
			if (diff.tv_sec == 0 && diff.tv_usec == 0 && microseconds > 0) {
				goto wait_again; 
			}
#endif
			
			/* decrease the number of waiters */
			queue->waiters--;
			
			vortex_mutex_unlock (&queue->mutex);
			return NULL;
		} /* end if */
	} /* end if */

	/* get data from the queue */
	_result = axl_list_get_last (queue->data);
	
	/* remove the data from the queue */
	axl_list_remove_last (queue->data);

	/* decrease the number of waiters */
	queue->waiters--;

	/* unlock the mutex */
	vortex_mutex_unlock (&queue->mutex);
	
	return _result;
}

/** 
 * @brief Allows to get current queue status.
 * 
 * @param queue The queue to oper.
 * 
 * @return The number of items stored minus the number of thread
 * waiting. The function returns 0 if the reference received is null.
 */
int                vortex_async_queue_length    (VortexAsyncQueue * queue)
{
	int result;

	v_return_val_if_fail (queue, 0);

	/* get the mutex */
	vortex_mutex_lock (&queue->mutex);

	/* check status */
	result = axl_list_length (queue->data) - queue->waiters;

	/* unlock the mutex */
	vortex_mutex_unlock (&queue->mutex);

	return result;
}

/** 
 * @brief Allows to get current waiting threads on the provided queue.
 * 
 * @param queue The queue that is being used to request the number of
 * waiting threads.
 * 
 * @return The number of waiting threads or -1 if it fails. The only
 * way to make the function to fail is to provide a null queue
 * reference.
 */
int                vortex_async_queue_waiters   (VortexAsyncQueue * queue)
{
	int result;

	v_return_val_if_fail (queue, -1);

	/* get the mutex */
	vortex_mutex_lock (&queue->mutex);

	/* check status */
	result = queue->waiters;

	/* unlock the mutex */
	vortex_mutex_unlock (&queue->mutex);

	return result;
}

/** 
 * @brief Allows to get current items installed on the queue, pending
 * to be readed.
 * 
 * @param queue A reference to the queue that will be checked for its
 * pending data.
 * 
 * @return 0 or the number of data pending. -1 is returned if a null
 * reference is received.
 */
int                vortex_async_queue_items     (VortexAsyncQueue * queue)
{

	int result;

	v_return_val_if_fail (queue, -1);

	/* get the mutex */
	vortex_mutex_lock (&queue->mutex);

	/* check status */
	result = axl_list_length (queue->data);

	/* unlock the mutex */
	vortex_mutex_unlock (&queue->mutex);

	return result;
	
}

/** 
 * @brief Allows to update the reference counting for the provided queue.
 * 
 * @param queue The async queue to increase its reference.
 *
 * @return axl_true in the case the reference was properly incremented
 * and acquired, otherwise axl_false is returned, signalling the
 * caller he doesn't owns a reference as a consequence of calling to
 * this function.
 */
axl_bool               vortex_async_queue_ref       (VortexAsyncQueue * queue)
{
	v_return_val_if_fail (queue, axl_false);
	
	if (! (queue->reference > 0))
	    return axl_false;

	/* get the mutex */
	vortex_mutex_lock (&queue->mutex);

	/* update reference */
	queue->reference++;

	/* unlock the mutex */
	vortex_mutex_unlock (&queue->mutex);

	return queue->reference > 1;
}

/** 
 * @brief Returns current reference counting for the provided queue.
 *
 * @param queue The async queue to get reference counting from.
 *
 * @return The reference counting or -1 if it fails.
 */
int                vortex_async_queue_ref_count (VortexAsyncQueue * queue)
{
	int result;

	v_return_val_if_fail (queue, -1);

	/* get the mutex */
	vortex_mutex_lock (&queue->mutex);

	/* update reference */
	result = queue->reference;

	/* unlock the mutex */
	vortex_mutex_unlock (&queue->mutex);

	return result;
}

/** 
 * @brief Decrease the reference counting deallocating all resources
 * associated with the queue if such counting reach zero.
 * 
 * @param queue The queue to decrease its reference counting.
 */
void               vortex_async_queue_unref     (VortexAsyncQueue * queue)
{
	v_return_if_fail (queue);

	/* get the mutex */
	vortex_mutex_lock (&queue->mutex);

	/* update reference */
	queue->reference--;

	/* check reference couting */
	if (queue->reference == 0) {

		/* free the list */
		axl_list_free (queue->data);
		queue->data = NULL;

		/* free the conditional var */
		vortex_cond_destroy (&queue->cond);
		
		/* unlock the mutex */
		vortex_mutex_unlock (&queue->mutex);

		/* destroy the mutex */
		vortex_mutex_destroy (&queue->mutex);

		/* free the node itself */
		axl_free (queue);

		return;
	} /* end if */

	/* unlock the mutex */
	vortex_mutex_unlock (&queue->mutex);
	
	return;
}

/** 
 * @internal Release memory used by queue without acquiring mutexes or
 * checking queue references. This is currently used by vortex
 * reinitialization after fork operations.
 */
void             vortex_async_queue_release (VortexAsyncQueue * queue)
{
	if (queue == NULL)
		return;
	axl_list_free (queue->data);
	queue->data = NULL;
	axl_free (queue);
	return;
}

/** 
 * @brief Allows to perform a safe unref operation (nullifying the
 * caller's queue reference).
 *
 * @param queue The queue where to perform the safe unref operation.
 */
void               vortex_async_queue_safe_unref (VortexAsyncQueue ** queue)
{
	VortexAsyncQueue * _queue = (*queue);

	v_return_if_fail (_queue);

	/* get the mutex */
	vortex_mutex_lock (&_queue->mutex);

	/* update reference */
	_queue->reference--;

	/* check reference couting */
	if (_queue->reference == 0) {

		/* nullify queue */
		(*queue) = NULL;

		/* free the list */
		axl_list_free (_queue->data);
		_queue->data = NULL;

		/* free the conditional var */
		vortex_cond_destroy (&_queue->cond);
		
		/* unlock the mutex */
		vortex_mutex_unlock (&_queue->mutex);

		/* destroy the mutex */
		vortex_mutex_destroy (&_queue->mutex);

		/* free the node itself */
		axl_free (_queue);

		return;
	} /* end if */

	/* unlock the mutex */
	vortex_mutex_unlock (&_queue->mutex);
	
	return;
}


/** 
 * @brief Allows to perform a foreach operation on the provided queue,
 * applying the provided function over all items stored.
 * 
 * @param queue The queue that will receive the foreach operation.
 * @param foreach_func The function to call for each item found.
 * @param user_data User defined pointer to be passed to the function.
 */
void               vortex_async_queue_foreach   (VortexAsyncQueue         * queue,
						 VortexAsyncQueueForeach    foreach_func,
						 axlPointer                 user_data)
{
	axlListCursor * cursor;
	int             iterator;
	axlPointer      ref;

	v_return_if_fail (queue);
	v_return_if_fail (foreach_func);

	/* get the mutex */
	vortex_mutex_lock (&queue->mutex);

	/* create a cursor */
	cursor   = axl_list_cursor_new (queue->data);
	iterator = 0;
	while (axl_list_cursor_has_item (cursor)) {
		
		/* call to the function */
		ref = axl_list_cursor_get (cursor);
		foreach_func (queue, ref, iterator, user_data);
		
		/* next item */
		axl_list_cursor_next (cursor);
		iterator++;

	} /* end while */

	/* free cursor */
	axl_list_cursor_free (cursor);

	/* unlock the mutex */
	vortex_mutex_unlock (&queue->mutex);

	return;
}

/** 
 * @brief Allows to iterate over queue elements applying a lookup
 * function to select one.
 *
 * @param queue The queue where the lookup operation will take place.
 *
 * @param lookup_func The looking up function to call over each item.
 *
 * @param user_data User defined data to be passed to the lookup
 * function along with the queue item.
 *
 * @return The first queue element that was found (lookup function
 * returns axl_true for the queue item) or NULL if no items are in the
 * queue or no item was selected by the lookup_func. Note the pointer
 * returned is still owned by the queue.
 */
axlPointer         vortex_async_queue_lookup    (VortexAsyncQueue         * queue,
						 axlLookupFunc              lookup_func,
						 axlPointer                 user_data)
{
	axlListCursor * cursor;
	axlPointer      ref = NULL;

	v_return_val_if_fail (queue, NULL);
	v_return_val_if_fail (lookup_func, NULL);

	/* get the mutex */
	vortex_mutex_lock (&queue->mutex);

	/* create a cursor */
	cursor   = axl_list_cursor_new (queue->data);
	while (axl_list_cursor_has_item (cursor)) {
		
		/* call to the function */
		ref = axl_list_cursor_get (cursor);
		if (lookup_func (ref, user_data))
			break;
		
		/* next item */
		axl_list_cursor_next (cursor);
	} /* end while */

	/* signal the item was found if all items were iterated by no
	 * item was found */
	if (! axl_list_cursor_has_item (cursor))
		ref = NULL;

	/* free cursor */
	axl_list_cursor_free (cursor);

	/* unlock the mutex */
	vortex_mutex_unlock (&queue->mutex);

	return ref;
}

/** 
 * @brief Allows to lock the queue, making the caller the only thread
 * owning the queue. This function should be used in conjuntion with
 * vortex_async_queue_unlocked_push. Call to vortex_async_queue_push
 * will lock the caller forever until a call to
 * vortex_async_queue_unlock is done.
 *
 * @param queue The queue to lock.
 * 
 * NOTE: To produce portable code, the thread calling to this function
 * must also call to \ref vortex_async_queue_unlock. It is not
 * supported by Microsoft Windows platforms to do a call to \ref
 * vortex_async_queue_unlock from a different thread that issue the
 * call to \ref vortex_async_queue_lock.
 */
void               vortex_async_queue_lock      (VortexAsyncQueue * queue)
{
	v_return_if_fail (queue);
	vortex_mutex_lock (&queue->mutex);
	return;
}

/** 
 * @brief Allows to unlock the queue. See \ref
 * vortex_async_queue_lock.
 * @param queue The queue to unlock.
 */
void               vortex_async_queue_unlock    (VortexAsyncQueue * queue)
{
	v_return_if_fail (queue);

	/* signal if waiters are available */
	if (queue->waiters > 0)
		vortex_cond_signal (&queue->cond);

	vortex_mutex_unlock (&queue->mutex);
	return;
}

/** 
 * @} 
 */
