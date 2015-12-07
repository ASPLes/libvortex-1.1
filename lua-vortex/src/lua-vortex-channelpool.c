/* 
 *  Lua Vortex:  Lua bindings for Vortex Library
 *  Copyright (C) 2015 Advanced Software Production Line, S.L.
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

static int lua_vortex_channelpool_gc (lua_State *L) {
	VortexChannelPool ** channelpool      = lua_touserdata (L, 1);
	lua_vortex_log (LUA_VORTEX_DEBUG, "Finishing VortexChannelPool reference associated to %p", *channelpool);

	/* nullify */
	(*channelpool) = NULL;
	return 0;
}

static int lua_vortex_channelpool_tostring (lua_State *L) {
	VortexChannelPool ** channelpool = lua_touserdata (L, 1);
	char         * string;

	/* build string */
	string = axl_strdup_printf ("vortex.channelpool %p", *channelpool);
	
	/* push the value into the stack */
	lua_pushstring (L, string);
	axl_free (string);
	
	return 1;
}

static int lua_vortex_channelpool_next_ready (lua_State *L) {
	int                  initial_top = lua_gettop (L);
	VortexChannelPool ** pool;
	axl_bool             auto_inc    = axl_true;
	VortexChannel      * channel;

	/* check params */
	if (! lua_vortex_check_params (L, "l|bd")) 
		return 0;

	/* get connection */
	pool = lua_touserdata (L, 1);
	
	if (initial_top > 1)
		auto_inc = lua_toboolean (L, 2);
	
	/* call to get next available channel */
	channel = vortex_channel_pool_get_next_ready_full (*pool, auto_inc, NULL);
	if (channel == NULL)
		return 0;

	/* return reference to the channel */
	return lua_vortex_channel_new (L, channel);
}

static int lua_vortex_channelpool_release (lua_State *L) {
	VortexChannelPool ** pool;
	VortexChannel     ** channel;

	/* check params */
	if (! lua_vortex_check_params (L, "lc")) 
		return 0;

	/* get connection */
	pool    = lua_touserdata (L, 1);
	channel = lua_touserdata (L, 2);
	
	/* release channel received */
	vortex_channel_pool_release_channel (*pool, *channel);

	/* return */
	return 0;
}

/** 
 * Handler used by vortex.connection module to implement create
 * channel handler.
 */
VortexChannel * lua_vortex_channelpool_bridge_create_handler (VortexConnection      * conn, 
							      int                     channel_num,              const char * profile, 
							      VortexOnCloseChannel    on_close,                 axlPointer   on_close_user_data, 
							      VortexOnFrameReceived   on_received,              axlPointer   on_received_user_data,
							      axlPointer              create_channel_user_data, axlPointer   get_next_data)
{
	LuaVortexRefs  * references = create_channel_user_data;
	lua_State      * L          = references->ref->L;
	int              initial_top;
	VortexChannel ** channel;
	int              error;

	lua_vortex_log (LUA_VORTEX_DEBUG, "Bridging channel pool create handler (L: %p), marshalling..", L);

	/* we do not lock because we execute using same calling thread
	 * (main one) */

	/* check if vortex is finish to skip bridging */
	if (vortex_is_exiting (CONN_CTX(conn)))
		return NULL;

	/* get new initial top */
	initial_top = lua_gettop (L);

	lua_vortex_log (LUA_VORTEX_DEBUG, "Found thread %p (top: %d), preparing call..", L, initial_top);

	/* push the handler to call */
	lua_rawgeti (L, LUA_REGISTRYINDEX, references->ref->ref_id);

	lua_vortex_log (LUA_VORTEX_DEBUG, "Pushed created handler handler, ref_id=%d..", references->ref->ref_id);

	/* push the connection */
	lua_vortex_connection_new_ref (L, conn, axl_true);

	/* push the channel number */
	lua_pushnumber (L, channel_num);

	/* push the channel profile */
	lua_pushstring (L, profile);

	/* push frame received and received data */
	lua_pushlightuserdata (L, on_received);
	lua_pushlightuserdata (L, on_received_user_data);

	/* push on close handler and data  */
	lua_pushlightuserdata (L, on_close);
	lua_pushlightuserdata (L, on_close_user_data);

	/* push user data */
	lua_rawgeti (L, LUA_REGISTRYINDEX, references->ref2->ref_id);

	/* push next data */
	lua_pushlightuserdata (L, get_next_data);

	/* now call */
	lua_vortex_log (LUA_VORTEX_DEBUG, "About to call create channel handler on lua space..");
	error = lua_pcall (L, 9, 1, 0);

	/* handle error */
	lua_vortex_handle_error (L, error, "create channel");

	/* on the stack is found the channel reference */
	if (! lua_vortex_channel_check_internal (L, lua_gettop (L))) {
		lua_vortex_error (L, "Expected to find vortex.channel object as a result but found something different");

		return NULL;
	} else {
		/* get reference to the channel */
		channel = lua_touserdata (L, lua_gettop (L));
	}

	/* return channel created */
	lua_vortex_log (LUA_VORTEX_DEBUG, "   returning channel from create handler: VortexChannel %p (real %p)..", channel, *channel);
	return *channel;
}


static int lua_vortex_channelpool_index (lua_State *L) {
	VortexChannelPool  ** channelpool       = lua_touserdata (L, 1);
	int                   initial_top = lua_gettop (L);
	const char          * key_index   = lua_tostring (L, 2);
	
	lua_vortex_log (LUA_VORTEX_DEBUG, "Received index event on vortex.channelpool (%p), stack items: %d", channelpool, initial_top);
	if (initial_top == 2) {
		/* get operation */
		lua_vortex_log (LUA_VORTEX_DEBUG, "   requested to return value at index: %s", key_index);
		if (axl_cmp (key_index, "id")) {
			lua_pushnumber (L, vortex_channel_pool_get_id (*channelpool));
			return 1;
		} else if (axl_cmp (key_index, "channel_count")) {
			lua_pushnumber (L, vortex_channel_pool_get_num (*channelpool));
			return 1;
		} else if (axl_cmp (key_index, "next_ready")) {
			lua_pushcfunction (L, lua_vortex_channelpool_next_ready);
			return 1;
		} else if (axl_cmp (key_index, "release")) {
			lua_pushcfunction (L, lua_vortex_channelpool_release);
			return 1;
		} else if (axl_cmp (key_index, "channel_available")) {
			lua_pushnumber (L, vortex_channel_pool_get_available_num (*channelpool));
			return 1;
		} else if (axl_cmp (key_index, "conn")) {
			lua_vortex_connection_new_ref (L, vortex_channel_pool_get_connection (*channelpool), axl_true);
			return 1;
		} else if (axl_cmp (key_index, "ctx")) {
			lua_vortex_ctx_new_ref (L, CONN_CTX (vortex_channel_pool_get_connection (*channelpool)));
			return 1;
		}
	}
	
	
	return 0;
}

/* init vortex.Channelpool metatable */
void lua_vortex_channelpool_set_metatable (lua_State * L, int obj_index, VortexChannelPool ** channelpool)
{
	int initial_top = lua_gettop (L);

	lua_vortex_log (LUA_VORTEX_DEBUG, "Setting vortex.channelpool metatable at index %d, initial top: %d..", obj_index, initial_top);

	/* create empty table */
	lua_newtable (L);

	/* configure __gc */
	lua_pushcfunction (L, lua_vortex_channelpool_gc);
	/* set the field: L, where is the table to be configured, the
	 * key to configure "__gc", and the value is taken from the
	 * current top stack */
	lua_setfield (L, initial_top + 1, "__gc");
	
	/* configure __tostring */
	lua_pushcfunction (L, lua_vortex_channelpool_tostring);
	/* set the field: L, where is the table to be configured, the
	 * key to configure "__gc", and the value is taken from the
	 * current top stack */
	lua_setfield (L, initial_top + 1, "__tostring");

	/* configure __index */
	lua_pushcfunction (L, lua_vortex_channelpool_index);
	/* set the field: L, where is the table to be configured, the
	 * key to configure "__gc", and the value is taken from the
	 * current top stack */
	lua_setfield (L, initial_top + 1, "__index");

	/* configure indication that this is a channelpool */
	lua_pushliteral (L, "vortex.channelpool");
	lua_setfield (L, initial_top + 1, "vortex.type");

	/* configure metatable */
	lua_setmetatable (L, obj_index);

	/* now pop the table from stack to grab a reference to it */
	return;
}

/** 
 * @internal Implementation of vortex.channelpool.new ()
 */
int lua_vortex_channelpool_new (lua_State* L, VortexChannelPool * _channelpool)
{
	VortexChannelPool ** channelpool;
	int            initial_top = lua_gettop (L);

	/* check channelpool reference received */
	if (_channelpool == NULL)
		return 0;

	/* create channelpool bucket and connect */
	channelpool    = lua_newuserdata (L, sizeof (channelpool));
	(*channelpool) = _channelpool;
		
	lua_vortex_log (LUA_VORTEX_DEBUG, "Created VortexChannelPool object: %p", channelpool);
	/* at this point, the channelpool is at initial_top + 1 */

	/* configure metatable */
	lua_vortex_channelpool_set_metatable (L, initial_top + 1, channelpool);

	return 1; /* number of arguments returned */
}


axl_bool lua_vortex_channelpool_check_internal (lua_State * L, int position)
{
	const char * vortex_type;
	axl_bool     result;
	int          initial_top = lua_gettop (L);

	/* get reference to the metatable */
	if (lua_getmetatable (L, position) == 0) {
		lua_vortex_error (L, "Passed an object that should be vortex.channelpool but found something different");
		return axl_false;
	}

	/* get the type */
	lua_getfield (L, initial_top + 1, "vortex.type");
	if (! lua_isstring (L, initial_top + 2)) {
		lua_vortex_error (L, "Passed an object that should be vortex.channelpool but found something different (expected an string value at 'vortex.type' key, but found something different");
		return axl_false;
	}

	/* check value */
	vortex_type = lua_tostring (L, initial_top + 2);
	result      = axl_cmp (vortex_type, "vortex.channelpool");

	/* check result */
	lua_vortex_log (result ? LUA_VORTEX_DEBUG : LUA_VORTEX_CRITICAL, "Checking vortex.type found='%s' with result %d",
			vortex_type, result);

	/* now remove metatable and string type */
	lua_pop (L, 2);
	return result;
}

/** 
 * @internal Implementation of vortex.channelpool.unref ()
 */
static int lua_vortex_channelpool_check (lua_State* L)
{
	axl_bool     result;

	/* call internal implementation */
	result = lua_vortex_channelpool_check_internal (L, 1);

	/* push value returned */
	lua_pushboolean (L, result);
	return 1;
}


static const luaL_Reg vortex_channelpool_funcs[] = {
	{ "check", lua_vortex_channelpool_check },
	{ "next_ready", lua_vortex_channelpool_next_ready },
	{ "release", lua_vortex_channelpool_release },
	{ NULL, NULL }
};

void lua_vortex_channelpool_init_module (lua_State *L)
{
	luaL_register(L, "vortex.channelpool", vortex_channelpool_funcs);

	return;
}


