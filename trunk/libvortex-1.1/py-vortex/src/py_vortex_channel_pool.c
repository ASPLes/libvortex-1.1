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

#include <py_vortex_channel_pool.h>

struct _PyVortexChannelPool {
	/* header required to initialize python required bits for
	   every python object */
	PyObject_HEAD

	/* pointer to the VortexChannelPool object */
	VortexChannelPool * pool;

	/** 
	 * @brief Reference to the PyVortexCtx that was used to create
	 * the channel pool. 
	 */ 
	PyObject       * py_vortex_ctx;

	/** 
	 * @brief Reference to the python connection.
	 */
	PyObject       * py_conn;
};


/** 
 * @brief Allows to get the VortexChannelPool reference inside the
 * PyVortexChannelPool.
 *
 * @param py_conn The reference that holds the channel_pool inside.
 *
 * @return A reference to the VortexChannelPool inside or NULL if it fails.
 */
VortexChannelPool * py_vortex_channel_pool_get  (PyObject * py_pool)
{
	PyVortexChannelPool * _py_pool = (PyVortexChannelPool *) py_pool;

	/* return NULL reference */
	if (_py_pool == NULL)
		return NULL;
	/* return py channel_pool */
	return _py_pool->pool;
}

static int py_vortex_channel_pool_init_type (PyVortexChannelPool *self, PyObject *args, PyObject *kwds)
{
    return 0;
}

/** 
 * @brief Function used to allocate memory required by the object vortex.ChannelPool
 */
static PyObject * py_vortex_channel_pool_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyVortexChannelPool * self;
	
	/* create the object */
	self = (PyVortexChannelPool *)type->tp_alloc(type, 0);

	return (PyObject *)self;
}

/** 
 * @brief Function used to finish and dealloc memory used by the object vortex.ChannelPool
 */
static void py_vortex_channel_pool_dealloc (PyVortexChannelPool* self)
{
	int pool_id = vortex_channel_pool_get_id (self->pool);

	py_vortex_log (PY_VORTEX_DEBUG, "finishing PyVortexChannelPool id: %d (%p)", 
		       pool_id, self);

	/* free the node it self */
	self->ob_type->tp_free ((PyObject*)self);

	return;
}

/** 
 * @brief This function implements the generic attribute getting that
 * allows to perform complex member resolution (not merely direct
 * member access).
 */
PyObject * py_vortex_channel_pool_get_attr (PyObject *o, PyObject *attr_name) {
	const char          * attr = NULL;
	PyObject            * result;
	PyVortexChannelPool * self = (PyVortexChannelPool *) o;

	/* now implement other attributes */
	if (! PyArg_Parse (attr_name, "s", &attr))
		return NULL;

	if (axl_cmp (attr, "id")) {
		/* found error_msg attribute */
		return Py_BuildValue ("i", vortex_channel_pool_get_id (self->pool));
	} else if (axl_cmp (attr, "ctx")) {
		/* found ctx attribute */
		Py_XINCREF (self->py_vortex_ctx);
		return self->py_vortex_ctx;
	} /* end if */

	/* first implement generic attr already defined */
	result = PyObject_GenericGetAttr (o, attr_name);
	if (result)
		return result;
	
	return NULL;
}

static PyObject * py_vortex_channel_pool_next_ready (PyVortexChannelPool * self, PyObject * args, PyObject * kwds)
{
	axl_bool        auto_inc  = axl_false;
	PyObject      * user_data = NULL;
	VortexChannel * channel;
	
	/* now parse arguments */
	static char *kwlist[] = {"auto_inc", "user_data", NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "|iO", kwlist, &auto_inc, user_data))
		return NULL;

	/* allow threads */
	Py_BEGIN_ALLOW_THREADS;

	/* call to return the channel */
	channel = vortex_channel_pool_get_next_ready_full (self->pool, auto_inc, user_data);

	/* end threads */
	Py_END_ALLOW_THREADS;

	if (channel == NULL) {
		/* set configured */
		Py_INCREF (Py_None);
	} /* end if */

	/* create the python channel reference */
	return py_vortex_channel_create (channel, self->py_conn);
}

static PyMethodDef py_vortex_channel_pool_methods[] = { 
	/* is_ok */
	{"next_ready", (PyCFunction) py_vortex_channel_pool_next_ready, METH_NOARGS,
	 "Allows to get next channel ready on the pool. "},
 	{NULL}  
}; 


static PyTypeObject PyVortexChannelPoolType = {
    PyObject_HEAD_INIT(NULL)
    0,                         /* ob_size*/
    "vortex.ChannelPool",       /* tp_name*/
    sizeof(PyVortexChannelPool),/* tp_basicsize*/
    0,                         /* tp_itemsize*/
    (destructor)py_vortex_channel_pool_dealloc, /* tp_dealloc*/
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
    py_vortex_channel_pool_get_attr, /* tp_getattro*/
    0,                         /* tp_setattro*/
    0,                         /* tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,  /* tp_flags*/
    "vortex.ChannelPool, the object used to represent a pool of channels (VortexChannelPool).",           /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    py_vortex_channel_pool_methods,     /* tp_methods */
    0, /* py_vortex_channel_pool_members, */ /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)py_vortex_channel_pool_init_type,      /* tp_init */
    0,                         /* tp_alloc */
    py_vortex_channel_pool_new,  /* tp_new */

};

/** 
 * @brief Creates an empty channel pool instance and set the internal
 * reference it represents.
 */
PyObject * py_vortex_channel_pool_create   (VortexChannelPool  * pool, 
					    PyObject           * py_conn,
					    PyObject           * ctx)
{
	/* return a new instance */
	PyVortexChannelPool * obj = (PyVortexChannelPool *) py_vortex_channel_pool_empty (py_conn, ctx);

	/* set channel reference received */
	if (obj && pool) {
		/* configure the reference */
		obj->pool = pool;
	} /* end if */
	
	/* return object */
	return (PyObject *) obj;
}

/** 
 * @brief Allows to create an empty vortex.ChannelPool object (still
 * without internal VortexChannelPool reference).
 */
PyObject            * py_vortex_channel_pool_empty    (PyObject           * py_conn,
						       PyObject           * ctx)
{
	/* return a new instance */
	PyVortexChannelPool * obj = (PyVortexChannelPool *) PyObject_CallObject ((PyObject *) &PyVortexChannelPoolType, NULL); 

	/* check ref created */
	if (obj == NULL) {
		py_vortex_log (PY_VORTEX_CRITICAL, "Failed to create PyVortexChannelPool object, returning NULL");
		return NULL;
	} /* end if */

	/* set context reference */
	obj->py_vortex_ctx = ctx;
	obj->py_conn       = py_conn;

	/* return object */
	return (PyObject *) obj;
}


/** 
 * @brief Allows to get a reference to the PyVortexCtx reference used
 * by the provided PyVortexChannelPool.
 * 
 * @param py_conn A reference to PyVortexChannelPool object.
 */
PyObject           * py_vortex_channel_pool_get_ctx  (PyObject         * py_pool)
{
	PyVortexChannelPool * _py_pool = (PyVortexChannelPool *) py_pool;

	/* check object received */
	if (! py_vortex_channel_pool_check (py_pool))
		return NULL;

	/* return context */
	return  _py_pool->py_vortex_ctx;
}

/** 
 * @brief Allows to check if the PyObject received represents a
 * PyVortexChannelPool reference.
 */
axl_bool             py_vortex_channel_pool_check    (PyObject          * obj)
{
	/* check null references */
	if (obj == NULL)
		return axl_false;

	/* return check result */
	return PyObject_TypeCheck (obj, &PyVortexChannelPoolType);
}

/** 
 * @brief Inits the vortex channel pool module. It is implemented as a type.
 */
void init_vortex_channel_pool (PyObject * module) 
{
    
	/* register type */
	if (PyType_Ready(&PyVortexChannelPoolType) < 0)
		return;
	
	Py_INCREF (&PyVortexChannelPoolType);
	PyModule_AddObject(module, "ChannelPool", (PyObject *)&PyVortexChannelPoolType);

	return;
}


