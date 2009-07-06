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

struct _PyVortexCtx {
	/* header required to initialize python required bits for
	   every python object */
	PyObject_HEAD

	/* pointer to the vortex context */
	VortexCtx * ctx;
	/* flags if the PyVortexCtx is pending to exit */
	axl_bool    exit_pending;
};

/** 
 * @brief Allows to get the VortexCtx type definition found inside the
 * PyVortexCtx encapsulation.
 *
 * @param py_vortex_ctx The PyVortexCtx that holds a reference to the
 * inner VortexCtx.
 *
 * @return A reference to the inner VortexCtx.
 */
VortexCtx * py_vortex_ctx_get (PyVortexCtx * py_vortex_ctx)
{
	if (py_vortex_ctx == NULL)
		return NULL;
	
	/* return current context created */
	return py_vortex_ctx->ctx;
}

static int py_vortex_ctx_init_type (PyVortexCtx *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

/** 
 * @brief Function used to allocate memory required by the object vortex.ctx
 */
static PyObject * py_vortex_ctx_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyVortexCtx *self;

	/* create the object */
	self = (PyVortexCtx *)type->tp_alloc(type, 0);

	/* create the context */
	self-> ctx = vortex_ctx_new ();

	return (PyObject *)self;
}

/** 
 * @brief Function used to finish and dealloc memory used by the object vortex.ctx
 */
static void py_vortex_ctx_dealloc (PyVortexCtx* self)
{
	py_vortex_log (PY_VORTEX_DEBUG, "collecting vortex.Ctx ref: %p (self->ctx: %p)", self, self->ctx);

	/* check for pending exit */
	if (self->exit_pending) {
		Py_DECREF ( py_vortex_ctx_exit (self) );
	} /* end if */

	/* free ctx */
	vortex_ctx_free (self->ctx);
	self->ctx = NULL;

	/* free the node it self */
	self->ob_type->tp_free ((PyObject*)self);

	return;
}

/** 
 * @brief Direct wrapper for the vortex_init_ctx function. This method
 * receives a vortex.ctx object and initializes it calling to
 * vortex_init_ctx.
 */
static PyObject * py_vortex_ctx_init (PyVortexCtx* self)
{
	PyObject *_result;

	/* check to not reinitialize */
	if (self->exit_pending) {
		py_vortex_log (PY_VORTEX_WARNING, "called to initialize a context twice");
		return Py_BuildValue ("i", axl_true);
	} /* end if */

	/* call to init context and build result value */
	self->exit_pending = vortex_init_ctx (self->ctx);
	_result = Py_BuildValue ("i", self->exit_pending);
	
	return _result;
}

/** 
 * @brief Direct wrapper for the vortex_init_ctx function. This method
 * receives a vortex.ctx object and initializes it calling to
 * vortex_init_ctx.
 */
PyObject * py_vortex_ctx_exit (PyVortexCtx* self)
{
	if (self->exit_pending) {
		py_vortex_log (PY_VORTEX_DEBUG, "finishing vortex.Ctx %p (self->ctx: %p)", self, self->ctx);
		/* call to finish context: do not dealloc ->ctx, this is
		   already done by the type deallocator */
		vortex_exit_ctx (self->ctx, axl_false);

		/* flag as exit done */
		self->exit_pending = axl_false;
	}

	/* return None */
	Py_INCREF (Py_None);
	return Py_None;
}


/* static PyMemberDef py_vortex_ctx_members[] = { */
/* 	{NULL}  /\* Definition end *\/ */
/* }; */

static PyMethodDef py_vortex_ctx_methods[] = { 
	{"init", (PyCFunction) py_vortex_ctx_init, METH_NOARGS,
	 "Inits the Vortex context starting all vortex functions associated. This API call is required before using the rest of the Vortex API."},
	{"exit", (PyCFunction) py_vortex_ctx_exit, METH_NOARGS,
	 "Finish the Vortex context. This call must be the last one Vortex API usage (for this context)."},
 	{NULL}  
}; 

static PyTypeObject PyVortexCtxType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size*/
    "vortex.Ctx",              /* tp_name*/
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
    py_vortex_ctx_methods,     /* tp_methods */
    0, /* py_vortex_ctx_members, */     /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)py_vortex_ctx_init_type,      /* tp_init */
    0,                         /* tp_alloc */
    py_vortex_ctx_new,         /* tp_new */

};

/** 
 * @brief Allows to create a PyVortexCtx wrapper for the VortexCtx
 * reference received.
 *
 * @param ctx The VortexCtx reference to wrap.
 *
 * @return A newly created PyVortexCtx reference.
 */
PyObject * py_vortex_ctx_create (VortexCtx * ctx)
{
	/* return a new instance */
	PyVortexCtx * obj = (PyVortexCtx *) PyObject_CallObject ((PyObject *) &PyVortexCtxType, NULL);

	if (obj) {
		/* acquire a reference to the VortexCtx */
		obj->ctx = ctx;
		vortex_ctx_ref (ctx);

		/* flag to not exit once the ctx is deallocated */
		obj->exit_pending = axl_false;
		
		return __PY_OBJECT (obj);
	} /* end if */

	/* failed to create object */
	return NULL;
}

/** 
 * @brief Inits the vortex ctx module. It is implemented as a type.
 */
void init_vortex_ctx (PyObject * module) 
{
    
	/* register type */
	if (PyType_Ready(&PyVortexCtxType) < 0)
		return;
	
	Py_INCREF (&PyVortexCtxType);
	PyModule_AddObject(module, "Ctx", (PyObject *)&PyVortexCtxType);

	return;
}


