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

static int lua_vortex_channel_gc (lua_State *L) {
	VortexChannel ** channel = lua_touserdata (L, 1);
	int              ref_count  = vortex_channel_ref_count (*channel);
	lua_vortex_log (LUA_VORTEX_DEBUG, "Finishing VortexChannel reference associated to %p (ref count: %d)", *channel, ref_count);

	/* release channel reference */
	vortex_channel_unref2 (*channel, "lua-vortex");

	/* nullify */
	(*channel) = NULL;
	return 0;
}

static int lua_vortex_channel_tostring (lua_State *L) {
	VortexChannel ** channel = lua_touserdata (L, 1);
	char       * string;

	/* build string */
	string = axl_strdup_printf ("vortex.channel %p", *channel);
	
	/* push the value into the stack */
	lua_pushstring (L, string);
	axl_free (string);
	
	return 1;
}

/** 
 * @internal Implementation of vortex.channel.close ()
 */
static int lua_vortex_channel_close (lua_State* L)
{
	VortexChannel ** channel;
	axl_bool            result;

	/* check parameters */
	if (! lua_vortex_check_params (L, "c"))
		return 0;

	/* get context */
	channel = lua_touserdata (L, 1);

	lua_vortex_log (LUA_VORTEX_DEBUG, "Calling to close channel: %d", vortex_channel_get_number (*channel));

	/* return result */
	result = vortex_channel_close (*channel, NULL);
	if (result) {
		/* release channel reference */
		vortex_channel_unref2 (*channel, "lua-vortex");
		(*channel) = NULL;
	}
	lua_pushboolean (L, result);
	return 1; /* number of arguments returned */
}

static int lua_vortex_channel_send_msg (lua_State *L)
{
	VortexChannel ** channel;
	int              msg_no;
	int              size;

	/* check parameters */
	if (! lua_vortex_check_params (L, "csi"))
		return 0;

	/* get context */
	channel = lua_touserdata (L, 1);

	/* get size */
	size    = lua_tonumber (L, 3);
	if (size == -1)
		size = strlen (lua_tostring (L, 2));

	if (! vortex_channel_send_msg (*channel, lua_tostring (L, 2), size, &msg_no)) {
		lua_vortex_error (L, "Failed to send message over channel num %d, over connection %d", 
				  vortex_channel_get_number (*channel), vortex_connection_get_id (vortex_channel_get_connection (*channel)));
		return 0;
	} /* end if */

	/* push msg no */
	lua_pushnumber (L, msg_no);
	return 1; /* number of arguments returned */
}

static int lua_vortex_channel_send_rpy (lua_State *L)
{
	VortexChannel ** channel;
	int              size;

	/* check parameters */
	if (! lua_vortex_check_params (L, "csii"))
		return 0;

	/* get context */
	channel = lua_touserdata (L, 1);

	/* get size */
	size    = lua_tonumber (L, 3);
	if (size == -1)
		size = strlen (lua_tostring (L, 2));

	if (! vortex_channel_send_rpy (*channel, lua_tostring (L, 2), size, lua_tonumber (L, 4))) {
		lua_vortex_error (L, "Failed to send reply message over channel num %d, over connection %d", 
				  vortex_channel_get_number (*channel), vortex_connection_get_id (vortex_channel_get_connection (*channel)));
		return 0;
	} /* end if */

	/* push msg no */
	lua_pushboolean (L, axl_true);
	return 1; /* number of arguments returned */
}

static int lua_vortex_channel_send_err (lua_State *L)
{
	VortexChannel ** channel;
	int              size;

	/* check parameters */
	if (! lua_vortex_check_params (L, "csii"))
		return 0;

	/* get context */
	channel = lua_touserdata (L, 1);

	/* get size */
	size    = lua_tonumber (L, 3);
	if (size == -1)
		size = strlen (lua_tostring (L, 2));

	if (! vortex_channel_send_err (*channel, lua_tostring (L, 2), size, lua_tonumber (L, 4))) {
		lua_vortex_error (L, "Failed to send error reply message over channel num %d, over connection %d", 
				  vortex_channel_get_number (*channel), vortex_connection_get_id (vortex_channel_get_connection (*channel)));
		return 0;
	} /* end if */

	/* push msg no */
	lua_pushboolean (L, axl_true);
	return 1; /* number of arguments returned */
}

int lua_vortex_channel_set_frame_received (lua_State  *L)
{
	VortexChannel    ** channel;
	VortexConnection  * conn;
	LuaVortexRefs     * references;
	
	/* check parameters */
	if (! lua_vortex_check_params (L, "cf|d"))
		return 0;

	/* get context */
	channel = lua_touserdata (L, 1);

	/* configure frame received */
	conn       = vortex_channel_get_connection (*channel);
	references = lua_vortex_acquire_references (CONN_CTX(conn), L, 2, 3, 0);

	/* configure frame received */
	vortex_channel_set_received_handler (*channel, lua_vortex_channel_bridge_frame_received, references);
	vortex_connection_set_data_full (conn, axl_strdup_printf ("%p", references), references,
					 axl_free, (axlDestroyFunc) lua_vortex_unref2); 

	return 0;
}

/** 
 * Implements vortex.channel.set_serialize
 */
int lua_vortex_channel_set_serialize (lua_State  *L)
{
	VortexChannel    ** channel;
	
	/* check parameters */
	if (! lua_vortex_check_params (L, "cb"))
		return 0;

	/* get context */
	channel = lua_touserdata (L, 1);
	
	/* call to enable/disable serialize */
	vortex_channel_set_serialize (*channel, lua_toboolean (L, 2));

	return 0;
}

/** 
 * @internal Implementation of vortex.channel.unref ()
 */
static int lua_vortex_channel_check (lua_State* L)
{
	axl_bool     result;

	/* call internal implementation */
	result = lua_vortex_channel_check_internal (L, 1);

	/* push value returned */
	lua_pushboolean (L, result);
	return 1;
}

static int lua_vortex_channel_index (lua_State *L) {
	VortexChannel  ** channel  = lua_touserdata (L, 1);
	int           initial_top = lua_gettop (L);
	const char  * key_index   = lua_tostring (L, 2);
	
	lua_vortex_log (LUA_VORTEX_DEBUG, "Received index event on vortex.channel (%p), stack items: %d", channel, initial_top);
	if (initial_top == 2) {
		/* get operation */
		lua_vortex_log (LUA_VORTEX_DEBUG, "   requested to return value at index: %s", key_index);

		if (axl_cmp (key_index, "ref_count")) {
			lua_pushnumber (L, vortex_channel_ref_count (*channel));
			return 1;
		} else if (axl_cmp (key_index, "number")) {
			lua_pushnumber (L, vortex_channel_get_number (*channel));
			return 1;
		} else if (axl_cmp (key_index, "profile")) {
			lua_pushstring (L, vortex_channel_get_profile (*channel));
			return 1;
		} else if (axl_cmp (key_index, "close")) {
			lua_pushcfunction (L, lua_vortex_channel_close);
			return 1;
		} else if (axl_cmp (key_index, "send_msg")) {
			lua_pushcfunction (L, lua_vortex_channel_send_msg);
			return 1;
		} else if (axl_cmp (key_index, "send_rpy")) {
			lua_pushcfunction (L, lua_vortex_channel_send_rpy);
			return 1;
		} else if (axl_cmp (key_index, "send_err")) {
			lua_pushcfunction (L, lua_vortex_channel_send_err);
			return 1;
		} else if (axl_cmp (key_index, "set_frame_received")) {
			lua_pushcfunction (L, lua_vortex_channel_set_frame_received);
			return 1;
		} else if (axl_cmp (key_index, "set_serialize")) {
			lua_pushcfunction (L, lua_vortex_channel_set_serialize);
			return 1;
		} else if (axl_cmp (key_index, "check")) {
			lua_pushcfunction (L, lua_vortex_channel_check);
			return 1;
		} else if (axl_cmp (key_index, "is_ready")) {
			lua_pushboolean (L, vortex_channel_is_ready (*channel));
			return 1;
		} else if (axl_cmp (key_index, "conn")) {
			/* return current channel connection */
			lua_vortex_connection_new_ref (L, vortex_channel_get_connection (*channel), axl_true);
			return 1;
		} /* end if */
	}
	
	
	return 0;
}

/* init vortex.Channel metatable */
void lua_vortex_channel_set_metatable (lua_State * L, int obj_index, VortexChannel ** channel)
{
	int initial_top = lua_gettop (L);

	lua_vortex_log (LUA_VORTEX_DEBUG, "Setting vortex.channel metatable at index %d, initial top: %d..", obj_index, initial_top);

	/* create empty table */
	lua_newtable (L);

	/* configure __gc */
	lua_pushcfunction (L, lua_vortex_channel_gc);
	/* set the field: L, where is the table to be configured, the
	 * key to configure "__gc", and the value is taken from the
	 * current top stack */
	lua_setfield (L, initial_top + 1, "__gc");
	
	/* configure __tostring */
	lua_pushcfunction (L, lua_vortex_channel_tostring);
	/* set the field: L, where is the table to be configured, the
	 * key to configure "__gc", and the value is taken from the
	 * current top stack */
	lua_setfield (L, initial_top + 1, "__tostring");

	/* configure __index */
	lua_pushcfunction (L, lua_vortex_channel_index);
	/* set the field: L, where is the table to be configured, the
	 * key to configure "__gc", and the value is taken from the
	 * current top stack */
	lua_setfield (L, initial_top + 1, "__index");

	/* configure indication that this is a channel */
	lua_pushliteral (L, "vortex.channel");
	lua_setfield (L, initial_top + 1, "vortex.type");

	/* configure metatable */
	lua_setmetatable (L, obj_index);

	/* now pop the table from stack to grab a reference to it */
	return;
}

/** 
 * @internal Implementation of vortex.channel.new ()
 */
int lua_vortex_channel_new (lua_State* L, VortexChannel * _channel)
{
	VortexChannel ** channel;
	int              initial_top = lua_gettop (L);

	/* check channel reference */
	if (_channel == NULL)
		return 0;
	if (! vortex_channel_ref2 (_channel, "lua-vortex"))
		return 0;

	/* create channel bucket and connect */
	channel    = lua_newuserdata (L, sizeof (channel));
	(*channel) = _channel;
		
	lua_vortex_log (LUA_VORTEX_DEBUG, "Created VortexChannel object: %p (real: %p)", channel, *channel);
	/* at this point, the channel is at initial_top + 1 */

	/* configure metatable */
	lua_vortex_channel_set_metatable (L, initial_top + 1, channel);

	return 1; /* number of arguments returned */
}

/** 
 * Bridge on channel started notification
 */
void     lua_vortex_channel_bridge_on_channel  (int                channel_num,
						VortexChannel    * channel,
						VortexConnection * conn,
						axlPointer         user_data)
{
	LuaVortexRefs * references = user_data;
	VortexCtx     * ctx        = CONN_CTX (conn);
	lua_State     * L          = references->ref->L;
	lua_State     * thread;
	int             error;

	lua_vortex_log (LUA_VORTEX_DEBUG, "Received on channel started notification on channel=%d, conn-id=%d", 
			vortex_channel_get_number (channel), vortex_connection_get_id (conn));

	/* create the thread to bridge frame received */
	thread = lua_vortex_create_thread (ctx, L, axl_true);

	/* check if vortex is finish to skip bridging */
	if (vortex_is_exiting (ctx) || thread == NULL)
		return;

	/* lock during operations */
	LUA_VORTEX_LOCK (L, axl_true);

	/* check if we are in process of finishing */
	if (vortex_is_exiting (ctx)) {
		LUA_VORTEX_UNLOCK (L, axl_true);
		return;
	}

	/* push the handler to call */
	lua_rawgeti (thread, LUA_REGISTRYINDEX, references->ref->ref_id);

	lua_vortex_log (LUA_VORTEX_DEBUG, "Pushed on channel started handler, ref_id=%d..", references->ref->ref_id);

	/* push channel number */
	lua_pushnumber (thread, channel_num);

	/* push channel if defined */
	if (channel)
		lua_vortex_channel_new (thread, channel);
	else
		lua_pushnil (thread);

	/* push the connection */
	lua_vortex_connection_new_ref (thread, conn, axl_true);

	/* push the user data to call */
	lua_rawgeti (thread, LUA_REGISTRYINDEX, references->ref2->ref_id);

	/* now call */
	lua_vortex_log (LUA_VORTEX_DEBUG, "About to call on channel started notification handler on lua space..");
	error = lua_pcall (thread, 4, 0, 0);

	/* handle error */
	lua_vortex_handle_error (thread, error, "on channel started notification");

	/* unlock during operations */
	LUA_VORTEX_UNLOCK (L, axl_true);
	
	return;
}

/** 
 * Bridge handler used to callback into user lua code when a frame is
 * received.
 */
void lua_vortex_channel_bridge_frame_received (VortexChannel    * channel, 
					       VortexConnection * connection, 
					       VortexFrame      * frame, 
					       axlPointer         user_data)
{
	LuaVortexRefs * references = user_data;
	VortexCtx     * ctx        = CONN_CTX (connection);
	lua_State     * L          = references->ref->L;
	lua_State     * thread;
	int             error;

	lua_vortex_log (LUA_VORTEX_DEBUG, "Received frame received event con channel=%d, conn-id=%d", 
			vortex_channel_get_number (channel), vortex_connection_get_id (connection));

	/* create the thread to bridge frame received */
	thread = lua_vortex_create_thread (ctx, L, axl_true);

	/* check if vortex is finish to skip bridging */
	if (vortex_is_exiting (ctx) || thread == NULL)
		return;

	/* lock during operations */
	LUA_VORTEX_LOCK (L, axl_true);

	/* check if we are in process of finishing */
	if (vortex_is_exiting (ctx)) {
		LUA_VORTEX_UNLOCK (L, axl_true);
		return;
	}

	/* push the handler to call */
	lua_rawgeti (thread, LUA_REGISTRYINDEX, references->ref->ref_id);

	lua_vortex_log (LUA_VORTEX_DEBUG, "Pushed frame received handler, ref_id=%d..", references->ref->ref_id);

	/* push the connection */
	lua_vortex_connection_new_ref (thread, connection, axl_true);

	/* push the channel */
	lua_vortex_channel_new (thread, channel);

	/* push the frame */
	lua_vortex_frame_new (thread, frame);

	/* push the user data to call */
	lua_rawgeti (thread, LUA_REGISTRYINDEX, references->ref2->ref_id);

	/* now call */
	lua_vortex_log (LUA_VORTEX_DEBUG, "About to call frame received handler on lua space..");
	error = lua_pcall (thread, 4, 0, 0);

	/* handle error */
	lua_vortex_handle_error (thread, error, "frame received");

	/* unlock during operations */
	LUA_VORTEX_UNLOCK (L, axl_true);
	
	return;
}

/** 
 * Bridge handler used to callback into user lua code when a frame is
 * received.
 */
axl_bool lua_vortex_channel_bridge_start_received (int                channel_num,
						   VortexConnection * conn,
						   axlPointer         user_data)
{
	LuaVortexRefs * references = user_data;
	VortexCtx     * ctx        = CONN_CTX (conn);
	lua_State     * L          = references->ref->L;
	lua_State     * thread;
	axl_bool        result;
	int             error;

	lua_vortex_log (LUA_VORTEX_DEBUG, "Received start received event con channel=%d, conn-id=%d", 
			channel_num, vortex_connection_get_id (conn));

	/* create the thread to bridge frame received */
	thread = lua_vortex_create_thread (ctx, L, axl_true);

	/* check if vortex is finish to skip bridging */
	if (vortex_is_exiting (ctx) || thread == NULL)
		return axl_false;

	/* lock during operations */
	LUA_VORTEX_LOCK (L, axl_true);

	/* check if we are in process of finishing */
	if (vortex_is_exiting (ctx)) {
		LUA_VORTEX_UNLOCK (L, axl_true);
		return axl_false;
	}

	/* push the handler to call */
	lua_rawgeti (thread, LUA_REGISTRYINDEX, references->ref->ref_id);

	lua_vortex_log (LUA_VORTEX_DEBUG, "Pushed start received handler, ref_id=%d..", references->ref->ref_id);

	/* push channel start number */
	lua_pushnumber (thread, channel_num);

	/* push the connection */
	lua_vortex_connection_new_ref (thread, conn, axl_true);

	/* push the user data to call */
	lua_rawgeti (thread, LUA_REGISTRYINDEX, references->ref2->ref_id);

	/* now call */
	lua_vortex_log (LUA_VORTEX_DEBUG, "About to call start received handler on lua space..");
	error = lua_pcall (thread, 3, 1, 0);

	/* handle error */
	if (lua_vortex_handle_error (thread, error, "start received"))
		result = axl_false;
	else
		result = lua_toboolean (thread, lua_gettop (thread));


	/* unlock during operations */
	LUA_VORTEX_UNLOCK (L, axl_true);
	
	return result;
}


axl_bool lua_vortex_channel_check_internal (lua_State * L, int position)
{
	const char * vortex_type;
	axl_bool     result;
	int          initial_top = lua_gettop (L);

	/* get reference to the metatable */
	if (lua_getmetatable (L, position) == 0) {
		lua_vortex_error (L, "Passed an object that should be vortex.channel but found something different");
		return axl_false;
	}

	/* get the type */
	lua_getfield (L, initial_top + 1, "vortex.type");
	if (! lua_isstring (L, initial_top + 2)) {
		lua_vortex_error (L, "Passed an object that should be vortex.channel but found something different (expected an string value at 'vortex.type' key, but found something different");
		return axl_false;
	}

	/* check value */
	vortex_type = lua_tostring (L, initial_top + 2);
	result      = axl_cmp (vortex_type, "vortex.channel");

	/* check result */
	lua_vortex_log (result ? LUA_VORTEX_DEBUG : LUA_VORTEX_CRITICAL, "Checking vortex.type found='%s' with result %d",
			vortex_type, result);

	/* now remove metatable and string type */
	lua_pop (L, 2);
	return result;
}


static const luaL_Reg vortex_channel_funcs[] = {
	{ "check", lua_vortex_channel_check },
	{ "close", lua_vortex_channel_close },
	{ "send_msg", lua_vortex_channel_send_msg },
	{ "send_rpy", lua_vortex_channel_send_rpy },
	{ "send_err", lua_vortex_channel_send_err },
	{ "set_frame_received", lua_vortex_channel_set_frame_received },
	{ "set_serialize", lua_vortex_channel_set_serialize },
	{ NULL, NULL }
};

void lua_vortex_channel_init_module (lua_State *L)
{
	luaL_register(L, "vortex.channel", vortex_channel_funcs);

	return;
}


