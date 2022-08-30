/* 
 *  Lua Vortex:  Lua bindings for Vortex Library
 *  Copyright (C) 2022 Advanced Software Production Line, S.L.
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

#ifndef __LUA_VORTEX_H__
#define __LUA_VORTEX_H__

/* lua includes */
#define LUA_LIB
#include <lua.h>
#include <lauxlib.h>

/* include vortex */
#include <vortex.h>

/* type definitions */
typedef struct _LuaVortexObject LuaVortexObject;
typedef struct _LuaVortexRef LuaVortexRef;
typedef struct _LuaVortexRefs LuaVortexRefs;

/* lua vortex includes */
#include <lua-vortex-ctx.h>
#include <lua-vortex-connection.h>
#include <lua-vortex-asyncqueue.h>
#include <lua-vortex-object.h>
#include <lua-vortex-channel.h>
#include <lua-vortex-frame.h>
#include <lua-vortex-channelpool.h>

/** 
 * @brief Enum used to configure debug level used by the binding.
 */
typedef enum {
	/** 
	 * @brief Debug and information messages.
	 */
	LUA_VORTEX_DEBUG   = 1,
	/** 
	 * @brief Warning messages
	 */
	LUA_VORTEX_WARNING = 2,
	/** 
	 * @brief Critical messages.
	 */
	LUA_VORTEX_CRITICAL   = 3,
} LuaVortexLog;

/* console debug support:
 *
 * If enabled, the log reporting is activated as usual. 
 */
#if defined(ENABLE_LUA_VORTEX_LOG)
# define lua_vortex_log(l, m, ...)    do{_lua_vortex_log  (__AXL_FILE__, __AXL_LINE__, l, m, ##__VA_ARGS__);}while(0)
# define lua_vortex_log2(l, m, ...)   do{_lua_vortex_log2  (__AXL_FILE__, __AXL_LINE__, l, m, ##__VA_ARGS__);}while(0)
#else
# if defined(AXL_OS_WIN32) && !( defined(__GNUC__) || _MSC_VER >= 1400)
/* default case where '...' is not supported but log is still
 * disabled */
#   define lua_vortex_log _lua_vortex_log
#   define lua_vortex_log2 _lua_vortex_log2
# else
#   define lua_vortex_log(l, m, ...) /* nothing */
#   define lua_vortex_log2(l, m, message, ...) /* nothing */
# endif
#endif

void _lua_vortex_log (const char          * file,
		     int                   line,
		     LuaVortexLog           log_level,
		     const char          * message,
		     ...);

void _lua_vortex_log2 (const char          * file,
		      int                   line,
		      LuaVortexLog           log_level,
		      const char          * message,
		      ...);

axl_bool lua_vortex_log_is_enabled       (void);

axl_bool lua_vortex_log2_is_enabled      (void);

axl_bool lua_vortex_color_log_is_enabled (void);

void lua_vortex_error (lua_State * L, const char * error,...);

axl_bool lua_vortex_handle_error (lua_State * L, int error_code, const char * label);

#define show_stack(L) lua_vortex_show_stack (L,__AXL_PRETTY_FUNCTION__, __AXL_FILE__, __AXL_LINE__)

void lua_vortex_show_stack (lua_State * L, const char * function, const char * file, int line);

axl_bool lua_vortex_check_params (lua_State * L, const char * params);

lua_State * lua_vortex_create_thread (VortexCtx * ctx, lua_State * L, axl_bool acquire_lock);

/*** lua state locking ***/
#define LUA_VORTEX_LOCK(L,signal_fd) do { lua_vortex_lock (L, signal_fd, __AXL_PRETTY_FUNCTION__, __AXL_LINE__); } while (0)
#define LUA_VORTEX_UNLOCK(L,signal_fd) do { lua_vortex_unlock (L, signal_fd, __AXL_PRETTY_FUNCTION__, __AXL_LINE__); } while (0)

void     lua_vortex_lock (lua_State * L, axl_bool signal_fd, const char * file, int line);

void     lua_vortex_unlock (lua_State * L, axl_bool signal_fd, const char * file, int line);

/*** internal references ***/
LuaVortexRef * lua_vortex_ref (lua_State * L);

void lua_vortex_unref (LuaVortexRef * unref);

LuaVortexRefs * lua_vortex_acquire_references  (VortexCtx * ctx, 
						lua_State * L, 
						int         ref_position, 
						int         ref_position2,
						int         ref_position3);

void lua_vortex_unref2 (LuaVortexRefs * references);

/*** metatable manipulation ***/
void         lua_vortex_metatable_set_bool     (lua_State  * L, 
						int          stack_position, 
						const char * key, 
						axl_bool     value);

axl_bool     lua_vortex_metatable_get_bool     (lua_State  * L, 
						int          stack_position, 
						const char * key);

/*** entry point for lua ***/
int luaopen_vortex (lua_State *L);

#endif 

