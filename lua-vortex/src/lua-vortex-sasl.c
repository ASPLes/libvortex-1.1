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
#include <lua-vortex-sasl.h>
#include <lua-vortex-private.h>
#include <vortex_sasl.h>

/** 
 * implements vortex.sasl.init
 */
int lua_vortex_sasl_init (lua_State * L) {
	VortexCtx      ** ctx;
	
	/* check parameters */
	if (! lua_vortex_check_params (L, "t")) 
		return 0;

	/* get context */
	ctx = lua_touserdata (L, 1);

	/* call to init*/
	lua_pushboolean (L, vortex_sasl_init (*ctx));

	return 1;
}

const char * lua_vortex_sasl_normalize_profile_name (const char * profile)
{
	/* check for profiles not started with http://iana.org/beep/SASL/ */
	if (axl_memcmp (profile, "http://iana.org/beep/SASL", 25))
		return profile;

	/* check for the appropriate profile */
	if (axl_casecmp (VORTEX_SASL_PLAIN + 26, profile))
		return VORTEX_SASL_PLAIN;

	if (axl_casecmp (VORTEX_SASL_ANONYMOUS + 26, profile))
		return VORTEX_SASL_ANONYMOUS;

	if (axl_casecmp (VORTEX_SASL_EXTERNAL + 26, profile))
		return VORTEX_SASL_EXTERNAL;

	if (axl_casecmp (VORTEX_SASL_CRAM_MD5 + 26, profile))
		return VORTEX_SASL_CRAM_MD5;

	if (axl_casecmp (VORTEX_SASL_DIGEST_MD5 + 26, profile))
		return VORTEX_SASL_DIGEST_MD5;

	if (axl_casecmp (VORTEX_SASL_GSSAPI + 26, profile))
		return VORTEX_SASL_GSSAPI;

	if (axl_casecmp (VORTEX_SASL_KERBEROS_V4 + 26, "kerberos_v4"))
		return VORTEX_SASL_KERBEROS_V4;
	
	return NULL;
}

/** 
 * Auxiliar function used to notify auth completion status
 */
void lua_vortex_sasl_auth_notify (VortexConnection * conn,
				  VortexStatus       status,
				  char             * status_message,
				  axlPointer         user_data)
{
	LuaVortexRefs * references = user_data;
	lua_State     * L          = references->ref->L;
	lua_State     * thread;
	int             initial_top;
	VortexCtx     * ctx         = CONN_CTX (conn);
	int             error;

	lua_vortex_log (LUA_VORTEX_DEBUG, "Received request bridge to bridge sasl auth notify (L: %p), marshalling..", L);

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

	/* push status */
	lua_pushnumber (thread, status);

	/* push status message */
	lua_pushstring (thread, status_message);

	/* push user data */
	lua_rawgeti (thread, LUA_REGISTRYINDEX, references->ref2->ref_id);

	/* now call */
	lua_vortex_log (LUA_VORTEX_DEBUG, "About to call sasl auth notify handler on lua space..");
	error = lua_pcall (thread, 4, 0, 0);
	
	/* handle error code if any */
	lua_vortex_handle_error (thread, error, "sasl auth notify");

	/* unlock during operations */
	LUA_VORTEX_UNLOCK (L, axl_true);

	return;
}

/** 
 * implements vortex.sasl.start_auth
 */
int lua_vortex_sasl_auth (lua_State * L) {
	VortexConnection    ** conn;
	VortexStatus           status           = -1;
	char                 * status_msg       = NULL;
	const char           * profile;
	LuaVortexRefs        * references       = NULL;
	
	/* check parameters:               123 45678 */
	if (! lua_vortex_check_params (L, "oss|zzzfd")) 
		return 0;

	/* get context */
	conn = lua_touserdata (L, 1);

	/* set properties */
	vortex_sasl_set_propertie (*conn, VORTEX_SASL_AUTH_ID, axl_strdup (lua_tostring (L, 3)), axl_free);
	vortex_sasl_set_propertie (*conn, VORTEX_SASL_PASSWORD, axl_strdup (lua_tostring (L, 4)), axl_free);
	vortex_sasl_set_propertie (*conn, VORTEX_SASL_AUTHORIZATION_ID, axl_strdup (lua_tostring (L, 5)), axl_free);
	vortex_sasl_set_propertie (*conn, VORTEX_SASL_REALM, axl_strdup (lua_tostring (L, 6)), axl_free);
	vortex_sasl_set_propertie (*conn, VORTEX_SASL_ANONYMOUS_TOKEN, axl_strdup (lua_tostring (L, 3)), axl_free);

	/* start SASL auth process */
	profile = lua_vortex_sasl_normalize_profile_name (lua_tostring (L, 2));

	if (lua_gettop (L) > 6) {
		/* grab references */
		references = lua_vortex_acquire_references (CONN_CTX(*conn), L, 7, 8, 0);

		/* store references in connection */
		vortex_connection_set_data_full (*conn, axl_strdup_printf ("%p", references), references,
						 axl_free, (axlDestroyFunc) lua_vortex_unref2); 

		/* start async auth */
		vortex_sasl_start_auth (*conn, profile, lua_vortex_sasl_auth_notify, references);

		/* return nil */
		lua_pushnil (L);
		return 1;
	}

	vortex_sasl_start_auth_sync (*conn, profile, &status, &status_msg);

	/* return values */
	lua_pushnumber (L, status);
	lua_pushstring (L, status_msg);
	
	/* return result */
	return 2;
}

axlPointer lua_vortex_sasl_auth_handler_bridge (VortexConnection * conn,
						VortexSaslProps  * props,
						axlPointer         user_data)
{
	axl_bool        status     = axl_false;
	LuaVortexRefs * references = user_data;
	lua_State     * L          = references->ref->L;
	lua_State     * thread;
	int             initial_top;
	VortexCtx     * ctx         = CONN_CTX (conn);
	char          * password    = NULL;
	int             error;

	lua_vortex_log (LUA_VORTEX_DEBUG, "Received auth handler notification on bridge function (L: %p), marshalling..", L);

	/* create thread to represent this context */
	thread = lua_vortex_create_thread (ctx, L, axl_true);

	/* check if vortex is finish to skip bridging */
	if (vortex_is_exiting (ctx) || thread == NULL)
		return INT_TO_PTR (axl_false); /* always fail to auth on exit */

	lua_vortex_log (LUA_VORTEX_DEBUG, "   thread created, acquiring lock ..");

	/* lock during operations */
	LUA_VORTEX_LOCK (L, axl_true);

	lua_vortex_log (LUA_VORTEX_DEBUG, "   lock acquired ..");

	/* check if we are in process of finishing */
	if (vortex_is_exiting (ctx)) {
		lua_vortex_log (LUA_VORTEX_DEBUG, "   found current state finishing ...");

		LUA_VORTEX_UNLOCK (L, axl_true);
		return INT_TO_PTR (axl_false); /* always fail to auth on exit */
	}

	/* get new initial top */
	initial_top = lua_gettop (thread);

	lua_vortex_log (LUA_VORTEX_DEBUG, "Found thread %p (top: %d), preparing call..", thread, initial_top);

	/* push the handler to call */
	lua_rawgeti (thread, LUA_REGISTRYINDEX, references->ref->ref_id);

	lua_vortex_log (LUA_VORTEX_DEBUG, "Pushed set_on_close handler, ref_id=%d..", references->ref->ref_id);

	/* push the connection */
	lua_vortex_connection_new_ref (thread, conn, axl_true);

	/* now push auth props */
	lua_newtable (thread);

	/* mech */
	lua_pushstring (thread, props->mech);
	lua_setfield (thread, -2, "mech");

	/* anonymous_token */
	lua_pushstring (thread, props->anonymous_token);
	lua_setfield (thread, -2, "anonymous_token");

	/* auth_id */
	lua_pushstring (thread, props->auth_id);
	lua_setfield (thread, -2, "auth_id");

	/* authorization_id */
	lua_pushstring (thread, props->authorization_id);
	lua_setfield (thread, -2, "authorization_id");

	/* password */
	lua_pushstring (thread, props->password);
	lua_setfield (thread, -2, "password");

	/* realm */
	lua_pushstring (thread, props->realm);
	lua_setfield (thread, -2, "realm");

	/* push user data */
	lua_rawgeti (thread, LUA_REGISTRYINDEX, references->ref2->ref_id);

	/* now call */
	lua_vortex_log (LUA_VORTEX_DEBUG, "About to call sasl auth handler on lua space..");
	error = lua_pcall (thread, 3, 1, 0);
	
	/* handle error code if any */
	if (lua_vortex_handle_error (thread, error, "sasl auth")) 
		status = axl_false;

	/* get status */
	if (error == 0 && lua_gettop (thread)) {
		if (lua_isboolean (thread, -1)) {
			status = lua_toboolean (thread, -1);
			lua_vortex_log (LUA_VORTEX_DEBUG, "   sasl auth handler returned boolean value: %d..", status);
		} else if (lua_isstring (thread, -1)) {
			password = (char *) lua_tostring (thread, -1);
			props->return_password = axl_true;
			lua_vortex_log (LUA_VORTEX_DEBUG, "   sasl auth handler returned string: %s..", password);
		}
	} 

	/* unlock during operations */
	LUA_VORTEX_UNLOCK (L, axl_true);

	/* return a password or status */
	if (props->return_password)
		return password;
	return INT_TO_PTR (status);
}

/** 
 * Implements vortex.sasl.accept_mech
 */
int lua_vortex_sasl_accept_mech (lua_State * L)
{
	VortexCtx      ** ctx;
	LuaVortexRefs   * references;
	const char      * profile;

	/* check params */
	if (! lua_vortex_check_params (L, "tsf|d")) 
		return 0;

	/* get reference to vortex.ctx */
	ctx = lua_touserdata (L, 1);
	
	/* grab references */
	references = lua_vortex_acquire_references (*ctx, L, 3, 4, 0);

	/* store references in ctx */
	vortex_ctx_set_data_full (*ctx, axl_strdup_printf ("%p", references), references,
				  axl_free, (axlDestroyFunc) lua_vortex_unref2); 

	/* register SASL profile handler */
	profile = lua_vortex_sasl_normalize_profile_name (lua_tostring (L, 2));
	lua_vortex_log (LUA_VORTEX_DEBUG, "Accepting SASL negotiation for profile: %s", profile);
	lua_pushboolean (L, vortex_sasl_accept_negotiation_common (*ctx, profile, lua_vortex_sasl_auth_handler_bridge, references));

	return 1;
}

/** 
 * Implements vortex.sasl.is_authenticated
 */
int lua_vortex_sasl_is_authenticated (lua_State * L)
{
	VortexConnection ** conn;

	if (! lua_vortex_check_params (L, "o"))
		return 0;

	/* get reference */
	conn = lua_touserdata (L, 1);

	/* push current state */
	lua_pushboolean (L, vortex_sasl_is_authenticated (*conn));

	return 1;
}

/** 
 * Implements vortex.sasl.method_used
 */
int lua_vortex_sasl_method_used (lua_State * L)
{
	VortexConnection ** conn;

	if (! lua_vortex_check_params (L, "o"))
		return 0;

	/* get reference */
	conn = lua_touserdata (L, 1);

	/* push current state */
	lua_pushstring (L, vortex_sasl_auth_method_used (*conn));

	return 1;
}

/** 
 * Implements vortex.sasl.auth_id
 */
int lua_vortex_sasl_auth_id (lua_State * L)
{
	VortexConnection ** conn;

	if (! lua_vortex_check_params (L, "o"))
		return 0;

	/* get reference */
	conn = lua_touserdata (L, 1);

	/* push current state */
	lua_pushstring (L, AUTH_ID_FROM_CONN (*conn));

	return 1;
}

static const luaL_Reg vortex_funcs[] = {
	{ "init", lua_vortex_sasl_init },
	{ "start_auth", lua_vortex_sasl_auth },
	{ "is_authenticated", lua_vortex_sasl_is_authenticated },
	{ "method_used", lua_vortex_sasl_method_used },
	{ "auth_id", lua_vortex_sasl_auth_id },
	{ "accept_mech", lua_vortex_sasl_accept_mech },
	{ NULL, NULL }
};

int luaopen_vortex_sasl (lua_State *L)
{
	/* init vortex module */
	luaL_register (L, "vortex.sasl", vortex_funcs);

	/* table is in the top, register variables */
	lua_pushstring (L, VORTEX_SASL_PLAIN);
	lua_setfield (L, -2, "PLAIN");

	lua_pushstring (L, VORTEX_SASL_ANONYMOUS);
	lua_setfield (L, -2, "ANONYMOUS");

	lua_pushstring (L, VORTEX_SASL_EXTERNAL);
	lua_setfield (L, -2, "EXTERNAL");

	lua_pushstring (L, VORTEX_SASL_CRAM_MD5);
	lua_setfield (L, -2, "CRAM_MD5");

	lua_pushstring (L, VORTEX_SASL_DIGEST_MD5);
	lua_setfield (L, -2, "DIGEST_MD5");

	return 1;
}



