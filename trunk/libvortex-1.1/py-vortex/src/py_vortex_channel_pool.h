/** 
 *  PyVortex: Vortex Library Python bindings
 *  Copyright (C) 2010 Advanced Software Production Line, S.L.
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
#ifndef __PY_VORTEX_CHANNEL_POOL_H__
#define __PY_VORTEX_CHANNEL_POOL_H__

/** 
 * @brief Type representation for the python object that holds a
 * VortexChannelPool inside it.
 */
typedef struct _PyVortexChannelPool PyVortexChannelPool;

/* include base header */
#include <py_vortex.h>

/** 
 * @brief Cast a PyObject reference into a PyVortexChannelPool.
 */
#define PY_VORTEX_CHANNEL_POOL(c) ((PyVortexChannelPool *)c)

void                  init_vortex_channel_pool        (PyObject           * module);

VortexChannelPool   * py_vortex_channel_pool_get      (PyObject           * py_pool);

PyObject            * py_vortex_channel_pool_create   (PyObject           * py_conn,
						       PyObject           * ctx,
						       const char         * profile,
						       int                  init_num,
						       PyObject           * create_channel,
						       PyObject           * create_channel_user_data,
						       PyObject           * close,
						       PyObject           * close_user_data,
						       PyObject           * received,
						       PyObject           * received_user_data,
						       PyObject           * on_channel_pool_created,
						       PyObject           * user_data);

PyObject            * py_vortex_channel_pool_empty    (PyObject           * py_conn,
						       PyObject           * ctx);

axl_bool              py_vortex_channel_pool_check    (PyObject           * py_pool);

PyObject            * py_vortex_channel_pool_get_ctx  (PyObject           * py_pool);

PyObject            * py_vortex_channel_pool_find_reference (VortexChannelPool * pool,
							     PyObject          * _py_conn,
							     PyObject          * py_ctx);

#define PY_CHANNEL_POOL_GET(py_obj) (((PyVortexChannelPool*)self)->pool)

#endif
