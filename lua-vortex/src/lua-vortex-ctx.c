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

static int lua_vortex_ctx_gc (lua_State *L) {
	VortexCtx ** ctx       = lua_touserdata (L, 1);
	int          ref_count = vortex_ctx_ref_count (*ctx);
	lua_vortex_log (LUA_VORTEX_DEBUG, "Finishing VortexCtx reference associated to %p (ref count: %d)", *ctx, ref_count);

	/* check if this is the last reference */
	if (lua_vortex_metatable_get_bool (L, 1, "initialized") && *ctx) {
		/* we are about to call finish, unlock to release any
		   waiting thread but signal other thread we are finish */
		LUA_VORTEX_UNLOCK (L, axl_false);

		lua_vortex_log (LUA_VORTEX_DEBUG, "   calling to terminate context", *ctx, ref_count);
		vortex_exit_ctx (*ctx, axl_true);

		/* relock again */
		LUA_VORTEX_LOCK (L, axl_false);

	} else {
		lua_vortex_log (LUA_VORTEX_DEBUG, "   releasing reference", *ctx, ref_count);
		/* call to unref */
		vortex_ctx_unref (ctx);
	}

	/* nullify */
	(*ctx) = NULL;
	return 0;
}

static int lua_vortex_ctx_tostring (lua_State *L) {
	VortexCtx ** ctx = lua_touserdata (L, 1);
	char       * string;

	/* build string */
	string = axl_strdup_printf ("vortex.ctx %p", *ctx);
	
	/* push the value into the stack */
	lua_pushstring (L, string);
	axl_free (string);
	
	return 1;
}

/** 
 * @internal Implementation of vortex.ctx.init ()
 */
static int lua_vortex_ctx_init (lua_State* L)
{
	VortexCtx ** ctx;
	axl_bool     result;

	lua_vortex_log (LUA_VORTEX_DEBUG, "Found call to vortex.ctx.init, with args: %d", lua_gettop (L));
	show_stack (L);

	/* call internal implementation */
	if (! lua_vortex_ctx_check_internal (L, 1))
		return 0;

	/* get reference */
	ctx = lua_touserdata (L, 1);

	lua_vortex_log (LUA_VORTEX_DEBUG, "Attempting to initialize vortex.ctx %p", *ctx);

	/* flag this ctx as initialized */
	result = vortex_init_ctx (*ctx);
	lua_vortex_metatable_set_bool (L, 1, "initialized", result);

	/* call to init and push result */
	lua_pushboolean (L, result);
	return 1; /* number of items */
}

/** 
 * @internal Implementation of vortex.ctx.init ()
 */
static int lua_vortex_ctx_exit (lua_State* L)
{
	VortexCtx ** ctx;

	lua_vortex_log (LUA_VORTEX_DEBUG, "Found call to vortex.ctx.exit, with args: %d", lua_gettop (L));

	/* call internal implementation */
	if (! lua_vortex_ctx_check_internal (L, 1))
		return 0;

	/* get reference */
	ctx = lua_touserdata (L, 1);

	/* check if this is the last reference */
	if (lua_vortex_metatable_get_bool (L, 1, "initialized") && *ctx) {
		/* we are about to call finish, unlock to release any
		   waiting thread but signal other thread we are finish */
		LUA_VORTEX_UNLOCK (L, axl_false);

		lua_vortex_log (LUA_VORTEX_DEBUG, "   calling to terminate context: %p", *ctx);
		vortex_exit_ctx (*ctx, axl_true);

		/* nullify */
		(*ctx) = NULL;

		/* relock again */
		LUA_VORTEX_LOCK (L, axl_false);
	}

	/* nullify */
	(*ctx) = NULL;

	return 0; /* done */
}

axl_bool lua_vortex_ctx_bridge_event (VortexCtx * ctx, axlPointer user_data, axlPointer user_data2)
{
	LuaVortexRefs * references = user_data;
	lua_State     * L          = references->ref->L;
	lua_State     * thread;
	int             initial_top;
	axl_bool        result      = axl_false;
	int             error;

	lua_vortex_log (LUA_VORTEX_DEBUG, "Bridge new event (L: %p), marshalling..", L);

	/* create thread to represent this context */
	thread = lua_vortex_create_thread (ctx, L, axl_true);

	/* check if vortex is finish to skip bridging */
	if (vortex_is_exiting (ctx) || thread == NULL)
		return axl_true; /* remove handler */

	/* lock during operations */
	LUA_VORTEX_LOCK (L, axl_true);
	
	/* check if we are in process of finishing */
	if (vortex_is_exiting (ctx)) {
		LUA_VORTEX_UNLOCK (L, axl_true);
		return axl_true; /* remove handler */
	}

	/* get new initial top */
	initial_top = lua_gettop (thread);

	lua_vortex_log (LUA_VORTEX_DEBUG, "Found thread %p (top: %d), preparing call..", thread, initial_top);

	/* push the handler to call */
	lua_rawgeti (thread, LUA_REGISTRYINDEX, references->ref->ref_id);
	lua_vortex_log (LUA_VORTEX_DEBUG, "Pushed event handler, ref_id=%d..", references->ref->ref_id);

	/* push context */
	lua_vortex_ctx_new_ref (thread, ctx);

	/* push user data */
	lua_rawgeti (thread, LUA_REGISTRYINDEX, references->ref2->ref_id);

	/* push user data */
	lua_rawgeti (thread, LUA_REGISTRYINDEX, references->ref3->ref_id);

	/* now call event */
	lua_vortex_log (LUA_VORTEX_DEBUG, "About to call event handler on lua space..");
	error = lua_pcall (thread, 3, 1, 0);

	/* handle error */
	lua_vortex_handle_error (thread, error, "event");

	/* get result from handler */
	if (lua_gettop (thread) > 0 && lua_isboolean (thread, 1))
		result = lua_toboolean (thread, 1);
	lua_vortex_log (LUA_VORTEX_DEBUG, "   bridge result was: %d..", result);

	/* unlock during operations */
	LUA_VORTEX_UNLOCK (L, axl_true);

	return result;
}

/** 
 * Implementation of vortex.ctx.new_event ()
 */
static int lua_vortex_ctx_new_event (lua_State * L)
{
	VortexCtx      ** ctx;
	LuaVortexRefs   * references = NULL;
	int             * event_id;
	
	/* check parameters */
	if (! lua_vortex_check_params (L, "tif|dd")) 
		return 0;

	/* get context */
	ctx = lua_touserdata (L, 1);

	/* acquire references to all objects */
	references = lua_vortex_acquire_references (*ctx, L, 3, 4, 5);

	if (references == NULL)
		return 0;

	/* allocate memory to store the event id to be passed to the
	   bridge handler */
	event_id    = axl_new (int, 1);
	if (event_id == NULL) {
		lua_vortex_error (L, "Failed to allocate memory for internal structure used by event");
		return 0;
	}

	/* register event */	
	
	(*event_id) = vortex_thread_pool_new_event (*ctx, lua_tonumber (L, 2), 
						 lua_vortex_ctx_bridge_event,
						 references, event_id);
	
	/* store data into context */
	vortex_ctx_set_data_full (*ctx, 
				  /* key and value */
				  axl_strdup_printf ("lua:ev:%d", *event_id), references, 
				  /* destroy functions */
				  axl_free, (axlDestroyFunc) lua_vortex_unref2);
	vortex_ctx_set_data_full (*ctx, 
				  /* key and value */
				  axl_strdup_printf ("%p", event_id), event_id, 
				  /* destroy functions */
				  axl_free, axl_free);

	lua_vortex_log (LUA_VORTEX_DEBUG, "Registered event id %d, every %d microseconds", event_id, lua_tonumber (L, 2));
	lua_pushnumber (L, *event_id);

	return 1;
}

static int lua_vortex_ctx_index (lua_State *L) {
	VortexCtx  ** ctx         = lua_touserdata (L, 1);
	int           initial_top = lua_gettop (L);
	const char  * key_index   = lua_tostring (L, 2);

	lua_vortex_log (LUA_VORTEX_DEBUG, "Received 'index' with key '%s' event on vortex.ctx (%p), stack items: %d", key_index, ctx, initial_top);

	if (initial_top == 2) {
		/* get operation */
		lua_vortex_log (LUA_VORTEX_DEBUG, "   requested to return value at index: %s", key_index);
		if (axl_cmp (key_index, "ref_count")) {
			lua_vortex_log (LUA_VORTEX_DEBUG, "   returning ref count: %d", vortex_ctx_ref_count (*ctx));
			lua_pushnumber (L, vortex_ctx_ref_count (*ctx));
			return 1;
		} else if (axl_cmp (key_index, "init")) {
			lua_pushcfunction (L, lua_vortex_ctx_init);
			return 1;
		} else if (axl_cmp (key_index, "exit")) {
			lua_pushcfunction (L, lua_vortex_ctx_exit);
			return 1;
		} else if (axl_cmp (key_index, "new_event")) {
			lua_pushcfunction (L, lua_vortex_ctx_new_event);
			return 1;
		} /* end if */
	}
	
	
	return 0;
}

/* init vortex.Ctx metatable */
void lua_vortex_ctx_set_metatable (lua_State * L, int obj_index)
{
	int initial_top = lua_gettop (L);

	lua_vortex_log (LUA_VORTEX_DEBUG, "Setting vortex.ctx metatable at index %d, initial top: %d..", obj_index, initial_top);

	/* create empty table */
	lua_newtable (L);

	lua_vortex_log (LUA_VORTEX_DEBUG, "   vortex.ctx metatable created at stack position: %d..", lua_gettop (L));

	/* configure __gc */
	lua_pushcfunction (L, lua_vortex_ctx_gc);
	/* set the field: L, where is the table to be configured, the
	 * key to configure "__gc", and the value is taken from the
	 * current top stack */
	lua_setfield (L, initial_top + 1, "__gc");
	
	lua_vortex_log (LUA_VORTEX_DEBUG, "   configured __gc method for vortex.ctx metatable (sp: %d)", lua_gettop (L));

	/* configure __tostring */
	lua_pushcfunction (L, lua_vortex_ctx_tostring);
	/* set the field: L, where is the table to be configured, the
	 * key to configure "__gc", and the value is taken from the
	 * current top stack */
	lua_setfield (L, initial_top + 1, "__tostring");

	/* configure __index */
	lua_pushcfunction (L, lua_vortex_ctx_index);
	/* set the field: L, where is the table to be configured, the
	 * key to configure "__gc", and the value is taken from the
	 * current top stack */
	lua_setfield (L, initial_top + 1, "__index");

	/* configure indication that this is a connection */
	lua_pushliteral (L, "vortex.ctx");
	lua_setfield (L, initial_top + 1, "vortex.type");

	/* configure metatable */
	lua_setmetatable (L, obj_index);

	return;
}

int lua_vortex_ctx_new_ref (lua_State *L, VortexCtx * _ctx)
{
	VortexCtx ** ctx;
	int          initial_top = lua_gettop (L);

	/* acquire a reference */
	vortex_ctx_ref (_ctx);

	/* create context and push */
	ctx    = lua_newuserdata (L, sizeof (ctx));
	(*ctx) = _ctx;

	lua_vortex_log (LUA_VORTEX_DEBUG, "Created VortexCtx object: %p (from existing: %p), configuring metatable", _ctx, *ctx);

	/* configure metatable */
	lua_vortex_ctx_set_metatable (L, initial_top + 1);

	return 1;
}

/** 
 * Implemenation of vortex.ctx.new ()
 */
static int lua_vortex_ctx_new (lua_State* L)
{
	VortexCtx ** ctx;

	/* create context and push */
	ctx    = lua_newuserdata (L, sizeof (ctx));
	(*ctx) = vortex_ctx_new ();

	lua_vortex_log (LUA_VORTEX_DEBUG, "Created VortexCtx object: %p (real: %p), configuring metatable", ctx, *ctx);

	/* configure metatable */
	lua_vortex_ctx_set_metatable (L, 1);

	return 1; /* number of arguments returned */
}

/** 
 * @internal Implementation of vortex.ctx.unref ()
 */
static int lua_vortex_ctx_unref (lua_State* L)
{
	VortexCtx ** ctx;

	/* call internal implementation */
	if (! lua_vortex_ctx_check_internal (L, 1))
		return 0;

	/* get arguments */
	ctx = lua_touserdata (L, 1);
	
	lua_vortex_log (LUA_VORTEX_DEBUG, "Unreferring VortexCtx: %p", *ctx);

	/* call to unref */
	vortex_ctx_unref (ctx); 

	return 0; /* number of arguments returned */
}

axl_bool lua_vortex_ctx_check_internal (lua_State * L, int position)
{
	axl_bool     result;
	int          initial_top = lua_gettop (L);
	const char * vortex_type;

	/* get reference to the metatable */
	if (lua_getmetatable (L, position) == 0) {
		lua_vortex_error (L, "Passed an object that should be vortex.ctx but found something different");
		return axl_false;
	}

	/* get the type */
	lua_getfield (L, initial_top + 1, "vortex.type");
	if (! lua_isstring (L, initial_top + 2)) {
		lua_vortex_error (L, "Passed an object that should be vortex.connection but found something different (expected an string value at 'vortex.type' key, but found something different");
		return axl_false;
	}

	/* check value */
	vortex_type = lua_tostring (L, initial_top + 2);
	result      = axl_cmp (vortex_type, "vortex.ctx");

	/* check result */
	lua_vortex_log (result ? LUA_VORTEX_DEBUG : LUA_VORTEX_CRITICAL, "Checking vortex.type found='%s' with result %d",
			vortex_type, result);

	/* now remove metatable and string type */
	lua_pop (L, 2);
	return result;
}

/** 
 * @internal Implementation of vortex.ctx.unref ()
 */
static int lua_vortex_ctx_check (lua_State* L)
{
	axl_bool     result;

	/* call internal implementation */
	result = lua_vortex_ctx_check_internal (L, 1);

	/* push value returned */
	lua_pushboolean (L, result);
	return 1;
}


static const luaL_Reg vortex_ctx_funcs[] = {
	{ "new", lua_vortex_ctx_new },
	{ "new_event", lua_vortex_ctx_new_event },
	{ "init", lua_vortex_ctx_init },
	{ "unref", lua_vortex_ctx_unref },
	{ "check", lua_vortex_ctx_check },
	{ "exit", lua_vortex_ctx_exit },
	{ NULL, NULL }
};

void lua_vortex_ctx_init_module (lua_State *L)
{

	/* init vortex.ctx module */
	luaL_register(L, "vortex.ctx", vortex_ctx_funcs);

	return;
}


