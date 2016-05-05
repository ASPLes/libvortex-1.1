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
#include <lua-vortex-object.h>
#include <lua-vortex-private.h>

/** 
 * @internal Allows to create a new LuaVortexObject instance taking
 * the value directly from the stack on the provided position.
 *
 * @param parent_index Points to the parent object that will acquire a
 * reference to the object. This parent object must have a metatable
 * defined.
 *
 * @return A newly created reference or NULL if it fails. The object
 * must be terminated with \ref lua_vortex_object_unref
 */
LuaVortexObject * lua_vortex_object_new_from_stack (lua_State * L, 
						    int parent_index, 
						    int obj_index)
{
	LuaVortexObject * object;
	int               initial_top = lua_gettop (L);

	/* get metatatable */
	if (lua_getmetatable (L, parent_index) == 0) {
		lua_vortex_log (LUA_VORTEX_DEBUG, "Found object at parent_index does not have a metatable associated");
		return NULL;
	} /* end if */

	/* now push according to value received */
	object       = axl_new (LuaVortexObject, 1);
	if (object == NULL) {
		/* pop metatable */
		lua_pop (L, 1);
		return NULL;
	}

	/* get type */
	object->type = lua_type (L, obj_index);
	switch (object->type) {
	case LUA_TNIL:
		/* nothing */
		break;
	case LUA_TLIGHTUSERDATA:
	case LUA_TTABLE:
	case LUA_TFUNCTION:
	case LUA_TUSERDATA:
	case LUA_TTHREAD:
	case LUA_TSTRING:
	case LUA_TNUMBER:
		/* repush value */
		lua_pushvalue (L, obj_index);

		/* now set the value on the metatable */
		object->ref_id = luaL_ref (L, initial_top + 1);
		break;
	default:
		/* never reached */
		break;
	} 

	/* pop metatable */
	lua_pop (L, 1);

	return object;
}

/** 
 * @internal Allows to take the object and push it on the provided lua
 * stack.
 *
 *
 */
void lua_vortex_object_push (lua_State * L, LuaVortexObject * object, int parent_index)
{
	int initial_top = lua_gettop (L);

	if (L == NULL || object == NULL)
		return;

	/* get metatable */
	if (lua_getmetatable (L, parent_index) == 0) {
		lua_vortex_log (LUA_VORTEX_DEBUG, "Found object at parent_index does not have a metatable associated");
		return;
	} /* end if */

	switch (object->type) {
	case LUA_TNIL:
		/* push on the stack */
		lua_pushnil (L);
		break;
	case LUA_TLIGHTUSERDATA:
	case LUA_TTABLE:
	case LUA_TFUNCTION:
	case LUA_TUSERDATA:
	case LUA_TTHREAD:
	case LUA_TSTRING:
	case LUA_TNUMBER:
		/* push value from metatable on the stack */
		lua_rawgeti (L, initial_top + 1, object->ref_id);

		break;
	}

	/* remove the metatable but not use pop so pushed value remains */
	lua_remove (L, initial_top + 1);

	return;
}

/** 
 * @internal Allows to release the object provided.
 *
 * @param object The object to be release (including internal
 * references if it applies).
 */
void lua_vortex_object_unref (lua_State * L, LuaVortexObject * object, int parent_index)
{
	int initial_top = lua_gettop (L);

	if (object == NULL)
		return;

	/* get metatable */
	if (lua_getmetatable (L, parent_index) == 0) {
		lua_vortex_log (LUA_VORTEX_DEBUG, "Found object at parent_index does not have a metatable associated");
		return;
	} /* end if */

	switch (object->type) {
	case LUA_TNIL:
		/* nothing to do */
		break;
	case LUA_TLIGHTUSERDATA:
	case LUA_TTABLE:
	case LUA_TFUNCTION:
	case LUA_TUSERDATA:
	case LUA_TTHREAD:
	case LUA_TSTRING:
	case LUA_TNUMBER:
		/* unref from metatable */
		luaL_unref (L, initial_top + 1, object->ref_id);
		break;
	}	

	/* pop metatable */
	lua_pop (L, 1);

	axl_free (object);
	
	return;
}

