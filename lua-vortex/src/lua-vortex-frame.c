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
#include <lua-vortex.h>
#include <lua-vortex-private.h>

static int lua_vortex_frame_gc (lua_State *L) {
	VortexFrame ** frame      = lua_touserdata (L, 1);
	int            ref_count  = vortex_frame_ref_count (*frame);
	lua_vortex_log (LUA_VORTEX_DEBUG, "Finishing VortexFrame reference associated to %p (ref count: %d)", *frame, ref_count);

	/* unref frame reference */
	vortex_frame_unref (*frame);

	/* nullify */
	(*frame) = NULL;
	return 0;
}

static int lua_vortex_frame_tostring (lua_State *L) {
	VortexFrame ** frame = lua_touserdata (L, 1);
	char         * string;

	/* build string */
	string = axl_strdup_printf ("vortex.frame %p", *frame);
	
	/* push the value into the stack */
	lua_pushstring (L, string);
	axl_free (string);
	
	return 1;
}

static int lua_vortex_frame_index (lua_State *L) {
	VortexFrame  ** frame       = lua_touserdata (L, 1);
	int             initial_top = lua_gettop (L);
	const char    * key_index   = lua_tostring (L, 2);
	
	lua_vortex_log (LUA_VORTEX_DEBUG, "Received index event on vortex.frame (%p), stack items: %d", frame, initial_top);
	if (initial_top == 2) {
		/* get operation */
		lua_vortex_log (LUA_VORTEX_DEBUG, "   requested to return value at index: %s", key_index);

		if (axl_cmp (key_index, "ref_count")) {
			lua_pushnumber (L, vortex_frame_ref_count (*frame));
			return 1;
		} else if (axl_cmp (key_index, "payload")) {
			lua_pushlstring (L, vortex_frame_get_payload (*frame), vortex_frame_get_payload_size (*frame));
			return 1;
		} else if (axl_cmp (key_index, "payload_size")) {
			lua_pushnumber (L, vortex_frame_get_payload_size (*frame));
			return 1;
		} else if (axl_cmp (key_index, "content")) {
			lua_pushlstring (L, vortex_frame_get_content (*frame), vortex_frame_get_content_size (*frame));
			return 1;
		} else if (axl_cmp (key_index, "content_size")) {
			lua_pushnumber (L, vortex_frame_get_content_size (*frame));
			return 1;
		} else if (axl_cmp (key_index, "msgno") || axl_cmp (key_index, "msg_no")) {
			lua_pushnumber (L, vortex_frame_get_msgno (*frame));
			return 1;
		} else if (axl_cmp (key_index, "seqno") || axl_cmp (key_index, "seq_no")) {
			lua_pushnumber (L, vortex_frame_get_seqno (*frame));
			return 1;
		} else if (axl_cmp (key_index, "ansno") || axl_cmp (key_index, "ans_no")) {
			lua_pushnumber (L, vortex_frame_get_ansno (*frame));
			return 1;
		} else if (axl_cmp (key_index, "more_flag")) {
			lua_pushnumber (L, vortex_frame_get_more_flag (*frame));
			return 1;
		} else if (axl_cmp (key_index, "id")) {
			lua_pushnumber (L, vortex_frame_get_id (*frame));
			return 1;
		} else if (axl_cmp (key_index, "type")) {
			switch (vortex_frame_get_type (*frame)) {
			case VORTEX_FRAME_TYPE_MSG:
				lua_pushstring (L, "MSG");
				break;
			case VORTEX_FRAME_TYPE_RPY:
				lua_pushstring (L, "RPY");
				break;
			case VORTEX_FRAME_TYPE_NUL:
				lua_pushstring (L, "NUL");
				break;
			case VORTEX_FRAME_TYPE_ERR:
				lua_pushstring (L, "ERR");
				break;
			case VORTEX_FRAME_TYPE_ANS:
				lua_pushstring (L, "ANS");
				break;
			default:
				lua_pushstring (L, "--unknown--");
				break;
			} /* end if */
				
			return 1;
		} /* end if */
	}
	
	
	return 0;
}

/* init vortex.Frame metatable */
void lua_vortex_frame_set_metatable (lua_State * L, int obj_index, VortexFrame ** frame)
{
	int initial_top = lua_gettop (L);

	lua_vortex_log (LUA_VORTEX_DEBUG, "Setting vortex.frame metatable at index %d, initial top: %d..", obj_index, initial_top);

	/* create empty table */
	lua_newtable (L);

	/* configure __gc */
	lua_pushcfunction (L, lua_vortex_frame_gc);
	/* set the field: L, where is the table to be configured, the
	 * key to configure "__gc", and the value is taken from the
	 * current top stack */
	lua_setfield (L, initial_top + 1, "__gc");
	
	/* configure __tostring */
	lua_pushcfunction (L, lua_vortex_frame_tostring);
	/* set the field: L, where is the table to be configured, the
	 * key to configure "__gc", and the value is taken from the
	 * current top stack */
	lua_setfield (L, initial_top + 1, "__tostring");

	/* configure __index */
	lua_pushcfunction (L, lua_vortex_frame_index);
	/* set the field: L, where is the table to be configured, the
	 * key to configure "__gc", and the value is taken from the
	 * current top stack */
	lua_setfield (L, initial_top + 1, "__index");

	/* configure indication that this is a frame */
	lua_pushliteral (L, "vortex.frame");
	lua_setfield (L, initial_top + 1, "vortex.type");

	/* configure metatable */
	lua_setmetatable (L, obj_index);

	/* now pop the table from stack to grab a reference to it */
	return;
}

/** 
 * @internal Implementation of vortex.frame.new ()
 */
int lua_vortex_frame_new (lua_State* L, VortexFrame * _frame)
{
	VortexFrame ** frame;
	int            initial_top = lua_gettop (L);

	/* check frame reference received */
	if (_frame == NULL)
		return 0;
	if (! vortex_frame_ref (_frame))
		return 0;

	/* create frame bucket and connect */
	frame    = lua_newuserdata (L, sizeof (frame));
	(*frame) = _frame;
		
	lua_vortex_log (LUA_VORTEX_DEBUG, "Created VortexFrame object: %p", frame);
	/* at this point, the frame is at initial_top + 1 */

	/* configure metatable */
	lua_vortex_frame_set_metatable (L, initial_top + 1, frame);

	return 1; /* number of arguments returned */
}


axl_bool lua_vortex_frame_check_internal (lua_State * L, int position)
{
	const char * vortex_type;
	axl_bool     result;
	int          initial_top = lua_gettop (L);

	/* get reference to the metatable */
	if (lua_getmetatable (L, position) == 0) {
		lua_vortex_error (L, "Passed an object that should be vortex.frame but found something different");
		return axl_false;
	}

	/* get the type */
	lua_getfield (L, initial_top + 1, "vortex.type");
	if (! lua_isstring (L, initial_top + 2)) {
		lua_vortex_error (L, "Passed an object that should be vortex.frame but found something different (expected an string value at 'vortex.type' key, but found something different");
		return axl_false;
	}

	/* check value */
	vortex_type = lua_tostring (L, initial_top + 2);
	result      = axl_cmp (vortex_type, "vortex.frame");

	/* check result */
	lua_vortex_log (result ? LUA_VORTEX_DEBUG : LUA_VORTEX_CRITICAL, "Checking vortex.type found='%s' with result %d",
			vortex_type, result);

	/* now remove metatable and string type */
	lua_pop (L, 2);
	return result;
}

/** 
 * @internal Implementation of vortex.frame.unref ()
 */
static int lua_vortex_frame_check (lua_State* L)
{
	axl_bool     result;

	/* call internal implementation */
	result = lua_vortex_frame_check_internal (L, 1);

	/* push value returned */
	lua_pushboolean (L, result);
	return 1;
}


static const luaL_Reg vortex_frame_funcs[] = {
	{ "check", lua_vortex_frame_check },
	{ NULL, NULL }
};

void lua_vortex_frame_init_module (lua_State *L)
{
	luaL_register(L, "vortex.frame", vortex_frame_funcs);

	return;
}


