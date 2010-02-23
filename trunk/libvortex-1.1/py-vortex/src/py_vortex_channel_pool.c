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
};


/** 
 * @brief Allows to get the VortexChannelPool reference inside the
 * PyVortexChannelPool.
 *
 * @param py_conn The reference that holds the channel_pool inside.
 *
 * @return A reference to the VortexChannelPool inside or NULL if it fails.
 */
VortexChannelPool * py_vortex_channel_pool_get  (PyObject * py_conn)
{
	PyVortexChannelPool * _py_conn = (PyVortexChannelPool *) py_conn;

	/* return NULL reference */
	if (_py_conn == NULL)
		return NULL;
	/* return py channel_pool */
	return _py_conn->conn;
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
	PyObject            * py_conn                   = NULL;
	const char          * profile                   = NULL;
	int                   init_num                  = -1;
	PyObject            * create_channel            = NULL;
	PyObject            * create_channel_user_data  = NULL;
	PyObject            * close                     = NULL;
	PyObject            * close_user_data           = NULL;
	PyObject            * received                  = NULL;
	PyObject            * received_user_data        = NULL;
	PyObject            * on_channel_pool_created   = NULL;
	PyObject            * user_data                 = NULL;

	/* now parse arguments */
	static char *kwlist[] = {"conn", "profile", "init_num", 
				 "create_channel", "create_channel_user_data", 
				 "close", "close_user_data", 
				 "received", "received_user_data",
				 "on_channel_pool_created", "user_data",
				 NULL};

	/* parse and check result */
	if (! PyArg_ParseTupleAndKeywords(args, kwds, "Osi|OOOOOOOO", kwlist, 
					  &py_conn, &profile, &init_num,
					  &create_channel, &create_channel_user_data,
					  &close, &close_user_data,
					  &received, CONTINUAR POR AQUÍ!!!!
		return NULL;

	/* check for empty creation */
	if (py_vortex_ctx == NULL) {
		py_vortex_log (PY_VORTEX_DEBUG, "found empty request to create a PyVortexChannelPool ref..");
		return (PyObject *) self;
	}

	/* check that py_vortex_ctx is indeed a vortex ctx */
	if (! py_vortex_ctx_check (py_vortex_ctx)) {
		PyErr_Format (PyExc_ValueError, "Expected to receive a vortex.Ctx object but received something different");
		return NULL;
	}
	
	/* create the object */
	self = (PyVortexChannelPool *)type->tp_alloc(type, 0);

		/* allow threads */
		Py_BEGIN_ALLOW_THREADS

		/* create the vortex channel_pool in a blocking manner */
		if (serverName)
			self->conn = vortex_channel_pool_new_full (py_vortex_ctx_get (py_vortex_ctx),
								 host, port,
								 CONN_OPTS (VORTEX_SERVERNAME_FEATURE, serverName),
								 NULL, NULL);
		else
			self->conn = vortex_channel_pool_new (py_vortex_ctx_get (py_vortex_ctx), host, port, NULL, NULL);

		/* end threads */
		Py_END_ALLOW_THREADS

		/* own a copy of py_vortex_ctx */
		self->py_vortex_ctx = py_vortex_ctx;
		Py_INCREF (__PY_OBJECT (py_vortex_ctx) );

		/* signal this instance as a master copy to be closed
		 * if the reference is collected and the channel_pool is
		 * working */
		self->close_ref = axl_true;

		if (vortex_channel_pool_is_ok (self->conn, axl_false)) {
			py_vortex_log (PY_VORTEX_DEBUG, "created channel_pool id %d, with %s:%s",
				       vortex_channel_pool_get_id (self->conn), 
				       vortex_channel_pool_get_host (self->conn),
				       vortex_channel_pool_get_port (self->conn));
		} else {
			py_vortex_log (PY_VORTEX_CRITICAL, "failed to connect with %s:%s, channel_pool id: %d",
				       vortex_channel_pool_get_host (self->conn),
				       vortex_channel_pool_get_port (self->conn),
				       vortex_channel_pool_get_id (self->conn));
		} /* end if */
	} /* end if */

	return (PyObject *)self;
}

/** 
 * @brief Function used to finish and dealloc memory used by the object vortex.ChannelPool
 */
static void py_vortex_channel_pool_dealloc (PyVortexChannelPool* self)
{
	int pool_id = vortex_channel_pool_get_id (self->pool);
	int ref_count;

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
 * @brief Allows to create a new PyVortexChannelPool instance using
 * the reference received.
 *
 * NOTE: At server side notification or reconstructing references that
 * was created, use py_vortex_channel_pool_find_reference to avoid
 * creating/finishing references for each notification.
 *
 * @param pool The channel pool to use as reference to wrap.
 *
 * @param acquire_ref Allows to configure if py_conn reference must
 * acquire a reference to the channel_pool.
 *
 * @param close_ref Allows to signal the object created to close or
 * not the channel_pool when the reference is garbage collected.
 *
 * @return A newly created PyVortexChannelPool reference.
 */
PyObject * py_vortex_channel_pool_create   (VortexChannelPool  * pool, 
					    PyObject           * ctx)
{
	/* return a new instance */
	PyVortexChannelPool * obj = (PyVortexChannelPool *) PyObject_CallObject ((PyObject *) &PyVortexChannelPoolType, NULL); 

	/* check ref created */
	if (obj == NULL) {
		py_vortex_log (PY_VORTEX_CRITICAL, "Failed to create PyVortexChannelPool object, returning NULL");
		return NULL;
	} /* end if */

	/* set channel reference received */
	if (obj && pool) {
		/* configure the reference */
		obj->poo = pool;
	} /* end if */
	
	/* set context reference */
	obj->py_vortex_ctx = ctx;

	/* return object */
	return (PyObject *) obj;
}


void py_vortex_channel_pool_find_reference_close_conn (VortexChannelPool * conn, axlPointer _pool)
{
	VortexChannelPool * pool = _pool;
	VortexCtx         * ctx = CONN_CTX (conn);
	char              * key = axl_strdup_printf ("py:vo:po:%d", vortex_channel_pool_get_id (pool));

	py_vortex_log (PY_VORTEX_DEBUG, "(find reference) releasing PyVortexChannelPool id=%d reference from vortex.Ctx",
		       vortex_channel_pool_get_id (conn));
	vortex_ctx_set_data (ctx, key, NULL);
	axl_free (key);
	return;
}

/** 
 * @internal Function used to reuse PyVortexChannelPool references
 * rather creating and finishing them especially at server side async
 * notification.
 *
 * This function is designed to avoid using
 * py_vortex_channel_pool_create providing a way to reuse references
 * that, not only saves memory, but are available after finishing the
 * python context that created the particular channel pool reference.
 *
 * @param pool The channel pool for which its reference will be looked up.
 *
 * @param py_ctx The vortex.Ctx object where to lookup for an already
 * created vortex.ChannelPool reference.
 */
PyObject * py_vortex_channel_pool_find_reference (VortexChannelPool  * pool,
						  PyObject           * py_ctx)
{
	PyObject  * py_pool;
	VortexCtx * ctx = py_vortex_ctx_get (py_ctx);
	char      * key;

	/* check if the channel_pool reference was created previosly */
	key     = axl_strdup_printf ("py:vo:po:%d", vortex_channel_pool_get_id (pool));
	py_vortex_log (PY_VORTEX_DEBUG, "Looking to reuse PyVortexChannelPool ref id=%d, key: %s",
		       vortex_channel_pool_get_id (conn), key);
	py_pool = vortex_ctx_get_data (ctx, key);
	if (py_pool != NULL) {
		py_vortex_log (PY_VORTEX_DEBUG, "Found reference (PyVortexChannelPool: %p), conn id=%d",
			       py_pool, vortex_channel_pool_get_id (pool));
		/* found, increase reference and return this */
		Py_INCREF (py_pool);
		axl_free (key);
		return py_pool;
	}
	
	/* reference do not exists, create one */
	py_conn  = py_vortex_channel_pool_create (
	        /* pool to wrap */
		pool, 
		/* context: create a copy */
		py_ctx);

	py_vortex_log (PY_VORTEX_DEBUG, "Not found reference, created a new one (PyVortexChannelPool %p) conn id=%d",
		       py_conn, vortex_channel_pool_get_id (conn));

	/* store the reference in the context for (re)use now and later */
	vortex_ctx_set_data_full (ctx, key, py_conn, axl_free, (axlDestroyFunc) py_vortex_decref);
	vortex_connection_set_on_close_full (vortex_channel_pool_get_connection (pool), 
					     py_vortex_channel_pool_find_reference_close_conn, ctx);

	/* now increase to return it */
	Py_INCREF (py_pool);
	return py_pool;
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
axl_bool             py_vortex_channel_pool_check    (PyObject          * py_pool)
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


