/* 
 *  Lua Vortex:  Lua bindings for Vortex Library
 *  Copyright (C) 2025 Advanced Software Production Line, S.L.
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
#include <lua-vortex-alive.h>
#include <lua-vortex-private.h>
#include <vortex_alive.h>

/** 
 * implements vortex.alive.init
 */
int lua_vortex_alive_init (lua_State * L) {
	VortexCtx      ** ctx;
	
	/* check parameters */
	if (! lua_vortex_check_params (L, "t")) 
		return 0;

	/* get context */
	ctx = lua_touserdata (L, 1);

	/* call to init*/
	lua_pushboolean (L, vortex_alive_init (*ctx));

	return 1;
}

/** 
 * Auxiliar function used by vortex.alive.enable_check to bridge
 * failure handler notifications
 */ 
void lua_vortex_alive_bridge_failure_handler (VortexConnection * conn, 
					      long               check_period, 
					      int                unreply_count)
{
	LuaVortexRefs * references = vortex_connection_get_data (conn, "lua:conn:alive:fh");
	lua_State     * L          = references->ref->L;
	lua_State     * thread;
	int             initial_top;
	VortexCtx     * ctx         = CONN_CTX (conn);
	int             error;

	lua_vortex_log (LUA_VORTEX_DEBUG, "Received notification to bridge failure handler function (L: %p), marshalling..", L);

	/* create thread to represent this context */
	thread = lua_vortex_create_thread (ctx, L, axl_true);

	/* check if vortex is finish to skip bridging */
	if (vortex_is_exiting (ctx) || thread == NULL)
		return;

	lua_vortex_log (LUA_VORTEX_DEBUG, "   thread created, acquiring lock ..");

	/* lock during operations */
	LUA_VORTEX_LOCK (L, axl_true);

	lua_vortex_log (LUA_VORTEX_DEBUG, "   lock acquired ..");

	/* check if we are in process of finishing */
	if (vortex_is_exiting (ctx)) {
		lua_vortex_log (LUA_VORTEX_DEBUG, "   found current state finishing ...");

		LUA_VORTEX_UNLOCK (L, axl_true);
		return;
	}

	/* get new initial top */
	initial_top = lua_gettop (thread);

	lua_vortex_log (LUA_VORTEX_DEBUG, "Found thread %p (top: %d), preparing call..", thread, initial_top);

	/* push the handler to call */
	lua_rawgeti (thread, LUA_REGISTRYINDEX, references->ref->ref_id);

	lua_vortex_log (LUA_VORTEX_DEBUG, "Pushed set_on_close handler, ref_id=%d..", references->ref->ref_id);

	/* push the connection */
	lua_vortex_connection_new_ref (thread, conn, axl_true);

	/* push rest of parameters */
	lua_pushnumber (thread, check_period);
	lua_pushnumber (thread, unreply_count);

	/* push user data */
	lua_rawgeti (thread, LUA_REGISTRYINDEX, references->ref2->ref_id);

	/* now call */
	lua_vortex_log (LUA_VORTEX_DEBUG, "About to call ALIVE failure handler on lua space..");
	error = lua_pcall (thread, 4, 0, 0);

	/* handle error */
	lua_vortex_handle_error (thread, error, "ALIVE failure handler");

	/* unlock during operations */
	LUA_VORTEX_UNLOCK (L, axl_true);

	return;
}

/** 
 * implements vortex.alive.enable_check
 */
int lua_vortex_alive_enable_check (lua_State * L) {
	VortexConnection ** conn;
	LuaVortexRefs     * references = NULL;
	
	/* check parameters */
	if (! lua_vortex_check_params (L, "oii|fd")) 
		return 0;

	/* get connection */
	conn = lua_touserdata (L, 1);

	if (lua_gettop (L) > 3) {
		/* get references */
		references = lua_vortex_acquire_references (CONN_CTX (*conn), L, 4, 5, 0);
		vortex_connection_set_data_full (*conn, axl_strdup_printf ("%p", references), references,
						 axl_free, (axlDestroyFunc) lua_vortex_unref2); 
		vortex_connection_set_data (*conn, "lua:conn:alive:fh", references);
	}

	/* call to enable check */
	lua_pushboolean (L, vortex_alive_enable_check (*conn, lua_tonumber (L, 2), lua_tonumber (L, 3),
						       references ? lua_vortex_alive_bridge_failure_handler : NULL));

	return 1;
}

static const luaL_Reg vortex_funcs[] = {
	{ "init", lua_vortex_alive_init },
	{ "enable_check", lua_vortex_alive_enable_check },
	{ NULL, NULL }
};

int luaopen_vortex_alive (lua_State *L)
{
	/* init vortex module */
	luaL_register (L, "vortex.alive", vortex_funcs);

	return 1;
}



