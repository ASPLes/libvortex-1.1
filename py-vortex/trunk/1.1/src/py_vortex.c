/** 
 *  PyVortex: Vortex Library Python bindings
 *  Copyright (C) 2009 Advanced Software Production Line, S.L.
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
#include <py_vortex.h>


/** 
 * @brief Function that implements vortex_channel_queue_reply handler
 * used as frame received handler.
 */
static PyObject * py_vortex_queue_reply (PyVortexChannel * self, PyObject * args)
{
	PyVortexConnection  * conn    = NULL;
	PyVortexChannel     * channel = NULL;
	PyVortexFrame       * frame   = NULL;
	PyVortexAsyncQueue  * data    = NULL;
	
	/* parse and check result */
	if (! PyArg_ParseTuple (args, "OOOO", &conn, &channel, &frame, &data))
		return NULL;

	/* call to the native API */
	vortex_channel_queue_reply (py_vortex_channel_get (channel),
				    py_vortex_connection_get (conn),
				    py_vortex_frame_get (frame),
				    py_vortex_async_queue_get (data));

	Py_INCREF (Py_None);
	return Py_None;
}

static PyMethodDef py_vortex_methods[] = { 
	{"queue_reply", (PyCFunction) py_vortex_queue_reply, METH_VARARGS,
	 "Implementation of vortex_channel_queue_reply. The function is used inside the queue reply method that requires this handler to be configured as frame received then to use channel.get_reply."},
 	{NULL}  
}; 

/** 
 * @internal Function that inits all vortex modules and classes.
 */
PyMODINIT_FUNC initvortex(void)
{
	PyObject * module;

	/* call to initilize threading API and to acquire the lock */
	PyEval_InitThreads();

	/* register vortex module */
	module = Py_InitModule3("vortex", py_vortex_methods, 
			   "Example module that creates an extension type.");
	if (module == NULL)
		return;

	/* call to register all vortex modules and types */
	init_vortex_ctx         (module);
	init_vortex_connection  (module);
	init_vortex_channel     (module);
	init_vortex_async_queue (module);
	init_vortex_frame       (module);

	return;
}
