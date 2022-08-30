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
#include <lua-vortex-tls.h>
#include <lua-vortex-private.h>
#include <vortex_tls.h>

/** 
 * implements vortex.tls.init
 */
int lua_vortex_tls_init (lua_State * L) {
	VortexCtx      ** ctx;
	
	/* check parameters */
	if (! lua_vortex_check_params (L, "t")) 
		return 0;

	/* get context */
	ctx = lua_touserdata (L, 1);

	/* call to init*/
	lua_pushboolean (L, vortex_tls_init (*ctx));

	return 1;
}

/** 
 * Auxiliar handler used to handle async notification reply for
 * vortex.tls.start_tls
 */
void lua_vortex_tls_bridge_async_notify (VortexConnection * connection,
					 VortexStatus       status,
					 char             * status_message,
					 axlPointer         user_data)
{
	LuaVortexRefs * references = user_data;
	lua_State     * L          = references->ref->L;
	lua_State     * thread;
	VortexCtx     * ctx        = CONN_CTX (connection);
	int             initial_top;
	int             error;

	lua_vortex_log (LUA_VORTEX_DEBUG, "Received notification to bridge async TLS notification (L: %p), marshalling..", L);

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

	/* push the connection */
	lua_vortex_connection_new_ref (thread, connection, axl_true);
	vortex_connection_unref (connection, "start tls");

	/* push status */
	lua_pushnumber (thread, status);

	/* push status_msg */
	lua_pushstring (thread, status_message);

	/* push user data */
	lua_rawgeti (thread, LUA_REGISTRYINDEX, references->ref2->ref_id);

	/* now call */
	lua_vortex_log (LUA_VORTEX_DEBUG, "About to call on async tls notify handler on lua space..");
	error = lua_pcall (thread, 4, 0, 0);

	/* handle error */
	lua_vortex_handle_error (thread, error, "on async tls notify");

	/* unlock during operations */
	LUA_VORTEX_UNLOCK (L, axl_true);

	return;
}

/** 
 * implements vortex.tls.start_tls
 */
int lua_vortex_tls_start_tls (lua_State * L) {
	VortexConnection   ** conn;
	VortexConnection    * conn2;
	VortexStatus          status              = VortexError;
	char                * status_msg          = NULL;
	const char          * serverName          = NULL;
	LuaVortexRefs       * references          = NULL;
	
	/* check parameters */
	if (! lua_vortex_check_params (L, "o|zfd")) 
		return 0;

	/* get context */
	conn = lua_touserdata (L, 1);

	/* get serverName value */
	if (lua_gettop (L) > 1)
		serverName = lua_tostring (L, 2);

	/* acquire a reference to the connection during the process */
	if (! vortex_connection_ref (*conn, "start tls")) {
		lua_vortex_error (L, "Failed to acquire reference to the connection during handshake..");
		return 0;
	} /* end if */

	lua_vortex_log (LUA_VORTEX_DEBUG, "Starting TLS process over conn-id=%d, with ref count=%d", 
			vortex_connection_get_id (*conn), vortex_connection_ref_count (*conn));

	/* check for async notification */
	if (lua_gettop (L) > 2) {
		lua_vortex_log (LUA_VORTEX_DEBUG, "Detected async tls activation..");
		/* create reference */
		references = lua_vortex_acquire_references (CONN_CTX (*conn), L, 3, 4, 0);
		
		/* call to start TLS async */
		vortex_tls_start_negotiation (*conn, lua_tostring (L, 2), 
					      lua_vortex_tls_bridge_async_notify,
					      references);
		
		return 0;
	} /* end if */

	/* unlock during operation */
	LUA_VORTEX_UNLOCK (L, axl_false);

	/* call to authenticate in a blocking manner */
	conn2   = vortex_tls_start_negotiation_sync (*conn, serverName, &status, &status_msg);

	lua_vortex_log (LUA_VORTEX_DEBUG, "Finished TLS process with status=%d, message=%s (old ref count: %d)", 
			status, status_msg, vortex_connection_ref_count (*conn));

	/* unlock during operation */
	LUA_VORTEX_LOCK (L, axl_false);

	if (status != 2) {
		/* reduce reference because tls process didn't finished */
		vortex_connection_unref (*conn, "start tls");
	} else {
		/* ok tls was ok, flag old connection as transient to
		 * only unref */
		lua_vortex_log (LUA_VORTEX_DEBUG, "Flagging connection id=%d as transient due to TLS ok status",
				vortex_connection_get_id (*conn));
		lua_vortex_metatable_set_bool (L, 1, "transient", axl_true);
	} /* end if */

	/* build connection result */
	lua_vortex_connection_new_ref (L, conn2, axl_false);
	vortex_connection_unref (conn2, "start tls");

	/* push status */
	lua_pushnumber (L, status);
	lua_pushstring (L, status_msg);
	
	return 3;
}

axl_bool lua_vortex_tls_bridge_accept_handler (VortexConnection * conn, 
					       const char       * serverName)
{
	VortexCtx     * ctx        = CONN_CTX (conn);
	LuaVortexRefs * references = vortex_ctx_get_data (ctx, "lua:tls:accept");
	lua_State     * L          = references->ref->L;
	lua_State     * thread;
	int             initial_top;
	int             error;
	axl_bool        result     = axl_false;

	lua_vortex_log (LUA_VORTEX_DEBUG, "Received notification to bridge accept tls handler (L: %p), marshalling..", L);

	/* create thread to represent this context */
	thread = lua_vortex_create_thread (ctx, L, axl_true);

	/* check if vortex is finish to skip bridging */
	if (vortex_is_exiting (ctx) || thread == NULL)
		return axl_false;

	lua_vortex_log (LUA_VORTEX_DEBUG, "   thread created, acquiring lock ..");

	/* lock during operations */
	LUA_VORTEX_LOCK (L, axl_true);

	lua_vortex_log (LUA_VORTEX_DEBUG, "   lock acquired ..");

	/* check if we are in process of finishing */
	if (vortex_is_exiting (ctx)) {
		lua_vortex_log (LUA_VORTEX_DEBUG, "   found current state finishing ...");

		LUA_VORTEX_UNLOCK (L, axl_true);
		return axl_false;
	}

	/* get new initial top */
	initial_top = lua_gettop (thread);

	lua_vortex_log (LUA_VORTEX_DEBUG, "Found thread %p (top: %d), preparing call..", thread, initial_top);

	/* push the handler to call */
	lua_rawgeti (thread, LUA_REGISTRYINDEX, references->ref->ref_id);

	/* push the connection */
	lua_vortex_connection_new_ref (thread, conn, axl_true);

	/* push serverName */
	lua_pushstring (thread, serverName);

	/* push user data */
	lua_rawgeti (thread, LUA_REGISTRYINDEX, references->ref2->ref_id);

	/* now call */
	lua_vortex_log (LUA_VORTEX_DEBUG, "About to call on accept tls handler on lua space..");
	error = lua_pcall (thread, 3, 1, 0);

	/* handle error */
	lua_vortex_handle_error (thread, error, "on accept tls");

	if (lua_gettop (thread) > 0 && lua_isboolean (thread, -1)) {
		result = lua_toboolean (thread, -1);
		lua_vortex_log (LUA_VORTEX_DEBUG, "lua accept tls handler returned: %d", result);
	} /* end if */

	/* unlock during operations */
	LUA_VORTEX_UNLOCK (L, axl_true);

	return result;
}

char * lua_vortex_tls_bridge_cert_handler (VortexConnection * conn, 
					   const char       * serverName)
{
	VortexCtx     * ctx        = CONN_CTX (conn);
	LuaVortexRefs * references = vortex_ctx_get_data (ctx, "lua:tls:cert");
	lua_State     * L          = references->ref->L;
	lua_State     * thread;
	int             initial_top;
	int             error;
	char          * result     = NULL;

	lua_vortex_log (LUA_VORTEX_DEBUG, "Received notification to bridge cert tls handler (L: %p), marshalling..", L);

	/* create thread to represent this context */
	thread = lua_vortex_create_thread (ctx, L, axl_true);

	/* check if vortex is finish to skip bridging */
	if (vortex_is_exiting (ctx) || thread == NULL)
		return axl_false;

	lua_vortex_log (LUA_VORTEX_DEBUG, "   thread created, acquiring lock ..");

	/* lock during operations */
	LUA_VORTEX_LOCK (L, axl_true);

	lua_vortex_log (LUA_VORTEX_DEBUG, "   lock acquired ..");

	/* check if we are in process of finishing */
	if (vortex_is_exiting (ctx)) {
		lua_vortex_log (LUA_VORTEX_DEBUG, "   found current state finishing ...");

		LUA_VORTEX_UNLOCK (L, axl_true);
		return axl_false;
	}

	/* get new initial top */
	initial_top = lua_gettop (thread);

	lua_vortex_log (LUA_VORTEX_DEBUG, "Found thread %p (top: %d), preparing call..", thread, initial_top);

	/* push the handler to call */
	lua_rawgeti (thread, LUA_REGISTRYINDEX, references->ref->ref_id);

	/* push the connection */
	lua_vortex_connection_new_ref (thread, conn, axl_true);

	/* push serverName */
	lua_pushstring (thread, serverName);

	/* push user data */
	lua_rawgeti (thread, LUA_REGISTRYINDEX, references->ref2->ref_id);

	/* now call */
	lua_vortex_log (LUA_VORTEX_DEBUG, "About to call on cert tls handler on lua space..");
	error = lua_pcall (thread, 3, 1, 0);

	/* handle error */
	lua_vortex_handle_error (thread, error, "on cert tls");

	/* get result from handler */
	if (lua_gettop (thread) > 0 && lua_isstring (thread, -1)) {
		result = axl_strdup (lua_tostring (thread, -1));
		lua_vortex_log (LUA_VORTEX_DEBUG, "lua cert handler returned: %s", result);
	}

	/* unlock during operations */
	LUA_VORTEX_UNLOCK (L, axl_true);

	return result;
}

char * lua_vortex_tls_bridge_key_handler (VortexConnection * conn, 
					   const char       * serverName)
{
	VortexCtx     * ctx        = CONN_CTX (conn);
	LuaVortexRefs * references = vortex_ctx_get_data (ctx, "lua:tls:key");
	lua_State     * L          = references->ref->L;
	lua_State     * thread;
	int             initial_top;
	int             error;
	char          * result     = NULL;

	lua_vortex_log (LUA_VORTEX_DEBUG, "Received notification to bridge key tls handler (L: %p), marshalling..", L);

	/* create thread to represent this context */
	thread = lua_vortex_create_thread (ctx, L, axl_true);

	/* check if vortex is finish to skip bridging */
	if (vortex_is_exiting (ctx) || thread == NULL)
		return axl_false;

	lua_vortex_log (LUA_VORTEX_DEBUG, "   thread created, acquiring lock ..");

	/* lock during operations */
	LUA_VORTEX_LOCK (L, axl_true);

	lua_vortex_log (LUA_VORTEX_DEBUG, "   lock acquired ..");

	/* check if we are in process of finishing */
	if (vortex_is_exiting (ctx)) {
		lua_vortex_log (LUA_VORTEX_DEBUG, "   found current state finishing ...");

		LUA_VORTEX_UNLOCK (L, axl_true);
		return axl_false;
	}

	/* get new initial top */
	initial_top = lua_gettop (thread);

	lua_vortex_log (LUA_VORTEX_DEBUG, "Found thread %p (top: %d), preparing call..", thread, initial_top);

	/* push the handler to call */
	lua_rawgeti (thread, LUA_REGISTRYINDEX, references->ref->ref_id);

	/* push the connection */
	lua_vortex_connection_new_ref (thread, conn, axl_true);

	/* push serverName */
	lua_pushstring (thread, serverName);

	/* push user data */
	lua_rawgeti (thread, LUA_REGISTRYINDEX, references->ref2->ref_id);

	/* now call */
	lua_vortex_log (LUA_VORTEX_DEBUG, "About to call on key tls handler on lua space..");
	error = lua_pcall (thread, 3, 1, 0);

	/* handle error */
	lua_vortex_handle_error (thread, error, "on key tls");

	/* get result from handler */
	if (lua_gettop (thread) > 0 && lua_isstring (thread, -1)) {
		result = axl_strdup (lua_tostring (thread, -1));
		lua_vortex_log (LUA_VORTEX_DEBUG, "lua key handler returned: %s", result);
	}

	/* unlock during operations */
	LUA_VORTEX_UNLOCK (L, axl_true);

	return result;
}

/** 
 * implements vortex.tls.accept_tls
 */
int lua_vortex_tls_accept_tls (lua_State * L) {

	VortexCtx      ** ctx;
	LuaVortexRefs   * references  = NULL;
	LuaVortexRefs   * references2 = NULL;
	LuaVortexRefs   * references3 = NULL;
	
	/* check parameters */
	if (! lua_vortex_check_params (L, "t|fdfdfd")) 
		return 0;

	/* get context */
	ctx = lua_touserdata (L, 1);

	/* acquire references to accept handler */
	if (lua_gettop (L) > 1) {
		/* get accept handler */
		references = lua_vortex_acquire_references (*ctx, L, 2, 3, 0);

		/* store reference */
		vortex_ctx_set_data_full (*ctx, axl_strdup_printf ("%p", references), references,
						 axl_free, (axlDestroyFunc) lua_vortex_unref2); 
		vortex_ctx_set_data (*ctx, "lua:tls:accept", references);
	} /* end if */

	/* acquire references to cert handler */
	if (lua_gettop (L) > 3) {
		/* get cert handler */
		references2 = lua_vortex_acquire_references (*ctx, L, 4, 5, 0);

		/* store reference */
		vortex_ctx_set_data_full (*ctx, axl_strdup_printf ("%p", references2), references2,
						 axl_free, (axlDestroyFunc) lua_vortex_unref2); 
		vortex_ctx_set_data (*ctx, "lua:tls:cert", references2);
	} /* end if */

	/* acquire references to key handler */
	if (lua_gettop (L) > 5) {
		/* get key handler */
		references3 = lua_vortex_acquire_references (*ctx, L, 6, 7, 0);

		/* store reference */
		vortex_ctx_set_data_full (*ctx, axl_strdup_printf ("%p", references3), references3,
						 axl_free, (axlDestroyFunc) lua_vortex_unref2); 
		vortex_ctx_set_data (*ctx, "lua:tls:key", references3);
	} /* end if */

	/* now configure handler */
	lua_pushboolean (L, vortex_tls_accept_negotiation (*ctx,
							   /* accept handler */
							   references ? lua_vortex_tls_bridge_accept_handler : NULL,
							   /* cert handler */
							   references2 ? lua_vortex_tls_bridge_cert_handler : NULL,
							   /* key handler */
							   references3 ? lua_vortex_tls_bridge_key_handler : NULL));
	return 1;
}

static const luaL_Reg vortex_funcs[] = {
	{ "init", lua_vortex_tls_init },
	{ "start_tls", lua_vortex_tls_start_tls },
	{ "accept_tls", lua_vortex_tls_accept_tls },
	{ NULL, NULL }
};

int luaopen_vortex_tls (lua_State *L)
{
	/* init vortex module */
	luaL_register (L, "vortex.tls", vortex_funcs);

	return 1;
}



