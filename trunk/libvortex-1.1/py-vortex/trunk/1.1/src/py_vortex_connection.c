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

#include <py_vortex_connection.h>

typedef struct {
	/* header required to initialize python required bits for
	   every python object */
	PyObject_HEAD
	/* pointer to the VortexConnection object */
	VortexConnection * conn;
} PyVortexConnection;

static int py_vortex_connection_init_type (PyVortexConnection *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

/** 
 * @brief Function used to allocate memory required by the object vortex.Connection
 */
static PyObject * py_vortex_connection_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyVortexConnection * self;
	const char         * host = NULL;
	const char         * port = NULL;
	PyVortexCtx        * py_vortex_ctx;

	/* create the object */
	self = (PyVortexConnection *)type->tp_alloc(type, 0);

	/* now parse arguments */
	static char *kwlist[] = {"ctx", "host", "port", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "Oss", kwlist, &py_vortex_ctx, &host, &port))
		return NULL;

	/* printf ("Received request to connect to: %s:%s\n", host, port); */
	
	/* create the vortex connection in a blocking manner */
	self->conn = vortex_connection_new (py_vortex_ctx_get (py_vortex_ctx), host, port, NULL, NULL);

	return (PyObject *)self;
}

/** 
 * @brief Function used to finish and dealloc memory used by the object vortex.Connection
 */
static void py_vortex_connection_dealloc (PyVortexConnection* self)
{
	/* free the node it self */
	self->ob_type->tp_free ((PyObject*)self);

	return;
}

/** 
 * @brief Direct wrapper for the vortex_connection_is_ok function. 
 */
static PyObject * py_vortex_connection_is_ok (PyVortexConnection* self)
{
	PyObject *_result;

	/* call to check connection and build the value with the
	   result. Do not free the connection in the case of
	   failure. */
	_result = Py_BuildValue ("i", vortex_connection_is_ok (self->conn, axl_false));
	
	return _result;
}

/** 
 * @brief Direct wrapper for the vortex_connection_is_ok function. 
 */
static PyObject * py_vortex_connection_close (PyVortexConnection* self)
{
	PyObject *_result;

	/* call to check connection and build the value with the
	   result. Do not free the connection in the case of
	   failure. */
	_result = Py_BuildValue ("i", vortex_connection_close (self->conn));
	
	return _result;
}

/** 
 * @brief Direct wrapper for the vortex_connection_status function. 
 */
PyObject * py_vortex_connection_status (PyVortexConnection* self)
{
	PyObject *_result;

	/* call to check connection and build the value with the
	   result. Do not free the connection in the case of
	   failure. */
	_result = Py_BuildValue ("i", vortex_connection_get_status (self->conn));
	
	return _result;
}

/** 
 * @brief Direct wrapper for the vortex_connection_get_message function. 
 */
PyObject * py_vortex_connection_error_msg (PyVortexConnection* self)
{
	PyObject *_result;
	/* printf ("Received request to return status message: %s\n", vortex_connection_get_message (self->conn)); */

	/* call to check connection and build the value with the
	   result. Do not free the connection in the case of
	   failure. */
	_result = Py_BuildValue ("z", vortex_connection_get_message (self->conn));
	
	return _result;
}

/** 
 * @brief This function implements the generic attribute getting that
 * allows to perform complex member resolution (not merely direct
 * member access).
 */
PyObject * py_vortex_connection_get_attr (PyObject *o, PyObject *attr_name) {
	const char         * attr = NULL;
	PyObject           * result;
	PyVortexConnection * self = (PyVortexConnection *) o;

	/* now implement other attributes */
	if (! PyArg_Parse (attr_name, "s", &attr))
		return NULL;

	/* printf ("received request to return attribute value of '%s'..\n", attr); */

	if (axl_cmp (attr, "error_msg")) {
		/* found error_msg attribute */
		return Py_BuildValue ("s", vortex_connection_get_message (self->conn));
	} else if (axl_cmp (attr, "status")) {
		/* found error_msg attribute */
		return Py_BuildValue ("i", vortex_connection_get_status (self->conn));
	} else if (axl_cmp (attr, "host")) {
		/* found error_msg attribute */
		return Py_BuildValue ("s", vortex_connection_get_host (self->conn));
	} else if (axl_cmp (attr, "port")) {
		/* found error_msg attribute */
		return Py_BuildValue ("s", vortex_connection_get_port (self->conn));
	} /* end if */

	/* printf ("Attribute not found: '%s'..\n", attr); */

	/* first implement generic attr already defined */
	result = PyObject_GenericGetAttr (o, attr_name);
	if (result)
		return result;
	
	return NULL;
}

static PyMethodDef py_vortex_connection_methods[] = { 
	{"is_ok", (PyCFunction) py_vortex_connection_is_ok, METH_NOARGS,
	 "Allows to check current vortex.Connection status. In the case False is returned the connection is no longer operative. "},
	{"close", (PyCFunction) py_vortex_connection_close, METH_NOARGS,
	 "Allows to close a the BEEP session (vortex.Connection) following all BEEP close negotation phase. The method returns True in the case the connection was cleanly closed, otherwise False is returned. "},
 	{NULL}  
}; 


static PyTypeObject PyVortexConnectionType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size*/
    "vortex.Connection",       /* tp_name*/
    sizeof(PyVortexConnection),/* tp_basicsize*/
    0,                         /* tp_itemsize*/
    (destructor)py_vortex_connection_dealloc, /* tp_dealloc*/
    0,                         /* tp_print*/
    0,                         /* tp_getattr*/
    0,                         /* tp_setattr*/
    0,                         /* tp_compare*/
    0,                         /* tp_repr*/
    0,                         /* tp_as_number*/
    0,                         /* tp_as_sequence*/
    0,                         /* tp_as_mapping*/
    0,                         /* tp_hash */
    0,                         /* tp_call*/
    0,                         /* tp_str*/
    py_vortex_connection_get_attr, /* tp_getattro*/
    0,                         /* tp_setattro*/
    0,                         /* tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /* tp_flags*/
    "vortex.Connection, the object used to represent a connected BEEP session.",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    py_vortex_connection_methods,     /* tp_methods */
    0, /* py_vortex_connection_members, */ /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)py_vortex_connection_init_type,      /* tp_init */
    0,                         /* tp_alloc */
    py_vortex_connection_new,  /* tp_new */

};

/** 
 * @brief Inits the vortex connection module. It is implemented as a type.
 */
void init_vortex_connection (PyObject * module) 
{
    
	/* register type */
	if (PyType_Ready(&PyVortexConnectionType) < 0)
		return;
	
	Py_INCREF (&PyVortexConnectionType);
	PyModule_AddObject(module, "Connection", (PyObject *)&PyVortexConnectionType);

	return;
}


