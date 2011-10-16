/* 
 *  Lua Vortex:  Lua bindings for Vortex Library
 *  Copyright (C) 2011 Advanced Software Production Line, S.L.
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
#include <lua-vortex.h>
#include <lua-vortex-private.h>
#include <stdarg.h>
#include <signal.h>

#if defined(ENABLE_LUA_VORTEX_LOG)
/** 
 * @brief Variable used to track if environment variables associated
 * to log were checked. 
 */
axl_bool _lua_vortex_log_checked = axl_false;

/** 
 * @brief Boolean variable that tracks current console log status. By
 * default it is disabled.
 */
axl_bool _lua_vortex_log_enabled = axl_false;

/** 
 * @brief Boolean variable that tracks current second level console
 * log status. By default it is disabled.
 */
axl_bool _lua_vortex_log2_enabled = axl_false;

/** 
 * @brief Boolean variable that tracks current console color log
 * status. By default it is disabled.
 */
axl_bool _lua_vortex_color_log_enabled = axl_false;
#endif

/** 
 * @brief Allows to get current log enabled status.
 *
 * @return axl_true if log is enabled, otherwise axl_false is returned.
 */
axl_bool lua_vortex_log_is_enabled (void)
{
	/* if log is not checked, check environment variables */
	if (! _lua_vortex_log_checked) {
		_lua_vortex_log_checked = axl_true;

		/* check for LUA_VORTEX_DEBUG */
		_lua_vortex_log_enabled = vortex_support_getenv_int ("LUA_VORTEX_DEBUG") > 0;

		/* check for LUA_VORTEX_DEBUG_COLOR */
		_lua_vortex_color_log_enabled = vortex_support_getenv_int ("LUA_VORTEX_DEBUG_COLOR") > 0;
	} /* end if */

	return _lua_vortex_log_enabled;
}

/** 
 * @brief Allows to get current second level log enabled status.
 *
 * @return axl_true if log is enabled, otherwise axl_false is returned.
 */
axl_bool lua_vortex_log2_is_enabled (void)
{
#if defined(ENABLE_LUA_VORTEX_LOG)
	return _lua_vortex_log2_enabled;
#else
	return axl_false;
#endif
}

/** 
 * @brief Allows to get current color log enabled status.
 *
 * @return axl_true if color log is enabled, otherwise axl_false is returned.
 */
axl_bool lua_vortex_color_log_is_enabled (void)
{
#if defined(ENABLE_LUA_VORTEX_LOG)
	return _lua_vortex_color_log_enabled;
#else
	return axl_false;
#endif
}

/** 
 * @internal Internal common log implementation to support several
 * levels of logs.
 * 
 * @param file The file that produce the log.
 * @param line The line that fired the log.
 * @param log_level The level of the log
 * @param message The message 
 * @param args Arguments for the message.
 */
void _lua_vortex_log_common (const char          * file,
			    int                   line,
			    LuaVortexLog           log_level,
			    const char          * message,
			    va_list               args)
{
#if defined(ENABLE_LUA_VORTEX_LOG)
	/* if not LUA_VORTEX_DEBUG FLAG, do not output anything */
	if (!lua_vortex_log_is_enabled ()) 
		return;

#if defined (__GNUC__)
	if (lua_vortex_color_log_is_enabled ()) 
		fprintf (stdout, "\e[1;36m(proc %d)\e[0m: ", getpid ());
	else 
#endif /* __GNUC__ */
		fprintf (stdout, "(proc %d): ", getpid ());
		
		
	/* drop a log according to the level */
#if defined (__GNUC__)
	if (lua_vortex_color_log_is_enabled ()) {
		switch (log_level) {
		case LUA_VORTEX_DEBUG:
			fprintf (stdout, "(\e[1;32mdebug\e[0m) ");
			break;
		case LUA_VORTEX_WARNING:
			fprintf (stdout, "(\e[1;33mwarning\e[0m) ");
			break;
		case LUA_VORTEX_CRITICAL:
			fprintf (stdout, "(\e[1;31mcritical\e[0m) ");
			break;
		}
	} else {
#endif /* __GNUC__ */
		switch (log_level) {
		case LUA_VORTEX_DEBUG:
			fprintf (stdout, "(debug) ");
			break;
		case LUA_VORTEX_WARNING:
			fprintf (stdout, "(warning) ");
			break;
		case LUA_VORTEX_CRITICAL:
			fprintf (stdout, "(critical) ");
			break;
		}
#if defined (__GNUC__)
	} /* end if */
#endif
	
	/* drop a log according to the domain */
	(file != NULL) ? fprintf (stdout, "%s:%d ", file, line) : fprintf (stdout, ": ");
	
	/* print the message */
	vfprintf (stdout, message, args);
	
	fprintf (stdout, "\n");
	
	/* ensure that the log is dropped to the console */
	fflush (stdout);
#endif /* end ENABLE_LUA_VORTEX_LOG */

	/* return */
	return;
}

/** 
 * @internal Log function used by lua_vortex to notify all messages that are
 * generated by the core. 
 *
 * Do no use this function directly, use <b>lua_vortex_log</b>, which is
 * activated/deactivated according to the compilation flags.
 * 
 * @param ctx The context where the operation will be performed.
 * @param file The file that produce the log.
 * @param line The line that fired the log.
 * @param log_level The message severity
 * @param message The message logged.
 */
void _lua_vortex_log (const char          * file,
		     int                   line,
		     LuaVortexLog           log_level,
		     const char          * message,
		     ...)
{

#ifndef ENABLE_LUA_VORTEX_LOG
	/* do no operation if not defined debug */
	return;
#else
	va_list   args;

	/* call to common implementation */
	va_start (args, message);
	_lua_vortex_log_common (file, line, log_level, message, args);
	va_end (args);

	return;
#endif
}

/** 
 * @internal Log function used by lua_vortex to notify all second level
 * messages that are generated by the core.
 *
 * Do no use this function directly, use <b>lua_vortex_log2</b>, which is
 * activated/deactivated according to the compilation flags.
 * 
 * @param ctx The context where the log will be dropped.
 * @param file The file that contains that fired the log.
 * @param line The line where the log was produced.
 * @param log_level The message severity
 * @param message The message logged.
 */
void _lua_vortex_log2 (const char          * file,
		      int                   line,
		      LuaVortexLog           log_level,
		      const char          * message,
		      ...)
{

#ifndef ENABLE_LUA_VORTEX_LOG
	/* do no operation if not defined debug */
	return;
#else
	va_list   args;

	/* if not LUA_VORTEX_DEBUG2 FLAG, do not output anything */
	if (!lua_vortex_log2_is_enabled ()) {
		return;
	} /* end if */
	
	/* call to common implementation */
	va_start (args, message);
	_lua_vortex_log_common (file, line, log_level, message, args);
	va_end (args);

	return;
#endif
}


void lua_vortex_error (lua_State * L, const char * error,...)
{
	va_list    args;
	char     * message;

	va_start (args, error);
	message = axl_strdup_printfv (error, args);
	va_end (args);

	lua_vortex_log (LUA_VORTEX_CRITICAL, "Found error during lua vortex process, error reported was: %s", message);
	lua_pushstring (L, message);
	lua_error (L);

	axl_free (message);

	return;
}

axl_bool lua_vortex_handle_error (lua_State * L, int error_code, const char * label)
{
	const char * error_msg;

	if (error_code == 0) {
		lua_vortex_log (LUA_VORTEX_DEBUG, "   %s handler on lua space returned, stack side after call: %d..", label, lua_gettop (L));
		return axl_false; /* no error found */
	}

	switch (error_code) {
	case LUA_ERRRUN: 
		error_msg = "Runtime error";
		break;
	case LUA_ERRMEM: 
		error_msg = "Memory allocation error";
		break;
	case LUA_ERRERR:
		error_msg = "Error while running the error handler function";
		break;
	default:
		error_msg = "Undefined run-time error";
		break;
	}

	lua_vortex_log (LUA_VORTEX_CRITICAL, "   found error during %s handler execution: %s. Error details: %s", label, error_msg, lua_tostring (L, -1));

	return axl_true; /* error found */
}

/** 
 * @brief Allows to show the current stack pointed by the provided lua
 * state.
 */
void lua_vortex_show_stack (lua_State * L, const char * function, const char * file, int line)
{
	int          iterator;
	int          initial_position = lua_gettop (L);
	
	lua_vortex_log (LUA_VORTEX_DEBUG, "(%s:%s:%d) Showing current stack status (%d args)", 
			function, file, line, initial_position);
	iterator = 1;
	while (iterator <= initial_position) {

		/* show log */
		lua_vortex_log (LUA_VORTEX_DEBUG, "  sp %d: %s (ptr: %p)", iterator, lua_typename (L, lua_type (L, iterator)), lua_touserdata (L, iterator));
		iterator++;
	} /* end while */
	
	return;
}

/** 
 * @internal Allows to check a set of parameters expected on the lua
 * stack. The following is the list of parameters supported and how
 * they are identified on each position.
 *
 * - 't' : vortex.ctx
 * - 'o' : vortex.connection
 * - 'q' : vortex.asyncqueue
 * - 'c' : vortex.channel
 * - 'r' : vortex.frame
 * - 'l' : vortex.channelpool
 * - 's' : string
 * - 'z' : string but also allowing passing nil
 * - 'i' : number
 * - 'b' : boolean
 * - 'f' : function
 * - 'g' : function or nil (optional function)
 * - 'd' : generic data (everything)
 *
 * @return The function returns axl_true if the number of paremters
 * matches the description in params and that each parameter matches
 * each position. Otherwise, axl_false is returned.
 */
axl_bool lua_vortex_check_params (lua_State * L, const char * params)
{
	int      iterator        = 0;
	axl_bool obligatory      = axl_true;
	int      params_received = lua_gettop (L);
	int      params_expected = 0;
	int      desp            = 1;

	/* first ensure we get minimum params expected */
	/* check all parameters according to spec */
	while (params[iterator]) {

		if (params[iterator] == '|')
			break;

		/* count params expected by programmer */
		params_expected++;

		/* next position */
		iterator++;
	} /* end if */

	/* check all parameters according to spec */
	iterator = 0;
	while ((params[iterator] != 0) && (iterator < params_received)) {

		switch (params[iterator]) {
		case '|':
			/* starting from here parameters aren't obligatory */
			obligatory = axl_false;
			desp--;
			break;
		case 't':
			/* expected vortex.ctx */
			lua_vortex_log (LUA_VORTEX_DEBUG, "  checking vortex.ctx parameter at sp %d", iterator + desp);
			if (! lua_vortex_ctx_check_internal (L, iterator + desp))
				return axl_false;
			break;
		case 'o':
			/* expected vortex.connection */
			lua_vortex_log (LUA_VORTEX_DEBUG, "  checking vortex.connection parameter at sp %d", iterator + desp);
			if (! lua_vortex_connection_check_internal (L, iterator + desp))
				return axl_false;
			break;
		case 'c':
			/* expected vortex.channel */
			lua_vortex_log (LUA_VORTEX_DEBUG, "  checking vortex.channel parameter at sp %d", iterator + desp);
			if (! lua_vortex_channel_check_internal (L, iterator + desp))
				return axl_false;
			break;
		case 'r':
			/* expected vortex.frame */
			lua_vortex_log (LUA_VORTEX_DEBUG, "  checking vortex.frame parameter at sp %d", iterator + desp);
			if (! lua_vortex_frame_check_internal (L, iterator + desp))
				return axl_false;
			break;
		case 'l':
			/* expected vortex.frame */
			lua_vortex_log (LUA_VORTEX_DEBUG, "  checking vortex.channelpool parameter at sp %d", iterator + desp);
			if (! lua_vortex_channelpool_check_internal (L, iterator + desp))
				return axl_false;
			break;
		case 'f':
			/* check function */
			lua_vortex_log (LUA_VORTEX_DEBUG, "  checking function parameter at sp %d", iterator + desp);
			if (! lua_isfunction (L, iterator + desp)) {
				lua_vortex_error (L, "Expected to receive a function at param %d (received params: %d)", iterator + desp, params_received);
				return axl_false;
			}
			break;
		case 'g':
			/* check optional function */
			lua_vortex_log (LUA_VORTEX_DEBUG, "  checking optional function parameter at sp %d", iterator + desp);
			if (! lua_isfunction (L, iterator + desp) && ! lua_isnil (L, iterator + desp)) {
				lua_vortex_error (L, "Expected to receive a function at param %d (received params: %d)", iterator + desp, params_received);
				return axl_false;
			}
			break;
		case 's':
			lua_vortex_log (LUA_VORTEX_DEBUG, "  checking string parameter at sp %d", iterator + desp);
			/* expected string */
			if (! lua_isstring (L, iterator + desp)) {
				lua_vortex_error (L, "Expected to receive a string at param %d (received params: %d)", iterator + desp, params_received);
				return axl_false;
			}
			break;
		case 'z':
			lua_vortex_log (LUA_VORTEX_DEBUG, "  checking string or nil parameter at sp %d", iterator + desp);
			/* expected string */
			if (! lua_isstring (L, iterator + desp) && ! lua_isnil (L, iterator + desp)) {
				lua_vortex_error (L, "Expected to receive a string or nil at param %d (received params: %d)", iterator + desp, params_received);
				return axl_false;
			}
			break;
		case 'i':
			lua_vortex_log (LUA_VORTEX_DEBUG, "  checking number parameter at sp %d", iterator + desp);
			/* expected number */
			if (! lua_isnumber (L, iterator + desp)) {
				lua_vortex_error (L, "Expected to receive a number at param %d (received params: %d)", iterator + desp, params_received);
				return axl_false;
			}
			break;
		case 'b':
			lua_vortex_log (LUA_VORTEX_DEBUG, "  checking boolean parameter at sp %d", iterator + desp);
			/* expected number */
			if (! lua_isboolean (L, iterator + desp)) {
				lua_vortex_error (L, "Expected to receive a boolean at param %d (received params: %d)", iterator + desp, params_received);
				return axl_false;
			}
			break;
		case 'd':
			/* check data */
			break;
		} /* end if */

		/* next position */
		iterator++;
	}

	if (obligatory) {
		if (params_received < params_expected) {
			lua_vortex_error (L, "Expected to receive more parameters");
			return axl_false;
		} else if (params_received > params_expected) {
			lua_vortex_error (L, "Expected to receive few parameters");
			return axl_false;
		} /* end if */
	} /* end if */
		
	lua_vortex_log (LUA_VORTEX_DEBUG, "  all parameters ok");
	
	return axl_true; /* all parameters checked */
}

/** 
 * @internal Allows to set the provided key and value (boolean) on the
 * metatable that is associated to the value located at the position
 * (stack_position) indicated inside the provided lua state.
 *
 * @param L Lua state where the change will be applied.
 *
 * @param stack_position The position where a valid object with an
 * associated metatable where changes will be applied.
 *
 * @param key The table key to configure
 *
 * @param value The value to configure associated to the key.
 */
void lua_vortex_metatable_set_bool (lua_State * L, int stack_position, 
				    const char * key, axl_bool value)
{
	int initial_top = lua_gettop (L);

	/* get reference to the metatable */
	if (lua_getmetatable (L, stack_position) == 0) {
		lua_vortex_log (LUA_VORTEX_DEBUG, "Failed to set value %s=%d because there is no object or metatable associated to stack position %d",
				key, value, stack_position);
		return;
	}

	/* now push the value */
	lua_pushboolean (L, value);
	lua_setfield (L, initial_top + 1, key);

	/* now pop metatable left on stack */
	lua_pop (L, 1);

	return;
}

/** 
 * @internal Allows to get value (boolean) associated to the key
 * stored on the metatable associated to the object found on the
 * provided position.
 *
 * @param L The lua state where the operation will take place.
 *
 * @param stack_position The stack position where the value with a metatable associated is found.
 *
 * @param key The key to get the boolean value associated.
 */
axl_bool lua_vortex_metatable_get_bool (lua_State  * L, 
					int          stack_position, 
					const char * key)
{
	int      initial_top = lua_gettop (L);
	axl_bool result;

	/* get reference to the metatable */
	if (lua_getmetatable (L, stack_position) == 0) {
		lua_vortex_log (LUA_VORTEX_DEBUG, "Failed to get value '%s' because there is no object or metatable associated to stack position %d",
				key, stack_position);
		return axl_false;
	}

	/* now push the value */
	lua_getfield (L, initial_top + 1, key);

	/* get the value */
	result = lua_toboolean (L, initial_top + 2);

	/* now pop metatable left on stack and the boolean value */
	lua_pop (L, 2);

	return result;
}


lua_State * lua_vortex_create_thread (VortexCtx * ctx, lua_State * L, axl_bool acquire_lock)
{
	lua_State     * thread;
	int             thread_id;
	char          * thread_id_str;

	/* first we have to lock the mutex */
	if (acquire_lock) 
		LUA_VORTEX_LOCK (L, axl_true);

	/* check if vortex is finish to skip bridging */
	if (vortex_is_exiting (ctx)) {
		if (acquire_lock) 
			LUA_VORTEX_UNLOCK (L, axl_true);
		lua_vortex_log (LUA_VORTEX_DEBUG, "not creating thread...found vortex context is finishing after lock acquired");

		/* return null thread reference */
		return NULL;
	} /* end if */

	/* get current thread id */
#if defined(AXL_OS_WIN32)
	thread_id = GetCurrentThreadId ();
#elif defined(AXL_OS_UNIX)
	thread_id = pthread_self ();
#endif
	/* get pointer to the thread id */ 
	thread_id_str = axl_strdup_printf ("%p", thread_id);
	thread        = vortex_ctx_get_data (ctx, thread_id_str);

	lua_vortex_log (LUA_VORTEX_DEBUG, "Thread id %s has a lua state defined as: %p", thread_id_str, thread);
	if (thread == NULL) {
		/* create new lua state */
		thread = lua_newthread (L);

		/* acquire a reference to thread until we finish the context */
		lua_pushvalue (L, lua_gettop (L));
		luaL_ref (L, LUA_REGISTRYINDEX);

		lua_vortex_log (LUA_VORTEX_DEBUG, " ...seems it is not defined, creating state %p for Thread id %s", thread, thread_id_str);

		/* lua_newtable (thread);
		lua_newtable (thread);
		
		lua_pushliteral (thread, "__index");
		lua_pushvalue (thread, LUA_GLOBALSINDEX);
		
		lua_settable (thread, -3);
		lua_setmetatable (thread, -2);
		
		lua_replace (thread, LUA_GLOBALSINDEX); */
		
		/* store this thread into the context associated to the
		 * current thread identifier */
		vortex_ctx_set_data_full (ctx, thread_id_str, thread, axl_free, NULL);
		thread_id_str = NULL;
	} /* end if */

	/* unlock */
	if (acquire_lock)
		LUA_VORTEX_UNLOCK (L, axl_true);

	/* free thread id str if it is still defined */
	axl_free (thread_id_str);

	return thread;
};


/** 
 * @brief Allows to acquire a reference to the next item on the stack,
 * poping it, and returning the reference id created in the form of
 * LuaVortexRef.
 *
 * @param L The lua state where the ref operation will take place.
 */
LuaVortexRef * lua_vortex_ref (lua_State * L)
{
	LuaVortexRef * ref  = axl_new (LuaVortexRef, 1);

	if (ref == NULL) {
		lua_vortex_error (L, "Failed to acquire references, unable to complete operation");
		return NULL;
	} /* end if */

	/* get a reference */
	ref->ref_id = luaL_ref (L, LUA_REGISTRYINDEX);
	ref->L      = L;

	lua_vortex_log (LUA_VORTEX_DEBUG, "Acquireing a reference %d into the lua state %p, index registry: %d",
			ref->ref_id, L, LUA_REGISTRYINDEX);
	
	return ref;
}

/** 
 * Allows to unref the reference from the lua registry.
 */
void lua_vortex_unref (LuaVortexRef * ref)
{
	if (ref == NULL)
		return;

	luaL_unref (ref->L, LUA_REGISTRYINDEX, ref->ref_id);
	axl_free (ref);
	return;
}

/** 
 * Allows to create a couple of referenced joined together into a
 * single structure mostly designed to acquire a reference to a
 * function and a user data.
 *
 */
LuaVortexRefs * lua_vortex_acquire_references (VortexCtx * ctx, 
						lua_State * L, 
						int         ref_position, 
						int         ref_position2,
						int         ref_position3)
{
	LuaVortexRef  * ref;
	LuaVortexRefs * references;

	/* FIRST REFERENCE: have a reference to the function */
	lua_pushvalue (L, ref_position);

	/* acquire a reference to the function */
	ref = lua_vortex_ref (L);
	if (ref == NULL)
		return 0;

	/* grab references */
	references      = axl_new (LuaVortexRefs, 1);
	references->ctx = ctx;
	if (references == NULL) {
		lua_vortex_unref2 (references);
		return 0;
	}
	references->ref = ref;

	/* SECOND REFERENCE: have a reference to the function */
	if (ref_position2 > 0 && lua_gettop (L) >= ref_position2) 
		lua_pushvalue (L, ref_position2);
	else
		lua_pushnil (L);

	/* acquire a reference to the function data */
	ref = lua_vortex_ref (L);
	if (ref == NULL) {
		lua_vortex_unref2 (references);
		return 0;
	}

	/* have a reference to the data */
	references->ref2 = ref;

	/* THIRD REFERENCE: have a reference to the function */
	if (ref_position3 > 0 && lua_gettop (L) >= ref_position3) 
		lua_pushvalue (L, ref_position3);
	else
		lua_pushnil (L);

	/* acquire a reference to the function data */
	ref = lua_vortex_ref (L);
	if (ref == NULL) {
		lua_vortex_unref2 (references);
		return 0;
	}
	
	/* have a reference to the data */
	references->ref3 = ref;

	return references;
}

/** 
 * Allows to unref the provide couple of references
 */
void lua_vortex_unref2 (LuaVortexRefs * references)
{
	if (references == NULL)
		return;

	lua_vortex_log (LUA_VORTEX_DEBUG, "Finishing LuaVortexRefs (%p)", references);

	lua_vortex_unref (references->ref);
	lua_vortex_unref (references->ref2);
	lua_vortex_unref (references->ref3);
	axl_free (references);

	return;
}

axlHash     * lua_vortex_locks = NULL;
VortexMutex   lua_vortex_locks_mutex;

typedef struct LuaVortexLock {
	VortexMutex mutex;
	VortexCond  cond;
	int         pipe[2];
	axl_bool    finishing;
	axlPointer  wait_object;
} LuaVortexLock;


void lua_vortex_release_lock (axlPointer _lock)
{
	LuaVortexLock * lock = _lock;

	/* destroy mutex and conditional */
	vortex_mutex_destroy (&lock->mutex);
	vortex_cond_destroy (&lock->cond);

	/* close sockets */
	if (lock->pipe[0] > 0)
		vortex_close_socket (lock->pipe[0]);
	if (lock->pipe[1] > 0)
		vortex_close_socket (lock->pipe[1]);

	/* free lock */
	axl_free (_lock);
	return;
}

/** 
 * @internal Allows to get current lua state lock associated.
 */
LuaVortexLock * lua_vortex_lock_get (lua_State * L)
{
	LuaVortexLock * lock;

	vortex_mutex_lock (&lua_vortex_locks_mutex);

	/* get the lua state lock */
	lock = axl_hash_get (lua_vortex_locks, L);
	if (lock == NULL) {
		/* create the lock and register it associated to the
		   provided lua state */
		lock = axl_new (LuaVortexLock, 1);
		vortex_mutex_create (&lock->mutex);
		vortex_cond_create (&lock->cond);

		axl_hash_insert_full (lua_vortex_locks, L, NULL, lock, lua_vortex_release_lock);
	}

	/* now we have the mutex, release global and lock on particular */
	vortex_mutex_unlock (&lua_vortex_locks_mutex);

	return lock;
}

/** 
 * Allows to acquire the lock associated to the provided main lua
 * state. The lua state provide must be main (not a lua state created
 * by lua_newthread).
 */
void lua_vortex_lock (lua_State * L, axl_bool signal_fd, const char * file, int line)
{
	LuaVortexLock * lock;

	/* get lua vortex lock */
	lock = lua_vortex_lock_get (L);

	/* if event fd is defined, write a wait char */
	if (signal_fd && lock->pipe[1] > 0) {
		lua_vortex_log (LUA_VORTEX_DEBUG, "(%s:%d): Signaling event fd on fd=%d", 
				file, line, lock->pipe[1]);
		/* send a we are waiting */
		if (send (lock->pipe[1], "w", 1, 0) != 1)
			lua_vortex_log (LUA_VORTEX_CRITICAL, "Failed to write signal character on pipe");
	}

	/* lock the lua state mutex */
	vortex_mutex_lock (&lock->mutex);

	return;
}

/** 
 * Allows to unlock the provide lua state's lock.
 */
void lua_vortex_unlock (lua_State * L, axl_bool signal_fd, const char * file, int line)
{
	LuaVortexLock * lock;
	char            bytes[2];

	vortex_mutex_lock (&lua_vortex_locks_mutex);

	/* get the lua state lock */
	lock = axl_hash_get (lua_vortex_locks, L);

	/* now we have the mutex, release global and lock on particular */
	vortex_mutex_unlock (&lua_vortex_locks_mutex);

	if (lock == NULL) {
		lua_vortex_log (LUA_VORTEX_CRITICAL, "Found null reference for internal lock..");
		return;
	}

	/* lock the lua state mutex */
	vortex_cond_signal (&lock->cond);
	vortex_mutex_unlock (&lock->mutex);

	/* if event fd is defined, read a wait char */
	if (signal_fd && lock->pipe[0] > 0) {
		lua_vortex_log (LUA_VORTEX_DEBUG, "(%s:%d) Consumed one event from event fd",
				file, line);
		/* send a we are waiting */
		recv (lock->pipe[0], bytes, 1, MSG_DONTWAIT);
	}

	return;
}

/** 
 * Allows to unlock mutex associated to the provided lua state during
 * the provided amount of time (timeout). The caller reacquires the
 * mutex after the call completes.
 */
void lua_vortex_yield (lua_State * L, long timeout)
{
	LuaVortexLock * lock;

	vortex_mutex_lock (&lua_vortex_locks_mutex);

	/* get the lua state lock */
	lock = axl_hash_get (lua_vortex_locks, L);

	/* now we have the mutex, release global and lock on particular */
	vortex_mutex_unlock (&lua_vortex_locks_mutex);

	if (lock == NULL)
		return;

	/* unlock and wait */
	lua_vortex_log (LUA_VORTEX_DEBUG, "Unlocking lua (state: %p) mutex during %d ns", L, timeout);
	if (timeout > 0)
		vortex_cond_timedwait (&lock->cond, &lock->mutex, timeout);
	else
		vortex_cond_wait (&lock->cond, &lock->mutex);
	lua_vortex_log (LUA_VORTEX_DEBUG, "   ...wait finished (state: %p)", L);

	return;
}

/** 
 * Implements vortex.yield method.
 */
int _lua_vortex_yield (lua_State * L)
{
	long timeout = 0;
	
	/* check params */
	if (! lua_vortex_check_params (L, "|i"))
		return 0;

	/* get timeout defined by the user */
	if (lua_gettop (L) > 0) 
		timeout = lua_tonumber (L, 1);

	/* call to yield */
	lua_vortex_yield (L, timeout);

	return 1;
}

axl_bool __lua_vortex_event_fd_init_pipe (lua_State * L, LuaVortexLock * lock)
{
	struct sockaddr_in      saddr;
	struct sockaddr_in      sin;

	VORTEX_SOCKET           listener_fd;
#if defined(AXL_OS_WIN32)
/*	BOOL                    unit      = axl_true; */
	int                     sin_size  = sizeof (sin);
#else    	
	int                     unit      = 1; 
	socklen_t               sin_size  = sizeof (sin);
#endif	  
	int                     bind_res;
	int                     result;

	/* create listener socket */
	if ((listener_fd = socket(AF_INET, SOCK_STREAM, 0)) <= 2) {
		/* do not allow creating sockets reusing stdin (0),
		   stdout (1), stderr (2) */
		lua_vortex_error (L, "failed to create listener socket: %d (errno=%d:%s)", listener_fd, errno, vortex_errno_get_error (errno));
		return -1;
        } /* end if */

#if defined(AXL_OS_WIN32)
	/* Do not issue a reuse addr which causes on windows to reuse
	 * the same address:port for the same process. Under linux,
	 * reusing the address means that consecutive process can
	 * reuse the address without being blocked by a wait
	 * state.  */
	/* setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char  *)&unit, sizeof(BOOL)); */
#else
	setsockopt (listener_fd, SOL_SOCKET, SO_REUSEADDR, &unit, sizeof (unit));
#endif 

	memset(&saddr, 0, sizeof(struct sockaddr_in));
	saddr.sin_family          = AF_INET;
	saddr.sin_port            = 0;
	saddr.sin_addr.s_addr     = htonl (INADDR_LOOPBACK);

	/* call to bind */
	bind_res = bind (listener_fd, (struct sockaddr *)&saddr,  sizeof (struct sockaddr_in));
	if (bind_res == VORTEX_SOCKET_ERROR) {
		lua_vortex_error (L, "unable to bind address (port already in use or insufficient permissions). Closing socket: %d", listener_fd);
		vortex_close_socket (listener_fd);
		return axl_false;
	}
	
	if (listen (listener_fd, 1) == VORTEX_SOCKET_ERROR) {
		lua_vortex_error (L, "an error have occur while executing listen");
		vortex_close_socket (listener_fd);
		return axl_false;
        } /* end if */

	/* notify listener */
	if (getsockname (listener_fd, (struct sockaddr *) &sin, &sin_size) < -1) {
		lua_vortex_error (L, "an error have happen while executing getsockname");
		vortex_close_socket (listener_fd);
		return axl_false;
	} /* end if */

	lua_vortex_log  (LUA_VORTEX_DEBUG, "created listener running listener at %s:%d (socket: %d)", inet_ntoa(sin.sin_addr), ntohs (sin.sin_port), listener_fd);

	/* on now connect: read side */
	lock->pipe[0]      = socket (AF_INET, SOCK_STREAM, 0);
	if (lock->pipe[0] == VORTEX_INVALID_SOCKET) {
		lua_vortex_error (L,  "Unable to create socket required for pipe");
		vortex_close_socket (listener_fd);
		return axl_false;
	} /* end if */

	/* disable nagle */
	vortex_connection_set_sock_tcp_nodelay (lock->pipe[0], axl_true);

	/* set non blocking connection */
	vortex_connection_set_sock_block (lock->pipe[0], axl_false);  

        memset(&saddr, 0, sizeof(saddr));
	saddr.sin_addr.s_addr     = htonl(INADDR_LOOPBACK);
        saddr.sin_family          = AF_INET;
        saddr.sin_port            = sin.sin_port;

	/* connect in non blocking manner */
	result = connect (lock->pipe[0], (struct sockaddr *)&saddr, sizeof (saddr));
	if (errno != VORTEX_EINPROGRESS) {
		lua_vortex_error (L, "connect () returned %d, errno=%d:%s", 
				  result, errno, vortex_errno_get_last_error ());
		vortex_close_socket (listener_fd);
		return axl_false;
	}

	/* accept connection */
	lua_vortex_log  (LUA_VORTEX_DEBUG, "calling to accept () socket");
	lock->pipe[1] = vortex_listener_accept (listener_fd);

	if (lock->pipe[1] <= 0) {
		lua_vortex_error (L, "Unable to accept connection, failed to create pipe");
		vortex_close_socket (listener_fd);
		return axl_false;
	}
	/* set pipe read end from result returned by thread */
	lua_vortex_log (LUA_VORTEX_DEBUG, "Created pipe [%d, %d] for lua state %p", lock->pipe[0], lock->pipe[1], L);

	/* disable nagle */
	vortex_connection_set_sock_tcp_nodelay (lock->pipe[1], axl_true);

	/* close listener */
	vortex_close_socket (listener_fd);

	/* report and return fd */
	return axl_true;
}

/** 
 * Implements vortex.event_fd method. We assume caller already has the
 * lock.
 */
int lua_vortex_event_fd (lua_State * L)
{
	LuaVortexLock * lock;

	/* get the lua state lock */
	lock = axl_hash_get (lua_vortex_locks, L);
	if (lock == NULL) {
		lua_vortex_error (L, "Failed to get internal lock structure to report event fd");
		return 0;
	}

	/* check if event fd is created */
	if (lock->pipe[0] > 0) {
		/* fd is created, report it */
		lua_pushnumber (L, lock->pipe[0]);
		return 1;
	} /* end if */
		
	/* reached this point, event fd is not created, init portable
	   pipe */
	if (! __lua_vortex_event_fd_init_pipe (L, lock)) {
		lua_vortex_error (L, "Failed to init event fd pipe");
		return 1;
	} /* end if */

	if (lock->pipe[0] > 0) {
		/* fd is created, report it */
		lua_pushnumber (L, lock->pipe[0]);
		return 1;
	} /* end if */

	/* never reached */
	return 0;
}

/** 
 * Implements vortex.event_fd method. We assume caller already has the
 * lock.
 *
 * @param microseconds 
 */
int lua_vortex_wait_events (lua_State * L)
{
	LuaVortexLock * lock;
	fd_set          set;
	struct timeval  tv;
	long            microseconds;
	int             result;

	/* get microseconds to wait */
	if (lua_gettop (L) == 0) {
		microseconds = 0;
	} else 
		microseconds = lua_tonumber (L, 1);

	/* get the lock */
	lock = lua_vortex_lock_get (L);

	/* clear the set */
 keep_waiting:
	FD_ZERO (&set);

	/* set the value */
	FD_SET (lock->pipe[0], &set);

	if (microseconds >= 1000000) {
		/* more than one second */
		tv.tv_sec  = microseconds / 1000000;
		tv.tv_usec = microseconds - (tv.tv_sec * 1000000);
	} else {
		tv.tv_sec  = 0;
		tv.tv_usec = microseconds;

		/* detect case where the user didn't configure waiting
		   period */
		if (tv.tv_usec == 0)
			tv.tv_usec = 500000;
	}

	lua_vortex_log (LUA_VORTEX_DEBUG, "Waiting %ld.%ld", tv.tv_sec, tv.tv_usec);
	result  = select (lock->pipe[0] + 1, &set, NULL, NULL, &tv);
	lua_vortex_log (LUA_VORTEX_DEBUG, "  waiting result was: %d", result);

	/* check for infinite wait, until something happens */
	if (microseconds == 0 && ! (result > 0))
		goto keep_waiting;

	lua_pushboolean (L, result > 0);

	/* report result */
	return 1;
}

/** 
 * @brief Implementation of vortex.register_profile
 *
 */
int lua_vortex_register_profile (lua_State * L)
{
	VortexCtx     ** ctx;
	const char     * profile;
	LuaVortexRefs  * references  = NULL;
	LuaVortexRefs  * references2 = NULL;

	/* check parameters */
	if (! lua_vortex_check_params (L, "tsf|dfd")) {
		lua_vortex_error (L, "Expected to receive connection but received something different");
		return 0;
	} /* end if */

	/* get context */
	ctx = lua_touserdata (L, 1);

	/* check if the profile is not registered */
	profile = lua_tostring (L, 2);
	if (! vortex_profiles_is_registered (*ctx, profile)) {

		/* acquire references to frame received */
		references  = lua_vortex_acquire_references (*ctx, L, 3, 4, 0);
		vortex_ctx_set_data_full (*ctx, axl_strdup_printf ("%p", references), references,
					  axl_free, (axlDestroyFunc) lua_vortex_unref2);

		/* if start handler is defined, also acquire references */
		if (lua_gettop (L) >= 5) {
			lua_vortex_log (LUA_VORTEX_DEBUG, "Setting start handler associated to profile %s", profile);
			references2 = lua_vortex_acquire_references (*ctx, L, 5, 6, 0);
			vortex_ctx_set_data_full (*ctx, axl_strdup_printf ("%p", references2), references2,
						  axl_free, (axlDestroyFunc) lua_vortex_unref2);
		} /* end if */

		/* register profile */
		vortex_profiles_register (*ctx, profile, 
					  /* start handler */
					  references2 ? lua_vortex_channel_bridge_start_received : NULL, references2,
					  /* close handler */
					  NULL, NULL,
					  /* received handler */
					  lua_vortex_channel_bridge_frame_received, references);
	} /* end if */
	
	

	return 0;
}

/** 
 * @brief Implementation of vortex.create_listener
 *
 */
int lua_vortex_create_listener (lua_State * L)
{
	VortexCtx        ** ctx;
	VortexConnection  * listener;

	/* check parameters */
	if (! lua_vortex_check_params (L, "tss")) {
		lua_vortex_error (L, "Expected to receive connection but received something different");
		return 0;
	} /* end if */

	/* get context */
	ctx = lua_touserdata (L, 1);

	/* create a connection reference */
	listener = vortex_listener_new (*ctx, lua_tostring (L, 2), lua_tostring (L, 3), NULL, NULL);

	/* build connection reference */
	lua_vortex_connection_new_ref (L, listener, axl_true);

	return 1;
}

VortexCtx * lua_vortex_wait_listeners_ctx = NULL;

void lua_vortex_wait_listeners_sig_handler (int _signal)
{
	VortexCtx * ctx = lua_vortex_wait_listeners_ctx;

	/* nullify context */
	lua_vortex_wait_listeners_ctx = NULL;

	lua_vortex_log (LUA_VORTEX_DEBUG, "received signal %d, unlocking (ctx: %p)", _signal, ctx);
	vortex_listener_unlock (ctx);
	return;
}

/** 
 * @brief Implementation of vortex.wait_listeners
 */
int lua_vortex_wait_listeners (lua_State * L)
{
	VortexCtx        ** ctx;

	/* check parameters */
	if (! lua_vortex_check_params (L, "t|d")) {
		lua_vortex_error (L, "Expected to receive connection but received something different");
		return 0;
	} /* end if */

	/* get context */
	ctx = lua_touserdata (L, 1);

	/* install signal handling if defined */
	if (lua_gettop (L) > 1 && lua_toboolean (L, 2)) {
		/* record ctx */
		lua_vortex_wait_listeners_ctx = (*ctx);

		/* configure handler */
		signal (SIGTERM, lua_vortex_wait_listeners_sig_handler);
		signal (SIGQUIT, lua_vortex_wait_listeners_sig_handler);
		signal (SIGINT,  lua_vortex_wait_listeners_sig_handler);
	}

	/* unlock before wait */
	LUA_VORTEX_UNLOCK (L, axl_false);

	/* call to wait */
	vortex_listener_wait (*ctx);

	return 1;
}

/** 
 * @brief Implementation of vortex.wait_listeners
 */
int lua_vortex_unlock_listeners (lua_State * L)
{
	VortexCtx        ** ctx;

	/* check parameters */
	if (! lua_vortex_check_params (L, "t")) {
		lua_vortex_error (L, "Expected to receive connection but received something different");
		return 0;
	} /* end if */

	/* get context */
	ctx = lua_touserdata (L, 1);

	/* call to wait */
	vortex_listener_unlock (*ctx);

	return 1;
}

void lua_vortex_cleanup (void)
{
	/* cleanup lua state lock hash */
	axl_hash_free (lua_vortex_locks);
	return;
}

static const luaL_Reg vortex_funcs[] = {
	{ "yield", _lua_vortex_yield },
	{ "event_fd", lua_vortex_event_fd },
	{ "wait_events", lua_vortex_wait_events },
	{ "register_profile", lua_vortex_register_profile },
	{ "create_listener", lua_vortex_create_listener },
	{ "wait_listeners", lua_vortex_wait_listeners },
	{ "unlock_listeners", lua_vortex_unlock_listeners },
	{ NULL, NULL }
};

int luaopen_vortex (lua_State *L)
{

	/* init global mutex */
	vortex_mutex_create (&lua_vortex_locks_mutex);
	lua_vortex_locks = axl_hash_new (axl_hash_int, axl_hash_equal_int);

	/* register cleanup */
	atexit (lua_vortex_cleanup);

	/* make the caller to have the lock to touch this lua state */
	LUA_VORTEX_LOCK (L, axl_false);

	/* init vortex module */
	luaL_register (L, "vortex", vortex_funcs);

	/* call to register vortex.ctx */
	lua_vortex_ctx_init_module (L);
	lua_vortex_connection_init_module (L);
	lua_vortex_asyncqueue_init_module (L);
	lua_vortex_channel_init_module (L);
	lua_vortex_frame_init_module (L);
	lua_vortex_channelpool_init_module (L);

	return 1;
}



