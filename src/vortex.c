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

/* global includes */
#include <vortex.h>
#include <signal.h>

/* private include */
#include <vortex_ctx_private.h>

#define LOG_DOMAIN "vortex"

/* Ugly hack to have access to vsnprintf function (secure form of
 * vsprintf where the output buffer is limited) but unfortunately is
 * not available in ANSI C. This is only required when compile vortex
 * with log support */
#if defined(ENABLE_VORTEX_LOG)
/* but avoid defining it on macosx because it seems to break the compiler */
#if !defined(__APPLE__)
int vsnprintf(char *str, size_t size, const char *format, va_list ap);
#endif
#endif

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

#if !defined(AXL_OS_WIN32)
void __vortex_sigpipe_do_nothing (int _signal)
{
	/* do nothing sigpipe handler to be able to manage EPIPE error
	 * returned by write. read calls do not fails because we use
	 * the vortex reader process that is waiting for changes over
	 * a connection and that changes include remote peer
	 * closing. So, EPIPE (or receive SIGPIPE) can't happen. */
	

	/* the following line is to ensure ancient glibc version that
	 * restores to the default handler once the signal handling is
	 * executed. */
	signal (SIGPIPE, __vortex_sigpipe_do_nothing);
	return;
}
#endif

/** 
 * @brief Allows to get current status for log debug info to console.
 * 
 * @param ctx The context that is required to return its current log
 * activation configuration.
 * 
 * @return axl_true if console debug is enabled. Otherwise axl_false.
 */
axl_bool      vortex_log_is_enabled (VortexCtx * ctx)
{
#ifdef ENABLE_VORTEX_LOG	
	/* no context, no log */
	if (ctx == NULL)
		return axl_false;

	/* check if the debug function was already checked */
	if (! ctx->debug_checked) {
		/* not checked, get the value and flag as checked */
		ctx->debug         = vortex_support_getenv_int ("VORTEX_DEBUG") > 0;
		ctx->debug_checked = axl_true;
	} /* end if */

	/* return current status */
	return ctx->debug;

#else
	return axl_false;
#endif
}

/** 
 * @brief Allows to get current status for second level log debug info
 * to console.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @return axl_true if console debug is enabled. Otherwise axl_false.
 */
axl_bool      vortex_log2_is_enabled (VortexCtx * ctx)
{
#ifdef ENABLE_VORTEX_LOG	

	/* no context, no log */
	if (ctx == NULL)
		return axl_false;

	/* check if the debug function was already checked */
	if (! ctx->debug2_checked) {
		/* not checked, get the value and flag as checked */
		ctx->debug2         = vortex_support_getenv_int ("VORTEX_DEBUG2") > 0;
		ctx->debug2_checked = axl_true;
	} /* end if */

	/* return current status */
	return ctx->debug2;

#else
	return axl_false;
#endif
}

/** 
 * @brief Enable console vortex log.
 *
 * You can also enable log by setting VORTEX_DEBUG environment
 * variable to 1.
 * 
 * @param ctx The context where the operation will be performed.
 *
 * @param status axl_true enable log, axl_false disables it.
 */
void     vortex_log_enable       (VortexCtx * ctx, axl_bool      status)
{
#ifdef ENABLE_VORTEX_LOG	
	/* no context, no log */
	if (ctx == NULL)
		return;

	ctx->debug         = status;
	ctx->debug_checked = axl_true;
	return;
#else
	/* just return */
	return;
#endif
}

/** 
 * @brief Enable console second level vortex log.
 * 
 * You can also enable log by setting VORTEX_DEBUG2 environment
 * variable to 1.
 *
 * Activating second level debug also requires to call to \ref
 * vortex_log_enable (axl_true). In practical terms \ref
 * vortex_log_enable (axl_false) disables all log reporting even
 * having \ref vortex_log2_enable (axl_true) enabled.
 * 
 * @param ctx The context where the operation will be performed.
 *
 * @param status axl_true enable log, axl_false disables it.
 */
void     vortex_log2_enable       (VortexCtx * ctx, axl_bool      status)
{
#ifdef ENABLE_VORTEX_LOG	
	/* no context, no log */
	if (ctx == NULL)
		return;

	ctx->debug2 = status;
	ctx->debug2_checked = axl_true;
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
 * @return axl_true if enabled, axl_false if not.
 */
axl_bool      vortex_color_log_is_enabled (VortexCtx * ctx)
{
#ifdef ENABLE_VORTEX_LOG	
	/* no context, no log */
	if (ctx == NULL)
		return axl_false;
	if (! ctx->debug_color_checked) {
		ctx->debug_color_checked = axl_true;
		ctx->debug_color         = vortex_support_getenv_int ("VORTEX_DEBUG_COLOR") > 0;
	} /* end if */

	/* return current result */
	return ctx->debug_color;
#else
	/* always return axl_false */
	return axl_false;
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
 * You can also enable color log by setting VORTEX_DEBUG_COLOR
 * environment variable to 1.
 * 
 * @param ctx The context where the operation will be performed.
 *
 * @param status axl_true enable color log, axl_false disables it.
 */
void     vortex_color_log_enable (VortexCtx * ctx, axl_bool      status)
{

#ifdef ENABLE_VORTEX_LOG
	/* no context, no log */
	if (ctx == NULL)
		return;
	ctx->debug_color_checked = status;
	ctx->debug_color = status;
	return;
#else
	return;
#endif
}

/** 
 * @brief Allows to configure which levels will be filtered from log
 * output. This can be useful to only show debug, warning or critical
 * messages (or any mix).
 *
 * For example, to only show critical, pass filter_string = "debug,
 * warning". To show warnings and criticals, pass filter_string =
 * "debug".
 *
 * To disable any filtering, use filter_string = NULL.
 *
 * @param ctx The vortex context that will be configured with a log filter.
 *
 * @param filter_string The filter string to be used. You can separate
 * allowed values as you wish. Allowed filter items are: debug,
 * warning, critical.
 *
 */
void     vortex_log_filter_level (VortexCtx * ctx, const char * filter_string)
{
	v_return_if_fail (ctx);

	/* set that debug filter was configured */
	ctx->debug_filter_checked = axl_true;

	/* enable all levels */
	if (filter_string == NULL) {
		ctx->debug_filter_is_enabled = axl_false;
		return;
	} /* end if */

	/* add each filter bit */
	if (strstr (filter_string, "debug"))
		ctx->debug_filter |= VORTEX_LEVEL_DEBUG;
	if (strstr (filter_string, "warning"))
		ctx->debug_filter |= VORTEX_LEVEL_WARNING;
	if (strstr (filter_string, "critical"))
		ctx->debug_filter |= VORTEX_LEVEL_CRITICAL;

	/* set as enabled */
	ctx->debug_filter_is_enabled = axl_true;
	return;
}

/** 
 * @brief Allows to check if current VORTEX_DEBUG_FILTER is enabled. 
 * @param ctx The context where the check will be implemented.
 *
 * @return axl_true if log filtering is enabled, otherwise axl_false
 * is returned.
 */ 
axl_bool    vortex_log_filter_is_enabled (VortexCtx * ctx)
{
	char * value;
	v_return_val_if_fail (ctx, axl_false);
	if (! ctx->debug_filter_checked) {
		/* set as checked */
		ctx->debug_filter_checked = axl_true;
		value = vortex_support_getenv ("VORTEX_DEBUG_FILTER");
		vortex_log_filter_level (ctx, value);
		axl_free (value);
	}

	/* return current status */
	return ctx->debug_filter_is_enabled;
}

/** 
 * @brief Allows to get a vortex configuration, providing a valid
 * vortex item.
 * 
 * The function requires the configuration item that is required and a
 * valid reference to a variable to store the result. 
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param item The configuration item that is being returned.
 *
 * @param value The variable reference required to fill the result.
 * 
 * @return The function returns axl_true if the configuration item is
 * returned. 
 */
axl_bool       vortex_conf_get             (VortexCtx      * ctx,
					    VortexConfItem   item, 
					    int            * value)
{
#if defined(AXL_OS_WIN32)

#elif defined(AXL_OS_UNIX)
	/* variables for nix world */
	struct rlimit _limit;
#endif	
	/* do common check */
	v_return_val_if_fail (ctx,   axl_false);
	v_return_val_if_fail (value, axl_false);

	/* no context, no configuration */
	if (ctx == NULL)
		return axl_false;

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
		return axl_true;
#elif defined (AXL_OS_UNIX)
		/* get the limit */
		if (getrlimit (RLIMIT_NOFILE, &_limit) != 0) {
			vortex_log (VORTEX_LEVEL_CRITICAL, "failed to get current soft limit: %s", vortex_errno_get_last_error ());
			return axl_false;
		} /* end if */

		/* return current limit */
		*value = _limit.rlim_cur;
		return axl_true;
#endif		
	case VORTEX_HARD_SOCK_LIMIT:
#if defined (AXL_OS_WIN32)
		/* return the hard sockt limit */
		*value = ctx->__vortex_conf_hard_sock_limit;
		return axl_true;
#elif defined (AXL_OS_UNIX)
		/* get the limit */
		if (getrlimit (RLIMIT_NOFILE, &_limit) != 0) {
			vortex_log (VORTEX_LEVEL_CRITICAL, "failed to get current soft limit: %s", vortex_errno_get_last_error ());
			return axl_false;
		} /* end if */

		/* return current limit */
		*value = _limit.rlim_max;
		return axl_true;
#endif		
	case VORTEX_LISTENER_BACKLOG:
		/* return current backlog value */
		*value = ctx->backlog;
		return axl_true;
	case VORTEX_ENFORCE_PROFILES_SUPPORTED:
		/* return current enforce profiles supported values */
		*value = ctx->enforce_profiles_supported;
		return axl_true;
	case VORTEX_AUTOMATIC_MIME_HANDLING:
		/* configure automatic MIME handling */
		*value = ctx->automatic_mime;
		return axl_true;
	case VORTEX_SKIP_THREAD_POOL_WAIT:
		*value = ctx->skip_thread_pool_wait;
		return axl_true;
	default:
		/* configuration found, return axl_false */
		vortex_log (VORTEX_LEVEL_CRITICAL, "found a requested for a non existent configuration item");
		return axl_false;
	} /* end if */

	return axl_true;
}

/** 
 * @brief Allows to configure the provided item, with either the
 * integer or the string value, according to the item configuration
 * documentation.
 * 
 * @param ctx The context where the configuration will take place.
 *
 * @param item The item configuration to be set.
 *
 * @param value The integer value to be configured if applies.
 *
 * @param str_value The string value to be configured if applies.
 * 
 * @return axl_true if the configuration was done properly, otherwise
 * axl_false is returned.
 */
axl_bool       vortex_conf_set             (VortexCtx      * ctx,
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
	v_return_val_if_fail (ctx,   axl_false);
	v_return_val_if_fail (value, axl_false);

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
			return axl_false;

		/* configure new soft limit */
		ctx->__vortex_conf_soft_sock_limit = value;
		return axl_true;
#elif defined (AXL_OS_UNIX)
		/* get the limit */
		if (getrlimit (RLIMIT_NOFILE, &_limit) != 0) {
			vortex_log (VORTEX_LEVEL_CRITICAL, "failed to get current soft limit: %s", vortex_errno_get_last_error ());
			return axl_false;
		} /* end if */

		/* configure new value */
		_limit.rlim_cur = value;

		/* set new limit */
		if (setrlimit (RLIMIT_NOFILE, &_limit) != 0) {
			vortex_log (VORTEX_LEVEL_CRITICAL, "failed to set current soft limit: %s", vortex_errno_get_last_error ());
			return axl_false;
		} /* end if */

		return axl_true;
#endif		
	case VORTEX_HARD_SOCK_LIMIT:
#if defined (AXL_OS_WIN32)
		/* current it is not possible to configure hard sock
		 * limit */
		return axl_false;
#elif defined (AXL_OS_UNIX)
		/* get the limit */
		if (getrlimit (RLIMIT_NOFILE, &_limit) != 0) {
			vortex_log (VORTEX_LEVEL_CRITICAL, "failed to get current soft limit: %s", vortex_errno_get_last_error ());
			return axl_false;
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
			return axl_false;
		} /* end if */
		
		return axl_true;
#endif		
	case VORTEX_LISTENER_BACKLOG:
		/* return current backlog value */
		ctx->backlog = value;
		return axl_true;

	case VORTEX_ENFORCE_PROFILES_SUPPORTED:
		/* return current enforce profiles supported values */
		ctx->enforce_profiles_supported = value;
		return axl_true;
	case VORTEX_AUTOMATIC_MIME_HANDLING:
		/* configure automatic MIME handling */
		ctx->automatic_mime    = value;
		return axl_true;
	case VORTEX_SKIP_THREAD_POOL_WAIT:
		ctx->skip_thread_pool_wait = value;
		return axl_true;
	default:
		/* configuration found, return axl_false */
		vortex_log (VORTEX_LEVEL_CRITICAL, "found a requested for a non existent configuration item");
		return axl_false;
	} /* end if */

	return axl_true;
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
 * - Acquiring a mutex could make the overall system to act in a
 * different way because the threading is now globally synchronized by
 * all calls done to the log. That is, it may be possible to not
 * reproduce a thread race condition if the log is activated with
 * mutex acquire.
 * 
 * @param ctx The context that is going to be configured.
 * 
 * @param status axl_true to acquire the mutex before logging, otherwise
 * log without locking the context mutex.
 *
 * You can use \ref vortex_log_is_enabled_acquire_mutex to check the
 * mutex status.
 */
void     vortex_log_acquire_mutex    (VortexCtx * ctx, 
				      axl_bool    status)
{
	/* get current context */
	v_return_if_fail (ctx);

	/* configure status */
	ctx->use_log_mutex = axl_true;
}

/** 
 * @brief Allows to check if the log mutex acquire is activated.
 *
 * @param ctx The context that will be required to return its
 * configuration.
 * 
 * @return Current status configured.
 */
axl_bool      vortex_log_is_enabled_acquire_mutex (VortexCtx * ctx)
{
	/* get current context */
	v_return_val_if_fail (ctx, axl_false);
	
	/* configure status */
	return ctx->use_log_mutex;
}

/** 
 * @brief Allows to configure an application handler that will be
 * called for each log produced by the vortex engine.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @param handler A reference to the handler to configure or NULL to
 * disable the notification.
 */
void     vortex_log_set_handler      (VortexCtx        * ctx, 
				      VortexLogHandler   handler)
{
	/* get current context */
	v_return_if_fail (ctx);
	
	/* configure status */
	ctx->debug_handler = handler;
	
	return;
}

/** 
 * @brief Allows to configure an application handler that will be
 * called for each log produced by the vortex engine. This is the same
 * version as \ref vortex_log_set_handler but this allows to configure
 * a handler that will receive the user pointer reference provided
 * here when called.
 *
 * @param ctx The context where the operation will be performed.
 *
 * @param handler A reference to the handler to configure or NULL to
 * disable the notification.
 *
 * @param user_data A reference to user data that is passed to the
 * handler.
 */
void     vortex_log_set_handler_full (VortexCtx             * ctx,
				      VortexLogHandlerFull    handler,
				      axlPointer              user_data)
{
	/* get current context */
	v_return_if_fail (ctx);
	
	/* configure status */
	ctx->debug_handler2 = handler;
	ctx->debug_handler2_user_data = user_data;
	
	return;
}


/** 
 * @brief Allows to instruct vortex to send string logs already
 * formated into the log handler configured (vortex_log_set_handler).
 *
 * This will make vortex to expand string arguments (message and
 * args), causing the argument \ref VortexLogHandler message argument
 * to receive full content. In this case, args argument will be
 * received as NULL.
 *
 * @param ctx The vortex context to configure.
 *
 * @param prepare_string axl_true to prepare string received by debug
 * handler, otherwise use axl_false to leave configured default
 * behaviour.
 */
void     vortex_log_set_prepare_log  (VortexCtx         * ctx,
				      axl_bool            prepare_string)
{
	v_return_if_fail (ctx);
	ctx->prepare_log_string = prepare_string;
}

/** 
 * @brief Allows to get current log handler configured. By default no
 * handler is configured so log produced by the vortex execution is
 * dropped to the console.
 *
 * @param ctx The context where the operation will be performed.
 * 
 * @return The handler configured (or NULL) if no handler was found
 * configured.
 */
VortexLogHandler     vortex_log_get_handler      (VortexCtx * ctx)
{
	/* get current context */
	v_return_val_if_fail (ctx, NULL);
	
	/* configure status */
	return ctx->debug_handler;
}


/** 
 * @internal Internal common log implementation to support several levels
 * of logs.
 * 
 * @param ctx The context where the operation will be performed.
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
	int    use_log_mutex = axl_false;
	char * log_string;
	struct timeval stamp;
	char   buffer[1024];

	/* if not VORTEX_DEBUG FLAG, do not output anything */
	if (! vortex_log_is_enabled (ctx)) {
		return;
	} /* end if */

	if (ctx == NULL) {
		goto ctx_not_defined;
	}

	/* check if debug is filtered */
	if (vortex_log_filter_is_enabled (ctx)) {
		/* if the filter removed the current log level, return */
		if ((ctx->debug_filter & log_level) == log_level)
			return;
	} /* end if */

	/* acquire the mutex so multiple threads will not mix their
	 * log messages together */
	use_log_mutex = ctx->use_log_mutex;
	if (use_log_mutex) 
		vortex_mutex_lock (&ctx->log_mutex);

	if( ctx->debug_handler || ctx->debug_handler2) {
		if (ctx->prepare_log_string) {
			/* pass the string already prepared */
			log_string = axl_strdup_printfv (message, args);

			/* debug handler (if defined) */
			if (ctx->debug_handler)
				ctx->debug_handler (file, line, log_level, log_string, args);
			/* debug handler2 (if defined) */
			if (ctx->debug_handler2)
				ctx->debug_handler2 (ctx, file, line, log_level, log_string, ctx->debug_handler2_user_data, args);
			
			axl_free (log_string);
		} else {
			/* call a custom debug handler if one has been set */
			if (ctx->debug_handler)
				ctx->debug_handler (file, line, log_level, message, args);
			if (ctx->debug_handler2)
				ctx->debug_handler2 (ctx, file, line, log_level, message, ctx->debug_handler2_user_data, args);
		} /* end if */

	} else {
		/* printout the process pid */
	ctx_not_defined:

		/* get current stamp */
		gettimeofday (&stamp, NULL);

		/* print the message */
		vsnprintf (buffer, 1023, message, args);
				
	/* drop a log according to the level */
#if defined (__GNUC__)
		if (vortex_color_log_is_enabled (ctx)) {
			switch (log_level) {
			case VORTEX_LEVEL_DEBUG:
				fprintf (stdout, "\e[1;36m(%d.%d proc %d)\e[0m: (\e[1;32mdebug\e[0m) %s:%d %s\n", 
					 (int) stamp.tv_sec, (int) stamp.tv_usec, getpid (), file ? file : "", line, buffer);
				break;
			case VORTEX_LEVEL_WARNING:
				fprintf (stdout, "\e[1;36m(%d.%d proc %d)\e[0m: (\e[1;33mwarning\e[0m) %s:%d %s\n", 
					 (int) stamp.tv_sec, (int) stamp.tv_usec, getpid (), file ? file : "", line, buffer);
				break;
			case VORTEX_LEVEL_CRITICAL:
				fprintf (stdout, "\e[1;36m(%d.%d proc %d)\e[0m: (\e[1;31mcritical\e[0m) %s:%d %s\n", 
					 (int) stamp.tv_sec, (int) stamp.tv_usec, getpid (), file ? file : "", line, buffer);
				break;
			}
		} else {
#endif /* __GNUC__ */
			switch (log_level) {
			case VORTEX_LEVEL_DEBUG:
				fprintf (stdout, "(%d.%d proc %d): (debug) %s:%d %s\n", 
					 (int) stamp.tv_sec, (int) stamp.tv_usec, getpid (), file ? file : "", line, buffer);
				break;
			case VORTEX_LEVEL_WARNING:
				fprintf (stdout, "(%d.%d proc %d): (warning) %s:%d %s\n", 
					 (int) stamp.tv_sec, (int) stamp.tv_usec, getpid (), file ? file : "", line, buffer);
				break;
			case VORTEX_LEVEL_CRITICAL:
				fprintf (stdout, "(%d.%d proc %d): (critical) %s:%d %s\n", 
					 (int) stamp.tv_sec, (int) stamp.tv_usec, getpid (), file ? file : "", line, buffer);
				break;
			}
#if defined (__GNUC__)
		} /* end if */
#endif
		/* ensure that the log is dropped to the console */
		fflush (stdout);
		
	} /* end if (ctx->debug_handler) */

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
 * @return axl_true if the context was initialized, otherwise axl_false is
 * returned.
 *
 * NOTE: This function is not thread safe, that is, calling twice from
 * different threads on the same object will cause improper
 * results. You can use \ref vortex_init_check to ensure if you
 * already initialized the context.
 */
axl_bool    vortex_init_ctx (VortexCtx * ctx)
{
	int          thread_num;
	int          soft_limit;

	v_return_val_if_fail (ctx, axl_false);

	/**** vortex_io.c: init io module */
	vortex_io_init (ctx);

	/**** vortex.c: init global mutex *****/
	vortex_mutex_create (&ctx->frame_id_mutex);
	vortex_mutex_create (&ctx->connection_id_mutex);
	vortex_mutex_create (&ctx->listener_mutex);
	vortex_mutex_create (&ctx->listener_unlock);
	vortex_mutex_create (&ctx->exit_mutex);
	vortex_mutex_create (&ctx->profiles_list_mutex);
	vortex_mutex_create (&ctx->port_share_mutex);

	/* init channel module */
	vortex_channel_init (ctx);

	/* init connection module */
	vortex_connection_init (ctx);

	/* init profiles module */
	vortex_profiles_init (ctx);

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
	if (! vortex_win32_init (ctx))
		return axl_false;
#endif

	/* init axl library */
	axl_init ();

	/* add default paths */
#if defined(AXL_OS_UNIX)
	vortex_log (VORTEX_LEVEL_DEBUG, "configuring context to use: %p", ctx);
	vortex_support_add_search_path_ref (ctx, vortex_support_build_filename ("libvortex-1.1", NULL ));
	vortex_support_add_search_path_ref (ctx, vortex_support_build_filename (PACKAGE_TOP_DIR, "libvortex-1.1", "data", NULL));
	vortex_support_add_search_path_ref (ctx, vortex_support_build_filename (PACKAGE_TOP_DIR, "data", NULL));
#endif
	/* do not use the add_search_path_ref version to force vortex
	   library to perform a local copy path */
	vortex_support_add_search_path (ctx, ".");
	
	/* init dtds */
	if (!vortex_dtds_init (ctx)) {
		fprintf (stderr, "VORTEX_ERROR: unable to load dtd files (this means some DTD (or all) file wasn't possible to be loaded.\n");
		return axl_false;
	}

	/* before starting, check if we are using select(2) system
	 * call method, to adequate the number of sockets that can
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
			 * privilege user to run the following
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
		return axl_false;
	
	/* init writer subsystem */
	/* vortex_log (VORTEX_LEVEL_DEBUG, "starting vortex writer..");
	   vortex_writer_run (); */

	/* init sequencer subsystem */
	vortex_log (VORTEX_LEVEL_DEBUG, "starting vortex sequencer..");
	if (! vortex_sequencer_run (ctx))
		return axl_false;
	
	/* init thread pool (for query receiving) */
	thread_num = vortex_thread_pool_get_num ();
	vortex_log (VORTEX_LEVEL_DEBUG, "starting vortex thread pool: (%d threads the pool have)..",
		    thread_num);
	vortex_thread_pool_init (ctx, thread_num);

	/* flag this context as initialized */
	ctx->vortex_initialized = axl_true;

	/* register the vortex exit function */
	return axl_true;
}

/** 
 * @brief Allows to check if the provided VortexCtx is initialized
 * (\ref vortex_init_ctx).
 * @param ctx The context to be checked for initialization.
 * @return axl_true if the context was initialized, otherwise axl_false is returned.
 */
axl_bool vortex_init_check (VortexCtx * ctx)
{
	if (ctx == NULL || ! ctx->vortex_initialized) {
		return axl_false;
	}
	return axl_true;
}


/** 
 * @brief Terminates the vortex library execution on the provided
 * context.
 *
 * Stops all internal vortex process and all allocated resources
 * associated to the context. It also close all channels for all
 * connection that where not closed until call this function.
 *
 * This function is reentrant, allowing several threads to call \ref
 * vortex_exit_ctx function at the same time. Only one thread will
 * actually release resources allocated.
 *
 * NOTE: Although it isn't explicitly stated, this function shouldn't
 * be called from inside a handler notification: \ref
 * VortexOnFrameReceived "Frame Receive", \ref VortexOnCloseChannel
 * "Channel close", etc. This is because those handlers works inside
 * the context of the vortex library execution. Making a call to this
 * function in the middle of that context, will produce undefined
 * behaviors.
 *
 * NOTE2: This function isn't designed to dealloc all resources
 * associated to the context and used by the vortex engine
 * function. According to the particular control structure used by
 * your application, you must first terminate using any Vortex API and
 * then call to \ref vortex_exit_ctx. In other words, this function
 * won't close all your opened connections.
 * 
 * @param ctx The context to terminate. The function do not dealloc
 * the context provided. 
 *
 * @param free_ctx Allows to signal the function if the context
 * provided must be deallocated (by calling to \ref vortex_ctx_free).
 *
 * <b>Notes about calling to terminate vortex from inside its handlers:</b>
 *
 * Currently this is allowed and supported only in the following handlers:
 *
 * - \ref VortexOnFrameReceived (\ref vortex_channel_set_received_handler)
 * - \ref VortexConnectionOnCloseFull (\ref vortex_connection_set_on_close_full)
 *
 * The rest of handlers has being not tested.
 */
void vortex_exit_ctx (VortexCtx * ctx, axl_bool  free_ctx)
{
	int            iterator;
	axlDestroyFunc func;

	/* check context is initialized */
	if (! vortex_init_check (ctx))
		return;

	/* check if the library is already started */
	if (ctx == NULL || ctx->vortex_exit)
		return;

	vortex_log (VORTEX_LEVEL_DEBUG, "shutting down vortex library, VortexCtx %p", ctx);

	vortex_mutex_lock (&ctx->exit_mutex);
	if (ctx->vortex_exit) {
		vortex_mutex_unlock (&ctx->exit_mutex);
		return;
	}
	/* flag other waiting functions to do nothing */
	vortex_mutex_lock (&ctx->ref_mutex);
	ctx->vortex_exit = axl_true;
	vortex_mutex_unlock (&ctx->ref_mutex);
	
	/* unlock */
	vortex_mutex_unlock  (&ctx->exit_mutex);

	/* flag the thread pool to not accept more jobs */
	vortex_thread_pool_being_closed (ctx);

	/* stop vortex writer */
	/* vortex_writer_stop (); */

	/* stop vortex reader process */
	vortex_reader_stop (ctx);

	/* stop vortex sequencer */
	vortex_sequencer_stop (ctx);

	/* stop vortex profiles process */
	vortex_profiles_cleanup (ctx);

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
	 * signals, emitted to all threads running (including the pool)
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
	vortex_mutex_destroy (&ctx->listener_mutex);
	vortex_mutex_destroy (&ctx->listener_unlock);
	vortex_mutex_destroy (&ctx->profiles_list_mutex);
	vortex_mutex_destroy (&ctx->port_share_mutex);

	/* release port share handlers (if any) */
	axl_list_free (ctx->port_share_handlers);

	/* lock/unlock to avoid race condition */
	vortex_mutex_lock  (&ctx->exit_mutex);
	vortex_mutex_unlock  (&ctx->exit_mutex);
	vortex_mutex_destroy (&ctx->exit_mutex);

	/* call to activate ctx cleanups */
	if (ctx->cleanups) {
		/* acquire lock */
		vortex_mutex_lock (&ctx->ref_mutex);

		iterator = 0;
		while (iterator < axl_list_length (ctx->cleanups)) {
			/* get clean up function */
			func = axl_list_get_nth (ctx->cleanups, iterator);

			/* call to clean */
			vortex_mutex_unlock (&ctx->ref_mutex);
			func (ctx);
			vortex_mutex_lock (&ctx->ref_mutex);

			/* next iterator */
			iterator++;
		} /* end while */

		/* terminate list */
		axl_list_free (ctx->cleanups);
		ctx->cleanups = NULL; 

		/* release lock */
		vortex_mutex_unlock (&ctx->ref_mutex);
	} /* end if */

#if defined(AXL_OS_WIN32)
	WSACleanup (); 
	vortex_log (VORTEX_LEVEL_DEBUG, "shutting down WinSock2(tm) API");
#endif
   
	/* release the ctx */
	if (free_ctx)
		vortex_ctx_free2 (ctx, "end ctx");

	return;
}

/** 
 * @brief Allows to check if vortex engine started on the provided
 * context is finishing (a call to \ref vortex_exit_ctx was done).
 *
 * @param ctx The context to check if it is exiting.
 *
 * @return axl_true in the case the context is finished, otherwise
 * axl_false is returned. The function also returns axl_false when
 * NULL reference is received.
 */
axl_bool vortex_is_exiting           (VortexCtx * ctx)
{
	if (ctx == NULL)
		return axl_false;
	return ctx->vortex_exit;
}

/* @} */


/** 
 * \mainpage Vortex Library 1.1:  A professional BEEP Core implementation
 *
 * \section intro Introduction 
 *
 * <b>Vortex Library</b> is an implementation of the <b>RFC 3080 / RFC
 * 3081</b> standard definition called the <b>BEEP Core protocol
 * mapped into TCP/IP</b> layer written in <b>C</b>. 
 *
 * Some of its features are: 
 *
 * - Robust and well tested BEEP implementation with a threaded design (non-blocking parallel comunications), written in ANSI C. 
 * - Context based API design making the library stateless. Support to run several execution contexts in the same process.
 * - A complete XML-RPC over BEEP <b>RFC 3529</b> with a IDL/XDL protocol compiler (<b>xml-rpc-gen-1.1</b>).
 * - A complete TUNNEL (<b>RFC3620</b>) support. 
 * - Complete implementation for TLS and SASL profiles.
 * - Modular design which allows to use only those components needed: See \ref vortex_components "vortex components"
 * - Support to proxy BEEP connections through HTTP proxy servers. See \ref vortex_http "Vortex HTTP CONNECT API".
 * - Support for single threaded (no async notification) programming. See \ref vortex_pull "Vortex Pull API" and \ref vortex_manual_pull_api "Notes and how to use PULL API".
 *
 * Vortex Library has been developed by <b>Advanced Software Production
 * Line, S.L.</b> (http://www.aspl.es). It is licensed under the LGPL
 * 2.1 which allows open source and commercial usage.
 *
 * Vortex Library has been implemented keeping in mind security. It
 * has a consistent and easy to use API which will allow you to write
 * application protocols really fast. The API provided is really
 * stable and any change is notified using a public change procedure
 * notification (http://www.aspl.es/change/change-notification.txt).
 *
 * Vortex Library is being run and tested regularly under GNU/Linux
 * and Microsoft Windows platforms, but it is known to work in other
 * platforms. Its development is also checked with a <b>regression test suite</b>
 * to ensure proper function across releases.
 *
 * The following section represents documents you may find useful to
 * get an idea if Vortex Library is right for you. It talks about
 * features, some insights about its implementation and product license.
 *
 * - \ref implementation
 * - \ref features
 * - \ref vortex_components
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
 * - \ref vortex_payload_feeder
 *  
 * </li>
 * <li><b>Vortex Library API profiles already implemented:</b>
 * - \ref vortex_tls
 * - \ref vortex_sasl
 * - \ref vortex_xml_rpc
 * - \ref vortex_xml_rpc_types
 * - \ref vortex_tunnel
 * </li>
 * <li><b>Vortex Library extension API </b>
 *
 * - \ref vortex_pull
 * - \ref vortex_http
 * - \ref vortex_alive
 * - \ref vortex_websocket
 * - \ref vortex_external
 *
 * </li>
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
 * You can reach us at the <b>Vortex mailing list:</b> at <a href="http://lists.aspl.es/cgi-bin/mailman/listinfo/vortex">vortex users</a>
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
 * As an example, on a debian similar system, with deb based packaging,
 * these dependencies can be installed using:
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
 * The <b>BEEP Core</b> definition found at (<b>RFC3080</b> and
 * <b>RFC3081</b>) defines what "must" and "should" be supported by
 * any implementation. At this moment the Vortex Library support all
 * sections including must and should requirements (including TLS and
 * SASL profiles).
 *
 * Vortex Library has been built using asynchronous sockets (non
 * blocking call) with a thread model, each one working on specific
 * tasks. Once the Vortex Library is started it creates 2 threads
 * apart from the main thread doing the following task:
 *
 * - <b>Vortex Sequencer: </b> its main purpose is to create the
 * package sequencing, split user level message into frames and queue
 * them into channel's buffer if no receiving window is available,
 * sending the data when possible in a round robin fashion.  This
 * process allows user space to not get blocked while sending message
 * no matter how big they are.
 *
 * - <b>Vortex Reader: </b> its main purpose is to read all frames for
 * all channels inside all connections. It checks all environment
 * conditions for a frame to be accepted: sequence number sync,
 * previous frame with more fragment flag activated, properly frame
 * format, and so on. Once the frame is accepted, the Vortex Reader
 * try to dispatch it using the \ref vortex_manual_dispatch_schema "Vortex Library frame dispatch schema".
 * 
 * Apart from the 2 threads created plus the main one, the Vortex
 * Library creates a thread pool already prepared to dispatch incoming
 * frames inside user space function.  The thread pool size can also
 * be tweaked.
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
 * channel starvation due to flooding attacks. 
 *
 * - It <b>supports both asynchronous and synchronous programing
 * models</b> while sending and receiving data. This means you can
 * program a handler to be executed on frame receive event (and keep
 * on doing other things) or write code that gets blocked until a
 * specific frame is received. This also means you are not required to use handler
 * or event notifications when you just want to keep on waiting for an
 * specific frame to be received. Of course, while programing
 * Graphical User Interfaces the asynchronous model will allow you to
 * get the better performance avoiding GUI blocking effect on requests ;-)
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
 * <b>xml-rpc-gen-1.1</b> which allows to produce server and client
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
 * License</b> allowing to create open source and proprietary
 * projects. See this \ref license "section" for more information
 * about license topic.
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
 * implementation (RFC3529) with a protocol compiler (xml-rpc-gen-1.1) and
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
 * You can also use Vortex Library through its XML-RPC API to produce
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
 * \page license Vortex Library License
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
 * \section license_about_static_linking About statically linking Vortex Library
 *
 * Statically linking Vortex Library, GSASL
 * (http://www.gnu.org/software/gsasl/), LibAxl
 * (http://www.aspl.es/xml), noPoll (http://www.aspl.es/nopoll) or any
 * other component based on GPL/LGPL <b>is strictly forbidden by the
 * license</b> unless all components taking part into the final
 * products are all of them GPL, LGPL, MIT, Bsds, etc, or similar
 * licenses that allow an end user or the customer to download the
 * entire product source code and clear instructions to rebuild it.
 *
 * If the library is being used by a proprietary product the only
 * allowed option is dynamic linking (so final user is capable of
 * updating that dynamic linked part) or a practical procedure where
 * the propritary binary object along with the instructions to relink
 * the LGPL part (including an update or modification of it) is
 * provided.
 * 
 * An end user or customer using a product using LGPL components must
 * be able to rebuild those components by introducing updates or
 * improvements.
 * 
 * Thus, statically linking a LGPL components without considering
 * previous points takes away this user/customer right because he/she
 * cannot replace/update that LGPL component anymore unless you can
 * have access to the whole solution.
 *
 * - See more about this at:  https://www.gnu.org/licenses/lgpl-2.1.html
 * - Also at: http://stackoverflow.com/questions/10130143/gpl-lgpl-and-static-linking
 * - LGPL 2.1 Guide: http://copyleft.org/guide/comprehensive-gpl-guidech11.html
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
 *         C/ Antonio Suarez NÂº 10, 
 *         Edificio Alius A, Despacho 102
 *         AlcalÃ¡ de Henares 28802 (Madrid)
 *         Spain
 *
 *      Email address:
 *         info@aspl.es - http://www.aspl.es/vortex
 *      Fax and Telephones:
 *         (00 34) 91 669 55 32 - (00 34) 91 231 44 50
 *         From outside Spain must use (00 34) prefix.
 * \endcode
 * 
 *
 */

/**
 * \page vortex_components Vortex Library components 
 *
 * \section Votex Library base library and its extensions
 *
 * One feature of Vortex Library 1.1 series is it's new module design separating the base core
 * BEEP library from other extensions that may be used independently. This has two main improvements from previous 1.0 design:
 * 
 * - Now it is posible to use only those component required, avoiding
 * to include all vortex code
 *
 * - Now it is possible to add different extensions without affecting
 * the base library. For example now it is possible to implement a
 * different TLS implementation or evolve some component without
 * requiring to release all the library.
 *
 * Vortex Library has the following components:
 * 
 * \image html vortex-components.png "Vortex base library and extension libraries"
 *
 * \section extensions_headers One header for the base library and each extension library
 *
 * Now each library must be used and included explicitly by the
 * developer. For example, to use base library, sasl and tls
 * implementation will require:
 *
 * \code
 * // base library
 * #include <vortex.h>
 * 
 * // include TLS header
 * #include <vortex_tls.h>
 *
 * // include SASL header
 * #include <vortex_sasl.h>
 *
 * // rest of available libraries with its associated header
 * // to include its function
 * #include <vortex_xml_rpc.h>
 * #include <vortex_tunnel.h>
 * #include <vortex_http.h>
 * #include <vortex_pull.h>
 *
 * \endcode
 *
 * In general, the naming convention to follow is:
 * \code
 * #include <vortex_EXTENSION.h>
 * \endcode
 * 
 * \section pkg_config_support Pkg-config support for each extension
 *
 * Now each extension provides its own pkg-config file. The following
 * is the list of available pkg-config files included:
 *
 * - vortex-1.1
 * - vortex-tls-1.1
 * - vortex-sasl-1.1
 * - vortex-xml-rpc-1.1
 * - vortex-tunnel-1.1
 * - vortex-pull-1.1
 * - vortex-http-1.1
 * 
 * For example, if you are using autoconf, you can use the following
 * example to include support for vortex base library and vortex http
 * CONNECT support:
 *
 * \code
 * dnl check for vortex
 * PKG_CHECK_MODULES(VORTEX, vortex-1.1 >= 1.1.0 vortex-http-1.1 >= 1.1.0)
 * AC_SUBST(VORTEX_CFLAGS)
 * AC_SUBST(VORTEX_LIBS)
 * \endcode
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
 *   >> svn co https://dolphin.aspl.es/svn/publico/af-arch/trunk/libvortex-1.1
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
 * \section compile_linux Compiling Vortex Library on GNU/linux environments (including Cygwin, mingw)
 *
 * If you are running a POSIX (unix-like including cygwin,mingw) environment you can use
 * autotools to configure the project so you can compile it. Type the
 * following once you have downloaded the source code.
 *
 * <b>From SVN:</b>
 *
 * \code
 *   >> svn co https://dolphin.aspl.es/svn/publico/af-arch/trunk/libvortex-1.1
 *   >> cd libvortex-1.1
 *   >> ./autogen.sh
 *   >> make clean
 *   >> make 
 * \endcode
 *
 * <b>From a vortex .tar.gz file:</b>
 *
 * \code
 *   >> wget http://www.aspl.es/vortex/downloads/vortex-1.1.14.b5292.g5292.tar.gz
 *   >> tar xzvf vortex-1.1.14.b5292.g5292.tar.gz
 *   >> cd vortex-1.1.14.b5292.g5292
 *   >> ./configure
 *   >> make clean
 *   >> make 
 * \endcode
 *
 * This will configure your project trying to find the dependencies
 * needed. 
 *
 * Once the configure process is done you can type: 
 * 
 * \code
 *   bash:~/libvortex-1.1$ make install
 * \endcode
 *
 * The previous command will require permissions to write inside the
 * destination directory. If you have problems, try to execute the
 * previous command as root.
 *
 * Because readline doesn't provide a standard way to get current
 * installation location, the following is provided to configure
 * readline installation. You have to use the <b>READLINE_PATH</b>
 * environment var as follows:
 * 
 * \code
 *   bash:~/libvortex-1.1$ READLINE_PATH=/some/path/to/readline make
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
 *  CC=gcc-3.3 vortex_dll=libvortex-1.1 MODE=console make -f Makefile.win
 * \endcode
 *
 * Of course, the <b>CC</b> variable may point to another gcc, check the one
 * that is installed on your system but, make sure you are not using
 * the gcc provided by a cygwin/mingw installation. It will produce a faulty
 * libvortex-1.1.dll not usable by any native Microsoft Windows program.
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
 *  CC=gcc-3.3 vortex_dll=libvortex-1.1 MODE=console BASE_DIR=c:/libraries make -f Makefile.win
 * \endcode
 *
 * This process will produce a libvortex-1.1.dll (actually the dynamic
 * libraries) and a import library called libvortex-1.1.dll.a. The import
 * library will be needed to compile your application under windows
 * against Vortex Library so it get linked to libvortex-1.1.dll.
 * 
 * \section using_linux Using Vortex on GNU/Linux platforms (including Cygwin/Mingw)
 * 
 * Once you have installed the library you can type: 
 * \code
 *   gcc `pkg-config --cflags --libs vortex-1.1` your-program.c -o your-program
 * \endcode
 *
 * On windows platform using cygwin/mingw the previous example also
 * works. 
 *
 * \section using_mingw Using Vortex on Microsoft Windows with Mingw
 *
 * On mingw environments you should use something like:
 *
 * \code
 *  gcc your-program.c -o your-program
 *        -Ic:/libraries/include/vortex-1.1/  \
 *        -I"c:/libraries/include" \
 *        -I"c:/libraries/include/axl" \
 *        -L"c:/libraries/lib" \
 *        -L"c:/libraries/bin" \
 *        -lws2_32 \
 *        -laxl -lm 
 * \endcode
 *
 * Where <b>c:/libraries</b> contains the installation of the Vortex
 * Library (headers files installed on c:/libraries/include/vortex-1.1,
 * import library: libvortex-1.1.dll.a and dll: libvortex-1.1.dll) and LibAxl installation.
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
 *  - \ref vortex_manual_configuring_serverName
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
 *  - \ref vortex_manual_transfering_files
 *  - \ref vortex_manual_http_support
 *  - \ref vortex_manual_pull_api
 *  - \ref vortex_manual_alive_api
 *  - \ref vortex_manual_feeder_api
 *  - \ref vortex_manual_external_api
 *
 *  <b>Section 5: </b>Securing and authenticating your BEEP sessions: TLS and SASL profiles
 * 
 *  - \ref vortex_manual_securing_your_session
 *  - \ref vortex_manual_creating_certificates
 *  - \ref vortex_manual_using_sasl
 *  - \ref vortex_manual_sasl_for_client_side
 *  - \ref vortex_manual_sasl_for_server_side
 *
 * <b>Appendix: other documents to consider</b>
 *
 *  - \ref vortex_android_usage
 * 
 * \section vortex_manual_concepts 1.1 Some concepts before starting to use Vortex
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
 * Now we know most of the concepts involving BEEP, here goes how
 * these concepts get mapped into a concrete example using
 * Vortex. Keep in mind this is a simplified version on how Vortex
 * Library could be.
 *
 * We we have to do first, is to create a context. This object will be
 * used by your application to keep the state and the current
 * configuration. This is done as follow:
 * 
 * \code
 *       VortexCtx * ctx;
 *
 *       // create an empty context 
 *       ctx = vortex_ctx_new ();
 *
 *       // do your required configuration here
 *
 *       // init the context and start vortex library execution 
 *       if (! vortex_init_ctx (ctx)) {
 *		// handle error 
 *              return -1;
 *       }
 * \endcode
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
 *        VortexConnection * connection = vortex_connection_new (ctx, host, port, NULL, NULL);
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
 *      // finally, terminate vortex context execution running
 *      vortex_exit_ctx (ctx, axl_true);
 * \endcode
 * 
 *
 * That's all. You have created a simple Vortex Library client that
 * have connected, created a channel, send a message, close the
 * channel and terminated Vortex Library function.
 * 
 * \section vortex_manual_listener 1.2 How a Vortex Listener works (or how to create one)
 *
 * To create a vortex listener, which waits for incoming beep connection
 * on a given port the following must be done:
 * 
 * \code  
 *   #include <vortex.h>
 *
 *   // vortex global context 
 *   VortexCtx * ctx = NULL;
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
 *
 *       // create an empty context 
 *       ctx = vortex_ctx_new ();
 *
 *       // init the context
 *       if (! vortex_init_ctx (ctx)) {
 *           printf ("failed to init the library..\n");
 *       } 
 *
 *       // register a profile
 *       vortex_profiles_register (ctx, SOME_PROFILE_URI,	
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
 *       vortex_listener_new (ctx, "0.0.0.0", "44000", on_ready);
 * 
 *       // wait for listeners
 *       vortex_listener_wait (ctx);
 *
 *       // finalize vortex running
 *       vortex_exit_ctx (ctx, axl_false);
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
 *   vortex_init_ctx. If \ref vortex_init_ctx function is not called,
 *   unexpected behaviors will happen.</li>
 *
 *   <li>Register one (or more profiles) the listener being
 *      created will accept as valid profiles to create new channels over
 *      session using \ref vortex_profiles_register.</li>
 *
 *   <li>Create the listener using \ref vortex_listener_new, specifying the hostname to be used to listen
 *      incoming connection and the port. If hostname used is 0.0.0.0 all
 *      hostname found will be used. If 0 is used as port, an automatic
 *      port assigned by the OS's tcp stack will be used.</li>
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
 * Listener agree to create the channel. If start handler returns axl_true
 * the channel will be created, otherwise not.
 * 
 * If you don't define a start handler a default one will be used
 * which always returns axl_true. This means: all channel creation
 * petition will be accepted.
 * 
 * The \ref VortexOnCloseChannel "close handler" works the same as
 * start handler but this time to notify if a channel can be
 * closed. Again, if close handler returns axl_true, the channel will be
 * closed, otherwise not.
 *
 * If you don't provide a close handler, a default one will be used,
 * which always returns axl_true, that is, all channel close petition will be
 * accepted.
 * 
 * The \ref VortexOnFrameReceived "frame received handler" is executed
 * to notify a new frame has arrived over a particular channel. The
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
 * \section vortex_manual_client 1.3 How a vortex client works (or how to create a connection)
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
 *     VortexCtx        * ctx;
 *
 *     // create an empty context 
 *     ctx = vortex_ctx_new ();
 *
 *     // init the context
 *     if (! vortex_init_ctx (ctx)) {
 *           printf ("failed to init the library..\n");
 *     } 
 * 
 *     // connect to remote vortex listener
 *     connection = vortex_connection_new (ctx, host, port, 
 *                                         // do not provide an on_connected_handler 
 *                                         NULL, NULL);
 * 
 *     // check if everything is ok
 *     if (!vortex_connection_is_ok (connection, axl_false)) {
 *            printf ("Connection have failed: %s\n", 
 * 		    vortex_connection_get_message (connection));
 *            vortex_connection_close (connection);
 *     }
 *   
 *     // connection ok, do some stuff
 *     
 *
 *     // and finally call to terminate vortex
 *     vortex_exit_ctx (ctx, axl_true);
 *   }
 *   
 * \endcode
 * 
 * Previous steps stands for:
 *   <ul>
 *   
 *   <li> Initialize Vortex Library calling to \ref vortex_init_ctx. As we have
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
 * \section vortex_manual_sending_frames 2.1 How an application must use Vortex Library to send and receive data
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
 * \section vortex_manual_dispatch_schema 2.2 The Vortex Library Frame receiving dispatch schema (or how incoming frames are read)
 *
 * Once a frame is received and validated, the Vortex Reader tries to
 * deliver it following next rules:
 *
 * - Invoke a second level handler for frame receive event. The second
 * level handler is a user space callback, optionally defined for each channel. Several
 * channel on the same connections may have different second level
 * handlers (or the same) according to its purpose.
 *
 * - If second level handler for frame receive event were not defined,
 * the Vortex Reader tries to dispatch the frame on the first level
 * handler, which is defined at profile level. This means that
 * channels using the same profiles shares frame receive
 * callback. This allows to defined a general purpose callback at user
 * space for every channel created on every connection.
 *
 * - Finally, it may happen that a thread wants to keep on waiting for
 * a specific frame to be received bypassing the second and first
 * level handlers. Its is useful for that batch process that can't
 * continue if the frame response is not received. This is called \ref vortex_manual_wait_reply "wait reply method".
 *
 * The second and first level handler dispatching method are called
 * asynchronous methods because allows user code to keep on doing
 * other things and to be notified only when frames are received.
 *
 * The wait reply method is called synchronous dispatch because it
 * blocks the caller thread until the specific frame reply is
 * received. The wait reply method disables the second and first level
 * handler execution.
 *
 * Having not defined a second or first level handler or wait reply
 * method will cause the Vortex Reader to drop the frame.
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
 * \section vortex_manual_printf_like 2.3 Printf like interface while sending messages and replies
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
 * \section vortex_manual_wait_reply 2.4 Sending data and wait for a specific reply (or how get blocked until a reply arrives)
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
 *    printf ("my reply has arrived: (size: %d):\n%s", 
 *             vortex_frame_get_payload_size (frame), 
 *             vortex_frame_get_payload (frame));
 *
 *    // that's all!
 * 
 *  \endcode
 * 
 * 
 * \section vortex_manual_invocation_chain 2.5 Invocation level for frames receive handler
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
 * \section vortex_manual_configuring_serverName 2.6 Controlling and configuring serverName value 
 *
 * BEEP provides support for serverName indication. This, like Host:
 * header in HTTP and similar protocols allows a listener peer to
 * provide different configurations, quotas, certificates and policies
 * (to name some).
 *
 * Here is how the serverName value is communicated through BEEP
 * channels to ensure both ends know what serverName to applying in
 * all cases, for example, to select the right certificate, apply some
 * policy, etc...
 *
 * Main points to consider about how serverName is handled with BEEP
 * are:
 *
 * - serverName value applies globally to the entire BEEP
 * session. This means you cannot have a set of channels running with
 * a serverName and another set running with a different value.
 *
 * - serverName value is setup on the first successfully accepted
 * channel created by any of the peers.
 *
 * This means that when you create a BEEP connection
 * (\ref vortex_connection_new or similar) the serverName is still not
 * configured.
 *
 * Once you are connected, the first channel opened will setup the
 * serverName for that session.
 *
 * To control this and have a consistent value, you can use different methods:
 *
 * <ol>
 * <li>Configure x-serverName header doing something like this:
 * \code
 *
 *	conn = vortex_connection_new_full (peer_address, peer_port,
 *				           CONN_OPTS (VORTEX_SERVERNAME_FEATURE, "the-server.name.youwant.com", VORTEX_OPTS_END),
 *					   NULL, NULL);
 * \endcode
 * ...this ensure that this is the serverName value to use for any channel created
 * inside this connection.
 * </li>
 *
 * <li>You can also use \ref VORTEX_SERVERNAME_ACQUIRE to make the
 * connection to do something similar like VORTEX_SERVERNAME_FEATURE
 * but taking the serverName information from the host address use to
 * create the \ref VortexConnection.</li>
 *
 * <li>Use serverName parameter at \ref vortex_channel_new_full to ensure the value
 * is consistent across all channels. </li>
 *
 * <li>Again, the first channel opened inside the connection configuring the serverName
 * will bind that connection to that serverName. Subsequent calls to configure a different
 * serverName will be ignored (\ref vortex_channel_new_full). </li>
 * </ol>
 * 
 * \section vortex_manual_profiles 3.1 Defining a profile inside Vortex (or How profiles concept confuse people)
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
 * the Vortex Library to fulfill the profile specification.
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
 * \section vortex_manual_piggyback_support 4.1 Using piggyback to save one round trip at channel startup
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
 *  axl_bool      extended_start_channel (char              * profile,
 *                                        int                 channel_num,
 *                                        VortexConnection  * connection,
 *                                        char              * serverName,
 *                                        char              * profile_content,
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
 *      return axl_true;
 * }
 * \endcode
 * 
 * Piggyback reply processing for client side is more simple. We have two
 * cases:
 * <ul>
 *
 * <li>1) If the channel creation request was performed by providing a
 * \ref VortexOnChannelCreated application programmer doesn't need to
 * do any special operation, it will receive piggyback reply as the
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
 * \section vortex_manual_using_mime 4.2 Using MIME configuration for data exchanged under Vortex Library
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
 * using? Again, this only depends on the requirements of the
 * application protocol, mainly because MIME is just an indicator. 
 *
 * In any case, any message exchanged by a BEEP peer must be a
 * conforming MIME message. This implies that at least the empty MIME
 * meader must be appended to each message sent. 
 *
 * For example, you if you send the following message: "test", you or
 * your BEEP toolkit must take care of adding the CR+LF prefix:
 * 
 * \code
 * MSG 3 1 . 11 6
 * 
 * testEND
 * \endcode
 *
 * So, the remote BEEP peer will receive a message with empty MIME headers: CR+LF + "test". 
 *
 * <h3>4.2.1 When should I consider using MIME for a BEEP profile?</h3>
 *
 * First of all, no matter how you design your profile, MIME will stay
 * at the core of BEEP, and therefore inside your profile. You can
 * ignore its function and nothing will happen (beyond its basic
 * implications that you must consider).
 *
 * Now, if you pretend to develop a profile that is able to transport
 * <b>everything</b> without previous knowledge (both peers can't make
 * assumptions about the content to be received), it is likely MIME is
 * required. Think about using the more appropiate helper program to
 * open the content received: PNG files, PDF or a C# assembly.
 *
 * Because MIME is implemented inside Vortex in a way you access to
 * the content (MIME body) and MIME headers (\ref
 * vortex_frame_get_mime_header if defined) in a separated way, it
 * becomes an interesting mechanism to allow extending your profile
 * without modifying its content. 
 *
 * As a conclusion: if your system will be the message producer and
 * the message consumer at the same time, you can safely ignore MIME
 * (but Vortex will produce MIME compliat messages for you, see \ref vortex_channel_set_automatic_mime), because
 * you can make assumptions about the kind of messages to be
 * exchanged. However, if a third party software is required to be
 * supported, that is initially unknown, or it is required to have some flexible
 * mechanism to notify "additional" information along with your
 * profile messages, you'll need MIME.
 *
 * <h3>4.2.2 How is MIME implemented inside Vortex Library</h3>
 *
 * The BEEP protocol definition states that all messages exchanged are
 * MIME objects, that is, arbitrary user application data, that have a
 * MIME header which configures/specify the content. MIME support
 * implemented inside Vortex Library is only structural, that is, it
 * implements MIME structure requirements defined at RFC 2045.
 *
 * Initially, if you send a message, without using MIME, because you
 * didn't configure anything, then frames generated won't include any
 * MIME header. <b><i>However even in this case, the MIME body start
 * indicator (CR+LF) will be added</i></b>, to allow remote BEEP peer to
 * detect the MIME header (nothing configured) and the MIME body (your
 * message).
 *
 * For example, if you send message "test" (4 bytes) and no MIME
 * header is configured at any level, it is required to send the
 * following:
 * 
 * \image html mime-structure.png "MIME struct overview and how it applies to a message without MIME headers"
 *
 * That is, even if you do not pay attention to MIME, your messages
 * will still include an inicial CR+LF appended to your message to
 * indicate the remote side no MIME header is defined and to make your
 * message MIME parseable.
 *
 * <h3>4.2.3 Implicit MIME headers to all messages without MIME information</h3>
 * 
 * <b>BEEP assume a MIME implicit configuration</b>, which have the
 * following values for those messages that do not configured
 * "content-type" and "content-transfer-encoding":
 *
 * \code
 *        Content-Type: application/octet-stream
 *        Content-Transfer-Encoding: binary
 * \endcode
 *
 * <h3>4.2.4 How can I access MIME information on a frame received?</h3>
 *
 * First of all, in order to complete MIME support, you must have
 * automatic frame joining activated (\ref
 * vortex_channel_set_complete_flag). This is by default activated. In
 * the case you are taking full control on frames received you must
 * take care of MIME structure parsing by other means. You still can
 * use \ref vortex_frame_mime_process function, but the function will
 * require a frame that contains all the MIME message to work.
 *
 * Assuming you did receive a complete frame with a MIME message, you
 * can access to the <b>MIME body by calling to</b>: \ref vortex_frame_get_payload. 
 * 
 * To access all the message received, including MIME headers and MIME
 * body separator, use \ref vortex_frame_get_content.
 *
 * You can use \ref vortex_frame_get_mime_header to access all MIME
 * headers found on the message received (stored on the frame). 
 *
 * Because MIME could allow to have several instances for the same
 * MIME header, \ref vortex_frame_get_mime_header returns a structure (\ref VortexMimeHeader)
 * that allows to get the content of the MIME header (\ref
 * vortex_frame_mime_header_content) but it also allows to get the
 * next instance found for the same MIME header by using \ref
 * vortex_frame_mime_header_next.
 *
 * 
 *
 * <h3>4.2.5 Automatic MIME configuration for outgoing messages</h3>
 *
 * The following set of function allows to configure (and check) the
 * values to be configured, on each message sent, for the MIME
 * headers: "Content-Type" and "Content-Transfer-Encoding", in an
 * automatic manner:
 * 
 * - \ref vortex_profiles_set_mime_type
 * - \ref vortex_profiles_get_mime_type
 * - \ref vortex_profiles_get_transfer_encoding
 *
 * However, this mechanism doesn't fit properly if it is required to
 * send arbitrary MIME objects (with diffent MIME headers) under the
 * same profile, because previous configuration will append the same
 * MIME information to every message being sent via:
 * 
 * - \ref vortex_channel_send_msg
 * - \ref vortex_channel_send_msgv
 * - \ref vortex_channel_send_msg_and_wait
 * - \ref vortex_channel_send_rpy
 * - \ref vortex_channel_send_rpyv
 * - \ref vortex_channel_send_err
 * - \ref vortex_channel_send_errv
 * - \ref vortex_channel_send_ans_rpy
 * - \ref vortex_channel_send_ans_rpyv
 *
 * In the case no MIME configuration is found for the profile,
 * previous functions will prepend the MIME header separator "CR+LF"
 * on each message sent. This allows to produce MIME compliant
 * messages that have an empty MIME header configuration.
 *
 * <h3>4.2.5 Disabling automatic MIME configuration for outgoing messages</h3>
 *
 * Under some situations it is required to send already configured MIME
 * objects through the set of functions previously described. Because
 * those functions will automatically add an empty MIME header (CR+LF)
 * to each message sent, is required to disable this behavior to avoid
 * breaking message MIME configuration. 
 * 
 * This is done using the following set of functions. They work at
 * library, profile and channel level, having preference the channel
 * level. In order of preference:
 *
 * - \ref vortex_channel_set_automatic_mime 
 * - \ref vortex_channel_get_automatic_mime 
 * - \ref vortex_profiles_set_automatic_mime
 * - \ref vortex_profiles_get_automatic_mime
 * - \ref vortex_conf_set (\ref VORTEX_AUTOMATIC_MIME_HANDLING)
 *
 * For example, disabling automatic MIME handling at profile level
 * while cause Vortex Engine to not append any MIME header (including
 * the body separator) to messages sent over a channel running a
 * particular profile:
 * 
 * \code
 * // disable MIME automatic headers for the following profile
 * vortex_profiles_set_automatic_mime ("urn:beep:some-profile", 2);
 * \endcode
 *
 * Special attention is required to the following code because it
 * doesn't disable MIME handling but deffer the decision to the global
 * library configuration, which is by default activated:
 *
 * \code
 * // signal to use library current configuration
 * vortex_profiles_set_automatic_mime ("urn:beep:some-profile", 0);
 * \endcode
 *
 * In any case, if the Vortex Engine finds the MIME automatic headers
 * disabled, it will take/send messages received "as is", being the
 * application level the responsible of producting MIME compliant
 * messages.
 *
 * <i><b>NOTE:</b> If the channel MIME handling (\ref
 * vortex_channel_set_automatic_mime) isn't configured (\ref
 * vortex_channel_get_automatic_mime returns 0) but the profile level
 * (\ref vortex_profiles_get_automatic_mime) or library level (\ref
 * vortex_conf_get \ref VORTEX_AUTOMATIC_MIME_HANDLING) are
 * configured, then the configuration is copied into the channel. This
 * is done to improve library performance.</i>
 * 
 * <h3>4.2.6 Default configuration for automatic MIME header handling. </h3>
 *
 * By default only library level comes activates (\ref vortex_conf_set
 * \ref VORTEX_AUTOMATIC_MIME_HANDLING). This means that, without any
 * configuration, all channels created will automatically add a MIME
 * header for each message sent.
 *
 * <i><b>NOTE:</b> In the case no configuration is found on every level (0 is returned
 * at \ref vortex_channel_get_automatic_mime, \ref
 * vortex_profiles_get_automatic_mime, \ref vortex_conf_get \ref
 * VORTEX_AUTOMATIC_MIME_HANDLING), then it is assumed automatic MIME
 * handling is activated.</i>
 *
 * <h3>4.2.7 What happens if a wrong MIME formated message is received. </h3>
 *
 * From a frame perspective (BEEP framing mechanism), the vortex engine
 * considers as valid frames all of them as long as BEEP framing
 * rules are observed. 
 *
 * From a MIME perspective, which is considered on top of the BEEP
 * framing mechanism, if the message inside a \ref VortexFrame is not
 * MIME ready, the content returned by the function \ref
 * vortex_frame_get_content and \ref vortex_frame_get_payload will be
 * same, that is, <b><i>the message received is returned untouched</i></b>. 
 *
 * Obviously, under this situation, all API that belongs to the MIME
 * support will provide no function:
 * 
 * - For example, \ref vortex_frame_get_mime_header and \ref
 * vortex_frame_get_mime_header_size will provide NULL and 0 content.
 *
 * So, <b><i>if a message not conforming MIME rules is received,
 * Vortex won't discard it, and it will be delivered "as is" to frame
 * delivery handlers defined.</i></b> Under this situation, a log will
 * be reported to signal that a MIME parse error was found:
 * \code
 * (proc 25045): (warning) vortex-reader: failed to update MIME status for the frame, continue delivery
 * \endcode
 *
 * <h3>4.2.8 Could new MIME support break compatibility with previous Vortex Library? </h3>
 *
 * It is possible under some situations. Before Vortex Library 1.0.15
 * and its corresponding 1.1.0 release, a bug was fixed in the way
 * messages was produced. In the case no MIME header was configured,
 * the message produced wasn't prefixed by a CR+LF pair. This is wrong
 * since the remote BEEP peer expects to receive a MIME compliant
 * message, with at least the CR+LF to signal that no MIME header was
 * configured.
 *
 * In any case, if problems are found, these are the solutions:
 *
 * 1) The obvious solution is to upgrade both (client and server)
 * peers to support same MIME implementation.
 *
 * 2) In the case you want to only update your BEEP client but still
 * connect to Vortex peers running 1.0.14 or previous, you have to
 * disable automatic MIME header handling. See previous sections. A
 * direct hack to disable it globally could be:
 *
 * \code
 * // library level desactivation for automatic MIME header 
 * vortex_conf_set (ctx, VORTEX_AUTOMATIC_MIME_HANDLING, 2, NULL);
 * \endcode
 *
 * 3) In the case you want to only upgrade your BEEP listener, but you still want
 * to receive connections from old and new clients, nothing special is
 * required. This is automatically supported by new vortex engine.
 *
 * No other compatibility issue is reported.
 *
 * \section vortex_manual_implementing_request_response_pattern 3.2 Implementing the request-response pattern
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
 * The key concept here is to ensure that every message reply is
 * processed by the right frame receive handler. This could be
 * accomplish using several techniques:
 *
 * <ul>
 * <li>If you are using a request/response pattern, you have to know
 * that several request/response interactions can be under the same
 * channel but each one will block the next (causing <b>head-of-line</b>). 
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
 * channel. The question is that this have a really poor
 * performance. Using this technique your program will do for every
 * request:
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
 * In this context it is recommended to use the \ref
 * vortex_channel_pool "Channel Pool" feature which will allow you to
 * avoid race conditions while getting a channel that is ready. It will also
 * negotiate for you a new channel to be created if the channel pool
 * doesn't have any channel in the ready state.
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
 *      result = vortex_channel_pool_get_next_ready (pool, axl_true);
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
 * \section vortex_manual_changing_vortex_io 3.3 Configuring Vortex Library IO layer
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
 * the presence of select(2), poll(2) and epoll(2) system call,
 * selecting the best mechanism to be used by default. However, you
 * can change to the desired mechanism at run time (even during
 * transmission!) using:
 * 
 * - \ref vortex_io_waiting_use, providing the appropriate value for \ref VortexIoWaitingType.
 *
 * - Use \ref vortex_io_waiting_get_current to know which is the I/O
 * mechanism currently installed.
 *
 * Previous functions, provides a built-in mechanism to select already
 * implemented mechanism. In the case you want to provide your own
 * user space implementation to handling I/O waiting, you can use the
 * following handlers to define the functions to be executed by the
 * Vortex I/O engine at the appropriate time:
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
 * \section vortex_manual_transfering_files 4.3 General considerations about transfering files
 *
 * There are several methods that can be used to transfer a file. They
 * differ in the way they consume memory and how difficult they are to
 * properly implement them. These methods are:
 *
 * - <b>FIRST METHOD:</b> Sending the file in one single MSG frame.
 * - <b>SECOND METHOD:</b> Sending the file in a set of MSG frames that are composed by the remote side.
 * - <b>THIRD METHOD:</b> Sending the file in a set of ANS frames that are composed by the remote side.
 *
 * The first method (sending one big single MSG) it's the most
 * simple. It is not required to split the file and assamble it in the
 * remote side. 
 * 
 * However, this method consumes a lot of memory because in general
 * terms you must load all the file into memory, them pass it to the
 * Vortex API, which also does its own copy, now you have twice memory
 * loaded and them the memory is retained until the message is
 * completely sent in smallers MSG frames. 
 *
 * In the same direction, people using this method usually do not
 * configure \ref vortex_channel_set_complete_flag which causes all
 * frames conforming the big message to be hold into memory at the
 * remote side until the last frame is received. In this case, once
 * the last message is received, Vortex will allocate enough memory to
 * consolidate all frames into one single content. Again, more memory
 * consumed.
 *
 * So, <b>FIRST METHOD</b> is easy, but <b>really poor in performance terms</b>.
 *
 * <i><b>NOTE:</b> the fact that vortex has the hability to split your
 * messages into the allowed remote channel window size is at the same
 * time a valuable feature and a source of problems. This automatic
 * splitting makes more easy to do not care about content size
 * sent. So, as a general rule, try to control the size of the content
 * sent.</i>
 * 
 *
 * <b>SECOND METHOD</b> involves the developer in the process of preparing
 * the content to be sent, and to take advantage of local store. In
 * this method, the sender open the file and reads chunks of
 * 2k/4k/8k/12k/16k, and send them by using MSG frames.
 * 
 * This makes memory consumption to be lower than previous case
 * because the entire file isn't loaded and, as the transfer progress,
 * the remote side can consolidate all chunks received directly into a
 * file rather holding it into memory. 
 *
 * This method also requires that \ref VortexOnFrameReceived "frame received" 
 * activation to be serialized because you have to place all
 * pieces received in other. This is archived by using \ref
 * vortex_channel_set_serialize.
 *
 * This method is more difficult but results are better. The same
 * happens to the following method.
 *
 * <b>THIRD METHOD</b> involves doing pretty much the same like SECOND
 * METHOD, but using ANS/NUL (one-to-many) exchange style.
 *
 * Because BEEP requires all issued MSG to be replied, in the second
 * method each MSG sent requires the receiving side to reply with a
 * RPY (usually empty). This is not required with ANS/NUL exchange
 * style.
 *
 * In this case, we ask the receiving side to issue a MSG request
 * (asking for download the file). Then, the sending side opens the
 * file to be transferred and send its pieces by using ANS messages.
 *
 * The receiving side consolidates into file all pieces received
 * without requiring to reply to each ANS message received. 
 *
 * <b>THIRD METHOD</b> is the best in terms of memory consuption and
 * network efficiency.
 *
 * We have also to consider other key factors for an effective and
 * fast transfer. In general they are two: window size and frame fragmentation.
 *
 * The first concept (<b>window size</b>) is part of the BEEP way to
 * do channel flow control (see
 * http://www.beepcore.org/seq_frames.html). The initial window size
 * for all channels is 4k. This value must be elevated to something
 * bigger that fits your environment. This is controlled via \ref
 * vortex_channel_set_window_size.
 *
 * The second concept (<b>frame fragmentation</b>) talks about how
 * your messages are splitted in the case they don't fit into the
 * remote window size. Take a look on the following handler: \ref
 * VortexChannelFrameSize
 *
 * Now take a look into the following examples included in the test directory:
 *
 * - vortex-file-transfer-client.c
 * - vortex-file-transfer-server.c
 *
 * \section vortex_manual_http_support 4.4 Doing BEEP connections through HTTP proxy servers
 *
 * In many cases a BEEP client will require to connect to a BEEP
 * server which is outside the local area network and that network is
 * limited by a firewall that constrains all internet connection, only
 * allowing HTTP/HTTPS connections if they are done through the local
 * proxy.
 *
 * By using a mecanism provided by the HTTP protocol, the CONNECT
 * method, a vortex client can connect to a remote BEEP server. This
 * is done by using the following function:
 *
 * - \ref vortex_http_connection_new
 *
 * Previous function will create a connection to a target BEEP host,
 * using proxy settings defined by \ref VortexHttpSetup. The
 * connection returned will work in the same way (with no difference)
 * as the ones returned by \ref vortex_connection_new.
 *
 * \section vortex_manual_pull_api 4.5 PULL API single thread event notification
 *
 * Vortex Library design is heavily threaded. In some cases due to
 * programing approach or environment conditions it is required a
 * single threaded API where a single loop can handle all async events
 * (frame received, connection received, etc).
 *
 * This API will allow the programmer to not receive async
 * notifications but pull them (\ref vortex_pull_next_event).
 *
 * Each call to pull an event returns an \ref VortexEvent object which
 * includes the event type (\ref vortex_event_get_type) that guides
 * the user to fetch for particular event data associated according to its type. 
 *
 * For example, if \ref vortex_event_get_type returns \ref
 * VORTEX_EVENT_FRAME_RECEIVED the programmer should call to \ref
 * vortex_event_get_channel to get the channel where the frame was
 * received and to call to \ref vortex_event_get_frame to get a
 * reference to the frame received.
 *
 * <b>4.5.1 PULL API activation:</b>
 *
 * Before using PULL event, it is required to activate the API. See
 * \ref vortex_pull_init for details on this.
 *
 * <b>4.5.2 PULLing events:</b>
 *
 * You must use \ref vortex_pull_next_event to get the next pending
 * event. If no pending event available the function will block the
 * caller until new events arrive.
 *
 * Use \ref vortex_pull_pending_events to check if there are pending
 * events before calling to \ref vortex_pull_next_event.
 *
 * <b>4.5.3 Event masking: how avoid receiving some events.</b>
 *
 * You can create a \ref VortexEventMask that configures a set of
 * events to be ignored. Keep in mind that ignoring events may
 * activate default action associated. 
 *
 * For example, disabling \ref VORTEX_EVENT_CHANNEL_START will cause
 * to accept all channel start request received. 
 *
 * <i><b>NOTE:</b> Default action associated to each particular event is described either by the
 * event documentation or by the default action taken by the async handler that the
 * event represents. Check documentation.</i>
 *
 * <b>4.5.4 Can I use select(2) or poll(2) system call to watch sockets for changes rather than using \ref vortex_pull_next_event?</b>
 *
 * It is possible but you have to consider that changes notified at
 * the socket level may not produce a fetchable event (\ref
 * vortex_pull_next_event). This is because there is a period between
 * a change is detected at the socket level and the time vortex engine
 * takes for processing incoming information so it can emit an event.
 *
 * Keep also in mind that some events may not depend on socket
 * traffic. For example \ref VORTEX_EVENT_CHANNEL_REMOVED is emited
 * when a channel is removed from a connection. For example, having a
 * connection closed suddently will make to emit a \ref
 * VORTEX_EVENT_CHANNEL_REMOVED for each channel found.
 *
 * <i>In any case <b>it is recommended to use</b> \ref
 * vortex_pull_pending_events <b>before calling to</b> \ref
 * vortex_pull_next_event to avoid blocking.</i>
 *
 * <b>The recommended approach</b> is is to check for pending events
 * and pull them on idle loops or to just get blocked on \ref vortex_pull_next_event.
 *
 * \section vortex_manual_alive_api 4.6 ALIVE API, active checks for connection status
 *
 * Vortex ALIVE API is an optional extension library that can be used
 * to improve connection/peer alive checks and notifications
 * optionally produced by \ref vortex_connection_set_on_close_full.
 *
 * Vortex ALIVE is implemented as a profile that exchanges "no
 * operation" messages waiting for a simple echo reply by the remote
 * peer. These "ping messages" are tracked and if a max error count is
 * reached an automatic connection close is triggered or, in the case
 * the user provides it, a user space handler is called to notify
 * failure.
 *
 * Vortex ALIVE will run in a transparent manner, mixed with user
 * profiles and it is fully compatible with any BEEP escenario.
 *
 * Currently, connection close notification is only received when an
 * active connection close was done either at the local or the remote
 * peer. However, in the case the connection becomes unavailable
 * because network unplug or because the remote peer has poweroff, or
 * because the remote peer application is hanged, this causes the
 * connection close to be not triggered until the TCP timeout is
 * reached in the case a write is done, and in the worst case, if the
 * remote process is hanged (or suspended) TCP stack won't timeout if
 * no operation is implemented.
 *
 * In this case, ALIVE check can be used to enforce a transparent and
 * active check implemented on top of a simply MSG/RPY where, if
 * reached some amount of unreplied messages, a connection close is
 * triggered, causing the code configured at \ref
 * vortex_connection_set_on_close_full to be called.
 *
 * To enable the check, the receiver must accept be "checked" by the
 * remote peer. This is done by calling:
 *
 * \code
 * // enable receiving alive checks from any peer 
 * vortex_alive_init ();
 * \endcode
 *
 * Now, at the watching side (may be the listener or the initiator), you
 * have to do the following to enable ALIVE connection check:
 *
 * \code
 * if (! vortex_alive_enable_check (conn, check_period, unreply_count, NULL)) {
 *      // failed to enable check 
 * } 
 * \endcode
 *
 * This will enable a period check (defined by check_period) and will
 * trigger a connection close in the case it is found that unreplied
 * count reaches unreply_count.
 *
 * <b>NOTES:</b>
 * 
 * - The check is implemented using a simply MSG/RPY protocol as
 * discussed in previous mails.
 *
 * - The check can be enabled at any time.
 *
 * - The check will trigger a connection close, allowing to reuse all
 * connection close code already configured.
 *
 * - It is not required to do anything to remove the check after a
 * connection was closed. This is done automatically.
 *
 * - If it is required to enable check in both directions, the same
 * reverse steps must be done.
 *
 * - ALIVE implementation checks if the connection has activity before
 * triggering the check. If it finds that the connection has being
 * working then the alive check is assumed to be ok, avoiding to
 * produce more connection traffic. This allows to silently disable
 * ALIVE in the case connection activity is found.
 *
 * - BEEP channel ALIVE uri is: urn:aspl.es:beep:profiles:ALIVE
 *
 * To use alive API, you must include the header:
 * \code
 * #include <vortex_alive.h>
 * \endcode
 *
 * And to add a link flag to use <b>libvortex-alive-1.1.dll</b>. In case of Linux/Unix you can use <b>-lvortex-alive-1.1</b> or: 
 *
 * \code
 * >> pkg-config --libs vortex-aliave-1.1
 * \endcode
 *
 * \section vortex_manual_feeder_api 4.7 How to use feeder API (streaming and transfering files efficiently)
 *
 * The usual pattern to send content is to issue a MSG frame (no
 * matter its size) using \ref vortex_channel_send_msg (or
 * similar functions) or replying to an incoming MSG sending content
 * in the form of a RPY frame or a series of ANS frames ended by a NUL
 * frame.
 * 
 * While this approach is the most suitable for small and unknown
 * sizes, it becomes a problem if we want to do continus transfer or
 * just send a huge file. 
 *
 * This is because every message we send with vortex, it is copied
 * into its internal structures so the caller is not blocked (nice
 * feature) and at the sime time it is allowed to release the content
 * right after returning from the send function. Obviously this is a
 * problem that grows with the size of the content to be transferred.
 *
 * Here is where the payload feeder API can be used to feed content
 * directly into the vortex sequencer (the vortex private thread in
 * charge of sending all pending content) without allocating it and
 * feeding the content with the optimal sizes at the right time.
 *
 * The \ref vortex_payload_feeder "feeder API" is built on top of the \ref VortexPayloadFeeder type
 * which encapsulates a handler defined by the user that must react
 * and complete a set of events issued by the vortex sending engine. 
 *
 * On top of this feeder API, it is already implemented a feeder to
 * read the content from a file, and stream it into vortex. Here is how to use it:
 *
 * \code
 * VortexPayloadFeeder * feeder;
 *
 * // create the feeder configured to read content from a file 
 * feeder = vortex_payload_feeder_file (FILE_TO_TRANSFER, axl_false);
 *
 * // use the feeder to issue a RPY frame
 * if (! vortex_channel_send_rpy_from_feeder (channel, feeder, vortex_frame_get_msgno (frame))) {
 *     printf ("ERROR: failed to send RPY using feeder..\n");
 *     return;
 * } 
 * \endcode
 *
 * In the case you want to feed content directly from a database or a
 * socket, etc, you can take a look on how it is implemented \ref
 * vortex_payload_feeder_file to create the appropriate handler that
 * implements your needs and then call to \ref
 * vortex_payload_feeder_new to create a feeder object governed by
 * that handler.
 *
 * A VortexPayloadFeeder represents a single message, so, triggering a
 * single operation with a feeder results into a single message
 * delivered to the remote peer (that may or may not be fragmented,
 * see next), which would be the same results as sending a single ANS
 * with the entire message.
 *
 * In the other hand, you might want to check \ref
 * vortex_channel_set_complete_flag to enable fragmentation (or
 * disable complete frame delivery) so even if the message is
 * fragmented, you get all pieces notified at the frame received as
 * they come.
 *
 * \section vortex_manual_external_api 4.8 Creating a BEEP over unknown transport (vortex external module)
 *
 * In the case you are looking for creating a BEEP session over a
 * transport that is not supported by the project but it has a socket
 * like (watchable) API and provides connection oriented session, you
 * can use "vortex external" module to create it easily.
 *
 * Here are some notes, on how to create a listener and a client for
 * your particular transport which will be called: xtransport 
 *
 * To use the Vortex external module, you'll have to include the
 * following header to use vortex external:
 * 
 * \code
 * #include <vortex_external.h>
 * \endcode
 * 
 * And also include the following linking flag:
 * 
 * \code
 * -lvortex-external-1.1
 * \endcode
 *
 * You can also use the following pkg-config instruction to get the linking flags:
 *
 * \code
 * >> pkg-config --libs vortex-external-1.1
 * \endcode
 *
 * <b>Creating a BEEP listener over an unknown transport (watchable, socket like API and connection oriented)</b>
 *
 * Now, the idea behind the new module is to use the new function \ref vortex_external_listener_new
 * and to provide a set of handlers that will be used by the Vortex engine to accept
 * and create new BEEP connections as they are received.
 *
 * You should use it as follows:
 *
 * <ol> <li>Create the xtransport listener as usual before creating
 * the BEEP listener. That will imply to create the listener socket up
 * to the listen (s, 1) call (or similar function). Let's
 * _listener_session is the listener socket for xtransport in next steps.  </li>
 * 
 * <li>After that, you'll have to create the listener using something like this:
 *
 * \code
 * listener = vortex_external_listener_new (ctx, _listener_session, 
 *                                               __xtransport_io_send, 
 *                                               __xtransport_io_receive,
 *                                               NULL, // let's make setup to be NULL for now 
 *                                               __xtransport_on_accept,  
 *                                               NULL); // this pointer can be NULL for now 
 * \endcode
 * </li>
 * <li> Now, the function __xtransport_io_send will have to implement writing bytes to the
 * write for a given xtransport's socket. It has to follow the next indication:
 *
 * \code
 * int __xtransport_io_send         (VortexConnection * connection,
 *                                   const char       * buffer,
 *                                   int                buffer_len)
 * {
 *        
 *       // write operation 
 *       int _session = vortex_connection_get_socket (connection);
 *       int  result;
 * 
 *       // customize here write xtransport operation 
 *       result =  write (_session, buffer, buffer_len);
 *        
 *       return result;
 * }
 * \endcode
 * </li>
 *        
 * <li>In the case of __xtransport_io_receive, it will be pretty similar but reading bytes (in order):
 *
 * \code
 * int __xtransport_io_receive         (VortexConnection * connection,
 *                                      char             * buffer,
 *                                      int                buffer_len)
 * {
 *      int   result;
 *      // read operation 
 *      int _session = vortex_connection_get_socket (connection);
 *
 *      // customize here read xtransport operation 
 *      result = read (_session, buffer, buffer_len);
 * 
 *      return result;
 * }
 * \endcode
 * </li>
 * <li>...and in the case of __xtransport_on_accept, it has to implement something similar to (see \ref VortexExternalOnAccept for more information):
 *
 * \code
 * VORTEX_SOCKET  __xtransport_on_accept (VortexCtx * ctx, VortexConnection * listener, 
 *                                       VORTEX_SOCKET listener_socket, axlPointer on_accept_data)
 *
 * {
 *      // customize here your accept function 
 *      int result = accept (listener_socket);
 *      printf ("INFO: accepting listener_socket=%d, result=%d\n", listener_socket, result);
 *      return result;
 * }
 * \endcode
 * </li>
 * </ol>
 *        
 * With all this, Vortex engine will create a listener where a watch
 * operation will be implemented over the provided _listener_session
 * (see point 2) and once a connection is received, it will call your
 * accept function (in this case __xtransport_on_accept -- \ref VortexExternalOnAccept).
 * 
 * With the socket returned, Vortex will configure everything,
 * including the I/O handlers needed to read and write from that
 * socket (__xtransport_io_send \ref VortexSendHandler and __xtransport_io_receive \ref VortexReceiveHandler), along with
 * all greetings, etc.
 *
 * After that, you'll have valid VortexConnection * objects which are
 * fully functional with the rest of the Vortex Library API.
 *
 * <b>Creating a BEEP client</b>
 *
 * Having a working listener it remains creating a client. This is done using the following code:
 *
 * <ol>
 * <li>Create the xtransport connection to the listener as usual. Once
 * you have the socket created you call to
 * vortex_external_connection_new with something like (assuming
 * conn_socket is your cliente socket):
 * 
 * \code
 * conn = vortex_external_connection_new (ctx, PTR_TO_INT (conn_socket), __xtransport_io_send, __xtransport_io_receive, NULL, NULL, NULL);
 * if (! vortex_connection_is_ok (conn, axl_false)) {
 *      printf ("ERROR: failed to create connection,..");
 *      vortex_connection_close (conn);
 *      return axl_false;
 * }
 * \endcode
 * </li>
 *
 * <li>As you can see, here you reuse __bluetooh_io_send and __bluetooh_io_receive because they
 * work the same. 
 * </li>
 * </ol>
 *
 * Reached this point, you should have a working connection, connected to the BEEP listener over 
 * xtransport,
 *
 * Inside regression test, you will find test_22 which includes a full example using this API (\ref vortex_external_connection_new and \ref vortex_external_listener_new):
 *
 * - https://raw.githubusercontent.com/ASPLes/libvortex-1.1/master/test/vortex-regression-client.c
 *
 *
 * \section vortex_manual_securing_your_session 5.1 Securing a Vortex Connection (or How to use the TLS profile)
 * 
 * The main advantage the BEEP protocol has is that it solves many
 * common problems that the network protocol designer will have to
 * face while designing an application protocol. Securing a connection
 * to avoid other parties to access data exchanged by BEEP peers is
 * one of them.
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
 * vortex_tls_start_negotiation.
 *
 * - 3. Then use the \ref VortexConnection "connection", as usual,
 * creating channels (\ref vortex_channel_new), sending data over them
 * (\ref vortex_channel_send_msg), etc. From the application
 * programmer's point of view there is no difference from using a
 * connection secured to one that is not.
 *
 * For the TLS profile listener side, we have two possibilities:
 * 
 * <ol>
 * <li>
 * <b>Use predefined handlers provided by the Vortex to activate
 * listener TLS profile support.</b>
 * 
 * This is a cheap-effort option because the library comes with a test
 * certificate and a test private key that are used in the case that
 * location handlers for such files are not provided.  
 * 
 * This enables starting developing the whole system and later, in the
 * production environment, create a private key and a certificate and
 * define needed handlers to locate them.
 * 
 * In this case, we could activate listener TLS profile support as follows:
 * \code
 * // activate TLS profile support using defaults
 * vortex_tls_accept_negotiation (ctx,   // context to configure
 *                                NULL,  // accept all TLS request received
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
 *  control if an incoming TLS requests is accepted. Here is an example:
 * \code
 * // return axl_true if we agree to accept the TLS negotiation
 * axl_bool      check_and_accept_tls_request (VortexConnection * connection, 
 *                                             char             * serverName)
 * {
 *     // perform some special operations against the serverName
 *     // value or the received connection, return axl_false to deny the 
 *     // TLS request, or axl_true to allow it.
 *     
 *     return axl_true;  
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
 *     // perform some special operation to choose which 
 *     // certificate file to be used, then return it:
 *    
 *     return vortex_support_find_data_file (ctx, "myCertificate.cert"); 
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
 *     // perform some special operation to choose which 
 *     // private key file to be used, then return it:
 *    
 *     return vortex_support_find_data_file (ctx, "myPrivateKey.pem"); 
 * }
 * \endcode
 *  </li>
 *
 * <li>Now use previous handlers to configure how TLS profile is
 * support for the current Vortex Library instance as follows:
 *  
 * \code
 * // activate TLS profile support using defaults
 * vortex_tls_accept_negotiation (ctx,
 *                                check_and_accept_tls_request,
 *                                certificate_file_location,
 *                                private_key_file_locatin);
 * \endcode
 *
 * <b> NOTE:</b> If some handler is not provided the default one will
 * be used. Not providing one of the file locators handler (either
 * certificate locator and private key locator) may cause to not work
 * TLS profile.
 * 
 * </li>
 * </ul>
 * </ol>
 *
 * There is also an alternative approach, which provides more control
 * to configure the TLS process. See \ref
 * vortex_tls_accept_negotiation for more information, especially \ref
 * vortex_tls_set_ctx_creation and \ref vortex_tls_set_default_ctx_creation.
 * 
 * Now your listener is prepared to receive incoming connections and
 * enable TLS on them. Next section provides information on how to
 * produce a certificate and a private key to avoid using the default
 * provided by Vortex Library. See next section.
 *
 * 
 * \section vortex_manual_creating_certificates 5.2 How to create a test certificate and a private key to be used by the TLS profile
 *
 * Now we have successfully configured the TLS profile for listener
 * side we need to create a certificate/private key pair. Currently
 * Vortex Library TLS support is built using <b>OpenSSL</b>
 * (http://www.openssl.org). This SSL toolkit comes with some tools to
 * create such files.  Here is an example to create a test certificate
 * and a private key that can be used for testing purposes:
 * 
 * <ol>
 * <li>Create a 1024 bits private key using:
 * 
 * \code
 *  >> openssl genrsa 1024 > test-private.key
 * \endcode
 * </li>
 *
 * <li>Now create the public certificate reusing previously created key as follows:
 *
 * \code
 *  >> openssl req -new -x509 -nodes -sha1 -days 3650 -key test-private.key > test-certificate.crt
 * \endcode
 * </li>
 *
 * <li>Once finished, you can check certificate data using:
 * \code
 *  >> openssl x509 -noout -fingerprint -text < test-certificate.crt
 * \endcode
 * </li>
 *</ol>
 *   
 * \section vortex_manual_using_sasl 5.3 Authenticating BEEP peers (or How to use SASL profiles family)
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
 * At this point, a malicious third party only could read a hash
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
 * \section vortex_manual_sasl_for_client_side 5.4 How to use SASL at the client side
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
 * \section vortex_manual_sasl_for_server_side 5.5 How to use SASL at the server side
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
 * vortex_sasl_set_plain_validation (ctx, sasl_plain_validation);
 * 
 * // accept SASL PLAIN incoming requests
 * if (! vortex_sasl_accept_negotiation (ctx, VORTEX_SASL_PLAIN)) {
 *	printf ("Unable  accept SASL PLAIN profile");
 *	return -1;
 * }
 * [...]
 * // validation handler for SASL PLAIN profile
 * axl_bool      sasl_plain_validation  (VortexConnection * connection,
 *                                       const char       * auth_id,
 *                                       const char       * authorization_id,
 *                                       const char       * password)
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
 *		return axl_true;
 *	}
 *	// deny SASL request to authenticate remote peer
 *	return axl_false;
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
 * module that the selected SASL profile must be announced as
 * supported, at the connection greetings, configuring all internal
 * SASL handlers. In this case, the example is not providing an user
 * data to the \ref vortex_sasl_accept_negotiation function. In the
 * case that a user data is required, the following function must be
 * used:
 *
 * - \ref vortex_sasl_accept_negotiation_full
 *
 * Now the SASL PLAIN profile is fully activated and waiting for
 * requests. Validating the rest of SASL profiles works the same
 * way. Some of them requires to return <b>axl_true</b> or <b>axl_false</b> to
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
 * the appropriate auth data: 
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

/** 
 * \page profile_example Implementing a profile tutorial (C API)
 * 
 * \section profile_example_intro Introduction
 *
 * <i><b>NOTE:</b> All code developed on this tutorial can be found
 * inside the Vortex Library repository, inside the <b>test/</b>
 * directory. Files created in this tutorial: vortex-simple-listener.c
 * and vortex-simple-client.c. The following are the subversion links: </i>
 *
 *  - https://dolphin.aspl.es/svn/publico/af-arch/trunk/libvortex-1.1/test/vortex-simple-client.c
 *  - https://dolphin.aspl.es/svn/publico/af-arch/trunk/libvortex-1.1/test/vortex-simple-listener.c
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
 *  <li>First of all, Vortex Library is initialized using \ref vortex_init_ctx.</li>
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
 * (jobs:0)[acinom@barbol libvortex-1.1]
 * $ vortex-client
 * Vortex-client v.0.8.3.b1498.g1498: a cmd tool to test vortex (and BEEP-enabled) peers
 * Copyright (c) 2010 Advanced Software Production Line, S.L.
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
 * channel is used to perform special operations such as create new
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
 * is used for special operations about channel management. 
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
 * <b>Section 3: </b> Using Vortex Library <b>xml-rpc-gen-1.1</b> tool
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
 * invoked. However, RPC developers usually fall into providing a
 * host and a port (TCP/IP) to locate the resource, bypassing the
 * location facilities.
 * 
 * The software that provides location services is usually called the
 * binder. For the case of XML-RPC, there is no binder. So, you have
 * to provide all information required to locate your component that
 * will accept to process the service invocation received.
 *
 * Obviously, in most cases, this isn't a problem, because most of system
 * network design is based on a client/server interaction, making only
 * necessary to know where is located the server component.
 *
 * Usually, these RPC environments provides two API (or way to use the
 * framework) that could be classified as: Raw invocation interfaces
 * and High level invocation interfaces.
 *
 * As you may guessing, there are more fun (and pain!) while using the
 * Raw invocation interface than the High level one. Both offers
 * (dis)advantages and both provides a specific functionality, that
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
 * The client stub is a small library that exposes the services as
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
 * // the ctx variable represents a context already initialized with
 * // vortex_ctx_new followed by vortex_init_ctx
 *
 * // call to enable XML-RPC on the context 
 * if (! vortex_xml_rpc_init (ctx)) {
 *     printf ("Unable to init XML-RPC support..\n");
 *     return;
 * }
 *
 * // create the connection to a known location (in a blocking manner for
 * // demostration purposes)
 * connection = vortex_connection_new (ctx, "localhost", "22000", NULL, NULL);
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
 * High level one while producing common RPC tasks.
 * 
 * However, this interface is required if you need to produce a custom
 * invocator that perform some special task before doing the actual
 * invocation, or just because you need a general invocation software
 * to track down all invocations received, at server side, so you can
 * re-send that invocation to another system, acting as a proxy. 
 * 
 * \section raw_client_invoke_considerations Raw client invocation considerations
 *
 * Before seeing previous example, here are some issues to be
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
 * application requires and integer, you can't reply a string.
 *
 * In this context, the usual operation performed on a \ref
 * XmlRpcMethodResponse received, is to get the value inside (\ref
 * XmlRpcMethodValue) calling to \ref vortex_xml_rpc_method_response_get_value.
 * 
 * Then, the next set of functions will help you to get the value
 * marshalled into the appropriate type:
 *
 *  - \ref vortex_xml_rpc_method_value_get_as_int
 *  - \ref vortex_xml_rpc_method_value_get_as_double
 *  - \ref vortex_xml_rpc_method_value_get_as_string
 *  - \ref vortex_xml_rpc_method_value_get_as_struct
 *  - \ref vortex_xml_rpc_method_value_get_as_array
 *
 * For the case of boolean values, you can use the same function for
 * the int type. boolean values, inside XML-RPC, are modeled using 0
 * and 1 states to represent axl_true and axl_false.
 * </li>
 *
 * </ul>
 *
 * \section receiving_an_invocation Processing an incoming XML-RPC invocation
 *
 * We have seen in previous sections how a XML-RPC invocation is
 * produced. Now, it is time to know what happens on the remote side,
 * because, until now, we have just moved the problem from a machine
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
 * So the resource "/version/1.0" and the resource "/version/1.2" could
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
 * <b>602</b> (because no port was specified by appending to the
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
 * code that a new method invocation has arrived (\ref XmlRpcMethodCall) and that it has to be dispatched to the
 * appropriate handler. This handler is mainly provided to allow
 * developers to be able to produce its own service dispatching policy.
 * 
 * Let's see a simple dispatching implementation for the sum service
 * introduced at the \ref raw_client_invoke "client invocation section". 
 * Let's see the example first to later see some considerations:
 *
 * First, the listener side must active a listener that is willing to
 * accept the XML-RPC profile: 
 * 
 * \code
 *
 * // vortex global context 
 * VortexCtx * ctx = NULL;
 *
 * int  main (int  argc, char  ** argv) 
 * {
 *
 *      // create an empty context 
 *      ctx = vortex_ctx_new ();
 *
 *      // init the context
 *      if (! vortex_init_ctx (ctx)) {
 *           printf ("failed to init the library..\n");
 *      } 
 *
 *      // enable XML-RPC profile 
 *      vortex_xml_rpc_accept_negotiation (ctx, validate_resource,
 *                                         // no user space data for
 *                                         // the validation resource
 *                                         // function. 
 *                                         NULL, 
 *                                         service_dispatch,
 *                                         // no user space data for
 *                                         // the dispatch function. 
 *                                         NULL);
 *
 *      // create a vortex server 
 *      vortex_listener_new (ctx, "0.0.0.0", "44000", NULL, NULL);
 *
 *      // wait for listeners (until vortex_exit is called) 
 *      vortex_listener_wait (ctx);
 *	
 *      // end vortex function 
 *      vortex_exit_ctx (ctx, axl_true);
 *
 *      return 0;
 * }
 * \endcode
 *
 * The example is quite simple, first, Vortex Library is initialized,
 * then a call to \ref vortex_xml_rpc_accept_negotiation is done to
 * active the XML-RPC. Then a call to activate a listener, for any
 * host name that the local machine may have, at the port 44000, is
 * done by using \ref vortex_listener_new. Finally, a call to \ref
 * vortex_listener_wait is done to ensure the server initialization
 * code is blocked until the server finish.
 *
 * Here is an example of a validation resource function that only
 * accept some resources names:
 * \code
 * axl_bool      validate_resource (VortexConnection * connection, 
 *                                  int                channel_number,
 *                                  char             * serverName,
 *                                  char             * resource_path)
 * {
 *      // check that resource received
 *      if (axl_cmp (resource_path, "/aritmetic-services/1.0"))
 *              return axl_true;
 *      if (axl_cmp (resource_path, "/aritmetic-services/1.1"))
 *              return axl_true;
 *    
 *      // resource not recognized, just return axl_false to signal
 *      // vortex engine that the channel must not be accepted.
 *      return axl_false;
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
 * service is supported and then call to the appropriate handler. 
 * 
 * Here is the first interesting thing, the function \ref
 * method_call_is. It is used to recognize service patterns like name,
 * number of parameter and the parameter type. This allows to easily
 * recognize the service to actually dispatch.
 *
 * In this function, <b>service_dispatch</b>, should be created a \ref XmlRpcMethodResponse to
 * be returned. So, the vortex engine could reply as fast as
 * possible. However, the implementation is prepared to defer the
 * reply. This allows, especially, to communicate with other language
 * runtimes. Once the runtime have generated the reply, it must be
 * used the following function \ref vortex_xml_rpc_notify_reply, to actually perform the reply.
 *
 * \section abstraction_required Abstraction required: The xml-rpc-gen-1.1 tool
 *
 * Until now, we have seen that producing RPC enabled solutions is
 * really complex. First, because we have to produce the server
 * implementation, and then, all source code required to actually
 * perform the invocation.
 * 
 * However, every RPC framework comes with a protocol compiler that
 * helps on producing lot of source code. For the XML-RPC over BEEP
 * implementation that comes with Vortex Library, this tool is
 * <b>xml-rpc-gen-1.1</b>.
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
 * >> ./xml-rpc-gen-1.1 reg-test01.idl
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
 * invocation, the <b>xml-rpc-gen-1.1</b> tool allows you define a xml-rpc
 * interface definition, that produces that code for you. In this
 * case, looking at: <b><i>out/client-test/test_xml_rpc.h</i></b>,
 * you'll find a C API that hides all details to actually invoke the
 * <b><i>sum</i></b> service.
 *
 * In the other hand, instead of producing all source code, at the
 * server side, to unmarshall and invoke the service, all this code is
 * produced by <b>xml-rpc-gen-1.1</b> tool. In this case, looking at:
 * <b><i>out/server-test/test_sum_int_int.c</i></b> you'll find the
 * <b>sum</b> service implementation. 
 *
 * In the following sections it is explained how to use the
 * <b>xml-rpc-gen-1.1</b> tool to produce RPC solutions.
 *
 * \section xml_rpc_gen_tool_language Using xml-rpc-gen-1.1 tool: language syntax
 *
 * <b>xml-rpc-gen-1.1</b> is a compiler that produces a client component
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
 * it is easy to parse and it is more portable (there is a XML parser
 * in every platform).
 * 
 * \section xml_rpc_gen_tool_language_types Types supported by xml-rpc-gen-1.1 tool
 *
 * There are 6 basic types supported by the tool (well, it is more
 * accurate to say by the XML-RPC definition) which are:
 * 
 *  -  <b>int</b>: Integer definition, four-byte signed integer (-21)
 *  -  <b>boolean</b>: Boolean definition (bound to 1/axl_true, 0/axl_false)
 *  -  <b>double</b>: double-precision signed floating point number -412.21
 *  -  <b>string</b>: a string definition "XML-RPC over BEEP!!"
 *  -  <b>data</b>: date/time (currently not supported)
 *  -  <b>base64</b>: a base64 encoded string (currently encoding is not done).
 *
 * And two compound type definitions which allows to define more types:
 *
 *  -  <b>struct</b>: a struct definition which holds named values (called members).
 *  -  <b>array</b>: An uniform storage for other types (including more arrays).
 *
 * Struct and array types allows to create richer type definitions
 * (even composing both). Here is an example that uses a struct
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
 * references to the type being defined. This allows, for example, to
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
 * use. However, there are exception: recursive definitions.
 *
 * - However, because the tool allows you to define source code
 * inside the services to be included inside the server output, you
 * have to use the pointer syntax. This could be obvious: remember
 * that xml-rpc-gen-1.1 tool just includes the source code. It doesn't
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
 * This is no special consideration while declaring services that
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
 * Due to the kind of output produced by the xml-rpc-gen-1.1 tool, it has
 * to create "method names" for services declared at the IDL processed
 * in a synchronized way to a client invocation, using a particular
 * service, is properly processed by the remote service entry point.
 *
 * Under some situations it is required to change the name that used
 * by default the xml-rpc-gen-1.1 tool. This is done by using a prefix
 * declaration before the service:
 * \include af-arch.idl
 *
 * In the example, the service <b>get_list</b> won't be invoked using
 * that name (the default xml-rpc-gen-1.1 behavior), but the name
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
 * achieve some interesting features:
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
 * xml-rpc-gen-1.1 tool to allow and check all resources used.
 *
 * \section xml_rpc_gen_tool_including_body_code Including additional code to be placed at the service module file
 *
 * Examples showed allows to include code that is placed at the
 * appropriate file at the server side created. However, real situation
 * requires calling to functions that are defined at the same modules
 * or other modules. This is because it is required a mechanism that
 * allows to include arbitrary code, not only in the service body.
 *
 * Suppose the following example:
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
 * \section xml_rpc_gen_tool_using_output_client Using the output produced by xml-rpc-gen-1.1 tool at the CLIENT SIDE
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
 * bash:~$ xml-rpc-gen-1.1 reg-test01.idl
 * \endcode
 * 
 * By default, the client library is placed at:
 * <b>out/client-&lt;component-name></b>. In this case, the output
 * will be <b>out/client-test</b>. You can modify this behavior by
 * using the <b>--out-dir</b> switch.
 * 
 * Inside a the <b>out/client-test</b> directory you'll find the main
 * API file <b>out/client-test/test_xml_rpc.h</b>. This file contains
 * all invocation functions required to perform a method call to all
 * services exported by the component compiled. 
 * 
 * Again, this file will follow the next naming convention according to
 * the component name: <b>out/client-&lt;component-name>/&lt;component-name>_xml_rpc.h</b>
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
 * <b>"_s"</b>, you'll find all parameters specified in the IDL/XDL
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
 * xml-rpc-gen-1.1 tool, the file <b>test_xml_rpc.h</b>. However, you also
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
 * \section xml_rpc_gen_tool_using_output_listener Using the output produced by xml-rpc-gen-1.1 tool at the LISTENER SIDE
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
 * Because xml-rpc-gen-1.1 tool have support to include the service source
 * code definition, into the IDL/XDL definition, the compiled product
 * only required to be compiled. However programing a XML-RPC service
 * usually is more complex than adding two integer. Here are some
 * considerations:
 * 
 * If the server component is produced (by default client and server
 * components are produced but it can be configured by using
 * <b>--only-client</b> or <b>--only-server</b>), it contains a xml
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
 * must return dynamically allocated objects. They will be deallocated
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
 * it is required some special TLS function at the server side, like
 * identifying the client certificate, it is not required to implement
 * anything special. Look at the following document with provides a
 * default TLS environment: \ref vortex_manual_securing_your_session.
 *
 * Because Vortex Library, at this moment, doesn't provide a profile
 * path, allowing to hide non-secured profiles behind TLS or SASL
 * profiles until they are entirely negotiated, you have to ensure
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
 * must be combined and mixed to build your specific solution that
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
 * // create a tunnel settings holder (being ctx a context already initialized
 * // with vortex_ctx_new followed by vortex_init_ctx
 * settings = vortex_tunnel_settings_new (ctx);
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
 * if (! vortex_connection_is_ok (connection, axl_false)) {
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

/** 
 * \page vortex_android_usage Android Vortex Library's usage
 *
 * 
 * \section vortex_android_usage Using Vortex Library's Android install and development kit
 *
 * Inside http://www.aspl.es/vortex/downloads/android you'll find two types of downloads. 
 * One prefixed with <b>-install.zip</b> that provides just those binaries needed and <b>-full.zip</b>
 * that provides a ready to use binaries with all the headers, etc needed to produce 
 * Android solutions using Vortex library.
 *
 *
 * \section vortex_general_notes General notes
 * 
 * The idea behind these installers is the following. Because there
 * are several platforms (android APIs) that you might want to support
 * and for each platform there could be several archs (up to 6), these
 * installers include all files compiled for each possible
 * combination.
 * 
 * That way, you only have to pick desired combinations to compile
 * your project with ready to use binaries.
 *
 *
 * \section vortex_platforms_archs Platforms and archs
 *
 * In particular the following platforms are supported by the
 * NDK. Each of them represents an android particular version:
 * 
 * - android-12  
 * - android-13  
 * - android-14  
 * - android-15  
 * - android-16  
 * - android-17  
 * - android-18  
 * - android-19  
 * - android-21  
 * - android-3  
 * - android-4  
 * - android-5  
 * - android-8  
 * - android-9
 * 
 * In the following link you have a declaration between each platform
 * and each Android version: https://developer.android.com/ndk/guides/stable_apis.html
 * 
 * Now, for each platform you may have different archs supported (cpu
 * style):
 * 
 * - arm  
 * - arm64  
 * - mips  
 * - mips64  
 * - x86  
 * - x86_64
 * 
 * More information about this at: https://developer.android.com/ndk/guides/abis.html
 * 
 * \section vortex_using_install_bundle Using the install bundle
 *
 * Assuming previous information, please, uncompress
 * vortex-VERSION-instal.zip bundle. Inside it, you'll find a
 * "install" folder that inside includes the following structure:
 * 
 * \code
 * install/<android-platform>/lib/<arch>/{libfiles}.so
 * \endcode
 * 
 * That way, if you need ready to use compiled libraries for android-21, arch mips64, the look at:
 * 
 * \code
 * install/android-21/<arch>/lib/mips64/{libfiles}.so files.
 * \endcode
 * 
 * You might wonder why don't use a <android-platform>/<arch>/lib
 * scheme? That's good question.  This is because Android
 * architectural design. See "Native code in app packages" in the
 * following link https://developer.android.com/ndk/guides/abis.html
 * to know more about this structure.
 * 
 * The idea is that you have to support all <archs> for a given
 * <android-platform> (android-21 i.e.).
 * 
 * In that case, the install.zip is prepared to pick the entire
 * directory content of a given android platform (for example
 * install/android-21/) so the structure follows the guide lines of
 * Android but also provides you full support for all archs, in that
 * platform, to all components that Vortex Library is made of.
 * 
 * \section vortex_use_development_kit Using development kit bundle (full.zip)
 * 
 * Ok, now for compiling your project for android using this bundle,
 * please grab a reference to the full installer and uncompress it.
 * 
 * Inside, you'll find the following structure (more handy for a
 * developer using autoconf or cmake):
 * 
 * \code
 * full/<arch>/<platform>/{ready to use devel files to compile using Vortex Library}
 * \endcode
 * 
 * 
 * Now, assuming you are working with an ARM device, running android
 * 4.0, then you can use files found at:
 * 
 * \code
 * full/arm/android-14/bin/
 *                     include/
 *                     lib/
 *                     share/
 *                     ssl/
 *  \endcode
 * 
 * 
 * In your case, you only have to provide the following gcc flags to
 * your cmake or autoconf environment as follows:
 * 
 * 1) Compiler flags:
 *     
 * \code
 *    CFLAGS="-I/full/arm/android-14/include -I/full/arm/android-14/include/vortex -I/full/arm/android-14/include/axl -I/full/arm/android-14/include/nopoll"
 * \endcode
 * 
 * 2) Linker flags:
 * 
 * \code
 *    LDFLAGS=" -L/full/arm/android-14/lib -lvortex -lvortex-tls-1.1 -l axl -lssl -lcrypto -lpthread -pthread -lm -lnopoll"
 * \endcode
 * 
 * 3) And your compiler must match the target platform, for example, for ARM:
 * 
 * \code
 *  CC := $(ANDROID_NDK_BIN)/arm-linux-androideabi-gcc
 *  CPP := $(ANDROID_NDK_BIN)/arm-linux-androideabi-g++
 *  AR := $(ANDROID_NDK_BIN)/arm-linux-androideabi-ar
 *  LD := $(ANDROID_NDK_BIN)/arm-linux-androideabi-ld
 * \endcode
 * 
 * After that, according to your compiling tool, have it to use these
 * indicatations to compile your source code using Vortex Library.
 * 
 * 
 */
 

