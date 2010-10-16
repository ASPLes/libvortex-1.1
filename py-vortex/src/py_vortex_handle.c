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

/* include header */
#include <py_vortex_handle.h>

struct _PyVortexHandle {
	/* header required to initialize python required bits for
	   every python object */
	PyObject_HEAD

	/* reference to the pointer that this PyVortexHandle wraps */
	axlPointer      data;

	/* reference to the destroy function that this PyVortexHandle
	   optionally may use to cleanup associated pointer */
	axlDestroyFunc  data_destroy;
};

/** 
 * @brief Function used to finish and dealloc memory used by the object vortex.Handle
 */
static void py_vortex_handle_dealloc (PyVortexHandle* self)
{
	py_vortex_log (PY_VORTEX_DEBUG, "finishing PyVortexHandle id: %p", self);
	
	/* call to destroy */
	if (self->data_destroy) {
		/* allow threads */
		Py_BEGIN_ALLOW_THREADS

		/* call user code */
		self->data_destroy (self->data);

		/* end threads */
		Py_END_ALLOW_THREADS
	}
	self->data         = NULL;
	self->data_destroy = NULL;

	/* free the node it self */
	self->ob_type->tp_free ((PyObject*)self);

	py_vortex_log (PY_VORTEX_DEBUG, "terminated PyVortexHandle dealloc with id: %p)", self);

	return;
}

/** 
 * @brief This function implements the generic attribute getting that
 * allows to perform complex member resolution (not merely direct
 * member access).
 */
PyObject * py_vortex_handle_get_attr (PyObject *o, PyObject *attr_name) {
	const char         * attr = NULL;
	PyObject           * result;
	/* PyVortexHandle * self = (PyVortexHandle *) o; */

	/* now implement other attributes */
	if (! PyArg_Parse (attr_name, "s", &attr))
		return NULL;

	/* first implement generic attr already defined */
	result = PyObject_GenericGetAttr (o, attr_name);
	if (result)
		return result;
	
	return NULL;
}

static int py_vortex_handle_init_type (PyVortexHandle *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

static PyMethodDef py_vortex_handle_methods[] = { 
 	{NULL}  
}; 

/** 
 * @brief Function used to allocate memory required by the object vortex.Handle
 */
static PyObject * py_vortex_handle_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	/* create the object */
	return type->tp_alloc(type, 0);
}

static PyTypeObject PyVortexHandleType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size*/
    "vortex.Handle",       /* tp_name*/
    sizeof(PyVortexHandle),/* tp_basicsize*/
    0,                         /* tp_itemsize*/
    (destructor)py_vortex_handle_dealloc, /* tp_dealloc*/
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
    py_vortex_handle_get_attr, /* tp_getattro*/
    0,                         /* tp_setattro*/
    0,                         /* tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /* tp_flags*/
    "vortex.Handle, the object used to represent a generic pointer used by Vortex API.",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    py_vortex_handle_methods,     /* tp_methods */
    0, /* py_vortex_handle_members, */ /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)py_vortex_handle_init_type,      /* tp_init */
    0,                         /* tp_alloc */
    py_vortex_handle_new,      /* tp_new */

};

void                 init_vortex_handle        (PyObject           * module)
{
	/* register type */
	if (PyType_Ready(&PyVortexHandleType) < 0)
		return;
	
	Py_INCREF (&PyVortexHandleType);
	PyModule_AddObject(module, "Handle", (PyObject *)&PyVortexHandleType);

	return;
}

/** 
 * @brief Allows to get the reference stored inside the provided vortex.Handle.
 *
 * @param obj The vortex.Handle that contains the pointer to return.
 *
 * @return The pointer stored (including NULL).
 */
axlPointer         py_vortex_handle_get      (PyObject           * obj)
{
	PyVortexHandle * handle = (PyVortexHandle *) obj;
	if (handle == NULL)
		return NULL;
	/* return stored pointer */
	return handle->data;
}

/** 
 * @brief Cleanup the provided PyVortexHandle to erase its internal
 * pointer and internal destroy function.
 */
void                 py_vortex_handle_nullify  (PyObject           * obj)
{
	PyVortexHandle * handle = (PyVortexHandle *) obj;
	if (handle == NULL)
		return;
	py_vortex_log (PY_VORTEX_DEBUG, "nullifying vortex.Handle (%p)", obj);
	/* clear handles */
	handle->data         = NULL;
	handle->data_destroy = NULL;

	return;
}

/** 
 * @brief Allows to check if the PyObject received represents a
 * PyVortexHandle reference.
 */
axl_bool             py_vortex_handle_check    (PyObject           * obj)
{
	/* check null references */
	if (obj == NULL)
		return axl_false;

	/* return check result */
	return PyObject_TypeCheck (obj, &PyVortexHandleType);
}

/** 
 * @brief Creates a new empty PyVortexHandle holding the provided
 * pointer and destroy handler.
 *
 * @param data A user defined pointer to the data that will be hold by this vortex.Handle
 *
 * @param data_destroy An optional destroy function used to terminate
 * the handle.
 */
PyObject         * py_vortex_handle_create   (axlPointer           data,
					      axlDestroyFunc       data_destroy)
{
	/* return a new instance */
	PyVortexHandle * obj = (PyVortexHandle *) PyObject_CallObject ((PyObject *) &PyVortexHandleType, NULL); 

	/* set references */
	obj->data         = data;
	obj->data_destroy = data_destroy;

	py_vortex_log (PY_VORTEX_DEBUG, "creating vortex.Handle (%p), storing %p", obj, data);

	return __PY_OBJECT (obj);
}


