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

#include <py_vortex_ctx.h>

typedef struct {
	/* header required to initialize python required bits for
	   every python object */
	PyObject_HEAD
	/* pointer to the vortex context */
	VortexCtx * ctx;
} PyVortexCtx;

static int py_vortex_ctx_init(PyVortexCtx *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

static void py_vortex_ctx_dealloc (PyVortexCtx* self)
{
	/* free ctx */
	vortex_ctx_free (self->ctx);
	self->ctx = NULL;

	/* free the node it self */
	self->ob_type->tp_free ((PyObject*)self);

	return;
}

static PyObject * py_vortex_ctx_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyVortexCtx *self;

	/* create the object */
	self = (PyVortexCtx *)type->tp_alloc(type, 0);

	return (PyObject *)self;
}

/* static PyMemberDef py_vortex_ctx_members[] = { */
/* 	{NULL}  /\* Definition end *\/ */
/* }; */

/* static PyMethodDef py_vortex_ctx_methods[] = { */
/* 	{NULL}  /\* Sentinel *\/ */
/* }; */



static PyTypeObject PyVortexCtxType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size*/
    "vortex.ctx",              /* tp_name*/
    sizeof(PyVortexCtx),       /* tp_basicsize*/
    0,                         /* tp_itemsize*/
    (destructor)py_vortex_ctx_dealloc, /* tp_dealloc*/
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
    0,                         /* tp_getattro*/
    0,                         /* tp_setattro*/
    0,                         /* tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /* tp_flags*/
    "Vortex context object required to function with Vortex API",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    0, /* py_vortex_ctx_methods, */     /* tp_methods */
    0, /* py_vortex_ctx_members, */     /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)py_vortex_ctx_init,      /* tp_init */
    0,                         /* tp_alloc */
    py_vortex_ctx_new,         /* tp_new */

};

/** 
 * @brief Inits the vortex ctx module. It is implemented as a type.
 */
void init_vortex_ctx (PyObject * module) 
{
    
	/* register type */
	if (PyType_Ready(&PyVortexCtxType) < 0)
		return;
	
	Py_INCREF (&PyVortexCtxType);
	PyModule_AddObject(module, "ctx", (PyObject *)&PyVortexCtxType);

	return;
}


