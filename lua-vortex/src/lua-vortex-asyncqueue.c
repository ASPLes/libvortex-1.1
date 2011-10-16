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

static int lua_vortex_asyncqueue_gc (lua_State *L) {
	VortexAsyncQueue ** asyncqueue       = lua_touserdata (L, 1);
	LuaVortexObject   * object;
	lua_vortex_log (LUA_VORTEX_DEBUG, "Finishing VortexAsyncqueue reference associated to %p", *asyncqueue);

	/* check if this is the last reference */
	lua_vortex_log (LUA_VORTEX_DEBUG, "   releasing reference");

	/* release all internal content if any */
	while (vortex_async_queue_items (*asyncqueue) > 0) {

		/* get object and unref */
		object = vortex_async_queue_pop (*asyncqueue);
		
		lua_vortex_object_unref (L, object, 1);
	} /* end while */

	/* call to unref */
	vortex_async_queue_unref (*asyncqueue);

	/* nullify */
	(*asyncqueue) = NULL;
	return 0;
}

static int lua_vortex_asyncqueue_tostring (lua_State *L) {
	VortexAsyncQueue ** asyncqueue = lua_touserdata (L, 1);
	char              * string;

	/* build string */
	string = axl_strdup_printf ("vortex.asyncqueue %p", *asyncqueue);
	
	/* push the value into the stack */
	lua_pushstring (L, string);
	axl_free (string);
	
	return 1;
}

static int lua_vortex_asyncqueue_push (lua_State *L)
{
	VortexAsyncQueue ** asyncqueue;
	int                 initial_top = lua_gettop (L);
	LuaVortexObject   * object;

	/* check we have two params */
	if (initial_top != 2) {
		lua_vortex_error (L, "Expected to receive 2 parameters (queue and items), but received something different");
		return 0;
	} /* end if */

	/* check first parameter is a queue */
	if (! lua_vortex_asyncqueue_check_internal (L, 1))
		return 0;

	/* get a reference to the queue */
	asyncqueue  = lua_touserdata (L, 1);
	
	/* create the object */
	object = lua_vortex_object_new_from_stack (L, 1, 2);

	/* push data */
	vortex_async_queue_push (*asyncqueue, object);

	return 0; /* number of arguments returned */
}

static int lua_vortex_asyncqueue_pop (lua_State *L)
{
	VortexAsyncQueue ** asyncqueue;
	int                 initial_top = lua_gettop (L);
	LuaVortexObject   * object;

	/* check we have two params */
	if (initial_top != 1) {
		lua_vortex_error (L, "Expected to receive 1 parameters (queue), but received something different");
		return 0;
	} /* end if */

	/* check first parameter is a queue */
	if (! lua_vortex_asyncqueue_check_internal (L, 1))
		return 0;

	/* get a reference to the queue */
	asyncqueue  = lua_touserdata (L, 1);

	/* unlock current state */
	LUA_VORTEX_UNLOCK (L, axl_false);
	lua_vortex_log (LUA_VORTEX_DEBUG, "L state unlocked, calling to queue_pop..");

	/* create the object */
	object = vortex_async_queue_pop (*asyncqueue);

	/* unlock current state */
	LUA_VORTEX_LOCK (L, axl_false);

	if (object) {
		/* according to the type, push it on the result stack */
		lua_vortex_object_push (L, object, 1);

		/* release memory hold by object */
		lua_vortex_object_unref (L, object, 1);
	}

	return 1; /* number of arguments returned */
}

static int lua_vortex_asyncqueue_timedpop (lua_State *L)
{
	VortexAsyncQueue ** asyncqueue;
	int                 initial_top = lua_gettop (L);
	LuaVortexObject   * object;
	long                microseconds;

	/* check we have two params */
	if (initial_top != 2) {
		lua_vortex_error (L, "Expected to receive 2 parameters (queue and timeout), but received something different");
		return 0;
	} /* end if */

	/* check first parameter is a queue */
	if (! lua_vortex_asyncqueue_check_internal (L, 1))
		return 0;

	/* get a reference to the queue */
	asyncqueue   = lua_touserdata (L, 1);
	microseconds = lua_tonumber (L, 2);

	/* unlock current state */
	LUA_VORTEX_UNLOCK (L, axl_false);
	
	/* create the object */
	object = vortex_async_queue_timedpop (*asyncqueue, microseconds);

	/* lock current state */
	LUA_VORTEX_LOCK (L, axl_false);

	if (object) {
		/* according to the type, push it on the result stack */
		lua_vortex_object_push (L, object, 1);

		/* release memory hold by object */
		lua_vortex_object_unref (L, object, 1);

		return 1;
	}

	return 0; /* no item */
}

static int lua_vortex_asyncqueue_items (lua_State *L)
{
	VortexAsyncQueue ** asyncqueue;
	int                 initial_top = lua_gettop (L);

	/* check we have two params */
	if (initial_top != 1) {
		lua_vortex_error (L, "Expected to receive 1 parameters (queue), but received something different");
		return 0;
	} /* end if */

	/* check first parameter is a queue */
	if (! lua_vortex_asyncqueue_check_internal (L, 1))
		return 0;

	/* get a reference to the queue */
	asyncqueue  = lua_touserdata (L, 1);
	
	/* create result */
	lua_pushnumber (L, vortex_async_queue_items (*asyncqueue));

	return 1; /* number of arguments returned */
}

static int lua_vortex_asyncqueue_index (lua_State *L) {
	VortexAsyncQueue  ** asyncqueue    = lua_touserdata (L, 1);
	int                  initial_top   = lua_gettop (L);
	const char         * key_index     = lua_tostring (L, 2);

	lua_vortex_log (LUA_VORTEX_DEBUG, "Received 'index' with key '%s' event on vortex.asyncqueue (%p), stack items: %d", key_index, asyncqueue, initial_top);

	if (initial_top == 2) {
		/* get operation */
		lua_vortex_log (LUA_VORTEX_DEBUG, "   requested to return value at index: %s", key_index);
		if (axl_cmp (key_index, "ref_count")) {
			lua_vortex_log (LUA_VORTEX_DEBUG, "   returning ref count: %d", vortex_async_queue_ref_count (*asyncqueue));
			lua_pushnumber (L, vortex_async_queue_ref_count (*asyncqueue));
			return 1;
		} else if (axl_cmp (key_index, "push")) {
			lua_pushcfunction (L, lua_vortex_asyncqueue_push);
			return 1;
		} else if (axl_cmp (key_index, "pop")) {
			lua_pushcfunction (L, lua_vortex_asyncqueue_pop);
			return 1;
		} else if (axl_cmp (key_index, "items")) {
			lua_pushcfunction (L, lua_vortex_asyncqueue_items);
			return 1;
		} else if (axl_cmp (key_index, "timedpop")) {
			lua_pushcfunction (L, lua_vortex_asyncqueue_timedpop);
			return 1;
		} /* end if */
	}
	
	
	return 0;
}

/* init vortex.Asyncqueue metatable */
void lua_vortex_asyncqueue_set_metatable (lua_State * L, int obj_index)
{
	int initial_top = lua_gettop (L);

	lua_vortex_log (LUA_VORTEX_DEBUG, "Setting vortex.asyncqueue metatable at index %d, initial top: %d..", obj_index, initial_top);

	/* create empty table */
	lua_newtable (L);

	lua_vortex_log (LUA_VORTEX_DEBUG, "   vortex.asyncqueue metatable created at stack position: %d..", lua_gettop (L));

	/* configure __gc */
	lua_pushcfunction (L, lua_vortex_asyncqueue_gc);
	/* set the field: L, where is the table to be configured, the
	 * key to configure "__gc", and the value is taken from the
	 * current top stack */
	lua_setfield (L, initial_top + 1, "__gc");
	
	lua_vortex_log (LUA_VORTEX_DEBUG, "   configured __gc method for vortex.asyncqueue metatable (sp: %d)", lua_gettop (L));

	/* configure __tostring */
	lua_pushcfunction (L, lua_vortex_asyncqueue_tostring);
	/* set the field: L, where is the table to be configured, the
	 * key to configure "__gc", and the value is taken from the
	 * current top stack */
	lua_setfield (L, initial_top + 1, "__tostring");

	/* configure __index */
	lua_pushcfunction (L, lua_vortex_asyncqueue_index);
	/* set the field: L, where is the table to be configured, the
	 * key to configure "__gc", and the value is taken from the
	 * current top stack */
	lua_setfield (L, initial_top + 1, "__index");

	/* configure indication that this is a connection */
	lua_pushliteral (L, "vortex.asyncqueue");
	lua_setfield (L, initial_top + 1, "vortex.type");

	/* configure metatable */
	lua_setmetatable (L, obj_index);

	return;
}

/** 
 * @internal Implementation of vortex.asyncqueue.new ()
 */
static int lua_vortex_asyncqueue_new (lua_State* L)
{
	VortexAsyncQueue ** asyncqueue;

	/* create context and push */
	asyncqueue    = lua_newuserdata (L, sizeof (asyncqueue));
	(*asyncqueue) = vortex_async_queue_new ();
		
	lua_vortex_log (LUA_VORTEX_DEBUG, "Created VortexAsyncqueue object: %p, configuring metatable", *asyncqueue);

	/* configure metatable */
	lua_vortex_asyncqueue_set_metatable (L, 1);

	return 1; /* number of arguments returned */
}

axl_bool lua_vortex_asyncqueue_check_internal (lua_State * L, int position)
{
	axl_bool            result;
	int                 initial_top = lua_gettop (L);
	const char        * vortex_type;

	/* get reference to the metatable */
	if (lua_getmetatable (L, position) == 0) {
		lua_vortex_error (L, "Passed an object that should be vortex.asyncqueue but found something different");
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
	result      = axl_cmp (vortex_type, "vortex.asyncqueue");

	/* check result */
	lua_vortex_log (result ? LUA_VORTEX_DEBUG : LUA_VORTEX_CRITICAL, "Checking vortex.type found='%s' with result %d",
			vortex_type, result);

	/* now remove metatable and string type */
	lua_pop (L, 2);
	return result;
}

/** 
 * @internal Implementation of vortex.asyncqueue.check ()
 */
static int lua_vortex_asyncqueue_check (lua_State* L)
{
	axl_bool     result;

	/* call internal implementation */
	result = lua_vortex_asyncqueue_check_internal (L, 1);

	/* push value returned */
	lua_pushboolean (L, result);
	return 1;
}


static const luaL_Reg vortex_asyncqueue_funcs[] = {
	{ "new", lua_vortex_asyncqueue_new },
	{ "push", lua_vortex_asyncqueue_push },
	{ "pop", lua_vortex_asyncqueue_pop },
	{ "timedpop", lua_vortex_asyncqueue_timedpop },
	{ "items", lua_vortex_asyncqueue_items },
	{ "check", lua_vortex_asyncqueue_check },
	{ NULL, NULL }
};

void lua_vortex_asyncqueue_init_module (lua_State *L)
{
	/* init vortex.asyncqueue module */
	luaL_register(L, "vortex.asyncqueue", vortex_asyncqueue_funcs);

	return;
}


