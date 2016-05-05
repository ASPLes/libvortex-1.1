/* 
 *  Lua Vortex:  Lua bindings for Vortex Library
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
#include <lua-vortex.h>
#include <lua-vortex-private.h>

int lua_vortex_connection_gc (lua_State *L) {
	VortexConnection ** connection = lua_touserdata (L, 1);
	int                 ref_count;

	/* check null reference */
	if (*connection == NULL)
		return 0;

	ref_count  = vortex_connection_ref_count (*connection);
	lua_vortex_log (LUA_VORTEX_DEBUG, "Finishing VortexConnection id=%d reference associated to %p (ref count: %d)", 
			vortex_connection_get_id (*connection), connection, ref_count);

	/* check if this is a transient reference (a reference created
	 * after the connection was created) */
	if (lua_vortex_metatable_get_bool (L, 1, "transient")) {
		lua_vortex_log (LUA_VORTEX_DEBUG, "   unref transient reference id=%d..", vortex_connection_get_id (*connection));
		/* call to unref and nullify */ 
		vortex_connection_unref (*connection, "lua vortex unref");
		(*connection) = NULL;
		return 0;
	}

	/* check connection type */
	if (vortex_connection_get_role (*connection) == VortexRoleInitiator) {
		if (vortex_connection_is_ok (*connection, axl_false)) {
			lua_vortex_log (LUA_VORTEX_DEBUG, "   client connection running, closing..");
			/* call to shutdown and then close */
			vortex_connection_shutdown (*connection);
		}

		/* now free references */
		vortex_connection_close (*connection);
	} else {
		lua_vortex_log (LUA_VORTEX_DEBUG, "   not closing reference, it is listener, closing..");
	} /* end if */

	/* nullify */
	(*connection) = NULL;
	return 0;
}

int lua_vortex_connection_tostring (lua_State *L) {
	VortexConnection ** connection = lua_touserdata (L, 1);
	char       * string;

	/* build string */
	string = axl_strdup_printf ("vortex.connection %p", connection);
	
	/* push the value into the stack */
	lua_pushstring (L, string);
	axl_free (string);
	
	return 1;
}

int lua_vortex_connection_shutdown (lua_State * L) {
	VortexConnection ** connection;

	/* check parameters */
	if (! lua_vortex_check_params (L, "o")) {
		lua_vortex_error (L, "Expected to receive connection but received something different");
		return 0;
	} /* end if */

	/* get reference to the connection */
	connection = lua_touserdata (L, 1);
	
	/* call to shutdown */
	vortex_connection_shutdown (*connection);
	
	return 0;
}

void lua_vortex_connection_on_close_bridge (VortexConnection * conn, axlPointer ptr)
{
	LuaVortexRefs * references = ptr;
	lua_State     * L          = references->ref->L;
	lua_State     * thread;
	int             initial_top;
	VortexCtx     * ctx         = CONN_CTX (conn);
	int             error;

	lua_vortex_log (LUA_VORTEX_DEBUG, "Received connection notification on bridge function (L: %p), marshalling..", L);

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

	/* push user data */
	lua_rawgeti (thread, LUA_REGISTRYINDEX, references->ref2->ref_id);

	/* now call */
	lua_vortex_log (LUA_VORTEX_DEBUG, "About to call set on close handler on lua space..");
	error = lua_pcall (thread, 2, 0, 0);

	/* handle error */
	lua_vortex_handle_error (thread, error, "set on close");

	/* unlock during operations */
	LUA_VORTEX_UNLOCK (L, axl_true);

	return;
}

int lua_vortex_connection_set_on_close (lua_State * L) {
	int                 initial_top = lua_gettop (L);
	VortexConnection ** conn;
	LuaVortexRefs     * references;

	lua_vortex_log (LUA_VORTEX_DEBUG, "Called set on close with %d args..", initial_top);

	/* check params */
	if (! lua_vortex_check_params (L, "ofd")) {
		lua_vortex_error (L, "Expected to receive 3 parameters (conn, func, data) but received something different");
		return 0;
	} /* end if */

	/* get connection */
	conn = lua_touserdata (L, 1);

	/* acquire references to the function and user data to be used later */
	references = lua_vortex_acquire_references (CONN_CTX (*conn), L, 2, 3, 0);
	if (references == NULL)
		return 0;

	/* set on close */
	vortex_connection_set_on_close_full (*conn, lua_vortex_connection_on_close_bridge, references);
	vortex_connection_set_data_full (*conn, axl_strdup_printf ("%p", references), references,
					 axl_free, (axlDestroyFunc) lua_vortex_unref2); 
	return 0;
}

int lua_vortex_connection_set_data (lua_State * L) {
	VortexConnection ** conn;
	LuaVortexRef      * ref;

	/* check params */
	if (! lua_vortex_check_params (L, "osd")) {
		lua_vortex_error (L, "Expected to receive 3 parameters (conn, key, data) but received something different");
		return 0;
	} /* end if */

	/* get connection */
	conn = lua_touserdata (L, 1);

	/* acquire a reference to the data */
	lua_pushvalue (L, 3);
	ref = lua_vortex_ref (L);

	/* now store it on the connection */
	vortex_connection_set_data_full (*conn, axl_strdup (lua_tostring (L, 2)), ref,
					 axl_free, (axlDestroyFunc) lua_vortex_unref); 
	return 0;
}

int lua_vortex_connection_get_data (lua_State * L) {
	VortexConnection ** conn;
	LuaVortexRef      * ref;

	/* check params */
	if (! lua_vortex_check_params (L, "os")) {
		lua_vortex_error (L, "Expected to receive 2 parameters (conn, key) but received something different");
		return 0;
	} /* end if */

	/* get connection */
	conn = lua_touserdata (L, 1);

	/* get value */
	ref = vortex_connection_get_data (*conn, lua_tostring (L, 2));
	if (ref == NULL) {
		lua_pushnil (L);
		return 1;
	}

	/* push ref value */
	lua_rawgeti (L, LUA_REGISTRYINDEX, ref->ref_id);
	return 1;
}

/** 
 * open_channel implementation..
 */
int lua_vortex_connection_open_channel (lua_State * L)
{
	int                 initial_top  = lua_gettop (L);
	VortexConnection ** conn;
	VortexChannel     * channel;
	LuaVortexRefs     * references   = NULL;
	LuaVortexRefs     * references2  = NULL;

	lua_vortex_log (LUA_VORTEX_DEBUG, "Called to open channel with %d args..", initial_top);

	/* check params                    123 4567*/
	if (! lua_vortex_check_params (L, "ois|gdgdgdis")) 
		return 0;

	/* get connection */
	conn = lua_touserdata (L, 1);

	/* check if we have an on channel start handle defined */
	if (lua_gettop (L) > 5 && ! lua_isnil (L, 6)) {
		lua_vortex_log (LUA_VORTEX_DEBUG, "Creating channel with async notification..");		

		/* acquire references to frame received */
		references = lua_vortex_acquire_references (CONN_CTX (*conn), L, 6, 7, 0);

		/* store reference created */
		vortex_connection_set_data_full (*conn, axl_strdup_printf ("%p", references), references,
						 axl_free, (axlDestroyFunc) lua_vortex_unref2); 
	} 

	/* check if we have a frame received handler defined */
	if (lua_gettop (L) > 3 && ! lua_isnil (L, 4)) {
		lua_vortex_log (LUA_VORTEX_DEBUG, "  channel created, setting frame received handlers..");		

		/* acquire references to frame received */
		references2 = lua_vortex_acquire_references (CONN_CTX (*conn), L, 4, 5, 0);

		/* store reference */
		vortex_connection_set_data_full (*conn, axl_strdup_printf ("%p", references2), references2,
						 axl_free, (axlDestroyFunc) lua_vortex_unref2); 
	}

	/* open the channel */
	channel = vortex_channel_new (*conn, 
				      /* channel number */
				      lua_tonumber (L, 2),
				      /* channel profile */
				      lua_tostring (L, 3),
				      /* on close */
				      NULL, NULL,
				      /* frame received */
				      references2 ? lua_vortex_channel_bridge_frame_received : NULL, references2,
				      /* on channel created */
				      references  ? lua_vortex_channel_bridge_on_channel : NULL, references);
	if (channel == NULL) {
		/* returning nil here may or may not be an error */
		lua_pushnil (L);
		return 1;
	} /* end if */

	lua_vortex_log (LUA_VORTEX_DEBUG, "Channel num=%d conn-id=%d created, creating lua object..",
			vortex_channel_get_number (channel), vortex_connection_get_id (*conn));
	return lua_vortex_channel_new (L, channel);
}



/** 
 * open_channel implementation..
 */
int lua_vortex_connection_pop_channel_error (lua_State * L)
{
	int                 initial_top = lua_gettop (L);
	VortexConnection ** conn;
	int                 code;
	char              * msg;

	lua_vortex_log (LUA_VORTEX_DEBUG, "Called to open channel with %d args..", initial_top);

	/* check params */
	if (! lua_vortex_check_params (L, "o")) 
		return 0;

	/* get connection */
	conn = lua_touserdata (L, 1);

	/* check for pending errors */
	if (! vortex_connection_pop_channel_error (*conn, &code, &msg)) {
		lua_pushnil (L);
		return 1;
	}

	/* create a table and set values */
	lua_newtable (L);

	/* set code */
	lua_pushnumber (L, 1);
	lua_pushnumber (L, code);
	lua_vortex_log (LUA_VORTEX_DEBUG, "Pushing code=%d into current top=%d, setting into table at: %d", code, lua_gettop (L), initial_top + 1);
	lua_settable (L, initial_top + 1);

	/* set msg */
	lua_pushnumber (L, 2);
	lua_pushstring (L, msg);
	lua_vortex_log (LUA_VORTEX_DEBUG, "Pushing msg=%s into current top=%d, setting into table at: %d", msg, lua_gettop (L), initial_top + 1);
	lua_settable (L, initial_top + 1);
	axl_free (msg);

	return 1;
}

typedef struct _LuaVortexFindByUri {
	const char * profile;
	lua_State  * L;
	int          position;
} LuaVortexFindByUri;

axl_bool lua_vortex_connection_find_by_uri_select_channels (VortexChannel * channel, axlPointer user_data)
{
	LuaVortexFindByUri * data = user_data;

	if (vortex_channel_is_running_profile (channel, data->profile)) {	
		/* set index and update */
		lua_pushnumber (data->L, data->position);
		data->position++;

		/* set the channel found on this position */
		lua_vortex_channel_new (data->L, channel);

		/* store in result table */
		lua_vortex_log (LUA_VORTEX_DEBUG, "Found channel=%s running profile searched: top=%d, position=%d", 
				data->profile, lua_gettop (data->L), data->position - 1);
		lua_settable (data->L, 3);
	}

	return axl_false;
}


/** 
 * find_by_uri implementation..
 */
int lua_vortex_connection_find_by_uri (lua_State * L)
{
	int                    initial_top = lua_gettop (L);
	VortexConnection    ** conn;
	LuaVortexFindByUri   * data;

	lua_vortex_log (LUA_VORTEX_DEBUG, "Called to find by uri with %d args..", initial_top);

	/* check params */
	if (! lua_vortex_check_params (L, "os")) 
		return 0;

	/* get connection */
	conn = lua_touserdata (L, 1);

	/* create empty table */
	lua_newtable (L);

	/* check if there are channels running the profile provided by the user */
	if (! vortex_connection_get_channel_by_uri (*conn, lua_tostring (L, 2))) 
		return 1;

	/* set values */
	data           = axl_new (LuaVortexFindByUri, 1);
	data->L        = L;
	data->profile  = lua_tostring (L, 2);
	data->position = 1;
	vortex_connection_get_channel_by_func (*conn,
					       lua_vortex_connection_find_by_uri_select_channels,
					       data);
	
	axl_free (data);

	return 1;
}

/** 
 * vortex.connection.block implementation..
 */
int lua_vortex_connection_block (lua_State * L)
{
	VortexConnection    ** conn;
	axl_bool               block = axl_true;

	/* check params */
	if (! lua_vortex_check_params (L, "o|b")) 
		return 0;

	/* get connection */
	conn = lua_touserdata (L, 1);

	if (lua_gettop (L) > 1)
		block = lua_toboolean (L, 2);

	/* call to enable/disable block */
	vortex_connection_block (*conn, block);

	return 0;
}

/** 
 * vortex.connection.is_blocked implementation..
 */
int lua_vortex_connection_is_blocked (lua_State * L)
{
	VortexConnection    ** conn;

	/* check params */
	if (! lua_vortex_check_params (L, "o")) 
		return 0;

	/* get connection */
	conn = lua_touserdata (L, 1);

	/* return current status */
	lua_pushboolean (L, vortex_connection_is_blocked (*conn));
	return 1;
}

int lua_vortex_connection_channel_pool_new (lua_State *L) {
	
	int                   initial_top = lua_gettop (L);
	VortexConnection   ** conn;
	VortexChannelPool   * pool;
	axl_bool              have_create_handler = axl_false;
	LuaVortexRefs       * references  = NULL;

	lua_vortex_log (LUA_VORTEX_DEBUG, "Creating a channel pool with %d args..", initial_top);

	/* check params */
	if (! lua_vortex_check_params (L, "osi|fd"))
		return 0;

	/* get connection */
	conn = lua_touserdata (L, 1);

	if (initial_top > 3) {
		lua_vortex_log (LUA_VORTEX_DEBUG, "  detected create channel handler (conn: %p)..", conn);

		/* create handler defined */
		have_create_handler = axl_true;
		
		/* grab references */
		references = lua_vortex_acquire_references (CONN_CTX (*conn), L, 4, 5, 0);

		/* store references in connection */
		vortex_connection_set_data_full (*conn, axl_strdup_printf ("%p", references), references,
						 axl_free, (axlDestroyFunc) lua_vortex_unref2); 
	}
	
	/* unlock */
	LUA_VORTEX_UNLOCK (L, axl_false);
	
	/* create the channel pool */
	pool = vortex_channel_pool_new_full (*conn, 
					     /* set the profile */
					     lua_tostring (L, 2),
					     /* number of channels initially 
						available in the pool */
					     lua_tonumber (L, 3),
					     /* create handler */
					     have_create_handler ? lua_vortex_channelpool_bridge_create_handler : NULL, references,
					     /* close handler */
					     NULL, NULL,
					     /* received handler */
					     NULL, NULL,
					     /* on channel pool created */
					     NULL, NULL);

	if (pool == NULL) {
		/* lock */
		LUA_VORTEX_LOCK (L, axl_false);

		lua_vortex_error (L, "Failed to create channel pool, null reference received");
		return 0;
	} /* end if */

	/* lock */
	LUA_VORTEX_LOCK (L, axl_false);

	/* return lua reference representing channel pool */
	return lua_vortex_channelpool_new (L, pool);
}

int lua_vortex_connection_pool (lua_State *L) {
	int                   initial_top = lua_gettop (L);
	VortexConnection   ** conn;
	VortexChannelPool   * pool;
	int                   pool_id = 1;

	lua_vortex_log (LUA_VORTEX_DEBUG, "Getting channel pool with %d args..", initial_top);

	/* check params */
	if (! lua_vortex_check_params (L, "o|i")) 
		return 0;

	/* get connection */
	conn = lua_touserdata (L, 1);

	/* get pool id */
	if (initial_top > 1)
		pool_id = lua_tonumber (L, 2);

	lua_vortex_log (LUA_VORTEX_DEBUG, "Getting channel pool with id %d from connection reference: %p", pool_id, conn);

	/* get channel pool */
	pool = vortex_connection_get_channel_pool (*conn, pool_id);
	if (pool == NULL)
		return 0;
	
	return lua_vortex_channelpool_new (L, pool);
}

/** 
 * @internal Implementation of vortex.connection.is_ok ()
 */
int lua_vortex_connection_is_ok (lua_State* L)
{
	VortexConnection ** connection;

	/* check parameters */
	if (! lua_vortex_check_params (L, "o"))
		return 0;

	/* get context */
	connection = lua_touserdata (L, 1);

	/* return result */
	lua_pushboolean (L, vortex_connection_is_ok (*connection, axl_false));
	return 1; /* number of arguments returned */
}

/** 
 * @internal Implementation of vortex.connection.close ()
 */
int lua_vortex_connection_close (lua_State* L)
{
	VortexConnection ** connection;
	axl_bool            result;

	/* check parameters */
	if (! lua_vortex_check_params (L, "o"))
		return 0;

	/* get context */
	connection = lua_touserdata (L, 1);

	/* return result */
	result = vortex_connection_close (*connection);
	if (result) 
		(*connection) = NULL;
	lua_pushboolean (L, result);
	return 1; /* number of arguments returned */
}



int lua_vortex_connection_index (lua_State *L) {
	VortexConnection  ** connection  = lua_touserdata (L, 1);
	int                  initial_top = lua_gettop (L);
	const char         * key_index   = lua_tostring (L, 2);
	
	lua_vortex_log (LUA_VORTEX_DEBUG, "Received index event on vortex.connection (%p), stack items: %d", connection, initial_top);
	if (initial_top == 2) {
		/* get operation */
		lua_vortex_log (LUA_VORTEX_DEBUG, "   requested to return value at index: %s", key_index);

		if (axl_cmp (key_index, "ref_count")) {
			lua_pushnumber (L, vortex_connection_ref_count (*connection));
			return 1;
		} else if (axl_cmp (key_index, "id")) {
			lua_pushnumber (L, vortex_connection_get_id (*connection));
			return 1;
		} else if (axl_cmp (key_index, "error_msg")) {
			lua_pushstring (L, vortex_connection_get_message (*connection));
			return 1;
		} else if (axl_cmp (key_index, "status")) {
			lua_pushnumber (L, vortex_connection_get_status (*connection));
			return 1;
		} else if (axl_cmp (key_index, "host")) {
			lua_pushstring (L, vortex_connection_get_host (*connection));
			return 1;
		} else if (axl_cmp (key_index, "host_ip")) {
			lua_pushstring (L, vortex_connection_get_host_ip (*connection));
			return 1;
		} else if (axl_cmp (key_index, "server_name")) {
			lua_pushstring (L, vortex_connection_get_server_name (*connection));
			return 1;
		} else if (axl_cmp (key_index, "num_channels")) {
			lua_pushnumber (L, vortex_connection_channels_count (*connection));
			return 1;
		} else if (axl_cmp (key_index, "local_addr")) {
			lua_pushstring (L, vortex_connection_get_local_addr (*connection));
			return 1;
		} else if (axl_cmp (key_index, "local_port")) {
			lua_pushstring (L, vortex_connection_get_local_port (*connection));
			return 1;
		} else if (axl_cmp (key_index, "role")) {
			switch (vortex_connection_get_role (*connection)) {
			case VortexRoleUnknown:
				lua_pushliteral (L, "unknown");
				break;
			case VortexRoleInitiator:
				lua_pushliteral (L, "initiator");
				break;
			case VortexRoleListener:
				lua_pushliteral (L, "listener");
				break;
			case VortexRoleMasterListener:
				lua_pushliteral (L, "master-listener");
				break;
			}
			return 1;
		} else if (axl_cmp (key_index, "port")) {
			lua_pushstring (L, vortex_connection_get_port (*connection));
			return 1;
		} else if (axl_cmp (key_index, "port")) {
			lua_pushstring (L, vortex_connection_get_port (*connection));
			return 1;
		} else if (axl_cmp (key_index, "is_ok")) {
			lua_pushcfunction (L, lua_vortex_connection_is_ok);
			return 1;
		} else if (axl_cmp (key_index, "close")) {
			lua_pushcfunction (L, lua_vortex_connection_close);
			return 1;
		} else if (axl_cmp (key_index, "shutdown")) {
			lua_pushcfunction (L, lua_vortex_connection_shutdown);
			return 1;
		} else if (axl_cmp (key_index, "set_on_close")) {
			lua_pushcfunction (L, lua_vortex_connection_set_on_close);
			return 1;
		} else if (axl_cmp (key_index, "set_data")) {
			lua_pushcfunction (L, lua_vortex_connection_set_data);
			return 1;
		} else if (axl_cmp (key_index, "get_data")) {
			lua_pushcfunction (L, lua_vortex_connection_get_data);
			return 1;
		} else if (axl_cmp (key_index, "open_channel")) {
			lua_pushcfunction (L, lua_vortex_connection_open_channel);
			return 1;
		} else if (axl_cmp (key_index, "channel_pool_new")) {
			lua_pushcfunction (L, lua_vortex_connection_channel_pool_new);
			return 1;
		} else if (axl_cmp (key_index, "pool")) {
			lua_pushcfunction (L, lua_vortex_connection_pool);
			return 1;
		} else if (axl_cmp (key_index, "pop_channel_error")) {
			lua_pushcfunction (L, lua_vortex_connection_pop_channel_error);
			return 1;
		} else if (axl_cmp (key_index, "find_by_uri")) {
			lua_pushcfunction (L, lua_vortex_connection_find_by_uri);
			return 1;
		} else if (axl_cmp (key_index, "block")) {
			lua_pushcfunction (L, lua_vortex_connection_block);
			return 1;
		} else if (axl_cmp (key_index, "is_blocked")) {
			lua_pushcfunction (L, lua_vortex_connection_is_blocked);
			return 1;
		} /* end if */
	}
	
	
	return 0;
}

/* init vortex.Connection metatable */
void lua_vortex_connection_set_metatable (lua_State * L, int obj_index)
{
	int initial_top = lua_gettop (L);

	lua_vortex_log (LUA_VORTEX_DEBUG, "Setting vortex.connection metatable at index %d, initial top: %d..", obj_index, initial_top);

	/* create empty table */
	lua_newtable (L);

	/* configure __gc */
	lua_pushcfunction (L, lua_vortex_connection_gc);
	/* set the field: L, where is the table to be configured, the
	 * key to configure "__gc", and the value is taken from the
	 * current top stack */
	lua_setfield (L, initial_top + 1, "__gc");
	
	/* configure __tostring */
	lua_pushcfunction (L, lua_vortex_connection_tostring);
	/* set the field: L, where is the table to be configured, the
	 * key to configure "__gc", and the value is taken from the
	 * current top stack */
	lua_setfield (L, initial_top + 1, "__tostring");

	/* configure __index */
	lua_pushcfunction (L, lua_vortex_connection_index);
	/* set the field: L, where is the table to be configured, the
	 * key to configure "__gc", and the value is taken from the
	 * current top stack */
	lua_setfield (L, initial_top + 1, "__index");

	/* configure indication that this is a connection */
	lua_pushliteral (L, "vortex.connection");
	lua_setfield (L, initial_top + 1, "vortex.type");

	/* configure metatable */
	lua_setmetatable (L, obj_index);

	/* now pop the table from stack to grab a reference to it */
	return;
}

/** 
 * Allows to create a reference from an existing connection.
 */
int lua_vortex_connection_new_ref (lua_State * L, VortexConnection * conn, axl_bool transient_status)
{
	VortexConnection ** connection;
	int                 initial_top = lua_gettop (L);

	/* create connection bucket and connect */
	connection    = lua_newuserdata (L, sizeof (connection));
	(*connection) = conn;
		
	lua_vortex_log (LUA_VORTEX_DEBUG, "Created (transient) VortexConnection object: %p", connection);
	/* at this point, the connection is at initial_top + 1 */

	/* configure metatable */
	lua_vortex_connection_set_metatable (L, initial_top + 1);

	/* set this reference as temporal */
	lua_vortex_metatable_set_bool (L, initial_top + 1, "transient", transient_status);

	/* acquire a reference to ensure this lua reference is
	 * consistent: but do not check if the connection is ok */
	vortex_connection_uncheck_ref (conn);

	lua_vortex_log (LUA_VORTEX_DEBUG, "Connection reference acquired id=%d (%p) refs: %d", 
			vortex_connection_get_id (conn), conn, vortex_connection_ref_count (conn));

	return 1; /* number of arguments returned */
}

/** 
 * @internal Implementation of vortex.connection.new ()
 */
int lua_vortex_connection_new (lua_State* L)
{
	VortexConnection ** connection;
	int                 initial_top = lua_gettop (L);
	VortexCtx        ** ctx;

	lua_vortex_log (LUA_VORTEX_DEBUG, "Received call to create vortex.connection (args: %d)", initial_top);
	show_stack (L);

	/* check parameters */
	if (! lua_vortex_check_params (L, "tss"))
		return 0;

	/* get context */
	ctx = lua_touserdata (L, 1);

	/* create connection bucket and connect */
	connection    = lua_newuserdata (L, sizeof (connection));
	(*connection) = vortex_connection_new (*ctx, lua_tostring (L, 2), lua_tostring (L, 3), NULL, NULL);
		
	lua_vortex_log (LUA_VORTEX_DEBUG, "Created VortexConnection object: %p", connection);
	/* at this point, the connection is at initial_top + 1 */

	/* configure metatable */
	lua_vortex_connection_set_metatable (L, initial_top + 1);

	return 1; /* number of arguments returned */
}

axl_bool lua_vortex_connection_check_internal (lua_State * L, int position)
{
	const char * vortex_type;
	axl_bool     result;
	int          initial_top = lua_gettop (L);

	/* get reference to the metatable */
	if (lua_getmetatable (L, position) == 0) {
		lua_vortex_error (L, "Passed an object that should be vortex.connection but found something different (reference has no metatable)");
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
	result      = axl_cmp (vortex_type, "vortex.connection");

	/* check result */
	lua_vortex_log (result ? LUA_VORTEX_DEBUG : LUA_VORTEX_CRITICAL, "Checking vortex.type found='%s' with result %d",
			vortex_type, result);

	/* now remove metatable and string type */
	lua_pop (L, 2);
	return result;
}

/** 
 * @internal Implementation of vortex.connection.unref ()
 */
int lua_vortex_connection_check (lua_State* L)
{
	axl_bool     result;

	/* call internal implementation */
	result = lua_vortex_connection_check_internal (L, 1);

	/* push value returned */
	lua_pushboolean (L, result);
	return 1;
}


static const luaL_Reg vortex_connection_funcs[] = {
	{ "new", lua_vortex_connection_new },
	{ "is_ok", lua_vortex_connection_is_ok },
	{ "close", lua_vortex_connection_close },
	{ "shutdown", lua_vortex_connection_shutdown },
	{ "set_on_close", lua_vortex_connection_set_on_close },
	{ "open_channel", lua_vortex_connection_open_channel },
	{ "channel_pool_new", lua_vortex_connection_channel_pool_new },
	{ "pool", lua_vortex_connection_pool },
	{ "check", lua_vortex_connection_check },
	{ "set_data", lua_vortex_connection_set_data },
	{ "get_data", lua_vortex_connection_get_data },
	{ "pop_channel_error", lua_vortex_connection_pop_channel_error },
	{ "find_by_uri", lua_vortex_connection_find_by_uri },
	{ "block", lua_vortex_connection_block },
	{ "is_blocked", lua_vortex_connection_is_blocked },
	{ NULL, NULL }
};

void lua_vortex_connection_init_module (lua_State *L)
{
	luaL_register(L, "vortex.connection", vortex_connection_funcs);

	return;
}


