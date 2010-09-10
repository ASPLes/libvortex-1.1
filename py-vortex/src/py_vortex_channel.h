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
#ifndef __PY_VORTEX_CHANNEL_H__
#define __PY_VORTEX_CHANNEL_H__

/* include base header */
#include <py_vortex.h>

typedef struct _PyVortexChannel PyVortexChannel;

void              init_vortex_channel      (PyObject * module);

PyObject      * py_vortex_channel_create (VortexChannel * channel,
					  PyObject      * py_conn);

PyObject      * py_vortex_channel_create_empty (PyObject * py_conn);

VortexChannel * py_vortex_channel_get    (PyObject        * channel);

void            py_vortex_channel_set    (PyVortexChannel * py_channel, 
					  VortexChannel   * channel);

axl_bool        py_vortex_channel_check  (PyObject        * obj);

/** internal handler used to link python frame received and C frame
 * received. It is available the public header because
 * connection.open_channel may use it. **/
void            py_vortex_channel_received     (VortexChannel    * channel,
						VortexConnection * connection,
						VortexFrame      * frame,
						axlPointer         user_data);

axl_bool        py_vortex_channel_configure_frame_received (PyVortexChannel * self, 
							    PyObject        * handler, 
							    PyObject        * data);

void            py_vortex_channel_create_notify  (int                channel_num,
						  VortexChannel    * channel,
						  VortexConnection * conn,
						  axlPointer         user_data);

axl_bool        py_vortex_channel_set_on_channel (PyObject * _channel,
						  PyObject * handler,
						  PyObject * data);

#endif
