/** 
 *  PyVortex: Vortex Library Python bindings
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
#ifndef __PY_VORTEX_CONNECTION_H__
#define __PY_VORTEX_CONNECTION_H__

/** 
 * @brief Type representation for the python object that holds a
 * VortexConnection inside it.
 */
typedef struct _PyVortexConnection PyVortexConnection;

/* include base header */
#include <py_vortex.h>

/** 
 * @brief Cast a PyObject reference into a PyVortexConnection.
 */
#define PY_VORTEX_CONNECTION(c) ((PyVortexConnection *)c)

void                 init_vortex_connection        (PyObject           * module);

VortexConnection   * py_vortex_connection_get      (PyObject           * py_conn);

PyObject           * py_vortex_connection_create   (VortexConnection   * conn, 
						    axl_bool             acquire_ref,
						    axl_bool             close_ref);

PyObject           * py_vortex_connection_shutdown (PyVortexConnection * self);

void                 py_vortex_connection_nullify  (PyObject           * py_conn);

axl_bool             py_vortex_connection_check    (PyObject           * obj);

void                 py_vortex_connection_register (PyObject   * py_conn, 
						    PyObject   * data,
						    const char * key,
						    ...);

PyObject           * py_vortex_connection_register_get (PyObject * py_conn,
							const char * key,
							...);

PyObject           * py_vortex_connection_find_reference (VortexConnection * conn);

axl_bool             __unlocked_vortex_connection_is_ok (VortexConnection * conn,
							 axl_bool           free_on_fail);

#define PY_CONN_GET(py_obj) (((PyVortexConnection*)self)->conn)

#endif
