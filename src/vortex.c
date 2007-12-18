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

/* global includes */
#include <vortex.h>
#include <signal.h>

/* private include */
#include <vortex_ctx_private.h>

#define LOG_DOMAIN "vortex"

/**
 * @internal
 *
 * Frees Vortex Writer Data, which is a raw representation for a
 * content to be send over the wire. In the past, this data was
 * handled by the vortex writer loop, but on Vortex 1.0 release this
 * loop was included inside the vortex sequencer.
 *
 * @param writer_data The data to be deallocated.
 */
void vortex_writer_data_free (VortexWriterData * writer_data)
{
	/* do not perform any operation if null value is received */
	if (writer_data == NULL) 
		return;

	axl_free (writer_data->the_frame);
	axl_free (writer_data);

	return;
}

void __vortex_sigpipe_do_nothing (int _signal)
{
	/* do nothing sigpipe handler to be able to manage EPIPE error
	 * returned by write. read calls do not fails because we use
	 * the vortex reader process that is waiting for changes over
	 * a connection and that changes include remote peer
	 * closing. So, EPIPE (or receive SIGPIPE) can't happen. */

#if !defined(AXL_OS_WIN32)
	/* the following line is to ensure ancient glibc version that
	 * restores to the default handler once the signal handling is
	 * executed. */
	signal (SIGPIPE, __vortex_sigpipe_do_nothing);
#endif
	
	return;
}

/** 
 * @brief Allows to get current status for log debug info to console.
 * 
 * @return true if console debug is enabled. Otherwise false.
 */
bool     vortex_log_is_enabled (VortexCtx * ctx)
{
#ifdef ENABLE_VORTEX_LOG	
	/* no context, no log */
	if (ctx == NULL)
		return false;

	/* check if the debug function was already checked */
	if (! ctx->debug_checked) {
		/* not checked, get the value and flag as checked */
		ctx->debug         = vortex_support_getenv_int ("VORTEX_DEBUG") > 0;
		ctx->debug_checked = true;
	} /* end if */

	/* return current status */
	return ctx->debug;

#else
	return false;
#endif
}

/** 
 * @brief Allows to get current status for second level log debug info
 * to console.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @return true if console debug is enabled. Otherwise false.
 */
bool     vortex_log2_is_enabled (VortexCtx * ctx)
{
#ifdef ENABLE_VORTEX_LOG	

	/* no context, no log */
	if (ctx == NULL)
		return false;

	/* check if the debug function was already checked */
	if (! ctx->debug2_checked) {
		/* not checked, get the value and flag as checked */
		ctx->debug2         = vortex_support_getenv_int ("VORTEX_DEBUG2") > 0;
		ctx->debug2_checked = true;
	} /* end if */

	/* return current status */
	return ctx->debug2;

#else
	return false;
#endif
}

/** 
 * @brief Enable console vortex log.
 * 
 * @param ctx The context where the operation will be performed.
 *
 * @param status true enable log, false disables it.
 */
void     vortex_log_enable       (VortexCtx * ctx, bool     status)
{
#ifdef ENABLE_VORTEX_LOG	
	/* no context, no log */
	if (ctx == NULL)
		return;

	if (status) {
		/* enable the environment variable */
		vortex_support_setenv ("VORTEX_DEBUG", "1");

		/* flag the new value */
		ctx->debug = true;
		
	} else
		vortex_support_unsetenv ("VORTEX_DEBUG");
	return;
#else
	/* just return */
	return;
#endif
}

/** 
 * @brief Enable console second level vortex log.
 * 
 * @param status true enable log, false disables it.
 *
 * @param ctx The context where the operation will be performed.
 */
void     vortex_log2_enable       (VortexCtx * ctx, bool     status)
{
#ifdef ENABLE_VORTEX_LOG	
	/* no context, no log */
	if (ctx == NULL)
		return;

	if (status) {
		/* set the variable */
		vortex_support_setenv ("VORTEX_DEBUG2", "1");
		
		/* flag the new value */
		ctx->debug2 = true;
	} else
		vortex_support_unsetenv ("VORTEX_DEBUG2");
	return;
#else
	/* just return */
	return;
#endif
}


/** 
 * @brief Allows to check if the color log is currently enabled.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @return true if enabled, false if not.
 */
bool     vortex_color_log_is_enabled (VortexCtx * ctx)
{
#ifdef ENABLE_VORTEX_LOG	
	/* no context, no log */
	if (ctx == NULL)
		return false;
	if (! ctx->debug_color_checked) {
		ctx->debug_color_checked = true;
		ctx->debug_color         = vortex_support_getenv_int ("VORTEX_DEBUG_COLOR") > 0;
	} /* end if */

	/* return current result */
	return ctx->debug_color;
#else
	/* always return false */
	return false;
#endif
	
}


/** 
 * @brief Enable console color log. 
 *
 * Note that this doesn't enable logging, just selects color messages if
 * logging is already enabled, see vortex_log_enable().
 *
 * This is mainly useful to hunt messages using its color: 
 *  - red:  errors, critical 
 *  - yellow: warnings
 *  - green: info, debug
 * 
 * @param ctx The context where the operation will be performed.
 *
 * @param status true enable color log, false disables it.
 */
void     vortex_color_log_enable (VortexCtx * ctx, bool     status)
{

#ifdef ENABLE_VORTEX_LOG
	/* no context, no log */
	if (ctx == NULL)
		return;

	if (status) {
		/* set the new value */
		vortex_support_setenv ("VORTEX_DEBUG_COLOR", "1");

		/* flag the new value */
		ctx->debug_color = true;
	} else
		vortex_support_unsetenv ("VORTEX_DEBUG_COLOR");
	return;
#else
	return;
#endif
}

/** 
 * @brief Allows to get a vortex configuration, providing a valid
 * vortex item.
 * 
 * The function requires the configuration item that is requried and a
 * valid reference to a variable to store the result. 
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param item The configuration item that is being returned.
 *
 * @param value The variable reference required to fill the result.
 * 
 * @return The function returns true if the configuration item is
 * returned. 
 */
bool      vortex_conf_get             (VortexCtx      * ctx,
				       VortexConfItem   item, 
				       int            * value)
{
#if defined(AXL_OS_WIN32)

#elif defined(AXL_OS_UNIX)
	/* variables for nix world */
	struct rlimit _limit;
#endif	
	/* do common check */
	v_return_val_if_fail (ctx,   false);
	v_return_val_if_fail (value, false);

	/* no context, no configuration */
	if (ctx == NULL)
		return false;

	/* clear value received */
	*value = 0;

#if defined (AXL_OS_WIN32)
#elif defined(AXL_OS_UNIX)
	/* clear not filled result */
	_limit.rlim_cur = 0;
	_limit.rlim_max = 0;	
#endif

	switch (item) {
	case VORTEX_SOFT_SOCK_LIMIT:
#if defined (AXL_OS_WIN32)
		/* return the soft sock limit */
		*value = ctx->__vortex_conf_soft_sock_limit;
		return true;
#elif defined (AXL_OS_UNIX)
		/* get the limit */
		if (getrlimit (RLIMIT_NOFILE, &_limit) != 0) {
			vortex_log (VORTEX_LEVEL_CRITICAL, "failed to get current soft limit: %s", vortex_errno_get_last_error ());
			return false;
		} /* end if */

		/* return current limit */
		*value = _limit.rlim_cur;
		return true;
#endif		
	case VORTEX_HARD_SOCK_LIMIT:
#if defined (AXL_OS_WIN32)
		/* return the hard sockt limit */
		*value = ctx->__vortex_conf_hard_sock_limit;
		return true;
#elif defined (AXL_OS_UNIX)
		/* get the limit */
		if (getrlimit (RLIMIT_NOFILE, &_limit) != 0) {
			vortex_log (VORTEX_LEVEL_CRITICAL, "failed to get current soft limit: %s", vortex_errno_get_last_error ());
			return false;
		} /* end if */

		/* return current limit */
		*value = _limit.rlim_max;
		return true;
#endif		
	case VORTEX_LISTENER_BACKLOG:
		/* return current backlog value */
		*value = ctx->backlog;
		return true;
	case VORTEX_ENFORCE_PROFILES_SUPPORTED:
		/* return current enforce profiles supported values */
		*value = ctx->enforce_profiles_supported;
		return true;
	default:
		/* configuration found, return false */
		vortex_log (VORTEX_LEVEL_CRITICAL, "found a requested for a non existent configuration item");
		return false;
	} /* end if */

	return true;
}

/** 
 * @brief Allows to configure the provided item, with either the
 * integer or the string value, according to the item configuration
 * documentation.
 * 
 * @param item The item configuration to be set.
 * @param value The integer value to be configured if applies.
 * @param str_value The string value to be configured if applies.
 * 
 * @return true if the configuration was done properly, otherwise
 * false is returned.
 */
bool      vortex_conf_set             (VortexCtx      * ctx,
				       VortexConfItem   item, 
				       int              value, 
				       const char     * str_value)
{
#if defined(AXL_OS_WIN32)

#elif defined(AXL_OS_UNIX)
	/* variables for nix world */
	struct rlimit _limit;
#endif	
	/* do common check */
	v_return_val_if_fail (ctx,   false);
	v_return_val_if_fail (value, false);

#if defined (AXL_OS_WIN32)
#elif defined(AXL_OS_UNIX)
	/* clear not filled result */
	_limit.rlim_cur = 0;
	_limit.rlim_max = 0;	
#endif

	switch (item) {
	case VORTEX_SOFT_SOCK_LIMIT:
#if defined (AXL_OS_WIN32)
		/* check soft limit received */
		if (value > ctx->__vortex_conf_hard_sock_limit)
			return false;

		/* configure new soft limit */
		ctx->__vortex_conf_soft_sock_limit = value;
		return true;
#elif defined (AXL_OS_UNIX)
		/* get the limit */
		if (getrlimit (RLIMIT_NOFILE, &_limit) != 0) {
			vortex_log (VORTEX_LEVEL_CRITICAL, "failed to get current soft limit: %s", vortex_errno_get_last_error ());
			return false;
		} /* end if */

		/* configure new value */
		_limit.rlim_cur = value;

		/* set new limit */
		if (setrlimit (RLIMIT_NOFILE, &_limit) != 0) {
			vortex_log (VORTEX_LEVEL_CRITICAL, "failed to set current soft limit: %s", vortex_errno_get_last_error ());
			return false;
		} /* end if */

		return true;
#endif		
	case VORTEX_HARD_SOCK_LIMIT:
#if defined (AXL_OS_WIN32)
		/* current it is not possible to configure hard sock
		 * limit */
		return false;
#elif defined (AXL_OS_UNIX)
		/* get the limit */
		if (getrlimit (RLIMIT_NOFILE, &_limit) != 0) {
			vortex_log (VORTEX_LEVEL_CRITICAL, "failed to get current soft limit: %s", vortex_errno_get_last_error ());
			return false;
		} /* end if */

		/* configure new value */
		_limit.rlim_max = value;
		
		/* if the hard limit gets lower than the soft limit,
		 * make the lower limit to be equal to the hard
		 * one. */
		if (_limit.rlim_max < _limit.rlim_cur)
			_limit.rlim_max = _limit.rlim_cur;

		/* set new limit */
		if (setrlimit (RLIMIT_NOFILE, &_limit) != 0) {
			vortex_log (VORTEX_LEVEL_CRITICAL, "failed to set current hard limit: %s", vortex_errno_get_last_error ());
			return false;
		} /* end if */
		
		return true;
#endif		
	case VORTEX_LISTENER_BACKLOG:
		/* return current backlog value */
		ctx->backlog = value;
		return true;

	case VORTEX_ENFORCE_PROFILES_SUPPORTED:
		/* return current enforce profiles supported values */
		ctx->enforce_profiles_supported = value;
		return true;
	default:
		/* configuration found, return false */
		vortex_log (VORTEX_LEVEL_CRITICAL, "found a requested for a non existent configuration item");
		return false;
	} /* end if */

	return true;
}

/** 
 * @brief If activate the console debug, it may happen that some
 * messages are mixed because several threads are producing them at
 * the same time.
 *
 * This function allows to make all messages logged to acquire a mutex
 * before continue. It is by default disabled because it has the
 * following considerations:
 * 
 * - Acquiring a mutex allow to clearly separate messages from
 * different threads, but has a great perform penalty (only if log is
 * activated).
 *
 * - Aquiring a mutex could make the overall system to act in a
 * different way because the threading is now globally synchronized by
 * all calls done to the log. That is, it may be possible to not
 * reproduce a thread race condition if the log is activated with
 * mutex acquicision.
 * 
 * @param ctx The context that is going to be configured.
 * 
 * @param status true to acquire the mutex before logging, otherwise
 * log without locking the context mutex.
 *
 * You can use \ref vortex_log_is_enabled_acquire_mutex to check the
 * mutex status.
 */
void     vortex_log_acquire_mutex    (VortexCtx * ctx, 
				      bool        status)
{
	/* get current context */
	v_return_if_fail (ctx);

	/* configure status */
	ctx->use_log_mutex = true;
}

/** 
 * @brief Allows to check if the log mutex acquicision is activated.
 *
 * @param ctx The context that will be required to return its
 * configuration.
 * 
 * @return Current status configured.
 */
bool     vortex_log_is_enabled_acquire_mutex (VortexCtx * ctx)
{
	/* get current context */
	v_return_val_if_fail (ctx, false);
	
	/* configure status */
	return ctx->use_log_mutex;
}

/** 
 * @internal Internal common log implementation to support several levels
 * of logs.
 * 
 * @param file The file that produce the log.
 * @param line The line that fired the log.
 * @param log_level The level of the log
 * @param message The message 
 * @param args Arguments for the message.
 */
void _vortex_log_common (VortexCtx        * ctx,
			 const       char * file,
			 int                line,
			 VortexDebugLevel   log_level,
			 const char       * message,
			 va_list            args)
{

#ifndef ENABLE_VORTEX_LOG
	/* do no operation if not defined debug */
	return;
#else
	/* log with mutex */
	bool use_log_mutex = false;

	if (ctx == NULL) {
#if defined (__GNUC__)
		fprintf (stdout, "\e[1;31m!!! CONTEXT NOT DEFINED !!!\e[0m: ");
#else
		fprintf (stdout, "!!! CONTEXT NOT DEFINED !!!: ");
#endif /* __GNUC__ */
		goto ctx_not_defined;
	}

	/* if not VORTEX_DEBUG FLAG, do not output anything */
	if (!vortex_log_is_enabled (ctx)) {
		return;
	} /* end if */

	/* acquire the mutex so multiple threads will not mix their
	 * log messages together */
	use_log_mutex = ctx->use_log_mutex;
	if (use_log_mutex) 
		vortex_mutex_lock (&ctx->log_mutex);

	/* printout the process pid */
#if defined (__GNUC__)
	if (vortex_color_log_is_enabled (ctx)) 
		fprintf (stdout, "\e[1;36m(proc %d)\e[0m: ", getpid ());
	else {
#endif /* __GNUC__ */
	ctx_not_defined:
		fprintf (stdout, "(proc %d): ", getpid ());
	}

	/* drop a log according to the level */
#if defined (__GNUC__)
	if (vortex_color_log_is_enabled (ctx)) {
		switch (log_level) {
		case VORTEX_LEVEL_DEBUG:
			fprintf (stdout, "(\e[1;32mdebug\e[0m) ");
			break;
		case VORTEX_LEVEL_WARNING:
			fprintf (stdout, "(\e[1;33mwarning\e[0m) ");
			break;
		case VORTEX_LEVEL_CRITICAL:
			fprintf (stdout, "(\e[1;31mcritical\e[0m) ");
			break;
		}
	}else {
#endif /* __GNUC__ */
		switch (log_level) {
		case VORTEX_LEVEL_DEBUG:
			fprintf (stdout, "(debug) ");
			break;
		case VORTEX_LEVEL_WARNING:
			fprintf (stdout, "(warning) ");
			break;
		case VORTEX_LEVEL_CRITICAL:
			fprintf (stdout, "(critical) ");
			break;
		}
#if defined (__GNUC__)
	}
#endif

	/* drop a log according to the domain */
	(file != NULL) ? fprintf (stdout, "%s:%d ", file, line) : fprintf (stdout, ": ");

	/* print the message */
	vfprintf (stdout, message, args);

	fprintf (stdout, "\n");

	/* ensure that the log is droped to the console */
	fflush (stdout);
	
	/* check to release the mutex if defined the context */
	if (use_log_mutex) 
		vortex_mutex_unlock (&ctx->log_mutex);

#endif /* end ENABLE_VORTEX_LOG */


	/* return */
	return;
}

/** 
 * @internal Log function used by vortex to notify all messages that are
 * generated by the core. 
 *
 * Do no use this function directly, use <b>vortex_log</b>, which is
 * activated/deactivated according to the compilation flags.
 * 
 * @param ctx The context where the operation will be performed.
 * @param file The file that produce the log.
 * @param line The line that fired the log.
 * @param log_level The message severity
 * @param message The message logged.
 */
void _vortex_log (VortexCtx        * ctx,
		  const       char * file,
		  int                line,
		  VortexDebugLevel   log_level,
		  const char       * message,
		  ...)
{

#ifndef ENABLE_VORTEX_LOG
	/* do no operation if not defined debug */
	return;
#else
	va_list   args;

	/* call to common implementation */
	va_start (args, message);
	_vortex_log_common (ctx, file, line, log_level, message, args);
	va_end (args);

	return;
#endif
}

/** 
 * @internal Log function used by vortex to notify all second level
 * messages that are generated by the core.
 *
 * Do no use this function directly, use <b>vortex_log2</b>, which is
 * activated/deactivated according to the compilation flags.
 * 
 * @param ctx The context where the log will be dropped.
 * @param file The file that contains that fired the log.
 * @param line The line where the log was produced.
 * @param log_level The message severity
 * @param message The message logged.
 */
void _vortex_log2 (VortexCtx        * ctx,
		   const       char * file,
		   int                line,
		   VortexDebugLevel   log_level,
		   const char       * message,
		  ...)
{

#ifndef ENABLE_VORTEX_LOG
	/* do no operation if not defined debug */
	return;
#else
	va_list   args;

	/* if not VORTEX_DEBUG2 FLAG, do not output anything */
	if (!vortex_log2_is_enabled (ctx)) {
		return;
	} /* end if */
	
	/* call to common implementation */
	va_start (args, message);
	_vortex_log_common (ctx, file, line, log_level, message, args);
	va_end (args);

	return;
#endif
}

/**
 * \defgroup vortex Vortex: Main Vortex Library Module (initialization and exit stuff)
 */

/**
 * \addtogroup vortex
 * @{
 */


/** 
 * @brief Context based vortex library init. Allows to init the vortex
 * library status on the provided context object (\ref VortexCtx).
 *
 * To init vortex library use: 
 * 
 * \code
 * VortexCtx * ctx;
 * 
 * // create an empty context 
 * ctx = vortex_ctx_new ();
 *
 * // init the context
 * if (! vortex_init_ctx (ctx)) {
 *      printf ("failed to init the library..\n");
 * } 
 *
 * // do API calls before this function 
 * 
 * // terminate the context 
 * vortex_exit_exit (ctx);
 *
 * // release the context 
 * vortex_ctx_free (ctx);
 * \endcode
 * 
 * @param ctx An already created context where the library
 * initialization will take place.
 * 
 * @return true if the context was initialized, otherwise false is
 * returned.
 */
bool   vortex_init_ctx (VortexCtx * ctx)
{
	int          thread_num;
	int          soft_limit;

	v_return_val_if_fail (ctx, false);

	/**** vortex_io.c: init io module */
	vortex_io_init (ctx);

	/**** vortex.c: init global mutex *****/
	vortex_mutex_create (&ctx->frame_id_mutex);
	vortex_mutex_create (&ctx->connection_id_mutex);
	vortex_mutex_create (&ctx->search_path_mutex);
	vortex_mutex_create (&ctx->tls_init_mutex);
	vortex_mutex_create (&ctx->listener_mutex);
	vortex_mutex_create (&ctx->listener_unlock);
	vortex_mutex_create (&ctx->exit_mutex);

	/* init channel module */
	vortex_channel_init (ctx);

	/* init connection module */
	vortex_connection_init (ctx);

	/* init profiles module */
	vortex_profiles_init (ctx);

	/* init xml-rpc module */
	vortex_xml_rpc_init (ctx); 

	/* init vortex support module on the context provided: 
	 * 
	 * A list containing all search paths with its domains to
	 * perform lookups, and its associated mutex to avoid the list
	 * to be corrupted by several access.
	 */
	vortex_support_init (ctx);

#if ! defined(AXL_OS_WIN32)
	/* install sigpipe handler */
	signal (SIGPIPE, __vortex_sigpipe_do_nothing);
#endif

#if defined(AXL_OS_WIN32)
	/* init winsock API */
	vortex_log (VORTEX_LEVEL_DEBUG, "init winsocket for windows");
	vortex_win32_init ();
#endif

	/* init axl library */
	axl_init ();

	/* add default paths */
#if defined(AXL_OS_UNIX)
	vortex_log (VORTEX_LEVEL_DEBUG, "configuring context to use: %p", ctx);
	vortex_support_add_search_path_ref (ctx, vortex_support_build_filename (PACKAGE_DTD_DIR, "libvortex", "data", NULL ));
	vortex_support_add_search_path_ref (ctx, vortex_support_build_filename (PACKAGE_DTD_DIR, "libvortex", NULL ));
	vortex_support_add_search_path_ref (ctx, vortex_support_build_filename ("libvortex", NULL ));
	vortex_support_add_search_path_ref (ctx, vortex_support_build_filename (PACKAGE_TOP_DIR, "libvortex", "data", NULL));
	vortex_support_add_search_path_ref (ctx, vortex_support_build_filename (PACKAGE_TOP_DIR, "data", NULL));
#endif
	/* do not use the add_search_path_ref version to force vortex
	   library to perform a local copy path */
	vortex_support_add_search_path (ctx, ".");
	
	/* init dtds */
	if (!vortex_dtds_init (ctx)) {
		fprintf (stderr, "VORTEX_ERROR: unable to load dtd files (this means some DTD (or all) file wasn't possible to be loaded.\n");
		return false;
	}

	/* before starting, check if we are using select(2) system
	 * call method, to adecuate the number of sockets that can
	 * *really* handle the FD_* function family, to the number of
	 * sockets that is allowed to handle the process. This is to
	 * avoid race conditions cause to properly create a
	 * connection, which is later not possible to handle at the
	 * select(2) I/O subsystem. */
	if (vortex_io_waiting_get_current (ctx) == VORTEX_IO_WAIT_SELECT) {
		/* now check if the current process soft limit is
		 * allowed to handle more connection than
		 * VORTEX_FD_SETSIZE */
		vortex_conf_get (ctx, VORTEX_SOFT_SOCK_LIMIT, &soft_limit);
		vortex_log (VORTEX_LEVEL_DEBUG, "select mechanism selected, reconfiguring current socket limit: soft_limit=%d > %d..",
			    soft_limit, VORTEX_FD_SETSIZE);
		if (soft_limit > (VORTEX_FD_SETSIZE - 1)) {
			/* decrease the limit to avoid funny
			 * problems. it is not required to be
			 * priviledge user to run the following
			 * command. */
			vortex_conf_set (ctx, VORTEX_SOFT_SOCK_LIMIT, (VORTEX_FD_SETSIZE - 1), NULL);
			vortex_log (VORTEX_LEVEL_WARNING, 
				    "found select(2) I/O configured, which can handled up to %d fds, reconfigured process with that value",
				    VORTEX_FD_SETSIZE -1);
		} /* end if */
	} 

	/* init reader subsystem */
	vortex_log (VORTEX_LEVEL_DEBUG, "starting vortex reader..");
	if (! vortex_reader_run (ctx))
		return false;
	
	/* init writer subsystem */
	/* vortex_log (VORTEX_LEVEL_DEBUG, "starting vortex writer..");
	   vortex_writer_run (); */

	/* init sequencer subsystem */
	vortex_log (VORTEX_LEVEL_DEBUG, "starting vortex sequencer..");
	if (! vortex_sequencer_run (ctx))
		return false;
	
	/* init thread pool (for query receiving) */
	thread_num = vortex_thread_pool_get_num ();
	vortex_log (VORTEX_LEVEL_DEBUG, "starting vortex thread pool: (%d threads the pool have)..",
	       thread_num);
	vortex_thread_pool_init (ctx, thread_num);

	/* register the vortex exit function */
	return true;
}

/** 
 * @brief Terminates the vortex library execution on the provided
 * context.
 *
 * Stops all internal vortex process and all allocated resources
 * assocated to the context. It also close all channels for all
 * connection that where not closed until call this function.
 *
 * This function is reentrant, allowing several threads to call \ref
 * vortex_exit_ctx function at the same time. Only one thread will
 * actually release resources allocated.
 *
 * NOTE: Although it isn't explicitly stated, this function shouldn't
 * be called from inside a handler notification: \ref
 * VortexOnFrameReceived "Frame Receive", \ref VortexOnCloseChannel
 * "Channel close", etc. This is because those handlers works indise
 * the context of the vortex library execution. Making a call to this
 * function in the middle of that context, will produce undefined
 * behaviours.
 *
 * NOTE2: This function isn't designed to dealloc all resources
 * associated to the context and used by the vortex engine
 * function. According to the particular control structure used by
 * your application, you must first terminate using any Vortex API and
 * then call to vortex_exit_ctx.
 * 
 * @param ctx The context to terminate. The function do not dealloc
 * the context provided. 
 *
 * @param free_ctx Allows to signal the function if the context
 * provided must be deallocated (by calling to \ref vortex_ctx_free).
 */
void vortex_exit_ctx (VortexCtx * ctx, bool free_ctx)
{

	/* check if the library is already started */
	if (ctx == NULL || ctx->vortex_exit)
		return;

	vortex_log (VORTEX_LEVEL_DEBUG, "shutting down vortex library");

	vortex_mutex_lock (&ctx->exit_mutex);
	if (ctx->vortex_exit) {
		vortex_mutex_unlock (&ctx->exit_mutex);
		return;
	}
	/* flag other waiting functions to do nothing */
	ctx->vortex_exit = true;
	
	/* unlock */
	vortex_mutex_unlock  (&ctx->exit_mutex);

	/* flag the thread pool to not accept more jobs */
	vortex_thread_pool_being_closed ();

	/* stop vortex writer */
	/* vortex_writer_stop (); */

	/* stop vortex sequencer */
	vortex_sequencer_stop (ctx);

	/* stop vortex reader process */
	vortex_reader_stop (ctx);

	/* stop vortex profiles process */
	vortex_profiles_cleanup (ctx);

	/* cleanup xml rpc module if defined */
	vortex_xml_rpc_cleanup (ctx); 

	/* clean up tls module */
	vortex_tls_cleanup (ctx);

#if defined(AXL_OS_WIN32)
	WSACleanup ();
	vortex_log (VORTEX_LEVEL_DEBUG, "shutting down WinSock2(tm) API");
#endif

	/* clean up vortex modules */
	vortex_log (VORTEX_LEVEL_DEBUG, "shutting down vortex xml subsystem");

	/* Cleanup function for the XML library. */
	vortex_log (VORTEX_LEVEL_DEBUG, "shutting down xml library");
	axl_end ();

	/* unlock listeners */
	vortex_log (VORTEX_LEVEL_DEBUG, "unlocking vortex listeners");
	vortex_listener_unlock (ctx);

	vortex_log (VORTEX_LEVEL_DEBUG, "vortex library stopped");

	/* stop the vortex thread pool: 
	 * 
	 * In the past, this call was done, however, it is showed that
	 * user applications on top of vortex that wants to handle
	 * signals, emited to all threads running (including the pool)
	 * causes many non-easy to solve problem related to race
	 * conditions.
	 * 
	 * At the end, to release the thread pool is not a big
	 * deal. */
	vortex_thread_pool_exit (ctx); 

	/* cleanup connection module */
	vortex_connection_cleanup (ctx); 

	/* cleanup channel module */
	vortex_channel_cleanup (ctx); 

	/* cleanup listener module */
	vortex_listener_cleanup (ctx);

	/* cleanup vortex_support module */
	vortex_support_cleanup (ctx);

	/* cleanup dtd module */
	vortex_dtds_cleanup (ctx);

	/* cleanup greetings module */
	vortex_greetings_cleanup (ctx);

	/* destroy global mutex */
	vortex_mutex_destroy (&ctx->frame_id_mutex);
	vortex_mutex_destroy (&ctx->connection_id_mutex);
	vortex_mutex_destroy (&ctx->search_path_mutex);
	vortex_mutex_destroy (&ctx->tls_init_mutex);
	vortex_mutex_destroy (&ctx->listener_mutex);
	vortex_mutex_destroy (&ctx->listener_unlock);

	/* lock/unlock to avoid race condition */
	vortex_mutex_lock  (&ctx->exit_mutex);
	vortex_mutex_unlock  (&ctx->exit_mutex);
	vortex_mutex_destroy (&ctx->exit_mutex);
   
	/* release the ctx */
	if (free_ctx)
		vortex_ctx_free (ctx);

	return;
}

/* @} */


/**
 * \mainpage Vortex Library:  A BEEP Core implementation
 *
 * \section intro Introduction 
 *
 * <b>Vortex Library</b> is an implementation of the <b>RFC 3080 / RFC
 * 3081</b> standard definition called the <b>BEEP Core protocol
 * mapped into TCP/IP</b> layer written in <b>C</b>. In addition, it
 * comes with a complete XML-RPC over BEEP <b>RFC 3529</b> and TUNNEL
 * (<b>RFC3620</b>) support. It has been developed by <b>Advanced
 * Software Production Line, S.L.</b> (http://www.aspl.es) as a
 * requirement for the <B>Af-Arch</B> project (http://fact.aspl.es).
 *
 * Vortex Library has been implemented keeping in mind security. It
 * has a consistent and easy to use API which will allow you to write
 * application protocols really fast. The API provided is really
 * stable and any change that could happen is notified using a public
 * change procedure notification (http://www.aspl.es/change/change-notification.txt).
 *
 * Vortex Library is being run and tested regularly under GNU/Linux
 * and Microsoft Windows platforms, but it is known to work in other
 * platforms. Its development is also checked with a regression test
 * to ensure proper function across releases.
 *
 * The Vortex Library is intensively used by the <b>Af-Arch</b> project but
 * as RFC implementation <b>it can be used in a stand alone way</b> apart from
 * <b>Af-Arch</b>.
 *
 * The following section represents documents you may find useful to
 * get an idea if Vortex Library is right for you. It talks about
 * features, some insights about its implementation and license issues.
 *
 * - \ref implementation
 * - \ref features
 * - \ref status
 * - \ref license
 *
 * The following documents shows <b>how to use Vortex Library and its
 * profiles/tools</b>. It is also showed, how to install and use
 * it, with some tutorials to get Vortex Library working for you as
 * soon as possible.
 * 
 * - \ref install
 * - \ref starting_to_program
 * - \ref profile_example
 * - \ref programming_with_xml_rpc
 * - \ref using_tunnel_profile
 *
 * You can also check the <b>Vortex Library API</b> documentation:
 * <ul>
 * <li><b>General support and common API used for all BEEP related functions:</b>
 * - \ref vortex_ctx
 * - \ref vortex
 * - \ref vortex_connection
 * - \ref vortex_channel 
 * - \ref vortex_channel_pool
 * - \ref vortex_frame
 * - \ref vortex_profiles
 * - \ref vortex_listener
 * - \ref vortex_greetings
 * - \ref vortex_handlers
 *  
 * </li>
 * <li><b>Profiles API for those already implemented on top of Vortex Library</b>
 * - \ref vortex_tls
 * - \ref vortex_sasl
 * - \ref vortex_xml_rpc
 * - \ref vortex_xml_rpc_types
 * - \ref vortex_tunnel
 *
 * <li><b>General library support, types, handlers, thread handling</b>
 *
 * - \ref vortex_thread
 * - \ref vortex_thread_pool
 * - \ref vortex_types
 * - \ref vortex_io
 * - \ref vortex_support
 * - \ref vortex_reader
 * - \ref vortex_hash
 * - \ref vortex_queue
 *
 * </li>
 * </ul>
 *
 * \section miscellaneous Miscellaneous documents
 *
 * The following section holds all documents that didn't fall into any previous category:
 *
 *  - \ref image_logos
 * 
 * 
 * \section contact_aspl Contact Us
 *
 * You can reach us at the <b>Vortex mailing list:</b> at <a href="http://www.aspl.es/fact/index.php?id=26">vortex users</a>
 * for any question you may find. 
 *
 * If you are interested on getting commercial support, you can also
 * contact us at: info@aspl.es.
 *
 */

/**
 *
 * \page implementation Vortex Library implementation and dependencies
 *
 * \section dep_notes Vortex Library dependencies
 * 
 * Vortex Library has been implemented using one obligatory
 * dependency: LibAxl (http://xml.aspl.es). 
 *
 * LibAxl is a runtime and memory efficient XML 1.0 implementation,
 * built as a separated library to support all projects inside the
 * Af-Arch initiative, including Vortex Library. The library is
 * available at the same download location where Vortex Library is
 * found. It also comes with a Microsoft Windows installer.
 *
 * In order to activate the TLS profile support it is also required
 * openssl (http://www.openssl.org) to be installed. This software is commonly found on every
 * unix/GNU/Linux installation. Binaries are also available for Microsoft Windows platforms.
 *
 * For the SASL profile family support it is also required GNU SASL
 * (http://josefsson.org/gsasl/) to be installed. 
 *
 * Keep in mind that Vortex Library design allows to configure and
 * build a library without TLS or SASL support. As a consequence, TLS
 * profile and SASL profiles dependencies are optional.
 * 
 * Additionally, you can enable building <b>vortex-client</b>, a tool
 * which allows to perform custom operations against BEEP peer (not
 * only Vortex ones), by adding a new dependency: libreadline. 
 *
 * \subsection linux_install Vortex Library dependencies on GNU/Linux Environment
 *
 * In the case of GNU/Linux (unix environment) the package system will
 * provide you optional packages: openssl and gsasl.
 *
 * To compile Axl Library you must follow standard autoconf procedure:
 * \code
 * tar xzvf axl-XXX.tar.gz
 * ./configure
 * make
 * make install
 * \endcode
 * 
 * See Axl Library installation manual for more details: http://www.aspl.es/axl/html/axl_install.html
 *
 * An an example, on a debian similar system, with deb based packaging,
 * these dependecies can be installed using:
 *
 * To activate TLS profile support:
 * \code
 *   apt-get install libssl-dev
 * \endcode
 *
 * To activate SASL profiles support:
 * \code
 *   apt-get install libgsasl7-dev
 * \endcode
 * 
 * If you know about the exact information about how to install Vortex
 * Library dependencies in your favorite system let us to know it. We
 * will add that information on this manual.
 *
 * \subsection mingw_install Vortex Library dependencies using Mingw Environment
 *
 * For the LibAxl library you can use the installer provided for the
 * Windows platform at the download page: http://www.sf.net/projects/vortexlibrary
 *
 * Openssl binaries for windows platform could be found at:
 * http://www.openssl.org/related/binaries.html
 *
 * Previous package described DO NOT have pkg-config file
 * configuration. This means you will have to configure the
 * Makefile.win provided. Check the \ref install "install section" to
 * know more about this.
 *
 * Starting from Vortex Library 1.0.2, GNU SASL binaries for windows
 * platform are also provided by the Vortex Library project at the
 * same location with a windows installer.
 *
 * \subsection cygwin_install Vortex Library dependencies using Cygwin Environment
 *
 * If you try to use Vortex Library on Windows platform, but using the
 * CygWin environment, the previous information is not valid for your
 * case. 
 *
 * Previous package are compiled against the Microsoft Runtime
 * (msvcrt.dll) and every program or library running inside the
 * cygwin environment is compiled against (or linked to)
 * cygwin1.dll. If you mix libraries or programs linked to both
 * libraries you will get estrange hang ups.
 *
 * On cygwin environment use the libraries provided by the cygwin
 * package installation system. Keep in mind that running Vortex Library
 * over Cygwin will force you to chose the GPL license for your
 * application because the cygwin1.dll is license under the GPL. 
 *
 * No proprietary (closed source) development can be done using Cygwin
 * environment.
 *
 * \section windows_installer Microsoft Windows installer
 *
 * For Microsoft Windows developers, there are already built win32
 * packages that includes an already built Vortex Library, with all
 * dependencies, including all profiles. Check the Vortex download
 * page: http://www.sf.net/projects/vortexlibrary
 *
 * \section imp_notes Notes about Vortex Library implementation
 * 
 * The <b>BEEP Core</b> definition (<b>RFC3080</b> and <b>RFC3081</b>)
 * defines what must or should be supported by any implementation. At
 * this moment the Vortex Library support all sections including must
 * and should requirements (including TLS and SASL profiles).
 *
 * Vortex Library has been build using asynchronous sockets (not
 * blocking model) with a thread model each one working on specific
 * tasks. Once the Vortex Library is started it creates 2 threads
 * apart from the main thread doing the following task:
 *
 * - <b>Vortex Sequencer: </b> its main purpose is to create the
 * package sequencing, split user level message into frames, queue
 * them into channel's buffer if no receiving window is available or
 * sending the data directly in a round robin fashion.  This process
 * allows user space to not get blocked while sending message no
 * matter how big they are.
 *
 * - <b>Vortex Reader: </b> its main purpose is to read all frames
 * for all channels inside all connections. It checks all environment
 * conditions for a frame to be accepted: sequence number sync,
 * previous frame with more fragment flag activated, properly frame
 * format, and so on. Once the frame is accepted the Vortex Reader try
 * to dispatch it using the Vortex Library frame dispatch schema.
 * 
 * Apart from the 2 threads created plus the main one, the Vortex
 * Library creates a pool of threads already prepared to dispatch
 * incoming frames inside user space function.  The thread pool size
 * can also be tweaked.
 *
 *
 */

/**
 * \page features Vortex Library features
 * 
 * \section fectura_list Some features the Vortex Library has
 *
 * Vortex Library has some interesting features you may find as key point to
 * chose it as your project to support your data transport layer.
 * 
 * - It has a <b>consistent API</b> exposed by a strong function
 * module naming. Vortex Library doesn't expose to your code any
 * detail that may compromise your code in the future due to internal
 * Vortex changes or future releases.  This is really important to us:
 * backward compatibility will be ensured and no change to the
 * internal Vortex Library implementation will break your code. This
 * is also very important while thinking about building
 * <b>bindings</b> for library. Its API is already prepared
 * because all interactions between user space code and the library is
 * done through functions and opaque types.
 *
 * 
 * - It has been <b>designed keeping in mind security:</b> 
 * buffer overflows, DOS attacks (socket handling is done using
 * non-blocking mode), internal or external frame fragment attack or
 * channel starvation due to flooding attacks. Actually, It is been
 * used by the Af-Arch project (http://fact.aspl.es) which requires
 * transferring really large chunks of data by several channels over
 * several connection for several clients at the same time.
 *
 * - It <b>supports both asynchronous and synchronous programing
 * models</b> while sending and receiving data. This means you can
 * program a handler to be executed on frame receive event (and keep
 * on doing other things) or write code that gets blocked until a
 * specific frame is received. This also means you are not required to use handler
 * or event notifications when you just want to keep on waiting for an
 * specific frame to be received. Of course, while programing
 * Graphical User Interfaces the asynchronous model will allow you to
 * get the better performance avoiding the my-application-gets-blank
 * effect on requests ;-)
 *
 * - Operations such as <b>open a connection, create a channel, close
 * a channel, activate TLS profile, negotiate authentication through
 * SASL profiles, can be made in an asynchronous way</b> avoiding you
 * getting blocked until these operation are completed. Of course
 * <b>synchronous</b> model is also provided. If you want a library
 * that allows you to start a connection creation process, keep on
 * doing other things and be notified only when the connection has
 * been created or denied, the Vortex Library is for you.
 *
 * - It support<b> a flexible</b> \ref vortex_manual_dispatch_schema "schema" <b>to dispatch
 * received frames</b> allowing you to tweak different
 * levels of handlers to be executed, including a \ref vortex_manual_wait_reply "wait reply method",
 * which disables the handlers installed. You can check more
 * about this issue on \ref vortex_manual_dispatch_schema "the following section".
 *
 * - Vortex Library comes with a tool called <b>vortex-client</b>
 * which allows to make operations against BEEP-enabled peers. This
 * tool can help you on debugging your applications not only running
 * Vortex Library but also other BEEP enabled implementations. It has
 * support to:
 *     -# Create connections, create channels, check status for both:
 *     sequence numbers, next expected sequence numbers, reply
 *     numbers, next expected reply number, message number status,
 *     profiles supported by remote side, etc...
 *     -# It allows to send message according to actual channel status or
 *     using the frame generator to create frames with user provided data.
 *     -# It allows to generate and send close messages and start message
 *     allowing you to check how your code works.
 *
 * - Vortex Library implements <b>the channel pool</b> concept. This
 * allows you to create a pool of channels ready to be used to send
 * data. The library will handle which channel to use from the pool
 * when the next channel ready is required, avoiding get blocked due
 * to possible previous operations over that channel. It also has the
 * ability to automatically increase the channel pool and negotiate
 * for you the channel creation when the next channel ready is
 * requested but no one is available. 
 *
 * - <b>You can close</b> channels and connections (including that one
 * that is receiving the frame) inside the frame receive handler you
 * may define for both second and first level.
 *
 * - <b>You can reply any message you may want </b> (including the one
 * which have caused to be invoke the frame receive handler) or to
 * send any new message inside the frame receive handler you may
 * define.
 *
 * - <b>The close channel and close connection protocol</b> is fully
 * implemented according to all circumstances that may happen described
 * at RFC3080 standard. This means your code will not be required to
 * implement a close notification between peers in other to avoid
 * sending message that may cause the transport layer to break.  You
 * can actually close any channel (including the complete session)
 * without worry about if remote peer is done.
 *
 * - If you are planning to use XML as your encapsulation format for
 * some of your profiles, Vortex Library comes with its own XML 1.0
 * implementation, called Axl (http://xml.aspl.es), which provides all
 * required functionality to parse and produce XML documents and
 * validate them. It is extremely memory efficient and fast. Check out
 * the memory report found that the Axl home page
 * (http://xml.aspl.es).
 *
 * - Vortex Library includes a complete XML-RPC over BEEP support,
 * with a Raw invocation API, and a protocol compiler,
 * <b>xml-rpc-gen</b> which allows to produce server and client
 * components really fast. See the \ref programming_with_xml_rpc "XML-RPC over BEEP manual" for more details.
 *
 * - Vortex Library includes a complete TUNNEL profile support
 * (RFC3620), allowing to "proxy" protocols you may design on top of
 * Vortex. See \ref using_tunnel_profile "TUNNEL profile manual".
 * 
 * - The Vortex Library <b>is well documented</b> (ok, this shouldn't
 * be a feature ;-), with examples of code, not only about how to use
 * Vortex Library but also how it has been programed internally. 
 *
 * - <b>Vortex Library is released under Lesser General Public
 * License</b> allowing to create open source projects but also
 * proprietary ones. See this \ref license
 * "section" for more information about license topic.
 * 
 */

/**
 * \page status Vortex Library status
 *
 * \section status_intro Introduction 
 * 
 * Vortex Library implements currently the standard defined for the
 * BEEP Core protocol at RFC3080 (http://www.ietf.org/rfc/rfc3080.txt)
 * and RFC3081 (http://www.ietf.org/rfc/rfc3081.txt), including SASL
 * and TLS profiles. It also includes a complete XML-RPC
 * implementation (RFC3529) with a protocol compiler (xml-rpc-gen) and
 * TUNNEL profile support (RFC3620).
 *
 * Vortex Library is stable for production environment. Currently it
 * is been used for several projects developed by Advanced Software
 * Production Line which involves client and servers nodes running
 * GNU/Linux and Microsoft Windows XP platforms.
 *
 * Future releases will add security patches, bindings to other
 * languages and new profiles defined at other BEEP standards.
 * 
 * This means you can use Vortex Library to implement your network
 * protocol, including securing networking session using the TLS
 * profile, and authenticating BEEP peers through SASL profiles.
 *
 * You can also use Vortex Library throught its XML-RPC API to produce
 * distributed RPC environments.
 *
 * \section not_supported_yet Known features not supported yet
 * 
 * <ul>
 *  <li> You can't propose several profiles for the channel to be created,
 * letting listener side to chose the profile finally selected for the
 * channel. 
 * 
 * These features could be easily implemented by initially connecting
 * to the remote peer and then checking profiles supported to select
 * the right profile.
 *
 *  </li>
 *
 * <li>
 * SASL profiles implemented currently are: ANONYMOUS, EXTERNAL,
 *   PLAIN, CRAM-MD5 and DIGEST-MD5.
 * 
 * DIGEST-MD5 profile also support enable security transport layer as
 * TLS does. However this is not supported, mainly because GSASL do
 * not support this feature. SASL profiles implemented only supports
 * authentication operation.
 *
 * Again, this could be easily solved by using TLS profile to secure
 * your network session.
 * 
 * </li>
 *
 * \section actual_status Actual Vortex Library Status
 *
 * You can check the following to see which items from the RFC 3080 and
 * RFC 3081 are already covered by the Vortex Library: 
 *
 * \verbinclude vortex-status.txt
 */

/**
 * \page license License issues
 *
 * \section licence_intro Vortex Library terms of use
 * The Vortex Library is release under the terms of the Lesser General
 * Public license (LGPL). You can find it at
 * http://www.gnu.org/licenses/licenses.html#LGPL.
 *
 * The main implication this license has is that you are allowed to
 * use the Vortex Library for commercial application as well on open
 * source application using GPL/LGPL compatible licenses. 
 *
 * Restrictions for proprietary development are the following: 
 *
 * - You have to provide back to the main repository or to your
 * customers any change, modifications, improvements you may do to the
 * Vortex Library. Of course, we would appreciate to add to the main
 * Vortex Library repository any patch, idea or improvement you send
 * us but you are not required to do so.
 *
 * - You cannot use our company name or image, the Vortex Library name
 * or any trademark associated to that product or any product the
 * Advanced Software Production Line company may have in a way that
 * they could be damaged. Of course, we would appreciate your project, in
 * the case it is a proprietary one, make a reference to us but you are
 * not required to do so. 
 *
 * \section legal_notice_about_openssl About OpenSSL dependency:
 * 
 * \code
 * LEGAL NOTICE: This product includes software developed by the
 * OpenSSL Project for use in the OpenSSL
 * Toolkit. (http://www.openssl.org/)
 * \endcode
 *
 * From the OpenSSL site:
 *
 * \code
 * 
 * This software package uses strong cryptography, so even if it is
 * created, maintained and distributed from liberal countries in
 * Europe (where it is legal to do this), it falls under certain
 * export/import and/or use restrictions in some other parts of the
 * world.
 *
 * PLEASE REMEMBER THAT EXPORT/IMPORT AND/OR USE OF STRONG
 * CRYPTOGRAPHY SOFTWARE, PROVIDING CRYPTOGRAPHY HOOKS OR EVEN JUST
 * COMMUNICATING TECHNICAL DETAILS ABOUT CRYPTOGRAPHY SOFTWARE IS
 * ILLEGAL IN SOME PARTS OF THE WORLD. SO, WHEN YOU IMPORT THIS
 * PACKAGE TO YOUR COUNTRY, RE-DISTRIBUTE IT FROM THERE OR EVEN JUST
 * EMAIL TECHNICAL SUGGESTIONS OR EVEN SOURCE PATCHES TO THE AUTHOR OR
 * OTHER PEOPLE YOU ARE STRONGLY ADVISED TO PAY CLOSE ATTENTION TO ANY
 * EXPORT/IMPORT AND/OR USE LAWS WHICH APPLY TO YOU. THE AUTHORS OF
 * OPENSSL ARE NOT LIABLE FOR ANY VIOLATIONS YOU MAKE HERE. SO BE
 * CAREFUL, IT IS YOUR RESPONSIBILITY.
 * \endcode
 *
 * \section other Contact us to know more about licenses.
 *
 * Use the following contact information to reach us about this issue.
 * 
 * \code
 *      Postal address:
 *         Advanced Software Production Line, S.L.
 *         C/ Dr. Michavila Nº 14
 *         Coslada 28820 Madrid
 *         Spain
 *      Email address:
 *         info@aspl.es
 *      Fax and Telephones:
 *         (00 34) 91 669 55 32 - (00 34) 91 231 44 50
 *         Pleope from outside Spain must use (00 34) prefix.
 * \endcode
 * 
 *
 */


/**
 * \page install Installing and Using Vortex Library
 * \section Index
 *  On this page you will find:
 *   - \ref vortex_source
 *   - \ref vortex_from_svn
 *   - \ref compile_linux
 *   - \ref compile_mingw
 *   - \ref using_linux
 *   - \ref using_mingw
 *
 * \section vortex_source Getting latest stable release 
 *
 * The <b>Vortex Library main repository</b> is included <b>inside the Af-Arch</b>
 * repository. Af-Arch project uses Vortex Library as its data
 * transport layer.  
 *
 * If you are planing to download Vortex Library and using it as a
 * stand alone library apart from Af-Arch you can <b>get current stable
 * releases</b> at: http://www.sourceforge.net/projects/vortexlibrary
 * 
 * \section vortex_from_svn Getting latest sources from svn repository
 * 
 * You can also get the latest repository status, which is considered
 * to be stable, by checking out a svn copy executing the following:
 *
 * \code
 *   svn co https://dolphin.aspl.es/svn/publico/af-arch/trunk/libvortex
 * \endcode
 * 
 * If you are using Microsoft Windows platform use the url
 * above. SubVersion client programs for windows can be found at:
 * http://tortoisesvn.tigris.org (really recommended) or
 * http://rapidsvn.tigris.org.
 *
 * Of course you will have to download the Vortex Library
 * dependencies. Check the \ref dep_notes "this section" to know
 * more about library dependencies.
 *
 * \section compile_linux Compiling Vortex Library on GNU/linux environments (including Cygwin)
 *
 * If you are running a POSIX (unix-like including cygwin) environment you can use
 * autotools to configure the project so you can compile it. Type the
 * following once you have downloaded the source code:
 *
 * \code
 *   bash:~$ cd libvortex
 *   bash:~/libvortex$ ./autogen.sh
 * \endcode
 *
 * This will configure your project trying to find the dependencies
 * needed. 
 *
 * Once the configure process is done you can type: 
 * 
 * \code
 *   bash:~/libvortex$ make install
 * \endcode
 *
 * The previous command will require permissions to write inside the
 * destination directory. If you have problems, try to execute the
 * previous command as root.
 *
 * Because readline doesn't provide an standard way to get current
 * installation location, the following is provided to configure
 * readline installation. You have to use the <b>READLINE_PATH</b>
 * environment var as follows:
 * 
 * \code
 *   bash:~/libvortex$ READLINE_PATH=/some/path/to/readline make
 * \endcode
 *
 * <b>make</b> program will use the content of <b>READLINE_PATH</b> var to build an
 * include header directive and an include lib directive as follows:
 * \code
 *   READLINE_PATH=/some/path/to/readline
 *
 *   -I$(READLINE_PATH)/include
 *   -L$(READLINE_PATH)/lib
 * \endcode
 *
 * You <b>don't need to pay attention to this description if you don't
 * have problems with your readline</b> installation.
 * 
 * 
 *
 * \section compile_mingw Compiling Vortex Library on Windows Platforms using Mingw
 * 
 * Inside the <b>src</b> directory can be found a Makefile.win file. This
 * Makefile.win have been created to be run under mingw
 * environment. You will have to download the mingw environment and the
 * msys package.
 *
 * Then configure your local Makefile.win file according to your
 * environment, and use some as follow:
 * 
 * \code
 *  CC=gcc-3.3 vortex_dll=libvortex MODE=console make -f Makefile.win
 * \endcode
 *
 * Of course, the <b>CC</b> variable may point to another gcc, check the one
 * that is installed on your system but, make sure you are not using
 * the gcc provided by a cygwin installation. It will produce a faulty
 * libvortex.dll not usable by any native Microsoft Windows program.
 *
 * The <b>MODE</b> variable can be set to <b>"windows"</b>. This will disable the
 * console output. <b>"console"</b> value will allow to enable vortex log
 * info to console.
 * 
 * The <b>vortex_dll</b> variable must not be changed. This variable is
 * controlled by a top level Makefile.win inside the Af-Arch project so
 * that Makefile.win can control the library naming. To follow the
 * same convention naming will save you lot of problems in the future.
 *
 * Additionally, if you chose to provide the libraries directly you
 * will have to download the libraries the \ref dep_notes "Vortex Library depends on" inside a directory, for example,
 * <i>"c:\libraries"</i>. Use lowercase letter for your files and directories names, it
 * will save you lot of time. Now, edit the Makefile.win to comment
 * out those lines referring to pkg-config (LIBS and CFLAGS) and
 * uncomment those lines referring to BASE_DIR variable.
 *
 * Now type:
 * \code
 *  CC=gcc-3.3 vortex_dll=libvortex MODE=console BASE_DIR=c:/libraries make -f Makefile.win
 * \endcode
 *
 * This process will produce a libvortex.dll (actually the dynamic
 * libraries) and a import library called libvortex.dll.a. The import
 * library will be needed to compile your application under windows
 * against Vortex Library so it get linked to libvortex.dll.
 * 
 * \section using_linux Using Vortex on GNU/Linux platforms (including Cygwin)
 * 
 * Once you have installed the library you can type: 
 * \code
 *   gcc `pkg-config --cflags --libs vortex` your-program.c -o your-program
 * \endcode
 *
 * On windows platform using cygwin the previous example also
 * works. 
 *
 * \section using_mingw Using Vortex on Microsoft Windows with Mingw
 *
 * On mingw environments you should use something like:
 *
 * \code
 *  gcc your-program.c -o your-program
 *        -Ic:/libraries/include/vortex/  \
 *        -I"c:/libraries/include" \
 *        -I"c:/libraries/include/axl" \
 *        -L"c:/libraries/lib" \
 *        -L"c:/libraries/bin" \
 *        -lws2_32 \
 *        -laxl -lm 
 * \endcode
 *
 * Where <b>c:/libraries</b> contains the installation of the Vortex
 * Library (headers files installed on c:/libraries/include/vortex,
 * import library: libvortex.dll.a and dll: libvortex.dll) and LibAxl installation.
 *
 * The <b>-lws2_32</b> will provide winsocks2 reference to your
 * program.
 */

/** 
 * \page starting_to_program Vortex Library Manual (C API)
 * 
 * \section Introduction
 * 
 *  On this manual you will find the following sections:
 *  
 *  <b>Section 1: </b>An introduction to BEEP and Vortex Library
 *
 *  - \ref vortex_manual_concepts
 *  - \ref vortex_manual_listener
 *  - \ref vortex_manual_client
 *
 *  <b>Section 2: </b>Sending and receiving data with Vortex Library
 *
 *  - \ref vortex_manual_sending_frames
 *  - \ref vortex_manual_dispatch_schema
 *  - \ref vortex_manual_printf_like
 *  - \ref vortex_manual_wait_reply
 *  - \ref vortex_manual_invocation_chain
 * 
 *  <b>Section 3: </b>Doing Vortex Library to work like you expect:
 *  profiles, internal configuration and other useful information to
 *  adapt Vortex Library to your needs.
 *
 *  - \ref vortex_manual_profiles
 *  - \ref vortex_manual_implementing_request_response_pattern
 *  - \ref vortex_manual_changing_vortex_io
 *
 *  <b>Section 4: </b>Advanced topics
 *
 *  - \ref vortex_manual_piggyback_support
 *  - \ref vortex_manual_using_mime
 *
 *  <b>Section 5: </b>Securing and authenticating your BEEP sessions: TLS and SASL profiles
 * 
 *  - \ref vortex_manual_securing_your_session
 *  - \ref vortex_manual_creating_certificates
 *  - \ref vortex_manual_using_sasl
 *  - \ref vortex_manual_sasl_for_client_side
 *  - \ref vortex_manual_sasl_for_server_side
 * 
 * \section vortex_manual_concepts Some concepts before starting to use Vortex
 * 
 * Before beginning, we have to review some definitions about the
 * protocol that <b>Vortex Library</b> implements. This will help you
 * to understand why Vortex Library uses some names to refer things such
 * as: <b>frames, channels, profiles</b>, etc.
 *
 * <b>Vortex Library is an implementation of the RFC 3080/RFC 3081
 * protocol</b>, also known as <b>BEEP: Block Extensible Exchange
 * Protocol</b>. In the past it was called BXXP because the Xs of
 * e(X)tensible and e(X)change. Due to some marketing naming decision
 * finally it was called BEEP because the Es of the (E)xtensible and
 * (E)xchange.
 *
 * \code
 *    BEEP:   the protocol
 *    Vortex: an implementation
 * \endcode
 *
 * From a simple point of view, the <a href="http://www.beepcore.org">BEEP protocol</a>
 * defines how data must be exchanged between applications, also
 * called BEEP peers, using several abstractions, that allows programmers
 * to write network applications with stronger features such as
 * asynchronous communication, several concurrent messages being
 * exchanged over the same connection and so on, without worrying
 * too much about details.
 * 
 * Previous abstractions defined by the BEEP RFC are really
 * important. To understand them is a key to understand, not only BEEP
 * as a protocol but Vortex Library as an implementation. This will
 * also help you to get better results while using BEEP to implement
 * your network protocol.
 * 
 * \code
 *       BEEP abstraction layer
 *       -----------------------
 *       |       Message       |
 *       -----------------------
 *       |        Frame        |
 *       |                     |
 *       |      (payload)      |
 *       -----------------------
 *       |  Channel / Profiles |
 *       -----------------------
 *       |       Session       |
 *       -----------------------
 * \endcode
 * 
 * Previous table <b>is not the BEEP API stack</b> or the <b>Vortex Library API
 * stack</b>. It represents the concepts you must use to be able to send and
 * receive data while using a <b>BEEP</b> implementation like Vortex Library.
 * 
 * A <b>message is actually your application data</b> to be sent to a
 * remote peer. It has no special meaning for a <b>BEEP</b>
 * implementation. Applications using the particular BEEP
 * implementation are the ones who finally pay attention to message
 * format, correction and content meaning.
 * 
 * When you send a message (or a reply) <b>these messages could be
 * splitted into frames</b>. 
 *
 * These frames have an special header, called the BEEP frame header,
 * which includes information about payload sequence, message type,
 * channels used and many things more. This data, included inside the
 * BEEP frame headers allows BEEP peers to track communication status,
 * making it possible to detect errors, sync lost, etc.
 *
 * <b>Payload</b> is the way network protocol designers usually call
 * to the application level data, that is, your data
 * application. However, this payload could represent only a piece of
 * your information. This is not important for you, at this moment,
 * because <b>Vortex Library</b> manage frame fragmentation
 * (internal/external) in a transparent way.
 * 
 * When <b>you send a message, you select a channel to do this
 * communication</b>. We have seen that a message is actually splitted
 * but from the application view, in most cases, this is not important:
 * <b>applications send messages over channels</b>.
 * 
 * Inside <a href="http://www.beepcore.org">BEEP protocol</a>,
 * <b>every channel created</b> must be running <b>under the semantic
 * of a profile definition</b>. In fact, the part of the application
 * that takes care about message format, correction, and content
 * meaning is the BEEP profile.
 * 
 * This simple concept, which usually confuse programmers new to BEEP,
 * is not anything estrange or special. A profile <b>only defines what
 * type of messages will be exchanged</b> inside a channel.  It is
 * just an agreement between BEEP peers about the messages to exchange
 * inside a channel and what they mean.
 * 
 * Actually, <b>to support a profile</b> means to register the string
 * which identifies the profile and to implement it on top of
 * Vortex Library so that profile can send and receive message
 * according to the format the profile defines. <b>A profile inside
 * Vortex Library is not</b>: 
 *
 * - A dll you have to implement or a plug in to be attached. It is a
 * piece of code written on top of the library. 
 * 
 * - This only applies to Vortex Library. If you use Turbulence
 * (http://www.turbulence.ws) you'll find that is allows to write new
 * BEEP profiles as plugins, but you still write code calling to the
 * Vortex Library API.
 * 
 * In order to use the Vortex Library, and any BEEP implementation, you
 * must define your own profile. Later, you can read: \ref vortex_manual_profiles "defining a profile inside Vortex Library".
 * 
 * The last one concept to understand is <b>the BEEP
 * session</b>. According to BEEP RFC, a session is an abstraction
 * which allows to hold all channels created to a remote BEEP
 * peer (no matter what profiles where used). Because Vortex Library
 * is a BEEP implementation mapped into TCP/IP, a session is actually
 * a TCP connection (with some additional data).
 * 
 * Now we know most of the concepts involving BEEP, here goes how this
 * concepts get mapped into a concrete example using Vortex. Keep in
 * mind this is a simplified version on how Vortex Library could be.
 * 
 * In order to send a message to a remote peer you'll have to create a
 * \ref VortexConnection, using \ref vortex_connection_new as follows:
 * 
 * \code
 *        char  * host = "myhost.at.frobnicate.com";
 *        char  * port = "55000";
 * 
 *        // Creates a Vortex Connection without providing a
 *        // OnConnected handler or user data. This will block us
 *        // until the connection is created or fails.
 *        VortexConnection * connection = vortex_connection_new (host, port, NULL, NULL);
 * \endcode
 * 
 * Once finished, you have actually created a BEEP session. Then you
 * have to create a \ref VortexChannel, using \ref vortex_channel_new
 * function, providing a profile defined by you or an existing one.
 * On that process it is needed to select the channel number, let's
 * say we want to create the channel 2.
 * 
 * \code
 *        // Create a Vortex Channel over the given connection and using as channel number: 2.
 *        VortexChannel * new_channel = NULL;
 *        new_channel = vortex_channel_new (connection, 2,
 *                                          "http://my.profile.com",
 *                                          // do not provide on
 *                                          // channel close handlers.
 *                                          NULL, NULL, 
 *                                          // provide frame receive
 *                                          // handler (second level one)
 *                                          // Now, on_receiving_data
 *                                          // will be executed for every
 *                                          // frame received on this
 *                                          // channel unless wait reply
 *                                          // method is used.
 *                                          on_receiving_data, NULL,
 *                                          // do not provide a
 *                                          // OnChannelCreated
 *                                          // handler. This will block
 *                                          // us until the channel is
 *                                          // created.
 *                                          NULL, NULL);
 * \endcode
 * 
 * Once the channel is created (keep in mind that the remote peer can
 * actually deny the channel creation) you could send messages to
 * remote peer using this channel as follows:
 * 
 * \code
 *       // send a reply import message to remote peer.
 *       if (vortex_channel_send_msg (new_channel,
 * 				   "this a message to be sent",
 * 				   25,
 * 				   NULL)) {
 * 	   printf ("Okey, my message was sent");
 *       }
 * \endcode
 * 
 * And finally, once the channel is no longer needed, you can close it as follows: 
 * 
 * \code
 *      // close the channel. This will block us until the channel is closed because
 *      // to close a channel can actually take longer time because the remote peer
 *      // may not accept close request until he is done.
 *      if (vortex_channel_close (new_channel, NULL)) {
 * 	   printf ("Okey, my channel have been closed");
 *      }
 *
 *      // finally, finalize vortex running
 *      vortex_exit ();
 * \endcode
 * 
 *
 * That's all. You have created a simple Vortex Library client that
 * have connected, created a channel, send a message, close the
 * channel and terminated Vortex Library function.
 * 
 * \section vortex_manual_listener How a Vortex Listener works (or how to create one)
 *
 * To create a vortex listener, which waits for incoming beep connection
 * on a given port the following must be done:
 * 
 * \code  
 *   #include <vortex.h>
 *
 *   void on_ready (char  * host, int  port, VortexStatus status,
 *                  char  * message, axlPointer user_data) {
 *        if (status != VortexOk) {
 *              printf ("Unable to initialize vortex listener: %s\n", message);
 *              // do not exit from here using: exit or vortex_exit. This is actually
 *              // done by the main thread
 *        }
 *        printf ("My vortex server is up and ready..\n");
 *        // do some stuff..
 *   }
 *
 *   int  main (int  argc, char  ** argv) {
 *       // init vortex library
 *       vortex_init ();
 * 
 *       // register a profile
 *       vortex_profiles_register (SOME_PROFILE_URI,	
 *                                 // provide a first level start
 *                                 // handler (start handler can only be
 *                                 // provided at first level)
 *                                 start_handler, start_data, 
 *                                 // provide a first level close handler
 *                                 close_handler, close_data,
 *                                 // provide a first level frame receive handler
 *                                 frame_received_handler, frame_received_data);
 * 
 *       // create a vortex server using any name the running host may have.
 *       // the on_ready handler will be executed on vortex listener creation finish.
 *       vortex_listener_new ("0.0.0.0", "44000", on_ready);
 * 
 *       // wait for listeners
 *       vortex_listener_wait ();
 *
 *       // finalize vortex running
 *       vortex_exit ();
 *
 *        return 0;
 *   }
 *
 * \endcode
 * 
 * These four steps does the follow:
 *   <ul>
 *
 *   <li>Initialize the library and its subsystems using \ref
 *   vortex_init. If \ref vortex_init function is not called,
 *   unexpected behaviors will happen.</li>
 *
 *   <li>Register one (or more profiles) the listener being
 *      created will accept as valid profiles to create new channels over
 *      session using \ref vortex_profiles_register.</li>
 *
 *   <li>Create the listener using \ref vortex_listener_new, specifying the hostname to be used to listen
 *      incoming connection and the port. If hostname used is 0.0.0.0 all
 *      hostname found will be used. If 0 is used as port, an automatic
 *      port assigned by the SO will be used.</li>
 *
 *   <li>Finally, call to wait listener created using \ref vortex_listener_wait.</li>
 *   </ul>
 *
 * On the 2) step, which register a profile called SOME_PROFILE_URI to
 * be used on channel creation, it also set handlers to be called on
 * events happening for this listener.
 * 
 * These handlers are: <b>start handler</b>, <b>close handler</b>, and
 * <b>frame_received</b> handler. 
 * 
 * The \ref VortexOnStartChannel "start handler" is executed to notify
 * that a new petition to create a new channel over some session have
 * arrived. But this handler is also executed to know if Vortex
 * Listener agree to create the channel. If start handler returns true
 * the channel will be created, otherwise not.
 * 
 * If you don't define a start handler a default one will be used
 * which always returns true. This means: all channel creation
 * petition will be accepted.
 * 
 * The \ref VortexOnCloseChannel "close handler" works the same as
 * start handler but this time to notify if a channel can be
 * closed. Again, if close handler returns true, the channel will be
 * closed, otherwise not.
 *
 * If you don't provide a close handler, a default one will be used,
 * which always returns true, that is, all channel close petition will be
 * accepted.
 * 
 * The \ref VortexOnFrameReceived "frame received handler" is executed
 * to notify a new frame have arrived over a particular channel. The
 * frame before been delivered, have been verified to be properly
 * defined. But, payload content must be actually checked by the
 * profile implementation. Vortex Library doesn't pay attention to the
 * payload actually being transported by frames.
 * 
 * All notification are run on newly created threads, that are
 * already created threads inside a thread pool. 
 *
 * As a test, you can run the server defined above, and use the
 * <b>vortex-client</b> tool to check it.
 * 
 * \section vortex_manual_client How a vortex client works (or how to create a connection)
 * 
 * A vortex client peer works in a different way than listener
 * does. In order to connect to a vortex listener server (or a BEEP
 * enabled peer) a vortex client peer have to:
 * 
 * \code
 *   #include <vortex.h>
 *   
 *   int  main (int  argc, char  ** argv) {
 *     // a connection reference 
 *     VortexConnection * connection;
 *
 *     // init vortex library
 *     vortex_init ();
 * 
 *     // connect to remote vortex listener
 *     connection = vortex_connection_new (host, port, 
 *                                         // do not provide an on_connected_handler 
 *                                         NULL, NULL);
 * 
 *     // check if everything is ok
 *     if (!vortex_connection_is_ok (connection, false)) {
 *            printf ("Connection have failed: %s\n", 
 * 		    vortex_connection_get_message (connection));
 *            vortex_connection_close (connection);
 *     }
 *   
 *     // connection ok, do some stuff
 *     
 *
 *     // and finally call to terminate vortex
 *     vortex_exit ();
 *   }
 *   
 * \endcode
 * 
 * Previous steps stands for:
 *   <ul>
 *   
 *   <li> Initialize Vortex Library calling to \ref vortex_init. As we have
 *   seen on vortex listener case, if vortex library is not initialized
 *   unexpected behaviors will occur.</li>
 *    
 *   <li>Connect to remote peer located at host and port using
 *   \ref vortex_connection_new. This function will create a BEEP session
 *   (actually a connection with some additional info) to remote
 *   site. 
 *
 *   This function will block the caller until connection is created
 *   because the example didn't provide an \ref VortexConnectionNew "on_connected_handler" function (that is,
 *   passing a NULL value).
 *   
 *   This makes code easy to understand, because it is linear to read
 *   but, using it on graphical user interfaces turns out that it is
 *   not a good option.  If you provide a \ref VortexConnectionNew
 *   "on_connected_handler" the function will not block the caller and
 *   will do the connection process in a separated thread. 
 * 
 *   Once the connection process have been finished vortex library
 *   will notify on the defined handler allowing caller thread to keep
 *   on doing other stuff such updating the user interface with some
 *   cute connection progress bar.
 *
 *   This initial connection creates not only a BEEP session, it
 *   also creates the channel 0. This channel is used for session
 *   management functions such as channel creation. </li>
 *
 *   <li>Finally, returned connection must be checked using \ref
 *   vortex_connection_is_ok.  This step must be done on both model:
 *   on block model and on threaded model.</li> 
 * 
 *   </ul>
 * 
 * Once a vortex connection is successfully completed, it is registered
 * on Vortex Reader thread. This allows Vortex Reader thread to
 * process and dispatch all incoming frame to its default channel handler.
 * 
 * We have talked about channel handlers: the \ref VortexOnStartChannel "start", 
 * \ref VortexOnCloseChannel "close" and \ref VortexOnFrameReceived "frame received" handler. 
 * Due to client peer nature, it will be common to
 * not pay attention to start and close events. If no handler is
 * defined, default ones will be used. Of course, if it is needed to
 * have more control over this process, event handlers should be
 * defined.
 * 
 * The \ref VortexOnFrameReceived "frame received" handler must be defined for each channel
 * created. If no frame received handler is defined for a channel
 * used by a vortex client, virtually you won't receive any
 * frame. 
 *
 * This is because a vortex client is not required to register a
 * profile before creating Vortex Connections. Of course, if you
 * register a profile with the handlers before creating the connection
 * those ones will be used if not handlers are provided on channel
 * creation. See \ref vortex_manual_dispatch_schema "this section" to understand how
 * the frame dispatch schema works.
 * 
 * \section vortex_manual_sending_frames How an application must use Vortex Library to send and receive data
 * 
 * As defined on RFC 3080, any BEEP enabled application should define
 * a profile to be used for its message exchange. That profile will
 * make a decision about which message-exchange style defined will use. There
 * are 3 message exchange style.
 * 
 * <ul>
 *
 * <li> <b>MSG/RPY</b>: this is a one-to-one message exchange style
 * and means a BEEP peer sends a message MSG and remote peer perform
 * some task to finally responds using a RPY message type.</li>
 *
 * <li><b>MSG/ERR</b>: this is a one-to-one message exchange style and
 * means the same as previous message exchange but remote peer have
 * responded with an error, so the task was not performed.</li>
 *
 * <li><b>MSG/ANS</b>: this is a one-to-many message exchange style
 * and works pretty much as MSG/RPY definition but defined to allows
 * remote peer to keep on send ANS frames during the task
 * execution. This type of message exchange replies to a MSG received
 * with several ANS replies, with no constrains about the limit of ANS
 * to be sent, ending that reply series with a NUL frame.</li>
 *
 * </ul>
 * 
 * While using Vortex Library you can send data to remote peer using the following
 * functions defined at the vortex channel API. 
 * 
 * \code
 * 1) vortex_channel_send_msg
 * 
 * 2) vortex_channel_send_rpy
 * 
 * 3) vortex_channel_send_err
 * 
 * 4) vortex_channel_send_ans_rpy
 * \endcode
 *
 * As you may observe to generate the different types of message to be
 * sent a function is provided: 
 *
 * The first one allows you to send a new message. Once the message is
 * queued to be sent the function returns you the message number used
 * for this sending attempt. This function never block and actually do
 * not send the message directly. It just signal the Vortex Sequencer
 * to do the frame sequencing which finally will make Vortex Writer to
 * send the frames generated.
 * 
 * The second function allows to positive reply to a specific message
 * received. In order to be able to perform a positive reply using
 * \ref vortex_channel_send_rpy or a negative reply using
 * \ref vortex_channel_send_err you have to provide the message number to
 * reply to.
 * 
 * The third function allows to reply an error to a specify message
 * received. As the previous function it is needed the message number
 * to reply to.
 *
 * The fourth function allows to reply an ANS message to a received
 * MSG. Several calls to that send ANS replies must be always ended
 * with \ref vortex_channel_finalize_ans_rpy which actually sends an
 * NUL frame.
 * 
 * Things <b>that cannot be done</b> by Vortex applications, and any
 * other BEEP framework, is to send MSG frames to each other without using
 * reply message (RPY/ERR/ANS/NUL). 
 * 
 * Actually you can use only MSG type message to send data to other
 * Vortex (or BEEP) enabled application but this is not the
 * point. Application have to think about MSG type as a request
 * message and RPY as a request reply message. The point is: <b>do not
 * use MSG to reply message received, use RPY/ERR/ANS/NUL types.</b>
 *
 * \section vortex_manual_dispatch_schema The Vortex Library Frame receiving dispatch schema (or how incoming frames are read)
 *
 * Once a frame is received and validated, the Vortex Reader try to
 * deliver it following next rules:
 *
 * - Invoke a second level handler for frame receive event. The second
 * level handler is a user space callback defined per channel. Several
 * channel on the same connections may have different second level
 * handlers (or the same) according to its purpose.
 *
 * - If second level handler for frame receive event where not
 * defined, the Vortex Reader tries to dispatch the frame on the first
 * level handler for frame receive handler event. The first level
 * handler is defined at profile level. This means that channels using
 * the same profiles shares frame receive callback. This allows to
 * defined a general purpose callback at user space for every channel
 * created on every connection.
 *
 * - Finally, it may happen that a thread wants to keep on waiting for
 * a specific frame to be received bypassing the second and first
 * level handlers. Its is useful for that batch process that can't
 * continue if the frame response is not received. This is called \ref vortex_manual_wait_reply "wait reply method".
 *
 * The second and first level handler dispatching method are called
 * asynchronous message receiving because allows user code to keep on
 * doing other things and be notified only when frames are received.
 *
 * The wait reply method is also called synchronous dispatch method
 * because it blocks the caller thread until the specific frame reply
 * is received. The wait reply method disables the second and first
 * level handler execution.
 *
 * Keep in mind that if no second level handler, first level handler
 * or wait reply method is defined, the Vortex Reader will drop the
 * frame.
 *
 * Because \ref vortex_manual_wait_reply "wait reply method" doesn't
 * support receiving all frames in a channel, to perform blocking
 * code, you may also be interested in a mechanism that is implemented
 * on top of the second (or first) level handlers, that allows to get
 * the same functionality that the \ref vortex_manual_wait_reply "wait reply method", but including
 * all frames received. Check the following \ref vortex_channel_get_reply "function to know more about this method".
 *   
 * 
 *
 * As you note, the Vortex Library support both method while receiving
 * data: asynchronous method and synchronous method. This is also
 * applied to sending user data.
 *
 * \section vortex_manual_printf_like Printf like interface while sending messages and replies
 *
 * Additionally, there are also function versions for the previous ones
 * which accepts a variable argument list so you can send message in a
 * printf like fashion.
 *
 * \code
 * 
 *   if (!vortex_channel_send_msgv (a_channel, NULL, 
 *                                  "Send this message with content: %s and size: %d", 
 *                                  content, content_size)) {
 *         printf ("Unable to send my message\n");
 *   }
 * \endcode 
 * 
 * They are the same function names but appending a "v" at the end.
 * 
 * \section vortex_manual_wait_reply Sending data and wait for a specific reply (or how get blocked until a reply arrives)
 *
 * We have seen in previous section we can use several function to send
 * message in a non-blocking fashion no matter how big the message is:
 * calling to those function will never block. But, what if it is
 * needed to get blocked until a reply is received.
 * 
 * Vortex Library defines a <b>wait reply method</b> allowing to bypass the
 * second and first level handlers defined \ref vortex_manual_dispatch_schema "inside the frame received dispatch schema".
 * 
 * Here is an example of code on how to use Wait Reply method:
 * 
 *  \code
 *   
 *    VortexFrame   * frame;
 *    int             msg_no;
 *    WaitReplyData * wait_reply;
 * 
 *    // create a wait reply 
 *    wait_reply = vortex_channel_create_wait_reply ();
 *     
 *    // now send the message using msg_and_wait/v
 *    if (!vortex_channel_send_msg_and_wait (channel, "my message", 
 *                                           strlen ("my message"), 
 *                                           &msg_no, wait_reply)) {
 *        printf ("Unable to send my message\n");
 *        vortex_channel_free_wait_reply (wait_reply);
 *    }
 *
 *    // get blocked until the reply arrives, the wait_reply object
 *    // must not be freed after this function because it already free it.
 *    frame = vortex_channel_wait_reply (channel, msg_no, wait_reply);
 *    if (frame == NULL) {
 *         printf ("there was an error while receiving the reply or a timeout have occur\n");
 *    }
 *    printf ("my reply have arrived: (size: %d):\n%s", 
 *             vortex_frame_get_payload_size (frame), 
 *             vortex_frame_get_payload (frame));
 *
 *    // that's all!
 * 
 *  \endcode
 * 
 * 
 * \section vortex_manual_invocation_chain Invocation level for frames receive handler
 * 
 * Application designer has to keep in mind the following invocation
 * order for frame received handler:
 * <ul>
 *
 * <li> First of all, if the frame received is a reply one, that is, a
 * RPY or ERR frame, then Vortex Library look up for waiting thread
 * blocked on \ref vortex_channel_wait_reply. If found, the frame is
 * delivered and invocation chain is stopped.</li>
 *
 * <li>If previous lookup was not successful, vortex search for a
 * second level handler defined for the channel which is receiving the
 * frame. This second level handler has been defined by
 * \ref vortex_channel_new at channel creation time. If frame received
 * handler is found for this level the invocation chain is
 * stopped.</li>
 *
 * <li>If previous lookup was not successful, vortex search for a
 * first level handler. This handler have been defined by
 * \ref vortex_profiles_register at profile registration.  If frame
 * received handler is found for this level the invocation chain is
 * stopped.</li>
 *
 * <li>Finally, if vortex do not find a way to deliver frame received,
 * then it is dropped (freeing the frame resources and registering a
 * log message)</li>
 *
 * </ul>
 *
 * As a consequence:
 * <ul>
 *
 * <li>If an application don't define any handler then it will only be
 * able to receive frames only through the \ref
 * vortex_channel_wait_reply function. And of course, this function
 * only allows to receive replies frames so, any message received that
 * is not a reply will be dropped.</li>
 * 
 * <li>If an application defines the first level handler (using
 * \ref vortex_profiles_register) this handler will be executed for all
 * frames received for all channel defined under the profile
 * selected.</li>
 * 
 * <li>If an application defines the second level handler (using
 * \ref vortex_channel_new) this handler will be executed for all frames
 * received for a particular channel, the one created by
 * \ref vortex_channel_new which received the frame received handler.</li>
 *
 * </ul>
 * 
 * \section vortex_manual_profiles Defining a profile inside Vortex (or How profiles concept confuse people)
 * 
 * Now we have to consider to spend some time learning more about
 * profiles. The profile concept inside the BEEP Core definition is
 * the most simple but at the same time seems to be the most
 * confusing. 
 *
 * From a simple point of view, <b>a BEEP profile is what you add to
 * the BEEP protocol to make it useful for you</b>. BEEP provides you
 * building blocks that you have to organize to create a useful
 * protocol. This "protocol configuration" usually involves creating a
 * BEEP profile (or reuse an existing one) extending the BEEP protocol
 * beyond its initial definition to reach your needs.
 *
 * From the source code point of view, creating a profile only means:
 * <ul>
 *
 * <li>To register a profile uri using \ref vortex_profiles_register
 * and ...</li>
 * 
 * <li>To implement the protocol described by the profile. That
 * profile description could be one already defined or one you are
 * defining yourself.  You will have to write source that enable your
 * application to send messages and process replies following the
 * profile semantic. This is actually done defining the:
 *
 *   <ul>
 * 
 *   <li>\ref VortexOnFrameReceived "On frame receive"
 *   handler. This address the part of the protocol where data is received.</li>
 *
 *   <li>And, implement the part of the protocol that sends (or replies) data.</li>
 *
 *  </ul>
 *  </li>
 * </ul>
 *
 * In other words, because the profile is only a definition on how you
 * should send messages, how to reply to them, and what types of
 * messages you will have to recognize, its content and format, or
 * what will happen on some particular circumstances, it is only needed to
 * register the profile name and to implement that behavior on top of
 * the Vortex Library to fulfill the profile especification.
 *
 * Maybe the main problem a new BEEP programmer have to face is the
 * fact that a BEEP implementation doesn't allows him to start 
 * sending and receiving messages out of the box.
 *
 * This is because <b>the BEEP definition and Vortex Library
 * implementation is more a toolkit to build application protocols
 * than a ready to use application protocol itself</b>. It is a
 * framework to allow BEEP programmers to define its network protocols
 * in a easy, consistent and maintainable way.
 *
 * Now see the tutorial about \ref profile_example "creating a simple profile" involving a simple server
 * creation with a simple client.
 *
 * \section vortex_manual_piggyback_support Using piggyback to save one round trip at channel startup
 *
 * Once defined the application protocol on top of Vortex Library or
 * any other BEEP implementation we could find that creating a
 * channel, involving a request-reply exchange, to later starting to
 * perform real work, represents an initial imposed latency. 
 *
 * This could be easily solved by using piggybacking for the initial
 * messages exchanged. Let's see how channel creation works, without
 * using piggybacking, to later starting to exchange data:
 *
 * \code
 *    (1) I: A BEEP peer send an <start> channel item --->
 *
 *                    <--- (2) L: Remote BEEP peer accept the <start> channel and reply
 *
 *    (3) I: The BEEP peer send the initial <message> --->
 *
 *                    <--- (4) L: Remote BEEP peer receives <message> and reply to it.
 * \endcode
 * 
 * Previous example shows that underlying BEEP negotiation forces us to
 * waste time, (1) and (2), by creating the channel, and later
 * perform real work for our protocol: (3) and (4). 
 *
 * To solve this, what we have to do is to piggyback the message (3)
 * into the initial (1) start message and to piggyback the reply (4) into
 * the initial reply (2). 
 * 
 * This allows to the peer receiving the initial start channel message
 * to process the channel request and later to process the initial
 * message receiving inside it as it where the initial frame received.
 *
 * Additionally, the BEEP peer that have received the initial message
 * not only reply to the channel start but also uses this first reply
 * done to also reply the initial piggyback received. 
 *
 * As a result, previous example is now like the following:
 * 
 * \code
 *    (1) I: A BEEP peer send an <start> channel item
 *    (3) I: [The BEEP peer send the initial <message>] --->
 *
 *                         (2) L: Remote BEEP peer accept the <start> channel and reply
 *                    <--- (4) L: [Remote BEEP peer receives <message> and reply to it.]
 * \endcode
 *
 * Conclusion: we have saved one round trip, the channel creation
 * initial exchange.
 *
 * Ok, but how this is actually implemented inside Vortex Library?
 * Well, piggybacking is mostly automatic while using Vortex
 * Library. Let's see how it works for each BEEP peer side.
 * 
 * For the client side, to request creating a new channel and using
 * this initial exchange to send the initial request you could use :
 * 
 *  - \ref vortex_channel_new_full
 *  - \ref vortex_channel_new_fullv
 *
 * Previous functions will produce a start channel request defining the
 * initial piggyback by using the <b>profile_content</b> parameter.
 * 
 * At the server side, supposing that is a Vortex Library one, it will
 * receive the channel start request and the initial piggyback
 * content, if the \ref VortexOnStartChannelExtended
 * "OnStartChannelExtended" handler is defined to process incoming
 * start request. 
 * 
 * On that handler, the initial piggyback received will be the content of the
 * <b>profile_content</b> parameter and the
 * <b>profile_content_reply</b> parameter provides the way to reply to
 * the initial piggyback received. 
 * 
 * Here is an example for the \ref VortexOnStartChannelExtended
 * "OnStartChannelExtended" handler:
 * \code
 *  bool     extended_start_channel (char              * profile,
 *                                   int                 channel_num,
 *                                   VortexConnection  * connection,
 *                                   char              * serverName,
 *                                   char              * profile_content,
 *                                   char             ** profile_content_reply,
 *                                   VortexEncoding      encoding,
 *                                   axlPointer          user_data)
 * {
 *      printf ("Received a channel start request!\n");
 *      if (profile_content != NULL) {
 *          // we have received an initial piggyback, reply to it
 *          // by filling up profile_content_reply
 *          (* profile_content_reply) = ...; // dynamically allocated message
 *
 *      }
 *      // accept the channel to be created.
 *      return true;
 * }
 * \endcode
 * 
 * Piggyback reply processing for client side is more simple. We have two
 * cases:
 * <ul>
 *
 * <li>1) If the channel creation request was performed by providing a
 * \ref VortexOnChannelCreated application programmer doesn't need to
 * do any especial operation, it will receive piggyback reply as the
 * first frame received after \ref VortexOnChannelCreated is executed.
 * </li>
 *
 * <li>2) If the channel creation was performed in a synchronous way,
 * without providing \ref VortexOnChannelCreated handler, application
 * programmer have to make a call to \ref
 * vortex_channel_have_piggyback to check if a piggyback reply was
 * received and later call to \ref vortex_channel_get_piggyback. 
 * 
 * Here is an example:
 * \code
 * // create the channel in a synchronous way
 * VortexChannel * channel = vortex_channel_new_full (...);
 * VortexFrame   * reply;
 * 
 * // check if the channel was created
 * if (channel == NULL) {
 *     // unable to create the channel, no piggyback received
 *     return;
 * }
 * 
 * // before continue, check for initial piggyback
 * if (vortex_channel_have_piggyback (channel)) {
 *      // get piggyback (the reply for the initial request)
 *      reply = vortex_channel_get_piggyback (channel);
 *
 *      // process it.
 *      process_reply (reply);
 * }
 * \endcode
 * </li> 
 * </ul>
 * 
 * If you think this is too complicated, that's ok. It means you can
 * survive without using piggyback feature for your protocol. However,
 * many standard BEEP profiles makes use of this feature to be more
 * efficient, like TLS and SASL profiles.
 *
 * \section vortex_manual_using_mime Using MIME configuration for data exchanged under Vortex Library
 *
 * We have to consider several issues while talking about MIME and
 * MIME inside BEEP.
 *
 * Initially, MIME was designed to allow transport application
 * protocols, especially SMTP (but later extended to many protocols
 * like HTTP), as a mechanism to notify the application level the
 * content type of the object being received. The same happens with
 * BEEP.
 *
 * But, what happens when application protocol designers just ignore
 * MIME information, no matter which transport protocol they are
 * using? Again, this is not clear, mainly because MIME is just an
 * indicator.
 *
 * If you pretend to develop a kind of bridge that is able to
 * transport <b>everything</b> without a previous knowledge (both
 * peers can't make assumptions about the content to be received), it
 * is likely MIME is required (or some short of content notificator).
 *
 * As a conclusion: if your system will be the message producer and
 * the message consumer at the same time, you can safely ignore MIME,
 * because you can make assumptions about the kind of messages to be
 * exchanged. However, if a third party software is required to be
 * supported, that is initially unknown, you'll need MIME.
 *
 * Now, let's talk about MIME support inside Vortex.
 *
 * The BEEP protocol definition states that all messages exchanged are
 * MIME objects, that is, arbitrary user application data, that have a
 * MIME header which configures/specify the content type.
 *
 * Initially, if you send a message, without taking into account the
 * MIME type, either because you didn't configure anything nor your
 * messages didn't include any MIME headers, then frames generated
 * won't have any MIME hearders. 
 * 
 * However, even in that case, BEEP suppose a MIME IMPLICIT
 * information, which are the following values:
 *
 * \code
 *        Content-Type: application/octet-stream
 *        Content-Transfer-Encoding: binary
 * \endcode
 *
 *
 * This means that the remote peer will have to recognize that the
 * frame received doesn't have any MIME configuration and previous
 * values should be implicitly notified (again, even when they are not
 * found).
 * 
 * If you require to set a MIME type configuration, but you don't want
 * to append such information for every message you send, you can
 * configure default MIME headers to be used for every profile
 * registered, using the following set of functions:
 * 
 * - \ref vortex_profiles_set_mime_type
 * - \ref vortex_profiles_get_mime_type
 * - \ref vortex_profiles_get_transfer_encoding
 * 
 * However, this mechanism doesn't fit very well if it is required to
 * send arbitrary MIME objects under the same profile, that is, the
 * same running channel, because previous configuration will append
 * the same MIME information to every message being sent.
 * 
 * Obviously this is not a good option, as well as the initial
 * solution: to create a profile per MIME object type you have. This
 * will create you lot of problems, making your software fragile to
 * future changes.
 * 
 * What you can do is to take advantage of the implicit MIME headers
 * configuration. Just don't configure anything about MIME headers,
 * and include your MIME configuration in the message you are passing
 * in to the sending API as normal.
 * 
 * Because you didn't configure anything about MIME headers, Vortex
 * will take you message as a hole, sending it to the remote peer. 
 *
 * At the remote peer, the framing mechanism will detect the MIME
 * configuration, setting it to frames delivered into your
 * application.
 * 
 * Conclusion: under the same profile, withtout configuring any MIME
 * information, you are able to send any arbitrary MIME object.
 *
 * \section vortex_manual_implementing_request_response_pattern Implementing the request-response pattern
 *
 * When it comes to implement the request/response interaction
 * pattern, BEEP is a great choice. On this section, it is provided
 * some tips to enable people to properly implement this pattern type
 * with Vortex Library.
 *
 * While implementing the request/response pattern the very first
 * thing to control is how a reply is processed. If pattern is
 * implemented using a synchronous invocation, there is not too much
 * problems. The fun comes while implementing it in an asynchronous
 * manner.
 * 
 * Asynchronous, request/response, pattern needs to solve how to
 * associate and process replies received with the proper handler
 * provided at the time the request was performed. Inside Vortex
 * Library, asynchronous request replies are processed by providing a
 * \ref VortexOnFrameReceived "frame received handler" (provided at
 * \ref vortex_channel_new or \ref vortex_channel_new_full).
 * 
 * However, this handler is provided at the channel creation time,
 * making it available to all requests performed under that
 * channel. 
 *
 * The very first thought to solve previous problem is to provide a
 * different frame receive with some application data for every
 * request performed so each reply is processed by its corresponding
 * handler. This is actually done by using \ref vortex_channel_set_received_handler.
 *
 * Well, this will cause a race condition making responses to be
 * processed by request handlers that are not the associated. This is
 * because the frame receive handler for a channel could be only one
 * at the same time, which is applied to all messages replies received
 * on the given channel. Several call to this function will make that
 * the frame received handler set, will be the value set on the last
 * call to this function.
 * 
 * This means that, if several request are performed, followed by its
 * corresponding call to this function, and knowing that several
 * request on the same channel are replied in the same other they were
 * issued, the frame received handler that will process the first
 * message will be the last one set not the one set for that first
 * message.
 *
 * Here is an example of code on how to produce a race condition:
 * \code
 * // set the frame received for the request A
 * vortex_channel_set_received_handler (channel, process_requestA, data_A);
 *
 * // perform request A
 * vortex_channel_send_msg (channel, "request A", 9, NULL);
 *
 *
 * // set the frame received for the request B
 * vortex_channel_set_received_handler (channel, process_requestB, data_B);
 *
 * // perform request B
 * vortex_channel_send_msg (channel, "request B", 9, NULL);
 * \endcode
 *
 * Obviously, this seems to be pretty clear if you place the problem
 * at a very few lines. However, previous interactions are usually
 * produced by a function called, in your code that is likely to be
 * named as <b>make_invocation</b>, which calls to \ref
 * vortex_channel_set_received_handler and then to \ref
 * vortex_channel_send_msg, which starts to be not so obvious.
 *
 * The key concept here is to ensure that every message reply to be
 * received must be processed by the right frame receive handler. This
 * could be accomplish using several techniques:
 *
 * <ul>
 * <li>If you are using a request/response pattern, you have to know
 * that several request/response patterns could not be under the same
 * channel without having blocking problems usually called
 * <b>head-of-line</b>. 
 * 
 * You are likely to be interested in using a different channel to
 * perform the request/response invocation or to wait to use an
 * already existing channel that is ready to be used without blocking
 * you. 
 * 
 * This <b>ready</b> feature is interpreted in our context as a
 * channel that is not waiting for any previous request performed,
 * that will make our new request to get blocked until the previous is
 * served.
 * 
 * To get current status for a channel to be <b>ready</b> you could
 * use \ref vortex_channel_is_ready function. In the case a channel is
 * <b>not ready</b>, that is, is already waiting for a reply, you
 * <b>should NOT</b> modify current frame receive handler because
 * you'll make your new frame receive handler to process previous
 * reply. 
 *
 * In the case a channel is not ready you can wait until the channel
 * is ready, or to create a new channel to perform the invocation. For
 * the second option, you'll be interested on using the \ref
 * vortex_channel_pool "Vortex Channel Pool" module and its features
 * to get the next ready channel automatically negotiating a new
 * channel if there is no ready channel available.
 *
 * </li>
 *
 * <li>
 *
 * The alternative is to create a channel and close it for each
 * request/response performed. This will allow to have a clear
 * implementation, with a particular frame receive handler for each
 * channel. The question is that this have a really great performance
 * impact. Using this technique your program will do for every request:
 *
 *  - Create a channel (sending a channel request and make your program wait for its reply).
 *  - Perform the request/response pattern (sending the request and waiting for the response).
 *  - Close the channel (sending a close channel request and make your program wait for its reply).
 *
 * As a consequence you have to exchange 6 messages for every request
 * done. For 100 request/response your program will have to exchange
 * <b>600 messages. </b>
 *
 * Using the \ref vortex_channel_pool "Vortex Channel Pool", with 10
 * channels, your program will exchange 20 messages (for channels to
 * be created) plus 20 messages (for channels to be closed) plus 200
 * exchanges (100 request + 100 responses), total = <b>220
 * messages.</b>
 *
 * </li>
 *
 * </ul>
 *
 * At any case it is recommended, while using the request/response
 * pattern, to use the \ref vortex_channel_pool "Channel Pool" feature
 * which will allow you to avoid race conditions while getting a
 * channel that is ready but being asked by several threads at the
 * same time. It will also negotiate for you a new channel to be
 * created if the channel pool doesn't have any channel in the ready
 * state.
 * 
 * It is really easy to change current code implemented to use vortex
 * channel pool. Here is an example:
 * 
 * \code
 * VortexChannelPool * pool;
 * void init_channel_pool (VortexConnection * connection) {
 *
 *    // create the channel pool (this should be done for each 
 *    // connection only one time avoiding several threads to call this
 *    // function
 *
 *     pool = vortex_channel_pool_new (connection,
 *                                     COYOTE_SIMPLE_URI,
 *                                     1, // how many channels to be created at first time
 *                                     // close handler stuff
 *                                     NULL, NULL, 
 *                                     // received handler stuff (set it to null)
 *                                     NULL, NULL,
 *                                     // async notification for pool creation stuff
 *                                     NULL, NULL);
 * }
 * 
 * VortexChannel * get_channel_available (VortexOnFrameReceived received,
 *                                        axlPointer            user_data) {
 *
 *      // get the next channel available (this function could be
 *      // called by several threads at the same time
 *
 *      result = vortex_channel_pool_get_next_ready (pool, true);
 * 
 *      // now, Vortex API is ensuring us we are the only one owner for the channel
 *      // result, let's change the frame receive handler
 *      vortex_channel_set_received_handler (result, received, user_data);
 * 
 *      // return the channel
 *      return result;
 * }
 *
 * void release_channel (VortexChannel * channel) {
 *
 *      // once finished release the channel, to return it to the
 *      // pool. The following must be do only, and only once the
 *      // channel have received its reply.
 *
 *      vortex_channel_pool_release_channel (pool, channel);
 * }
 * \endcode
 *
 * Previous source code have tree functions. Here is the
 * explanation:
 *
 * - <b>init_channel_pool:</b> For every connection to be used, this
 * function should be called to create a channel pool. Channel pool
 * created on a connection get closely associated to it. This must be
 * done only one time, commonly at the connection creation process.
 * 
 * - <b>get_channel_available:</b> Instead of create a channel calling
 * normal interface provided (\ref vortex_channel_new and \ref
 * vortex_channel_new_full), a call to this function is done,
 * providing the frame received handler and its associated user space
 * data. This function is usually called just before the request is
 * performed to get the channel to be used. The channel returned is
 * ensured to be not waiting for a previous request, and the the next
 * request to be performed, will be processed with the \ref VortexOnFrameReceived "frame received" handler provided.
 * 
 * - <b>release_channel: </b> Once the reply is received, and only
 * once, the channel must be release to the channel pool so it could
 * be reused for other request.
 *
 * \section vortex_manual_changing_vortex_io Configuring Vortex Library IO layer
 *
 * Default Vortex Library implementation doesn't require you to pay
 * attention to such details like: 
 *
 * - How Vortex Library sends and receives data from underlying
 * transport.
 *
 * - How Vortex Library IO blocking mechanism is used to wait until a set
 * of file descriptors changes.
 *
 * However, it turns out that these are topics that are likely to be
 * asked. For your information, Vortex Library internal function have
 * a default configuration that makes uses of the BSD <b>send</b> and
 * <b>recv</b> function to perform IO read and write operations.  This
 * default configuration could be changed by using the following
 * function which are specific for each connection:
 * 
 *  - \ref vortex_connection_set_receive_handler
 *  - \ref vortex_connection_set_send_handler
 *
 * As an example, Vortex TLS implementation uses previous handlers to
 * configure how Vortex Library reads and sends its data once the TLS
 * negotiation have finished. This allows to keep on writing higher
 * level code that expect to have a function that is able to send and
 * receive data from its underlying transport socket, no matter if it
 * is TLS-ficated (\ref vortex_connection_is_tlsficated).
 *
 * User space only needs to implement a small piece of code inside the
 * handler required and Vortex Library will call it at the right
 * time. Previous function requires to define the following handlers:
 * 
 * - To perform read operations from a \ref VortexConnection a handler
 * \ref VortexReceiveHandler must be provided.
 * 
 * - To perform send operations from a \ref VortexConnection a handler
 *  \ref VortexSendHandler must be provided.
 *
 * You'll find that previous definition doesn't allows to pass in a
 * pointer to the function so you could implement a kind of special
 * operation based on the data received. To accomplish this task, you
 * should use the following set of function which will allow you to
 * store arbitrary data inside a connection, key-index hash like, even
 * allowing the destroy function to be used once the connection is
 * closed or the value is replaced:
 * 
 *   - \ref vortex_connection_set_data
 *   - \ref vortex_connection_set_data_full
 *   - \ref vortex_connection_get_data
 * 
 * For the case of the IO blocking mechanism, Vortex now checks for
 * the presense of select(2), poll(2) and epoll(2) system call,
 * selecting the best mechanism to be used by default. However, you
 * can change to the desired mechanism at run time (even during
 * transmission!) using:
 * 
 * - \ref vortex_io_waiting_use, providing the appropiate value for \ref VortexIoWaitingType.
 *
 * - Use \ref vortex_io_waiting_get_current to know which is the I/O
 * mechanism currently installed.
 *
 * Previous functions, provides a built-in mechanism to select already
 * implemented mechanism. In the case you want to provide your own
 * user space implementation to handling I/O waiting, you can use the
 * following handlers to define the functions to be executed by the
 * Vortex I/O engine at the appropiate time:
 * 
 * <ul>
 *
 * <li>The following handler should be used to set the function that
 * will create the structured commonly used that holds the set of
 * socket descriptors that will be watched:
 *
 *  - \ref vortex_io_waiting_set_create_fd_group
 * </li>
 *
 * <li>Then, the following function should define handlers to perform
 * a clearing operation, a adding socket descriptor into a fd set operation and a destroy
 * descriptor operation. This last one is used to destroy the fd set returned by executing the handler \ref
 * vortex_io_waiting_set_create_fd_group.
 *
 * 
 *  - \ref vortex_io_waiting_set_clear_fd_group
 *  - \ref vortex_io_waiting_set_add_to_fd_group
 *  - \ref vortex_io_waiting_set_destroy_fd_group
 *
 * </li>
 *
 * <li>Once done that, a handler to implement the IO blocking
 * operation until some socket descriptor change must be defined at
 * the following function:
 * 
 *   - \ref vortex_io_waiting_set_wait_on_fd_group
 *
 * </li>
 *
 * <li>Once the previous function returns because there are socket
 * with operations to perform, a handler to get current activation
 * status, for a given socket against the fd set, must be defined.
 * 
 *  - \ref vortex_io_waiting_set_is_set_fd_group
 *
 *  </li>
 *
 * </ul>
 *
 * Previous handlers must be defined as a whole, it is not possible to
 * only define a certain piece reusing the rest of the handlers. If
 * the handlers are properly implemented, they will allow Vortex
 * Library to perform IO operation with the API you have provided.
 *
 * As an example, inside the IO module (<b>vortex_io.c</b>) can be
 * found current implementation for all I/O mechanism supported by the library.
 *
 * \section vortex_manual_securing_your_session Securing a Vortex Connection (or How to use the TLS profile)
 * 
 * As we have said, the main advantage the BEEP protocol have is that
 * is solves many common problems that the network protocol designer
 * will have to face over again. Securing a connection to avoid other
 * parties to access data exchanged by BEEP peers is one of them.
 * 
 * The idea behind the TLS profile is to enable user level
 * applications to activate the TLS profile for a given session and then create
 * channels, exchange data, etc. Inside Vortex Library, the is no
 * difference about using a \ref VortexConnection "connection" that is
 * secured from one that is not.
 * 
 * Common scenario for a Vortex Library client application is:
 * 
 * - 1. Create a connection to a remote peer using \ref
 * vortex_connection_new.
 *
 * - 2. Secure the connection already created using \ref
 * vortex_tls_start_negociation.
 *
 * - 3. Then use the \ref VortexConnection "connection", as normal,
 * creating channels (\ref vortex_channel_new), sending data over them (\ref vortex_channel_send_msg), etc. From the
 * application programmer's point of view there is no difference while
 * using a connection secured from one that is not secured.
 *
 * For the listener side of the TLS profile, we have two possibilities: 
 * 
 * <ol>
 * <li>
 * <b>Use predefined handlers provided by the Vortex to activate
 * listener TLS profile support.</b>
 * 
 * This is a cheap-effort option because the library comes with a test
 * certificate and a test private key that are used in the case that
 * locator handlers for such files are not provided.  
 * 
 * This enables to start developing the whole system and later, in the
 * production environment, create a private key and a certificate and
 * define needed handlers to locate them.
 * 
 * On this case, we could activate listener TLS profile support as follows:
 * \code
 * // activate TLS profile support using defaults
 * vortex_tls_accept_negociation (NULL,  // accept all TLS request received
 *                                NULL,  // use default certificate file
 *                                NULL); // use default private key file
 *
 * \endcode
 * 
 * <b>NOTE:</b> This option is highly not recommended on production
 * environment because the private key and certificate file are public!
 *
 * </li>
 * <li><b>Define all handlers required, especially those providing the certificate file and the private key file:</b>
 *
 * In this case:
 * <ul>
 *
 *  <li>A \ref VortexTlsAcceptQuery handler should be defined to
 *  control how are accepted incoming TLS requests. Here is an example:
 * \code
 * // return true if we agree to accept the TLS negotiation
 * bool     check_and_accept_tls_request (VortexConnection *connection, 
 *                                        char  *serverName)
 * {
 *     // perform some especial operations against the serverName
 *     // value or the received connection, return false to deny the 
 *     // TLS request, or true to allow it.
 *     
 *     return true;  
 * }
 * \endcode
 *  </li>
 *
 *  <li>A \ref VortexTlsCertificateFileLocator handler should be
 *  defined to control which certificate file is to be used for each
 *  connection. Here is an example:
 * \code
 * char * certificate_file_location (VortexConnection * connection, 
 *                                   char             * serverName)
 * {
 *     // perform some especial operation to choose which 
 *     // certificate file to be used, then return it:
 *    
 *     return vortex_support_find_data_file ("myCertificate.cert"); 
 * }
 * \endcode
 * 
 * Please note the use of \ref vortex_support_find_data_file function
 * which allows to write portable source code avoiding
 * native-full-paths. \ref vortex_support_find_data_file "Check out this document to know more about this."
 * 
 *  </li>
 * 
 *  <li>A \ref VortexTlsPrivateKeyFileLocator handler should be
 *  defined to control which private key file is to be used for each
 *  connection. Here is an example:
 * \code
 * char * private_key_file_location (VortexConnection * connection, 
 *                                   char             * serverName)
 * {
 *     // perform some especial operation to choose which 
 *     // private key file to be used, then return it:
 *    
 *     return vortex_support_find_data_file ("myPrivateKey.pem"); 
 * }
 * \endcode
 *  </li>
 *
 * <li>Now use previous handlers to configure how TLS profile is
 * support for the current Vortex Library instance as follows:
 *  
 * \code
 * // activate TLS profile support using defaults
 * vortex_tls_accept_negociation (check_and_accept_tls_request,
 *                                certificate_file_location,
 *                                private_key_file_locatin);
 * \endcode
 *
 * <b> NOTE:</b> If some handler is not provided the default one will
 * be used. Not providing one of the file locators handler (either
 * certificate locator and private key locator) will cause to not work
 * TLS profile.
 * 
 * </li>
 * </ul>
 * </ol>
 *
 * There is also an alternative approach, which provides more control
 * to configure the TLS process. See \ref
 * vortex_tls_accept_negociation for more information, especially \ref
 * vortex_tls_set_ctx_creation and \ref vortex_tls_set_default_ctx_creation.
 * 
 * Now your listener is prepared to receive incoming connections and
 * TLS-fixate them. The only step required to finish the TLS issue is
 * to produce a certificate and a private key to avoid using the
 * default provided by Vortex Library. See next section.
 *
 * 
 * \section vortex_manual_creating_certificates How to create a certificate and a private key to be used by the TLS profile
 *
 * Now we have successfully configured the TLS profile for listener
 * side we need to create a certificate/private key pair. Currently
 * Vortex Library TLS support is built using <b>OpenSSL</b>
 * (http://www.openssl.org). This SSL toolkit comes with some tools to
 * create such files.  Here is an example:
 * 
 * <ol>
 * <li>Use <b>CA.pl</b> utility to create a certificate with a private key as follows:
 * 
 * \code
 *  $ /usr/lib/ssl/misc/CA.pl -newcert
 *  Generating a 1024 bit RSA private key
 *  ............++++++
 *  ....++++++
 *  writing new private key to 'newreq.pem'
 *  Enter PEM pass phrase:
 *  Verifying - Enter PEM pass phrase:
 *  -----
 *  You are about to be asked to enter information that will be incorporated
 *  into your certificate request.
 *  What you are about to enter is what is called a Distinguished Name or a DN.
 *  There are quite a few fields but you can leave some blank
 *  For some fields there will be a default value,
 *  If you enter '.', the field will be left blank.
 *  -----
 *  Country Name (2 letter code) [AU]:ES
 *  State or Province Name (full name) [Some-State]:Spain
 *  Locality Name (eg, city) []:Coslada
 *  Organization Name (eg, company) [Internet Widgits Pty Ltd]:Advanced Software Production Line, S.L.
 *  Organizational Unit Name (eg, section) []:Software Development
 *  Common Name (eg, YOUR name) []:Francis Brosnan Blázquez
 *  Email Address []:francis@aspl.es
 *  Certificate (and private key) is in newreq.pem
 * \endcode
 *
 * Now you have a certificate file at <b>newreq.pem</b> having both items: the certificate
 * and the private key inside it. The content of the file should look like this:
 * \code
 * -----BEGIN RSA PRIVATE KEY-----
 *  Proc-Type: 4,ENCRYPTED
 *  DEK-Info: DES-EDE3-CBC,91635CC2D1C00C1F
 *  
 *  3Ufod8GGhMuGJIliIRDaZ8RPcoYfPayXWDFGQlE4nIOjudi80a+bt7XUl2L8E/2G
 *  DgzZ4YAeeIVJMv4BtlQUGX5dbKT38aUWmwfHkQBv4+xAlfiwzDdOWPS1fIahgoZR
 *  W3dU0i2Xa62ZFVZLrS18c1a8wyUIdmNX9dVV1XsncDDyZ2JQ26wQihvvwiuQYg/c
 *  Dgugs9f5SfvFVetjg3SdgRRyQWqOc+g43sReXRiuWkKPIBR0RCLpN8pivbUQxO7h
 *  TrlAQIH3KG4xcHsYVSVE5J0s9vN2j440M4oCF5NEufLQyNzEdGqEhhsYvEkCLunc
 *  XeUtxekWn6/hTjhO450G/VXWy+o/+UPOuArEBaiR3sjnaL9FvHrUg0xUoSR1TC+O
 *  wbvr0ORHoaTWpfzbKGnyeZHQ7sy7rfxnQxXYyXjPqK80gJ2n3aBLxmfJcD0FNK6/
 *  H0zhbR6/gtJnLaaL3DfHAI3xw5x7IhRQxXXPo2vLHNhJVS/wPHRAtSCub+Tb5BJ/
 *  41IdpiDQVWxUNQ1mp6hvQxhO6ZXJHK86swk7mFd01wIl+ik426uHsmg8SPmS9+ZQ
 *  FyLbQybyBQvUK9K+GIGojrPEpTloR9pFaE+xumExeVb1y38Stw9TeGu8EQsCdu5p
 *  TYQ13e37KrJVB1CuYy+EA0DdlErChhmOKNIrFqUt4hTcZDDEK7UotcAqP0mZzDvP
 *  ZLPeh3vuMQECkzFbbDg8s2RDi+WC7xobh3HksJSJba2H41WYLqQyV1bMBGArvmmX
 *  7Y+xhqYQKFo1WxJ0dLArdlnj2+OTy6m6hYR43GxMj2oXJ/ZO8wiKdA==
 *  -----END RSA PRIVATE KEY-----
 *  -----BEGIN CERTIFICATE-----
 *  MIIENzCCA6CgAwIBAgIJALXMknfgqNogMA0GCSqGSIb3DQEBBAUAMIHEMQswCQYD
 *  VQQGEwJFUzEOMAwGA1UECBMFU3BhaW4xEDAOBgNVBAcTB0Nvc2xhZGExMDAuBgNV
 *  BAoTJ0FkdmFuY2VkIFNvZnR3YXJlIFByb2R1Y3Rpb24gTGluZSwgUy5MLjEeMBwG
 *  A1UECxMVU29mdHdhcmUgRGV2ZWxvcGVtZW50MSEwHwYDVQQDFBhGcmFuY2lzIEJy
 *  b3NuYW4gQmzhenF1ZXoxHjAcBgkqhkiG9w0BCQEWD2ZyYW5jaXNAYXNwbC5lczAe
 *  Fw0wNTEyMDQxODMyMDRaFw0wNjEyMDQxODMyMDRaMIHEMQswCQYDVQQGEwJFUzEO
 *  MAwGA1UECBMFU3BhaW4xEDAOBgNVBAcTB0Nvc2xhZGExMDAuBgNVBAoTJ0FkdmFu
 *  Y2VkIFNvZnR3YXJlIFByb2R1Y3Rpb24gTGluZSwgUy5MLjEeMBwGA1UECxMVU29m
 *  dHdhcmUgRGV2ZWxvcGVtZW50MSEwHwYDVQQDFBhGcmFuY2lzIEJyb3NuYW4gQmzh
 *  enF1ZXoxHjAcBgkqhkiG9w0BCQEWD2ZyYW5jaXNAYXNwbC5lczCBnzANBgkqhkiG
 *  9w0BAQEFAAOBjQAwgYkCgYEA4i4/XJ5us6YJHt1OmKZBlaGXztXXSkuTtsnazSwv
 *  zYgPa8Ctd0KnDGp8WEEcjmsG8vzjJ+0/UmdxL7N2WCycq9qIeutOU/oXKp5u5eDO
 *  UmQ1v/ehqvxAzkvziQPlbR6QBmWcd+MIJjswGmZwX2JLB6/EZmtloDuCsTP1BLWH
 *  OckCAwEAAaOCAS0wggEpMB0GA1UdDgQWBBQCTZrh3SA3Hm59A6bR2iz2Jzz1YTCB
 *  +QYDVR0jBIHxMIHugBQCTZrh3SA3Hm59A6bR2iz2Jzz1YaGByqSBxzCBxDELMAkG
 *  A1UEBhMCRVMxDjAMBgNVBAgTBVNwYWluMRAwDgYDVQQHEwdDb3NsYWRhMTAwLgYD
 *  VQQKEydBZHZhbmNlZCBTb2Z0d2FyZSBQcm9kdWN0aW9uIExpbmUsIFMuTC4xHjAc
 *  BgNVBAsTFVNvZnR3YXJlIERldmVsb3BlbWVudDEhMB8GA1UEAxQYRnJhbmNpcyBC
 *  cm9zbmFuIEJs4XpxdWV6MR4wHAYJKoZIhvcNAQkBFg9mcmFuY2lzQGFzcGwuZXOC
 *  CQC1zJJ34KjaIDAMBgNVHRMEBTADAQH/MA0GCSqGSIb3DQEBBAUAA4GBALEEf7Z8
 *  zqJApYw3OyhLZBd1NfIeKOwkyHUIVzvvGnpyNq5T+metNDtu9D4XW8aM9x66glMq
 *  H3bTM6Wq3dGv5Hi5ZrGEjISkEgn6TnndIlHVqyS4D/EDPVj1lOiujSptowmdLieQ
 *  JXRwm/Hmf7mCCJEYAsMR9KfhctrvnwYiVW6a
 *  -----END CERTIFICATE-----
 * \endcode
 *
 * You can split the content of the file generated into two files: the
 * private key and the public certificate part. The private key file
 * is the piece which is enclosed by <b>BEGIN/END-"RSA PRIVATE KEY"</b>. The
 * public certificate is the piece enclosed by <b>BEGIN/END-"CERTIFICATE"</b>. 
 * 
 * Splitting the certificate produced into two files is not required
 * beucase openssl lookup for the appropriate part while providing the
 * same file for the certificate or the private key.
 * </li>
 * 
 * <li>Now it is required to remove the pass phrase because Vortex
 * Library already doesn't support providing a callback to configure
 * it. This is done by doing the following:
 * \code
 * openssl rsa -in newreq.pem -out private-key-file.pem
 * Enter pass phrase for newreq.pem:
 * writing RSA key
 * \endcode
 * 
 * As a result for the previous operation, the resulting file should look like:
 * \code
 * $ less private-key-file.pem 
 * -----BEGIN RSA PRIVATE KEY-----
 * MIICXgIBAAKBgQDiLj9cnm6zpgke3U6YpkGVoZfO1ddKS5O2ydrNLC/NiA9rwK13
 * QqcManxYQRyOawby/OMn7T9SZ3Evs3ZYLJyr2oh6605T+hcqnm7l4M5SZDW/96Gq
 * /EDOS/OJA+VtHpAGZZx34wgmOzAaZnBfYksHr8Rma2WgO4KxM/UEtYc5yQIDAQAB
 * AoGAPSl4ZNlK4jWR3dDGgizjK01JOdtFnoeVaCZpjnXWb2PNl7vArLFPbuIUweDJ
 * khGLDYYo/xD+wI/MYbPL2sgljSj7LzMd1bcO70vzbcoZGug+a1Clc8j3xbz75lGZ
 * +IW0QhkQr7T7iDCj6Ay+1AdAknxO0h/7/yq0ShWHLvEK+4kCQQD9CgIA3NkQ/AMk
 * v20ChILgz/Ne86Aokx9FtoE25l9e+sDwpoPL+8uxBvM2pWDAd8GoW+a1GWDsVe6H
 * /PWKyhx/AkEA5NPIpk3f9QdNG2ef9tUbVOweQT7kzPtydWoyVcKro/PN6stbhfhu
 * Wy7kcJxiA8jn1S1pSAU/EWoc3vi3idGltwJBAJMH+qwHp/XPigATX0NEPkxlaRP2
 * WkzZWCWI68I70JT+/ZeYGiMwN2axFCffpr2PmK68X+1BRuls8UKBgSfZUv8CQQCX
 * AOs4U8um9tp7aza0vIz8zZRpmgeC/avar+nnjj+WQh1xBCGxlu+8XIWDiq9jsADN
 * PNptHIkyBMRon9j+qcqhAkEAtDD9wo0gJETs4qzauQ+UCtAyzY65ZHSyJVf0bC1v
 * +4GxygDp0nEgM16lFqw1zMdFvmTjPuZrtViCI2WPWtB2CA==
 * -----END RSA PRIVATE KEY-----
 * \endcode
 * </li>
 *
 * <li>Now, you can remove the private key part from the initial
 * certificate generated and put it into a file to store the private
 * key with pass phrase. However, this is not required because the
 * openssl provides a way to configure the pass phrase to private key
 * without it.
 * </li>
 *
 *</ol>
 *   
 * \section vortex_manual_using_sasl Authenticating BEEP peers (or How to use SASL profiles family)
 * 
 * While using or designing network protocols, a common issue to solve
 * and support is to identify and authenticate users on top of
 * it. This security issue is supported inside BEEP through SASL
 * profiles.
 * 
 * SASL (RFC2222) is a security framework which defines how security
 * protocols are implemented so the same program structure could take
 * advantage no matter which security protocol is being used. 
 * 
 * SASL (Simple Authentication and Security Layer) provides not only a
 * way to identify users but also to secure connections. At this
 * moment, SASL implementation inside Vortex Library only provides
 * authentication (that is the authentication part of SASL). 
 * 
 * This is not a big problem knowing that TLS profile could be
 * enabled, making the connection secure (providing the security layer)
 * and then negotiate user identification (and its
 * authorization) using SASL.
 *
 * Again, the idea behind this is to design your application protocol
 * without taking into account details such as: how to ensure that the
 * protocol session is secure and who is the user at the other side of
 * the peer using the protocol.
 * 
 * Currently these are the SASL profiles implemented and tested inside
 * Vortex Library:
 * <ul>
 *
 * <li>
 * \ref VORTEX_SASL_ANONYMOUS "ANONYMOUS" (\ref VORTEX_SASL_ANONYMOUS): provides a way to perform
 * anonymous login providing an anonymous token to track the
 * session. This profile does not provide authentication. However,
 * server side authentication could also implement anonymous
 * acceptance according to some selected anonymous logins that are
 * recognized. This SASL profile works like anonymous FTP login does.
 * </li>
 *
 * <li>\ref VORTEX_SASL_PLAIN "PLAIN" (\ref VORTEX_SASL_PLAIN): provides support to perform login
 * into a BEEP server providing a user and a password. Optionally,
 * this SASL profile also support to provide an authorization
 * identification. This is used to perform a login request as user
 * <b>bob</b> but requesting to act as <b>alice</b>. 
 * 
 * This SASL profile works as people usually expect: client side
 * provides a user identification (for example: <b>bob</b>) and a
 * password (for example: <b>secret</b>). Then, server side receive
 * these values and decide to allow or to deny SASL authentication
 * according to the internal user database state.
 * 
 * This SASL profile exchange user and password data in clear
 * text. This is insecure because a third party could intercept your
 * TCP session and read sensible information. To use this profile it
 * is recommended to activate \ref vortex_manual_securing_your_session "the TLS profile" 
 * or any other external mechanism such as IPSec.
 *
 * Advantages of this SASL profile are:
 *
 * <ul>
 * <li>It is faster than its brothers-in-features: \ref
 * VORTEX_SASL_CRAM_MD5 "CRAM-MD5" and \ref VORTEX_SASL_DIGEST_MD5
 * "DIGEST-MD5" and ..</li>
 *  
 *
 * <li>It is the only way to support systems where user/password pair
 * is not stored in clear-text. This is because this is the only one
 * SASL profile which actually sends the user/password pair in clear
 * text, allowing to the server side to apply them the corresponding
 * cryptographic method and compare it with the ciphered stored
 * value. 
 *
 * Database oriented applications usually store passwords using MD5 or
 * other hashing method, making necessary to have the clear text
 * version for the password (and maybe, but not usually, the
 * user). 
 * 
 * </li> </ul>
 *
 * <li>
 *
 * \ref VORTEX_SASL_CRAM_MD5 "CRAM-MD5" (\ref VORTEX_SASL_CRAM_MD5):
 * From the programmer's point of view, this SASL profile is
 * implemented in the same way that \ref VORTEX_SASL_PLAIN "PLAIN" but
 * underlying SASL negotiation is more secure. This is because
 * CRAM-MD5 handshake definition does not require to transfer clear
 * user and password but a hashed version of it. 
 * 
 * Server side generates a hash token, that is used only for the given
 * SASL negotiation, and sends it the client. Then, client side use
 * this hash to create a new hash token as a result to apply it to the
 * users password. Then, this result is sent to the server. 
 * 
 * At this point, an malicious third party only could read a hash
 * version that only have meaning for the server side. Then, server
 * side requires from the application level to provide the password
 * for the user selected hashing it with the initially created hash,
 * comparing the result with received hash from the client.
 *
 * If the previous two hashes match, then the SASL authentication is
 * done. This is roughly how CRAM-MD5 works.
 *
 * This SASL profile is preferred over PLAIN because it is more secure
 * and can be used over non-secured connections. However, it is slow
 * than PLAIN because requires computational operations such as
 * hashing. This could impact at the number of successful logins that could
 * provide the system designed.
 *
 * Another question to take into account is that this SASL profile
 * does not provide a way to provide a user identification and act as
 * another user like PLAIN does. 
 *
 * Because this SASL profile is more secure than PLAIN, it could be
 * used without having negotiated an underlying security layer.
 *
 * One disadvantage this profile have (and \ref VORTEX_SASL_DIGEST_MD5
 * "DIGEST-MD5") is that server application must have access to clear
 * text passwords. This is because it is needed to produce a hashed
 * version (including the user) to compare it to the hash received
 * from the client side. 
 * 
 * Obviously, this is not possible in many cases, starting from those
 * applications based on SQL (or any other back end) which stores
 * passwords using MD5 (or any other password cipher method). In this
 * case, application designers could use PLAIN profile on top of \ref
 * vortex_manual_securing_your_session "TLS profile".
 *
 * <i><b>NOTE: </b> It seems that this SASL profile is deprecated in
 * favor of DIGEST-MD5. The only reason that I've found is that
 * DIGEST-MD5 provides the same features like CRAM-MD5 but with a
 * stronger cipher. However, I found <b>DIGEST-MD5</b> 2 times
 * slower, and requiring some SASL properties such as the realm, that
 * doesn't fit into the common network application design.</i>
 *
 * </li>
 * <li>
 *
 * \ref VORTEX_SASL_DIGEST_MD5 "DIGEST-MD5" (\ref
 * VORTEX_SASL_DIGEST_MD5): This SASL profile have some interesting
 * features over the previous one. As starting point, the profile
 * implements the same hash-exchange concept like CRAM-MD5 does for
 * authenticating users. However, the cipher used is stronger and it
 * also support login as a user and request acting as another.
 *
 * Additionally, this SASL profile provides not only authentication
 * but also security layer. However, it is not supported mainly
 * because SASL implementation used does not support it and because
 * security layer is already provided by TLS profile. 
 *
 * This SASL profile not only requires to provide a user and a
 * password, but also to provide the realm where the selected user
 * will be authenticated. 
 * </li>
 * 
 * <li>
 * \ref VORTEX_SASL_EXTERNAL "EXTERNAL" (\ref VORTEX_SASL_EXTERNAL):
 * This profile is provided to support changing SASL state to
 * authenticated for those environments where underlying session
 * already ensures authentication.
 * </li>
 * </ul>
 *
 * \section vortex_manual_sasl_for_client_side How to use SASL at the client side
 * Now you have an overview for SASL profiles supported here is how to
 * use them. 
 *
 * Vortex Library SASL implementation uses for the client side (the
 * BEEP peer that wants to be authenticated) \ref
 * vortex_sasl_set_propertie and then \ref vortex_sasl_start_auth to
 * begin SASL negotiation. 
 * 
 * With \ref vortex_sasl_set_propertie you set the values required by
 * the process such as: the user and its password. Once all values
 * required are set a call to \ref vortex_sasl_start_auth is done to
 * activate SASL layer. 
 * 
 * Every SASL profile requires different properties to be set. Some of
 * them are optional and some of them are common to all profiles. 
 * 
 * Let's start with an example on how to CRAM-MD5 profile to
 * authenticate a client peer:
 * 
 * \code
 * VortexStatus    status;
 * char          * status_message;
 * 
 * // STEP 1: check if SASL is activated for the given Vortex Library
 * if (! vortex_sasl_is_enabled ()) {
 *      // unable to activate SASL profile. This only happens when
 *      // Vortex Library wasn't built with SASL support
 *      printf ("Unable to initialize SASL profiles.\n");
 * }
 * 
 * // STEP 2: set required properties according to SASL profile selected
 * // set user to authenticate
 * vortex_sasl_set_propertie (connection, VORTEX_SASL_AUTH_ID, "bob", NULL);
 *
 * // set the password
 * vortex_sasl_set_propertie (connection, VORTEX_SASL_PASSWORD, "secret", NULL);
 * 
 * // STEP 3: begin SASL negotiation
 * vortex_sasl_start_auth_sync (// the connection where SASL will take place
 *                              connection, 
 *                              // SASL profile selected
 *                              VORTEX_SASL_CRAM_MD5,
 *                              // SASL status variables
 *                              &status, &status_message);
 *
 * // STEP 4: once finished, check of authentication
 * if (vortex_sasl_is_authenticated (connection)) {
 *      printf ("SASL negotiation OK, user %s is authenticated\n",
 *               vortex_sasl_get_propertie (connection, VORTEX_SASL_AUTH_ID));
 * }else {
 *      printf ("SASL negotiation have failed: status=%d, message=%s\n",
 *               status, message);
 * }
 * // roughly, that's all for the client side
 * \endcode
 *
 * Previous code will look the same for all SASL profile selected. The
 * only part that will change will be properties provided (that are
 * required by the SASL profile <b>STEP 2</b>). 
 * 
 * On <b>step 3</b> \ref vortex_sasl_start_auth_sync is used to perform
 * SASL negotiation. This function is the blocking version for \ref
 * vortex_sasl_start_auth. Synchronous version is only recommended for
 * bath process because the caller get blocked until SASL profile
 * finish. However it is easy to explain SASL function inside Vortex
 * using synchronous version.
 * 
 * Asynchronous version (through \ref vortex_sasl_start_auth) is
 * preferred because allows to perform other tasks (like updating GUI
 * interfaces) while SASL negotiation is running.
 * 
 * Here is a table with properties that are required (or are optional) for
 * each SASL profile used.
 * 
 * <table>
 *  <tr><td></td><td><b>ANONYMOUS</b></td><td><b>PLAIN</b></td><td><b>CRAM-MD5</b></td><td><b>DIGEST-MD5</b></td><td><b>EXTERNAL</b></td></tr>
 *  <tr><td><b>VORTEX_SASL_AUTH_ID</b></td><td></td><td>required</td><td>required</td><td>required</td><td></td></tr>
 *  <tr><td><b>VORTEX_SASL_AUTHORIZATION_ID</b></td><td></td><td>optional</td><td></td><td>optional</td><td>required</td></tr>
 *  <tr><td><b>VORTEX_SASL_PASSWORD</b></td><td></td><td>required</td><td>required</td><td>required</td><td></td></tr>
 *  <tr><td><b>VORTEX_SASL_REALM</b></td><td></td><td></td><td></td><td></td>required<td></td></tr>
 *  <tr><td><b>VORTEX_SASL_ANONYMOUS_TOKEN</b></td><td>required</td><td></td><td></td><td></td><td></td></tr>
 * </table>
 *
 * \section vortex_manual_sasl_for_server_side How to use SASL at the server side
 *
 * Well, as we have seeing in the previous section, SASL at the client
 * side is entirely driven by properties (through \ref
 * vortex_sasl_set_propertie). However at the server is SASL is
 * entirely driven by call backs. 
 *
 * There is one callback for each SASL profile supported inside Vortex
 * Library. They allow to your server application to connect SASL
 * authentication process with legacy user/password databases.
 * 
 * Vortex Library supports, through SASL, transporting and
 * authenticating users but, at the end, the programmer must provide
 * the final decision to allow or to deny SASL authentication request.
 * 
 * Here is an example on how to activate PLAIN support and validate
 * request received for this profile:
 * \code
 * [...] at some part of your program (likely to be at the main)
 * // check for SASL support
 * if (!vortex_sasl_is_enabled ()) {
 *    // drop a log about Vortex Library not supporting SASL
 *    return -1;
 * }
 * 
 * // set default plain validation handler
 * vortex_sasl_set_plain_validation (sasl_plain_validation);
 * 
 * // accept SASL PLAIN incoming requests
 * if (! vortex_sasl_accept_negociation (VORTEX_SASL_PLAIN)) {
 *	printf ("Unable  accept SASL PLAIN profile");
 *	return -1;
 * }
 * [...]
 * // validation handler for SASL PLAIN profile
 * bool     sasl_plain_validation  (VortexConnection * connection,
 *                                  const char       * auth_id,
 *                                  const char       * authorization_id,
 *                                  const char       * password)
 * {
 *
 *      // At this point your server application should connect
 *      // to its internal user/password database to validate
 *      // incoming request. 
 * 
 *      // In this case we perform a validation based on receiving
 *      // a pair based on bob/secret allowing it, and denying
 *      // the rest of user/password pairs.
 *
 *	if (axl_cmp (auth_id, "bob") && 
 *	    axl_cmp (password, "secret")) {
 *              // notify Vortex that the given SASL request
 *              // have been accepted by the application level.
 *		return true;
 *	}
 *	// deny SASL request to authenticate remote peer
 *	return false;
 * }
 * \endcode
 *
 * Previous example show the way to activate the SASL authentication
 * support. First it is configured a handler to manage the authentication
 * request, according to the SASL method desired. In this case the
 * following function is used:
 * 
 * - \ref vortex_sasl_set_plain_validation
 *
 * Keep in mind that previous function doesn't allows to configure an
 * user data pointer to be passed to the handler. If you require this
 * you must use the extended API, which is called the same but
 * appending "_full". In this case, the function to be used is:
 * 
 * - \ref vortex_sasl_set_plain_validation_full
 *
 * For every SASL mechanism supported by Vortex Library there are two
 * functions to configure the validation handler and the extended
 * validation handler. 
 *
 * Then, a call to register and activate the profile \ref
 * VORTEX_SASL_PLAIN is done. This step will notify vortex profiles
 * module that the selected SASL profile must be anonnounced as
 * supported, at the connetion greetings, configuring all internal
 * SASL handlers. In this case, the example is not providing an user
 * data to the \ref vortex_sasl_accept_negociation function. In the
 * case that a user data is required, the following function must be
 * used:
 *
 * - \ref vortex_sasl_accept_negociation_full
 *
 * Now the SASL PLAIN profile is fully activated and waiting for
 * requests. Validating the rest of SASL profiles works the same
 * way. Some of them requires to return <b>true</b> or <b>false</b> to
 * <b>allow</b> or to <b>deny</b> received request, and other profiles
 * requires to return the password for a given user or NULL to deny
 * it.
 *
 * Here is a table for each profile listing the profile activation
 * function and its handler.
 * 
 * <table>
 *  <tr><td></td><td><b>Validation handler setting</b></td><td><b>Validation handler</b></td></tr>
 *  <tr><td><b>ANONYMOUS</b></td><td>\ref vortex_sasl_set_anonymous_validation</td><td>\ref VortexSaslAuthAnonymous</td></tr>
 *  <tr><td><b>ANONYMOUS</b></td><td>\ref vortex_sasl_set_anonymous_validation_full</td><td>\ref VortexSaslAuthAnonymousFull</td></tr>
 *  <tr><td><b>PLAIN</b></td><td>\ref vortex_sasl_set_plain_validation</td><td>\ref VortexSaslAuthPlain</td></tr>
 *  <tr><td><b>PLAIN</b></td><td>\ref vortex_sasl_set_plain_validation_full</td><td>\ref VortexSaslAuthPlainFull</td></tr>
 *  <tr><td><b>CRAM-MD5</b></td><td>\ref vortex_sasl_set_cram_md5_validation</td><td>\ref VortexSaslAuthCramMd5</td></tr>
 *  <tr><td><b>CRAM-MD5</b></td><td>\ref vortex_sasl_set_cram_md5_validation_full</td><td>\ref VortexSaslAuthCramMd5Full</td></tr>
 *  <tr><td><b>DIGEST-MD5</b></td><td>\ref vortex_sasl_set_digest_md5_validation</td><td>\ref VortexSaslAuthDigestMd5</td></tr>
 *  <tr><td><b>DIGEST-MD5</b></td><td>\ref vortex_sasl_set_digest_md5_validation_full</td><td>\ref VortexSaslAuthDigestMd5Full</td></tr>
 *  <tr><td><b>EXTERNAL</b></td></td><td>\ref vortex_sasl_set_external_validation</td><td>\ref VortexSaslAuthExternal</td></tr>
 *  <tr><td><b>EXTERNAL</b></td></td><td>\ref vortex_sasl_set_external_validation_full</td><td>\ref VortexSaslAuthExternalFull</td></tr>
 * </table>
 *
 * Once the validation is done. You can use the following two
 * functions to check authentication status for the connection in the
 * future, commonly, at the frame receive handler. This allows you to
 * authenticate, and then, at the frame receive handler check the
 * authentication status (or at any other place of course).
 *
 *  - \ref vortex_sasl_is_authenticated
 *  - \ref vortex_sasl_auth_method_used
 *
 * The first function is really important, and should be used before
 * any further check. This ensures you that you are managing an
 * authenticated connection, and then you can call to the next
 * function, \ref vortex_sasl_auth_method_used, to get the auth method
 * that was used, and finally call to the following function to get
 * the appropiate auth data: 
 * 
 *  - \ref vortex_sasl_get_propertie
 *
 * Here is an example about checking the auth status, and getting auth
 * properties, at a frame receive handler:
 * \code
 * // drop a log about the sasl properties 
 * if (vortex_sasl_is_authenticated (connection)) {
 *     // check the connection to be authenticated before assuming anything 
 *     printf ("The connection is authenticated, using the method: %s\n", 
 *              vortex_sasl_auth_method_used (connection));
 *
 *     printf ("Auth data provided: \n  authid=%s\n  authorization id=%s\n  password=%s\n  realm=%s\n  anonymous token=%s\n",
 *              vortex_sasl_get_propertie (connection, VORTEX_SASL_AUTH_ID),
 *              vortex_sasl_get_propertie (connection, VORTEX_SASL_AUTHORIZATION_ID),
 *              vortex_sasl_get_propertie (connection, VORTEX_SASL_PASSWORD),
 *              vortex_sasl_get_propertie (connection, VORTEX_SASL_REALM),
 *              vortex_sasl_get_propertie (connection, VORTEX_SASL_ANONYMOUS_TOKEN));
 * } else {
 *     printf ("Connection not authenticated..\n");
 *     // at this point DON'T RELAY ON data returned by
 *     // - vortex_sasl_auth_method_used
 *     // - vortex_sasl_get_propertie 
 * }
 * \endcode
 * 
 */

/** \page profile_example Implementing a profile tutorial (C API)
 * 
 * \section profile_example_intro Introduction
 *
 * We are going to create a simple server which implements a simple
 * profile defined as: <i>every message it receives, is replied with the payload
 * received appending "Received OK: ".</i>
 *
 * Additionally, we are going to create a simple client which
 * tries to connect to the server and then send a message to him
 * printing to the console the server's reply.
 *
 * The profile implemented is called
 * <i>"http://fact.aspl.es/profiles/plain_profile"</i>. While
 * implementing a profile you must chose a name based on a uri
 * style. You can't chose already defined profiles name so it is a
 * good idea to chose your profile name appending as prefix your
 * project name. An example of this can be:
 * <i>"http://your.project.org/profiles/profile_name"</i>.
 *
 * See how the <i>plain_profile</i> is implemented to get an idea on
 * how more complex, and useful profiles could be implemented.
 *
 * \section profile_example_server Implementing the server.
 *
 * First, we have to create the source code for the server:
 *
 * \include vortex-simple-listener.c
 * 
 * As you can see, the server code is fairly easy to understand. The following steps are done:
 * <ul>
 *  <li>First of all, Vortex Library is initialized using \ref vortex_init.</li>
 *
 *  <li>Then, <i>PLAIN_PROFILE</i> is registered inside Vortex Library
 *  using \ref vortex_profiles_register.  This means, the vortex
 *  listener we are building will recognize peer wanting to create new
 *  channels based on <i>PLAIN_PROFILE</i>.
 *
 *  <i><b>NOTE:</b> on connection startup every BEEP listener must
 *  report what profiles support. This allows BEEP initiators to
 *  figure out if the profile requested will be supported. Inside the
 *  Vortex Library implementation the registered profiles using \ref
 *  vortex_profiles_register will be used to create the supported
 *  profiles list sent to BEEP initiators.  
 *  
 *  The other interesting question the BEEP Core definition have is
 *  that BEEP initiators, the one which is actually connecting to a
 *  listener doesn't need to report its supported profiles. As a
 *  consequence, you can create a Vortex Client connecting to a remote
 *  server without registering a profile. 
 *
 *  Obviously, this doesn't means you are not required to implement
 *  the profile. A profile is always needed by definition by both
 *  peers to know the semantic under which the messages exchange will
 *  take place.</i>
 *
 *  While registering a profile, Vortex Library will allow you to
 *  register several call backs to be called on event such us channel
 *  start, channel close and frame receive during the channel's
 *  life. This event will be called (actually registered handlers)
 *  only for those channels working under the semantic of PLAIN
 *  PROFILE.
 *
 *  As a conclusion, you can have several profiles implemented, having
 *  several channels opened on the same connection <i>"running"</i>
 *  different profiles at the same time.
 *
 *  Additionally, you may require to know current connection
 *  role. This is done by using \ref vortex_connection_get_role
 *  function.  </li>
 *  
 *  <li>Next, a call to \ref vortex_listener_new creates a server
 * listener prepared to receive connection to any name the host may
 * have listening on port 44000. In fact, you can actually perform several
 * calls to \ref vortex_listener_new to listen several port at the
 * same time.</li>
 *
 *  <li>Before previous call, it is needed to call \ref vortex_listener_wait
 *  to block the main thread until Vortex Library is finished.</li>
 * 
 *  </ul> 
 *  
 *  That's all, you have created a Vortex Server supporting a
 *  user defined profile which replies to all message received
 *  appending "Received OK: " to them. To compile it you can \ref install "check out this section".
 *
 * \section profile_example_testing_server Testing the server created using the vortex-client tool and telnet command.
 *
 *  Now, you can run the server and test it using a telnet tool to
 *  check some interesting things. The output you should get should be
 *  somewhat similar to the following:
 *
 * \code
 * (jobs:1)[acinom@barbol test]
 * $ ./vortex-simple-listener &
 * [2] 7397
 * 
 * (jobs:2)[acinom@barbol test]
 * $ telnet localhost 44000
 * Trying 127.0.0.1...
 * Connected to localhost.
 * Escape character is '^]'.
 * RPY 0 0 . 0 128
 * Content-Type: application/BEEP+xml
 * 
 * <greeting>
 *    <profile uri='http://fact.aspl.es/profiles/plain_profile' />
 * </greeting>
 * END
 * \endcode
 *
 * As you can see, the server replies immediately its availability
 * reporting the profiles it actually support. Inside the greeting
 * element we can observe the server support the <i>PLAIN_PROFILE</i>.
 *
 * Before starting to implement the vortex client, we can use
 * <b>vortex-client</b> tool to check our new server. Launch the
 * <b>vortex-client</b> tool and perform the following operations.
 *
 * <ul>
 *
 * <li> First, connect to the server located at localhost, port 44000
 * using <b>vortex-client</b> tool and once connected, show supported
 * profiles by the remote host. 
 *
 * \code
 * (jobs:0)[acinom@barbol libvortex]
 * $ vortex-client
 * Vortex-client v.0.8.3.b1498.g1498: a cmd tool to test vortex (and BEEP-enabled) peers
 * Copyright (c) 2005 Advanced Software Production Line, S.L.
 * [=|=] vortex-client > connect
 * server to connect to: localhost
 * port to connect to: 44000
 * connecting to localhost:44000..ok: vortex message: session established and ready
 * [===] vortex-client > connection status
 * Created channel over this session:
 *  channel: 0, profile: not applicable
 * [===] vortex-client > show profiles
 * Supported remote peer profiles:
 *  http://fact.aspl.es/profiles/plain_profile
 * \endcode
 *
 * As you can observe, <b>vortex-client</b> tool is showing we are
 * already connected to remote peer and the connection created already
 * have a channel created with number 0. This channel number is the
 * BEEP administrative channel and every connection have it. This
 * channel is used to perform especial operations such as create new
 * channels, close them and channel tuning.
 * 
 * </li>
 * 
 * <li>Now, create a new channel choosing the plain profile as follows:
 * 
 * \code
 * [===] vortex-client > new channel
 * This procedure will request to create a new channel using Vortex API.
 * Select what profile to use to create for the new channel?
 * profiles for this peer: 1
 *  1) http://fact.aspl.es/profiles/plain_profile
 *  0) cancel process
 * you chose: 1
 * What channel number to create: you chose: 4
 * creating new channel..ok, channel created: 4
 * \endcode
 * </li> 
 *
 * <li>Now, send a test message and check if the server reply is
 * following the implementation, that is, the message should have
 * "Received OK: " preceding the text sent. Notify to vortex-client
 * you want to wait for the reply.
 *
 * \code
 * [===] vortex-client > send message
 * This procedure will send a message using the vortex API.
 * What channel do you want to use to send the message? you chose: 4
 * Type the message to send:
 * This is a test, reply my message
 * Do you want to wait for message reply? (Y/n) y
 * Message number 0 was sent..
 * waiting for reply...reply received: 
 * Received Ok: This is a test, reply my message
 * \endcode
 * </li> 
 *
 * <li>Now, check connection status and channel status to get more data about them. This will be
 * useful for you in the future is you want to debug BEEP peers.
 * \code
 * [===] vortex-client > connection status
 * Created channel over this session:
 *  channel: 0, profile: not applicable
 *  channel: 4, profile: http://fact.aspl.es/profiles/plain_profile
 * [===] vortex-client > channel status
 * Channel number to get status: 4
 * Channel 4 status is: 
 *  Profile definition: 
 *     http://fact.aspl.es/profiles/plain_profile
 *  Synchronization: 
 *     Last msqno sent:          0
 *     Next msqno to use:        1
 *     Last msgno received:      no message received yet
 *     Next reply to sent:       0
 *     Next reply exp. to recv:  1
 *     Next exp. msgno to recv:  0
 *     Next seqno to sent:       32
 *     Next seqno to receive:    45
 * [===] vortex-client > channel status
 * Channel number to get status: 0
 * Channel 0 status is: 
 *  Profile definition: 
 *     not applicable
 *  Synchronization: 
 *     Last msqno sent:          0
 *     Next msqno to use:        1
 *     Last msgno received:      no message received yet
 *     Next reply to sent:       0
 *     Next reply exp. to recv:  1
 *     Next exp. msgno to recv:  0
 *     Next seqno to sent:       185
 *     Next seqno to receive:    228
 * \endcode
 *</li>
 *
 *<li> Close the channel created as well as the connection.
 * \code
 * [===] vortex-client > close channel
 * closing the channel..
 * This procedure will close a channel using the vortex API.
 * What channel number to close: you chose: 4
 * Channel close: ok
 * [===] vortex-client > close
 * [=|=] vortex-client > quit
 * \endcode
 *</li>
 * </ul>
 * \section profile_example_implement_client Implementing a client for our server
 *
 * Now we have implemented our server supporting the <i>PLAIN
 * PROFILE</i>, we need some code to enable us sending data. 
 *
 * The following is the client implementation which connects, creates
 * a new channel and send a message:
 *
 * \include vortex-simple-client.c
 *
 * As you can observe the client is somewhat more complicated than the
 * server because it has to create not only the connection but also
 * the channel, sending the message and use the wait reply method to
 * read remote server reply.
 *
 * Due to the test nature, we have used wait reply method so the test
 * code gets linear in the sense <i>"I send the message and I get blocked
 * until the reply is received"</i> but this is not the preferred
 * method. 
 * 
 * The Vortex Library preferred method is to install a frame receive
 * handler and receive data replies or new message in an asynchronous
 * way. But, doing this on this example maybe will produce to increase
 * the complexity. If you want to know more about receiving data using
 * other methods, check this \ref vortex_manual_dispatch_schema "section" to know more
 * about how can data is received.
 *
 *
 * \section profile_example_conclusion Conclusion
 *
 * We have seen how to create not only a profile but also a simple
 * Vortex Server and a Vortex Client. 
 *
 * We have also seen how we can use <b>vortex-client</b> tool to test
 * and perform operations against BEEP enabled peers in general and
 * against Vortex Library peers in particular.
 *
 * We have also talked about the administrative channel: the channel
 * 0. This channel is present on every connection established and it
 * is used for especial operations about channel management. 
 * 
 * In fact, the channel 0 is running under the definition of a profile
 * defined at the RFC 3080 called <i>Channel Management
 * Profile</i>. This is another example on how profiles are
 * implemented: they only are definitions that must be implemented in
 * order BEEP peers could notify others that they actually support
 * it. In this case, *the <i>Channel Profile Management</i> is
 * mandatory.
 *
 * As another example for the previous point is the <b>Coyote
 * Layer</b> inside the Af-Arch project. Coyote layer implements the
 * profile:
 *
 * \code
 *  http://fact.aspl.es/profiles/coyote_profile
 * \endcode
 * 
 * On Af-Arch project, remote procedure invocation is done through a
 * XML-RPC like message exchange defined and implemented at the Coyote
 * layer (which is not XML-RPC defined at RFC3529).
 * 
 * If upper levels want to send a message to a remote Af-Arch enabled
 * node, they do it through the Coyote layer. Coyote layer takes the
 * message and transform it into a <b>coyote_profile</b> compliant
 * message so remote peer, running also a Coyote layer, can recognize
 * it. 
 * 
 * In other words, the profile is registered using \ref
 * vortex_profiles_register and implemented on top of the Vortex
 * Library.
 *
 * <i><b>NOTE:</b> All code developed on this tutorial can be
 * found inside the Vortex Library repository, directory <b>test</b>.</i>
 */

/**
 * \page programming_with_xml_rpc Vortex XML-RPC programming manual (C API)
 *
 * \section intro Introduction
 * 
 * This manual shows how to use the XML-RPC invocation interface built
 * of top Vortex Library. The implementation is based on the
 * experimental protocol defined at RFC3529. This manual assumes
 * you are already familiar with the Vortex API. If you don't, you can
 * check \ref starting_to_program "this tutorial" as a starting
 * point.
 *
 * On this manual you'll find the following sections:
 * 
 *
 *  <b>Section 1: </b> An introduction to RPC systems
 * 
 *  - \ref xml_rpc_background
 *  - \ref explaining_a_bit_interfaces
 *
 *  <b>Section 2: </b> Using Vortex Library XML-RPC 
 *
 *  - \ref raw_client_invocation
 *  - \ref raw_client_invoke
 *  - \ref raw_client_invoke_considerations
 *  - \ref receiving_an_invocation
 *
 * <b>Section 3: </b> Using Vortex Library <b>xml-rpc-gen</b> tool
 *  
 *  - \ref abstraction_required
 *  - \ref xml_rpc_gen_tool_language
 *  - \ref xml_rpc_gen_tool_language_types
 *  - \ref xml_rpc_gen_tool_language_recursive
 *  - \ref xml_rpc_gen_tool_language_considerations
 *  - \ref xml_rpc_gen_tool_language_array_declaration
 *  - \ref xml_rpc_gen_tool_language_no_parameters
 *  - \ref xml_rpc_gen_tool_language_boolean
 *  - \ref xml_rpc_gen_tool_language_double
 *  - \ref xml_rpc_gen_tool_changing_method_name
 *  - \ref xml_rpc_gen_tool_using_resources
 *  - \ref xml_rpc_gen_tool_enforce_resources
 *  - \ref xml_rpc_gen_tool_including_body_code
 *
 * <b>Section 4: </b> Using the output produced
 *
 *  - \ref xml_rpc_gen_tool_using_output_client
 *  - \ref xml_rpc_gen_tool_using_output_listener
 *
 * <b>Section 5: </b> Additional topics
 *
 * - \ref xml_rpc_authentication_and_security
 * 
 *
 *
 * \section xml_rpc_background Some concepts and background before starting
 * 
 * If you are familiar with RPC environments such CORBA, Web services,
 * Sun RPC or Af-Arch, you can safely skip this section. If you didn't
 * use any kind of RPC software, you should read this section to get
 * valuable information that will help you to not only understand
 * XML-RPC.
 * 
 * All RPC environments are based on a basic concept: to be able to
 * cross the process boundary to execute a method/service/procedure
 * hosted by other process that is reachable through a decoupled
 * link. This decoupled link could be a loopback connection or a
 * network connection to a remote station.
 *
 * In fact, RPC stands for Remote Produce Call. There are
 * several types of RPC environments, but all of them are
 * characterized by its encapsulation format and its invocation
 * model. While the first characterization is based on textual or
 * binary encapsulation format, the second one is based on
 * environments that execute method inside remote objects or
 * services/procedures hosted by a remote process.
 *
 * XML-RPC is service-invocation based, using textual encapsulation
 * format, that is, XML. For the case of CORBA, it is
 * method-invocation based, using binary format (its GIOP and IIOP). 
 *
 * Method-invocation environments requires a reference to the remote
 * object that will receive the method invocation. This is not
 * required for service-invocation environments. They just execute the
 * service/produce exposed by the remote object.
 *
 * All of these environments provide an infrastructure to locate
 * the server that are actually exposing services/methods/procedures to be
 * invoked. Howerver, RPC developers usually fall into providing a
 * host and a port (TCP/IP) to locate the resource, bypassing the
 * location facilities.
 * 
 * The software that provides location services is usually called the
 * binder. For the case of XML-RPC, there is no binder. So, you have
 * to provide all information required to locate your component that
 * will accept to process the service invocation received.
 *
 * Obviously, in most cases, this isn't a problem, because most of system
 * network design is based on a client/server interation, making only
 * necessary to know where is located the server component.
 *
 * Usually, these RPC environments provides two API (or way to use the
 * framework) that could be classified as: Raw invocation interfaces
 * and High level invocation interfaces.
 *
 * As you may guessing, there are more fun (and pain!) while using the
 * Raw invocation interface than the High level one. Both offers
 * (dis)advantanges and both provides an especific functionality, that
 * is required in particular situations.
 *
 * \section explaining_a_bit_interfaces The Raw invocation, The High level invocation and the protocol compiler
 *
 * In general term, the Raw invocation interface provides an API to
 * produce a invocation, defining all the details, while the High
 * level invocation interface is built on top of the Raw interface,
 * and it is usually produced by a protocol compiler tool. This tools
 * is usually called the Interface Definition Language compiler, or
 * just IDL compiler.
 *
 * This protocol compiler reads a service definition description
 * (every RPC platform has its own version about this language), and
 * produces two products: the client stub and the server skeleton.
 *
 * The client stub is an small library that exposes the services as
 * they were local functions. Inside these local functions are
 * implemented all the voodoo, using the Raw interface, to actually
 * produce the invocation.
 * 
 * This is great because this client stub component makes the
 * invocation to be really easy, like you were interfacing with local
 * function, making you to forget you are actually calling to a remote
 * object.
 *
 * The server stub component is the piece of software provided to
 * enable the programmer to actually implement the method to be
 * invoked. It is called skeleton because it is a server with all
 * method/procedures/services without being implemented.
 *
 * In most cases, RPC developers just don't want to hear about the
 * Raw invocation interface, they use the IDL compiler. 
 * 
 * \section raw_client_invocation Using the Raw API invocation
 *
 * XML-RPC Raw interface, inside Vortex, is composed by the 
 * \ref vortex_xml_rpc_types "XML-RPC type API" and the 
 * \ref vortex_xml_rpc "invocation interface API".
 * 
 * The first one exposes all types and enum values that are used
 * across all XML-RPC API. The second one is the API that actually do
 * the useful work.
 *
 * There are two high level type definitions, \ref XmlRpcMethodCall
 * and \ref XmlRpcMethodResponse that, as their names shows, represents
 * the method call object and the method response.
 * 
 * The idea is that you use the \ref vortex_xml_rpc_types "XML-RPC type API" 
 * to create the \ref XmlRpcMethodCall, that represents your method at
 * the remote side to be executed, and then, using the invocation interface, you actually produce 
 * a service invocation. 
 * 
 * \section raw_client_invoke Performing a client invocation
 *
 * Let's assume we are going to invoke a simple method, called
 * <b>sum</b>, which receives integer 2 arguments, <b>a</b> and
 * <b>b</b> and return an integer, as a result of adding <b>a</b> to
 * <b>b</b>. What we need is to produce a representation of our method
 * so we can use it to actually produce an invocation. Here is an
 * example on how it could be done:
 * 
 * \code
 * // declare our method call reference
 * XmlRpcMethodCall  * invocator;
 * XmlRpcMethodValue * value;
 *
 * // create the method call, called sum, with 2 arguments
 * invocator = vortex_xml_rpc_method_call_new ("sum", 2);
 *
 * // now create method parameters 
 * value = vortex_xml_rpc_method_value_new (XML_RPC_INT_VALUE, 
 *                                          PTR_TO_INT(2));
 *
 * // add the method value to the invocator object
 * vortex_xml_rpc_method_call_add_value (invocator, value);
 *
 * // create the other method parameter 
 * value = vortex_xml_rpc_method_value_new (XML_RPC_INT_VALUE,
 *                                          PTR_TO_INT(3));
 *
 * // add the method value to the invocator object
 * vortex_xml_rpc_method_call_add_value (invocator, value);
 * \endcode
 *
 * Now we have a representation of our method call. What we need now,
 * is to use this it to produce an invocation. This is done by first
 * creating a connection to the remote server, booting a XML-RPC
 * channel. Here is an example:
 *
 * \code
 * // a method response declaration
 * XmlRpcMethodResponse * response;
 *
 * // a vortex connection to the remote peer
 * VortexConnection * connection;
 *
 * // a channel referece to the remote peer
 * VortexChannel    * channel;
 *
 * // some variables to get textual status
 * VortexStatus       status;
 * char             * message;
 *
 * // create the connection to a known location (in a blocking manner for
 * // demostration purposes)
 * connection = vortex_connection_new ("localhost", "22000", NULL, NULL);
 *
 * // boot an XML-RPC channel
 * channel = vortex_xml_rpc_boot_channel_sync (connection, NULL, NULL, &status, &message);
 *
 * // show message returned 
 * if (status == VortexError) {
 *      printf ("Unable to create XML-RPC channel, message was: %s\n", message);
 *      return;
 * }else {
 *      printf ("XML-RPC channel created, message was: %s\n", message);
 * }
 * 
 * // peform a synchronous method 
 * printf ("   Performing XML-RPC invocation..\n");
 * response = vortex_xml_rpc_invoke_sync (channel, invocator);
 *
 * switch (method_response_get_status (response)) {
 * case XML_RPC_OK:
 *     printf ("Reply received ok, result is: %s\n", method_response_stringify (response));
 *     break;
 * default:
 *     printf ("RPC invocation have failed: (code: %d) : %s\n",
 *              method_response_get_fault_code (response),
 *              method_response_get_fault_string (response));
 *     break;
 * }
 *
 * // free the response received 
 * method_response_free (response);
 *
 * // free the method invocator used 
 * method_call_free (invocator);
 * \endcode
 *
 * Well, it is impressive the huge amount of things to be done to
 * simple invoke a sum operation that is on the remote side, isn't it?
 * That's why people doesn't use Raw interface, preferring to use the
 * High level one while producting common RPC tasks.
 * 
 * However, this interface is required if you need to produce a custom
 * invocator that perform some especial task before doing the actual
 * invocation, or just because you need a general invocation software
 * to track down all invocations received, at server side, so you can
 * re-send that invocation to another system, acting as a proxy. 
 * 
 * \section raw_client_invoke_considerations Raw client invocation considerations
 *
 * Beforing seeing previous example, here are some issues to be
 * considered:
 *
 * <ul> 
 *
 * <li>To actually produce an invocation, you need an already booted
 * channel (\ref VortexChannel) and an invocator object (\ref
 * XmlRpcMethodCall). Once initialized they could be used over and
 * over again.
 * 
 * So, you can actually reuse the same channel (\ref VortexChannel) to
 * perform invocations, with some restrictions, explained bellow,
 * without requiring to boot one channel every time.
 * 
 * You can also reuse the \ref XmlRpcMethodCall to produce an
 * invocation for the same function, without needing to create it over
 * and over again.</li> 
 * 
 * <li>
 *
 * A channel could be reused for any number of invocation, with no
 * restriction. However, you have to know that once an invocation is
 * in progress, that channel couldn't be used again to produce a new
 * independent invocation. If done, the new invocation will be blocked
 * until the previous is finished.
 * 
 * A channel have an invocation in progress if the channel is
 * performing an invocation, activated by a call to \ref
 * vortex_xml_rpc_invoke or \ref vortex_xml_rpc_invoke_sync, and the
 * reply wasn't received yet.
 *
 * To produce several independent invocations at the same type over
 * the same connection, you have to boot several channels. This method
 * produce good performance results. You can read more about this
 * here: \ref vortex_manual_implementing_request_response_pattern.
 *
 * </li>
 *
 * <li> Previous example, uses the synchronous invocation method. You
 * should consider using \ref vortex_xml_rpc_invoke function to
 * produce a non blocking invocation, to get better results,
 * especially while working with graphical user interfaces.  </li>
 *
 * <li>Previous example stringify the results received (\ref
 * XmlRpcMethodResponse) using \ref
 * vortex_xml_rpc_method_response_stringify. This function is great
 * for an example, but, in real life application, if the user
 * application requires and integer, you can't reply an string.
 *
 * In this context, the usual operation performed on a \ref
 * XmlRpcMethodResponse received, is to get the value inside (\ref
 * XmlRpcMethodValue) calling to \ref vortex_xml_rpc_method_response_get_value.
 * 
 * Then, the next set of functions will help you to get the value
 * marshalled into the appropiate type:
 *
 *  - \ref vortex_xml_rpc_method_value_get_as_int
 *  - \ref vortex_xml_rpc_method_value_get_as_double
 *  - \ref vortex_xml_rpc_method_value_get_as_string
 *  - \ref vortex_xml_rpc_method_value_get_as_struct
 *  - \ref vortex_xml_rpc_method_value_get_as_array
 *
 * For the case of boolean values, you can use the same function for
 * the int type. boolean values, inside XML-RPC, are modeled using 0
 * and 1 states to represent true and false.
 * </li>
 *
 * </ul>
 *
 * \section receiving_an_invocation Processing an incoming XML-RPC invocation
 *
 * We have seen in previous sections how an XML-RPC invocation is
 * produced. Now, it is time to know what happens on the remote side,
 * because, until now, we have just moved the problem from an machime
 * to another. However, the problem remains unresolved.
 * 
 * Listener side implementation is built on top of two handlers: \ref
 * VortexXmlRpcValidateResource "the validation resource handler" and
 * the \ref VortexXmlRpcServiceDispatch "the service dispatch handler". 
 *
 * The first is used to provide a way to validate resource context,
 * when a channel boot request is received. According to the RFC3529,
 * a resource could be interpreted in many ways, for example, as a
 * supported interface or as a domain that encloses several
 * services. It could be used also to provide a versioning mechanism
 * for the same service.
 * 
 * So the resouce "/version/1.0" and the resource "/version/1.2" could
 * allow to support the same service name, but with different
 * versions.
 * 
 * Let's considered the following XML-RPC uri value to clearly
 * understand the resource concept:
 *
 * \code
 *   xmlrpc.beep://example.server.com/NumberToName
 * \endcode
 *
 * Previous XML-RPC uri states that there is a XML-RPC listener server at
 * <b>example.server.com</b> located at the IANA registered port
 * <b>602</b> (because no port was especified by appending to the
 * server name, the value in the form <b>:port-num</b>), and it is required
 * to ask for the resource <b>NumberToName</b> before producing the actual
 * invocation.
 *
 * <b>NumberToName</b> is not the service name. It is just a resource
 * that the XML-RPC listener may reply that it is actually supported
 * or not. In fact, you can still export services without doing
 * anything with resources. Here is an example of a XML-RPC uri used
 * to connect to a particular server at the default resource <b>"/"</b>
 *
 * \code
 *   xmlrpc.beep://example.server.com/
 * \endcode
 * 
 * In this context, the validation resource handler is used to notify
 * the Vortex engine if the listener is willing to accept a particular
 * resource value once a XML-RPC channel is being started. In many
 * cases, you can safely avoid setting the resource handler. This will
 * make Vortex XML-RPC engine to accept all resources requested.
 *
 * Now, let's talk about the \ref VortexXmlRpcServiceDispatch "service dispatch handler". As it names
 * shows, it is used to enable the Vortex engine to notify user space
 * code that a new method invocation have arrived (\ref XmlRpcMethodCall) and that it has to be dispatched to the
 * appropiate handler. This handler is mainly provided to allow
 * developers to be able to produce its own service dispaching policy.
 * 
 * Let's see a simple dispatching implementation for the sum service
 * introduced at the \ref raw_client_invoke "client invocation section". 
 * Let's see the example first to later see some considerations:
 *
 * First, the listener side must active a listener that is willing to
 * accept the XML-RPC profile: 
 * 
 * \code
 * int  main (int  argc, char  ** argv) 
 * {
 *
 *	// init vortex library 
 *	vortex_init ();
 *
 *	// enable XML-RPC profile 
 *	vortex_xml_rpc_accept_negociation (validate_resource,
 *					   // no user space data for
 *					   // the validation resource
 *					   // function. 
 *					   NULL, 
 *					   service_dispatch,
 *					   // no user space data for
 *					   // the dispatch function. 
 *					   NULL);
 *
 *	// create a vortex server 
 *	vortex_listener_new ("0.0.0.0", "44000", NULL, NULL);
 *
 *	// wait for listeners (until vortex_exit is called) 
 *	vortex_listener_wait ();
 *	
 *	// end vortex function 
 *	vortex_exit ();
 *
 *	return 0;
 * }
 * \endcode
 *
 * The example is quite simple, first, Vortex Library is initialized,
 * then a call to \ref vortex_xml_rpc_accept_negociation is done to
 * active the XML-RPC. Then a call to activate a listener, for any
 * host name that the local machine may have, at the port 44000, is
 * done by using \ref vortex_listener_new. Finally, a call to \ref
 * vortex_listener_wait is done to ensure the server initialization
 * code is blocked until the server finish.
 *
 * Here is an example of a validation resource function that only
 * accept some resources names:
 * \code
 * bool     validate_resource (VortexConnection * connection, 
 *                             int                channel_number,
 *                             char             * serverName,
 *                             char             * resource_path)
 * {
 *      // check that resource received
 *      if (axl_cmp (resource_path, "/aritmetic-services/1.0"))
 *              return true;
 *      if (axl_cmp (resource_path, "/aritmetic-services/1.1"))
 *              return true;
 *    
 *      // resource not recognized, just return false to signal
 *      // vortex engine that the channel must not be accepted.
 *      return false;
 * }
 * \endcode
 *
 * And here is a service dispatch implementation, for our service
 * example <b>sum</b>:
 * 
 * \code
 * int  __sum_2_int_int (int  a, int  b, char  ** fault_error, int  * fault_code)
 * {
 *      if (a == 2 && b == 7) {
 *              // this is an example on how the return a fault reply that includes
 *              // an error code an error string.
 *		REPLY_FAULT ("Current implementation is not allowed to sum the 2 and 7 values", -1, 0);
 *      }
 *	
 *      // for the rest of cases, just sum the two incoming values 
 *      return a + b;
 * }
 *
 *
 * XmlRpcMethodResponse * sum_2_int_int (XmlRpcMethodCall * method_call)
 * {
 *
 *      int                 result;
 *      char              * fault_error = NULL;
 *      int                 fault_code  = -1;
 *
 *      // get values inside the method call
 *      int  a = method_call_get_param_value_as_int (method_call, 0);
 *      int  b = method_call_get_param_value_as_int (method_call, 1);
 *
 *      // perform invocation 
 *      result = __sum_2_int_int (a, b, &fault_error, &fault_code);
 *	
 *      // check error reply looking at the fault_error 
 *      if (fault_error != NULL) {
 *          // we have a error reply 
 *	    return CREATE_FAULT_REPLY (fault_code, fault_error);
 *      }
 *
 *      // return reply generated 
 *      return CREATE_OK_REPLY (XML_RPC_INT_VALUE, INT_TO_PTR (result));
 * }
 *
 * XmlRpcMethodResponse *  service_dispatch (VortexChannel * channel, XmlRpcMethodCall * method_call, axlPointer user_data) 
 * {
 *	
 *
 *      // check if the incoming method call is called sum, and has
 *      // two arguments 
 *      if (method_call_is (method_call, "sum", 2, XML_RPC_INT_VALUE, XML_RPC_INT_VALUE, -1)) {
 *          return sum_2_int_int (method_call);
 *      }
 *	       
 *      // return that the method to be invoked, is not supported 
 *      return CREATE_FAULT_REPLY (-1, "Method call received couldn't be dispatched because it not supported by this server");
 * }
 * \endcode
 * 
 * The example shows how the service dispatch handler is the
 * <b>service_dispatch</b> function, which first recognizes if the
 * service is supported and then call to the appropiate handler. 
 * 
 * Here is the first interesting thing, the function \ref
 * method_call_is. It is used to recognize service patterns like name,
 * number of paremter and the parameter type. This allows to easily
 * recognize the service to actually dispatch.
 *
 * In this function, <b>service_dispatch</b>, should be created a \ref XmlRpcMethodResponse to
 * be returned. So, the vortex engine could reply as fast as
 * possible. However, the implementation is prepared to deffer the
 * reply. This allows, especially, to communicate with other language
 * runtimes. Once the runtime have generated the reply, it must be
 * used the following function \ref vortex_xml_rpc_notify_reply, to actually perform the reply.
 *
 * \section abstraction_required Abstraction required: The xml-rpc-gen tool
 *
 * Until now, we have seen that producing RPC enabled solutions is
 * really complex. First, because we have to produce the server
 * implementation, and then, all source code required to actually
 * perform the invocation.
 * 
 * However, every RPC framework comes with a protocol compiler that
 * helps on producing lot of source code. For the XML-RPC over BEEP
 * implementation that comes with Vortex Library, this tool is
 * <b>xml-rpc-gen</b>.
 *
 * This tool reads IDL and XDL interface definitions and produce a
 * ready server and a client library (usually called
 * <b><i>stub</i></b>) that allows to perform the service invocation
 * without paying attention to XML-RPC invocation details. Let's see
 * an example to introduce the tool:
 * 
 * \include reg-test01.idl
 *
 * Now write previous example (just copy the example) into a file
 * (let's say <b><i>reg-test01.idl</i></b>, you have a copy inside the
 * <b><i>../xml-rpc-gen/</i></b> directory, bundled with the Vortex Library
 * source code) and exec the following:
 * 
 * \code
 * >> ./xml-rpc-gen reg-test01.idl
 * [*] compiling: reg-test01.idl..
 * [*] detected IDL format definition..
 * [*] detected xml-rpc definition: 'test'..
 * [*] document is well-formed: reg-test01.idl..
 * [*] document is valid: reg-test01.idl..
 * [*] component name: 'test'..
 * [*] using 'out' as out directory..
 * [*] generating client stub at: out/client-test..
 * [*] generating server stub at: out/server-test..
 * [*] generating autoconf files..
 * [*] compilation ok
 * \endcode
 *
 * Previous interface definition have produced two components: 
 *
 * - <b>A client stub</b>: a library that have an interface to invoke
 * services, located at: <b><i>out/client-test</i></b>
 *
 * - <b>A server component</b>: that implements service exported,
 * located at: <b><i>out/client-test</i></b>.
 *
 * Now, instead of producing all source code required to perform the
 * invocation, the <b>xml-rpc-gen</b> tool allows you define a xml-rpc
 * interface definition, that produces that code for you. In this
 * case, looking at: <b><i>out/client-test/test_xml_rpc.h</i></b>,
 * you'll find a C API that hides all details to actually invoke the
 * <b><i>sum</i></b> service.
 *
 * In the other hand, instead of producing all source code, at the
 * server side, to unmarshall and invoke the service, all this code is
 * produced by <b>xml-rpc-gen</b> tool. In this case, looking at:
 * <b><i>out/server-test/test_sum_int_int.c</i></b> you'll find the
 * <b>sum</b> service implementation. 
 *
 * In the following sections it is explained how to use the
 * <b>xml-rpc-gen</b> tool to produce RPC solutions.
 *
 * \section xml_rpc_gen_tool_language Using xml-rpc-gen tool: language syntax
 *
 * <b>xml-rpc-gen</b> is a compiler that produces a client component
 * and a server component. The client component is just a library that
 * hides all details required to perform the invocation. The tool
 * support two formats: one structured, more based on the IDL
 * (Interface Definition Language) language found in other platforms,
 * and a XML based format, called XDL (XML Definition Language).
 *
 * Internally, the tool is programmed on top of XDL definition
 * language. If the tool detects that the input file is written using
 * the IDL format, it translate that representation into an equivalent
 * XDL representation, starting the processing as the input were
 * XDL. In any case, both language representations are equivalent and
 * well supported.
 *
 * The interface definition is composed by the global interface
 * declaration and, at least, one service definition. Here is an
 * example using both formats:
 * 
 * <b>IDL format:</b>
 * \include reg-test01.idl
 *
 * <b>XDL format:</b>
 * \include reg-test01.xdl
 *
 * As you can see, <b>XDL format</b> is more verbose than IDL format,
 * but provides a standard format to cross boundaries between process,
 * it is easy to parse and it is more portable (there is an XML parser
 * in every platform).
 * 
 * \section xml_rpc_gen_tool_language_types Types supported by xml-rpc-gen tool
 *
 * There are 6 basic types supported by the tool (well, it is more
 * accurate to say by the XML-RPC definition) which are:
 * 
 *  -  <b>int</b>: Integer definition, four-byte signed integer (-21)
 *  -  <b>boolean</b>: Boolean definition (bound to 1/true, 0/false)
 *  -  <b>double</b>: double-precision signed floating point number -412.21
 *  -  <b>string</b>: An string definition "XML-RPC over BEEP!!"
 *  -  <b>data</b>: date/time (currently not supported)
 *  -  <b>base64</b>: An base64 encoded string (currently encoding is not done).
 *
 * And two compound type definitions which allows to define more types:
 *
 *  -  <b>struct</b>: An struct definition which holds named values (called members).
 *  -  <b>array</b>: An uniform storage for other types (including more arrays).
 *
 * Struct and array types allows to create richer type definitions
 * (even composing both). Here is an example that uses an struct
 * declaration:
 * 
 * <b>IDL format:</b>
 * \include reg-test05.idl
 *
 * <b>XDL format:</b>
 * \include reg-test05.xdl
 *
 * \section xml_rpc_gen_tool_language_recursive Recursive declarations with Struct
 * 
 * You can also define recursive type declarations, which makes
 * refence to the type being defined. This allows, for example, to
 * create a list with linked nodes represented by struct
 * declarations. Here is an example:
 * 
 * <b>IDL format:</b>
 * \include reg-test07.idl
 *
 * <b>XDL format:</b>
 * \include reg-test07.xdl
 *
 * \section xml_rpc_gen_tool_language_considerations Considerations while using composing types: Struct and Arrays.
 * 
 *
 * There are several question to consider while using structures and arrays:
 *  
 * - There is no need to specify that the struct/array that is
 * received or returned is a pointer type. There is no <b>*</b>. Types
 * that are struct or arrays are used as basic types, referring to
 * them by using its definition name. The same applies to <b>string</b> type definition.
 *
 * - You have to first define a type before using it. You can't use an
 * struct/array definition if it is defined after it first
 * use. Howerver, there are exception: recursive definitions.
 *
 * - Howerver, because the tool allows you to define source code
 * inside the services to be included inside the server output, you
 * have to use the pointer syntax. This could be obvious: remember
 * that xml-rpc-gen tool just includes the source code. It doesn't
 * perform any syntax validation.
 *
 *
 * \section xml_rpc_gen_tool_language_array_declaration Array declaration
 *
 * Here is an example to define an array type (which is quite
 * different from the array definition found in C/C#/Java):
 * 
 * <b>IDL format:</b>
 * \include reg-test06.idl
 *
 * <b>XDL format:</b>
 * \include reg-test06.xdl
 * 
 *
 * \section xml_rpc_gen_tool_language_no_parameters Services without parameters
 *
 * Services could have no parameter. This is clear while using the IDL
 * format, but for the XDL format, it is required to place the
 * &lt;params> declaration, with no &lt;param> childs. Here is an example:
 * 
 * <b>IDL format:</b>
 * \include reg-test02.idl
 *
 * <b>XDL format:</b>
 * \include reg-test02.xdl
 *
 * \section xml_rpc_gen_tool_language_boolean Services using and returning boolean types
 *
 * Declaring services that receive or return boolean types is pretty
 * straightforward. Here is an example:
 * 
 * <b>IDL format:</b>
 * \include reg-test03.idl
 *
 * <b>XDL format:</b>
 * \include reg-test03.xdl
 *
 * \section xml_rpc_gen_tool_language_double Services using and returning double types
 *
 * This is no especial consideration while declaring services that
 * makes use of the double type. Here is an example:
 * 
 * 
 * <b>IDL format:</b>
 * \include reg-test04.idl
 *
 * <b>XDL format:</b>
 * \include reg-test04.xdl
 *
 * \section xml_rpc_gen_tool_changing_method_name Changing the method name for a service declared
 *
 * Due to the kind of output produced by the xml-rpc-gen tool, it has
 * to create "method names" for services declared at the IDL processed
 * in a synchronized way to a client invocation, using a particular
 * service, is properly processed by the remote service entry point.
 *
 * Under some situations it is required to change the name that used
 * by default the xml-rpc-gen tool. This is done by using a prefix
 * declaration before the service:
 * \include af-arch.idl
 *
 * In the example, the service <b>get_list</b> won't be invoked using
 * that name (the default xml-rpc-gen behaviour), but the name
 * declared at the <b>method_name</b> will be used.
 *
 * Previous IDL declaration is used by shaper to invoke a service
 * exported by the Af-Arch central server, through the XML-RPC bridge,
 * to get host location information.
 *
 * \section xml_rpc_gen_tool_using_resources Using resources to group your services
 *
 * During the XML-RPC channel negotiation phase, the client
 * request to create a channel under a particular <b>resource</b>. By
 * default all services declared at the IDL are grouped under the same
 * resource: <b>"/"</b>. 
 *
 * This <b>resource</b> declaration can be used in several ways to
 * archieve some interesting features:
 * 
 * - If services provided by your server are grouped into a defined
 * resource, different from the default ("/"), it is possible to use
 * that resource at the \ref VortexXmlRpcValidateResource "validate resource handler" 
 * to allow client applications to know if the set of desired services 
 * are exported by the server component.
 *
 * - Grouping services under several resources improves service
 * dispatching efficiency mainly because it is not required to check
 * all services (under the same resource) to perform the dispatch
 * operation.
 *
 * To change the resource under which the service is grouped you must
 * use the attribute <b>"resource"</b> as used in: \ref
 * xml_rpc_gen_tool_changing_method_name.
 *
 * \include change-resource.idl
 *
 * Note this will produce a client and a server component that handles
 * the service under the provided resource. However the \ref
 * VortexXmlRpcValidateResource "validation handler" is not provided.
 *
 * \section xml_rpc_gen_tool_enforce_resources Enforcing resources to be used at the IDL
 *
 * Previous section provided information about declaring resources and
 * grouping services under them. However, knowing that a resource
 * declaration is a string, it could be required to enforce resources
 * that are usable across the IDL. This is done by using the
 * following:
 *
 * \code
 * allowed resources "resource1", "resource2", "resource3";
 * \endcode
 *
 * This list declaration must be used before any service with the
 * attribute <b>"resource"</b>. The list configured will enforce the
 * xml-rpc-gen tool to allow and check all resources used.
 *
 * \section xml_rpc_gen_tool_including_body_code Including additional code to be placed at the service module file
 *
 * Examples showed allows to include code that is placed at the
 * appropiate file at the server side created. However, real situation
 * requires calling to functions that are defined at the same modules
 * or other modules. This is because it is required a mechanism that
 * allows to include arbitrary code, not only in the service body.
 *
 * Supose the following example:
 *
 * <b>IDL format:</b>
 * \include body-include.idl
 * 
 * The service <b>sum</b> is implemented making a call to a user
 * defined function <b>do_sum_operation</b>. Using the <i>"include on
 * body"</i> declaration it is possible to include the content of the
 * function. The content provide will be included at the top of the
 * body implementation for the service <b>sum</b> to avoid prototypes.
 *
 * There is also a way to include content into the module header. This
 * is done using the same structure: <i>"include on header"</i>.
 *
 * In the case the body content to be included is to large you can
 * place it into a file and make the tool to include it. This is done
 * as follows:
 *
 * <b>IDL format:</b>
 * \include body-include-file.idl
 *
 * In this case the implementation of the <b>do_sum_operation</b> is
 * included at the file: <i>"do_sum_operation.c"</i>. The same applies
 * to the content included at the module header file.
 * 
 * \section xml_rpc_gen_tool_using_output_client Using the output produced by xml-rpc-gen tool at the CLIENT SIDE
 * 
 * The output produced by the tool are two software pieces: the client
 * stub library and the server component. The client stub is small
 * library that hides all the details to produce the invocation,
 * marshalling all data, and to unmarshall replies received. 
 * 
 * Let's recall our first example, to see what is the result generated
 * for the client stub:
 * 
 * <b>IDL format:</b>
 * \include reg-test01.idl
 *
 * If previous example is compiled (supposing the file is located at
 * reg-test01.idl) as follows:
 * 
 * \code
 * bash:~$ xml-rpc-gen reg-test01.idl
 * \endcode
 * 
 * By default, the client library is placed at:
 * <b>out/client-&lt;component-name></b>. In this case, the output
 * will be <b>out/client-test</b>. You can modify this behaviour by
 * using the <b>--out-dir</b> switch.
 * 
 * Inside a the <b>out/client-test</b> directory you'll find the main
 * API file <b>out/client-test/test_xml_rpc.h</b>. This file contains
 * all invocation functions required to perform a method call to all
 * services exported by the componet compiled. 
 * 
 * Again, this file will follow the next naming convetion accoring to
 * the component name: <b>out/client-&lt;component-name>/&lt;componet-name>_xml_rpc.h</b>
 *
 * Let's see its content:
 * 
 * \include test_xml_rpc.h
 *
 * As you can see, there are two function declarations for every
 * service exported by the XML-RPC component compiled. Both represent
 * the same service to be invoked, however, one serve as a synchronous
 * invocation, the one with the prefix <b>"_s"</b> and the other one
 * serves for the asynchronous (non-blocking) invocation.
 * 
 * In the synchronous invocation case, the function ending with
 * <b>"_s"</b>, you'll find all parameters especified in the IDL/XDL
 * definition, in this case: <b><i>int a, and int b</i></b>, plus
 * three additional parameters, that are optional, and helps to get invocation error
 * reporting.
 * 
 * - \ref XmlRpcResponseStatus <b>status</b>: Allows to get current
 * invocation status (\ref XML_RPC_OK) or (\ref XML_RPC_FAIL and the
 * rest of fail errors).
 *
 * -  <b>int * fault_code</b>: allows to get a integer fault code, defined
 * by the remote service (actually defined by the service
 * developer). This is a custom value not defined by the framework. See \ref REPLY_FAULT.
 * 
 * - <b>char ** fault_string</b>: allows to get the error string, a
 * textual diagnostic string, defined by the remote service. Again,
 * see \ref REPLY_FAULT. <i><b>NOTE:</b> While using the synchronous
 * invocation, and this variable is provided and an error is found,
 * then this string holding the textual diagnostic must be deallocated
 * after using it using: axl_free.</i>
 *
 * Apart from the service parameters, in both cases, synchronous and
 * asynchronous, you must also provide a reference to a \ref
 * VortexChannel already initialized (XML-RPC booted), with the
 * XML-RPC profile, and under the particular resource where the
 * service will be run. 
 * 
 * To boot a channel there are several functions provided:
 * 
 * - \ref vortex_xml_rpc_boot_channel that will create a \ref VortexChannel
 * running the XML-RPC profile, in an asynchronous way, without
 * blocking the caller and notifying once the creation process have
 * finished (without considering its termination status).
 *
 * - \ref vortex_xml_rpc_boot_channel_sync that will create a
 * \ref VortexChannel running the XML-RPC profile, in a synchronous way,
 * blocking the caller until the process is totally finished.
 *
 * - \ref BOOT_CHANNEL (alias to \ref vortex_xml_rpc_boot_channel).
 * 
 * 
 * Finally, the asynchronous API is the same as the synchronous,
 * providing the service parameters, the channel where the invocation
 * will be run, and a handler where the service reply will be
 * notified. 
 *
 * Now we know a bit more about the main API created by the
 * xml-rpc-gen tool, the file <b>test_xml_rpc.h</b>. However, you also
 * have to take a look to <b>test_types.h</b> file. It contains all
 * complex type definitions, that is, struct and array
 * declarations. In this case, that file is empty.
 * 
 * <i><b>NOTE:</b> Both products generated, <b>client-test</b> and
 * <b>server-test</b> have support for auto-tools. You have to follow
 * the standard process to configure and compile:
 * 
 * \code
 *  bash:~$ cd Test/out/client-test
 *  bash:~/Test/out/client-test$ ./autogen
 *  bash:~/Test/out/client-test$ ./make
 *  bash:~/Test/out/client-test$ ./make install
 * \endcode
 * </i>
 *
 * Ok, but, what about performing an invocation using this files. Here is
 * a simple example:
 * \include reg-test01a.c
 *
 * In the test we didn't perform any check, which isn't a good thing,
 * (using \ref vortex_connection_is_ok to check the connection, checking for NULL reference the
 * channel created, and providing some variables to get invocation
 * status, etc), but it helps you to get an idea.
 * 
 * \section xml_rpc_gen_tool_using_output_listener Using the output produced by xml-rpc-gen tool at the LISTENER SIDE
 *
 * Following the example of the previous section, there is not too
 * much to say. You have to compile the server and then run it.  Try
 * to compile the previous example and to compile the server as
 * follows:
 *
 * \code
 *  bash:~$ cd Test/out/server-test
 *  bash:~/Test/out/server-test$ ./autogen
 *  bash:~/Test/out/server-test$ ./make
 *  bash:~/Test/out/server-test$ ./make install
 *  bash:~/Test/out/server-test$ ./server-test
 * \endcode
 *
 * Because xml-rpc-gen tool have support to include the service source
 * code definition, into the IDL/XDL definition, the compiled product
 * only required to be compiled. However programing an XML-RPC service
 * usually is more complex than adding two integer. Here are some
 * considerations:
 * 
 * If the server component is produced (by default client and server
 * components are produced but it can be configured by using
 * <b>--only-client</b> or <b>--only-server</b>), it contains an xml
 * file that allows to configure the TCP/IP location. Look at:
 * <b>out/server-test/conf.xml</b>. Here is the content:
 *
 * \include conf.xml
 * 
 * You can modify and add more &lt;listener> nodes to make your
 * XML-RPC component to listen in other ports than the default one
 * (produced 0.0.0.0:44000). It is supposed that the IANA authority
 * have registered the 602 TCP port for the XML-RPC over BEEP
 * protocol, but this isn't required.
 *
 * While programming the server component, inside the IDL/XDL file,
 * you'll have to use the \ref REPLY_FAULT macro if you want to reply
 * to the client component a faultCode and a faultString. The first
 * two parameters are the fault string and the fault code. The last
 * one is the empty value to be returned in order to compile the
 * server. In most cases you can use 0.
 * 
 * If the service returns pointer types: \ref XML_RPC_STRING_VALUE,
 * \ref XML_RPC_STRUCT_VALUE and \ref XML_RPC_ARRAY_VALUE, all of them
 * must return dinamically allocated objects. They will be deallocated
 * by the XML-RPC engine, at the proper time.
 * 
 *
 * \section xml_rpc_authentication_and_security XML-RPC authentication and Security
 *
 * Until now, we didn't talk too much about these topics, mainly because they
 * are fully integrated with the BEEP environment, and hence, inside
 * Vortex Library. However, for those new to BEEP, here are some tips about this issue.
 *
 * Every profile implemented on top of BEEP (the protocol) have
 * already-made support to authenticate peers using \ref vortex_manual_using_sasl "SASL" and to
 * secure connections using \ref vortex_manual_securing_your_session "TLS". 
 *
 * So, in the case a secure invocation is required you'll have to
 * first create a connection with the XML-RPC listener, and then
 * secure that connection. Then, once the connection is secured,
 * perform the invocation creating channels as usual. All this tasks
 * could be considered as client side one. Look at the following
 * document to know about this issue: \ref vortex_manual_securing_your_session.
 *
 * At the server side you have to provide the enough mechanism to
 * allow/ensure the client that is connected is doing so in a proper
 * way. You can implement all your security politics at the \ref
 * VortexOnStartChannel "start handler" or at the \ref
 * VortexXmlRpcValidateResource "resource validation handler". Unless
 * it is required some especial TLS function at the server side, like
 * identifing the client certificate, it is not required to implement
 * anything especial. Look at the following document with provides a
 * default TLS environment: \ref vortex_manual_securing_your_session.
 *
 * Because Vortex Library, at this moment, doesn't provide a profile
 * path, allowing to hide non-secured profiles behind TLS or SASL
 * profiles until they are entirely negociated, you have to ensure
 * that TLS is already activated, before accepting to dispatch a
 * function, at the \ref VortexXmlRpcServiceDispatch "service dispatch handler". 
 * See also \ref vortex_connection_is_tlsficated for more details.
 *
 * The same philosofy applies to the authentication problem. You have
 * to first establish the user authentication (and its authorization)
 * and the perform the XML-RPC invocation. This means that, for a
 * connection, first it is required to activate the user
 * authentication through the \ref vortex_sasl "SASL API", and
 * then perform the XML-RPC invocation as normal. 
 * 
 * Then, at the server side, inside the service dispatch function, the
 * listener must check the connection credentials already activated to
 * ensure the user that is actually executing the XML-RPC method, is
 * allowed to do so. See the following document to know more about
 * this: \ref vortex_manual_using_sasl.
 *
 * To summarize, there is no way to describe the <b>"unique security and authentication"</b> solution inside BEEP. Authentication
 * (SASL), Security (TLS) and, for this case, the XML-RPC invocation
 * mechanism are implemented in an independent way, like blocks that
 * must be combined and mixed to build your especific solution that
 * meets your requirements.
 *
 * Maybe you only require to use TLS, or maybe you just need to do a
 * SASL plain auth. However, the key concept is that: your required
 * provisioning mechanism must be first activated before you actually
 * perform useful work.
 *
 * 
 */

/**
 * \page using_tunnel_profile Vortex TUNNEL programming manual (C API)
 *
 *
 * \section intro Introduction
 * 
 * This manual shows you how to use the TUNNEL profile (RFC3620)
 * implemented in Vortex Library, to allow performing BEEP connections
 * through a BEEP proxy running also the TUNNEL profile.
 *
 * The main idea behind BEEP is to allow you writing new network
 * application protocols in an easy way reusing as much as possible
 * mechanisms already tested in the past. In the same direction, the
 * TUNNEL profile allows you to provide proxy support, with its all
 * associated security advantages, to <b><i>any</i></b> protocol layered on top of
 * BEEP.
 *
 *
 * The TUNNEL profile design is simple and elegant. You configure a
 * TUNNEL to be created to the remote endpoint, and once establish the
 * link, endpoint-to-endpoint, a new BEEP session is negotiated,
 * allowing to create channels as it were no proxy in the middle.
 *
 * \section tunnel_requirements TUNNEL profile requirements 
 * 
 * You must compile Vortex Library with TUNNEL support. Please refer
 * to the installation manual: \ref install
 *
 * You can also use at runtime \ref vortex_tunnel_is_enabled to check
 * if the TUNNEL profile was included in your Vortex Library.
 *
 * \section setting_a_tunnel Creating a connection to the remote endpoint: setting a TUNNEL.
 *
 * The TUNNEL profile is only used to establish the
 * endpoint-to-endpoint connection. Once created, channels are created
 * and used as usual. In fact, creating connection (\ref
 * VortexConnection) with TUNNEL returns the same object.
 *
 * Here is a simple example creating a connection, using a BEEP proxy:
 *
 * \code
 * VortexTunnelSettings * settings;
 * VortexConnection     * connection;
 * 
 * // create a tunnel settings holder
 * settings = vortex_tunnel_settings_new ();
 *
 * // configure the tunnel
 * vortex_tunnel_settings_add_hop (settings,
 *                                 TUNNEL_IP4, "80.43.98.134"
 *                                 TUNNEL_PORT, "55000",
 *                                 TUNNEL_END_CONF);
 * 
 * vortex_tunnel_settings_add_hop (settings,
 *                                 TUNNEL_IP4, "192.168.0.133",
 *                                 TUNNEL_PORT, "604",
 *                                 TUNNEL_END_CONF);
 * 
 * // create the connection using tunnel settings
 * connection = vortex_tunnel_new (settings, NULL, NULL);
 * if (! vortex_connection_is_ok (connection, false)) {
 *       // manage tunnel creation error 
 * }
 * 
 * // tunnel created!
 * \endcode
 *
 * Let's see what's happens here:
 * <ol>
 *
 *  <li>First, a empty tunnel settings is created with: \ref vortex_tunnel_settings_new</li>
 *
 *  <li>Then, we configure the remote end point to connect to. Note
 *  this is not the actual BEEP proxy server in our network. In this
 *  case, we want to connect to the remote BEEP peer located at
 *  <b>80.43.98.134</b>, running at the tcp port: <b>55000</b>.</li>
 *
 *  <li>Finally, we configured the next hop to use, in this case, our
 *  local network BEEP proxy, which is located at the well known BEEP
 *  TUNNEL port: <b>192.168.0.133:604</b>.
 *
 * </ol>
 * 
 * Once the connection is created, you can use the usual API to create
 * channels and to exchange data over those channels. A connection
 * created by the \ref vortex_tunnel "TUNNEL API" works with the same
 * properties: you must close them using \ref vortex_connection_close
 * when no longer needed.
 *
 * The example also shows that a settings configuration can be reused
 * as many times as required. Once finished, it is also required to
 * call: \ref vortex_tunnel_settings_free.
 * 
 * \section tunnel_hop_inverse_order Configuring next hop in inverse order
 *
 * As you might note with previous example, the hop configuration
 * first set the remote end point, and then the proxy to use. This is because
 * every time you configure a next hop, this is added to the hop list
 * to be traversed in the first position.
 *
 * So, to create a tunnel with the following configuration:
 * 
 * \code
 *    [BEEP PROXY 1] -> [BEEP PROXY 2] -> [ENDPOINT]
 * \endcode
 * 
 * You must build the \ref vortex_tunnel_settings_new "tunnel setting"
 * in the inverse order, that is, calling to
 * \ref vortex_tunnel_settings_add_hop with the address of <i>"ENDPOINT"</i>,
 * followed by <i>"BEEP PROXY2"</i> and <i>"BEEP PROXY1"</i>.
 *
 * \section vortex_tunnel_servers Are there any BEEP TUNNEL server already implemented?
 *
 * Sure, you can use <b>mod-tunnel</b> from <b>turbulence</b>. It
 * currently provides full TUNNEL support, including \ref
 * TUNNEL_ENDPOINT and \ref TUNNEL_URI tunnel configurations.
 *
 * 
 */


/**
 * \page image_logos Vortex Library Image logos
 * \section image_intro Logos available
 * 
 * You can use the following logs for your site or whatever you want
 * inside your project using Vortex Library. 
 * 
 * \image html vortex-big.png "A really big Vortex logo in PNG"
 * \image html vortex-200x200.png "A Vortex logo in PNG format (size 200x200)"
 * \image html vortex-100x100.png "A Vortex log in PNG format (size 100x100)"
 * \image html wide-logo-349x100.png "A wide logo suitable for a web site in PNG format (size 349x100)"
 * \image html vortex-has-you-420x100.png "Did Vortex catch you? A vortex logo in PNG format (size 420x100)"
 * \image html powered-by-vortex-361x100.png "A powered-by image (size 361x100)"
 */

 

